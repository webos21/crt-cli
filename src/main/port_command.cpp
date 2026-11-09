// `crt-cli port ...`: builds a reviewed source-port recipe against the project's SDK and publishes the result into the port
// store (D3). Experimental; the surface, like the store manifest format, is CLI-owned until the D4 contract exists.
#include <crtcli/archive.h>
#include <crtcli/host_tools.h>
#include <crtcli/port_store.h>
#include <crtcli/toolchain.h>
#include "store_lock.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <map>
#include <set>

namespace crtcli {
namespace {

const char* usage =
    "usage: port build <name> --archive <file> --host-tools <dir> --host-tools-sha256 <hex> [--project <dir>] [--jobs <n>] [--timeout <seconds>]\n"
    "       port list\n"
    "       port verify <build-key> [--project <dir>]\n"
    "       port recover";

bool lower_hex(const std::string& value, size_t length) {
    if (value.size() != length) return false;
    for (unsigned char c : value) if (!(std::isdigit(c) || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
std::map<std::string, std::string> options(const std::vector<std::string>& args, size_t first, const std::set<std::string>& known) {
    std::map<std::string, std::string> result;
    for (size_t i = first; i < args.size(); i += 2) {
        if (i + 1 >= args.size() || !known.count(args[i])) throw Error(usage, 2);
        if (!result.emplace(args[i], args[i + 1]).second) throw Error("duplicate port option: " + args[i], 2);
    }
    return result;
}
unsigned number(const std::map<std::string, std::string>& given, const char* name, unsigned low, unsigned high, unsigned fallback) {
    if (!given.count(name)) return fallback;
    const auto& text = given.at(name);
    require(!text.empty() && text.size() <= 5 && std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c); }),
            std::string(name) + " must be a number");
    auto value = std::stoul(text);
    require(value >= low && value <= high, std::string(name) + " must be " + std::to_string(low) + ".." + std::to_string(high));
    return static_cast<unsigned>(value);
}
fs::path project_of(const std::map<std::string, std::string>& given) {
    return fs::canonical(given.count("--project") ? fs::path(given.at("--project")) : fs::current_path());
}
PortStore make_store() { return PortStore(sdk_store_root().parent_path() / "ports"); }

}  // namespace

Reply port_command(const std::vector<std::string>& args) {
    if (args.size() < 2) throw Error(usage, 2);
    const std::string& action = args[1];

    if (action == "list" || action == "recover") {
        if (args.size() != 2) throw Error(usage, 2);
        auto store = make_store();
        Json keys = Json::array();
        for (const auto& key : action == "list" ? store.list() : store.recover()) keys.push_back(key);
        return {0, "port", {{action == "list" ? "items" : "recovered", keys}, {"store", store.root().string()}},
                std::to_string(keys.size()) + (action == "list" ? " port store item(s)" : " leftover(s) removed")};
    }

    if (action == "verify") {
        if (args.size() < 3) throw Error(usage, 2);
        const std::string key = args[2];
        require(lower_hex(key, 64), "a build key is 64 lowercase hex digits");
        auto given = options(args, 3, {"--project"});
        auto toolchain = Toolchain::load(project_of(given));
        auto checked = make_store().verify(key, toolchain.sdk);
        return {0, "port", {{"build_key", key}, {"inventory_entries", checked}}, "port item verified: " + key};
    }

    if (action == "build") {
        if (args.size() < 3) throw Error(usage, 2);
        const std::string name = args[2];
        require(!name.empty() && std::all_of(name.begin(), name.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; }),
                "invalid port name");
        auto given = options(args, 3, {"--archive", "--host-tools", "--host-tools-sha256", "--project", "--jobs", "--timeout"});
        for (const char* required : {"--archive", "--host-tools", "--host-tools-sha256"}) require(given.count(required), std::string("port build requires ") + required);
        require(lower_hex(given.at("--host-tools-sha256"), 64), "--host-tools-sha256 must be 64 lowercase hex digits");
        auto toolchain = Toolchain::load(project_of(given));
        auto tools = HostTools::load(given.at("--host-tools"), given.at("--host-tools-sha256"));

        // The recipe is the SDK's own file, accepted only when its digest is in the reviewed adapter table.
        auto recipe_path = toolchain.sdk / "porting/recipes" / (name + ".json");
        require(fs::is_regular_file(recipe_path), "the selected SDK has no recipe for '" + name + "'");
        auto recipe = parse_port_recipe(read_text(recipe_path), native_target().at("os").get<std::string>());
        require(recipe.name == name, "recipe file names another port");
        const fs::path archive = given.at("--archive");
        require(fs::is_regular_file(archive), "source archive not found: " + archive.string());
        require(file_sha256(archive) == recipe.source.sha256, "the source archive is not the recipe's pinned source (SHA-256 mismatch)");

        PortBuildRequest request;
        request.recipe = recipe;
        request.test_root = toolchain.sdk;
        request.toolchain = &toolchain;
        request.tools = &tools;
        request.jobs = number(given, "--jobs", 1, 256, 0);
        request.step_timeout_seconds = number(given, "--timeout", 1, 3600, request.step_timeout_seconds);

        auto store = make_store();
        auto inputs = port_key_inputs(request);
        auto key = port_build_key(inputs);
        Json data{{"port", name}, {"version", recipe.version}, {"build_key", key}, {"item", store.item_path(key).string()},
                  {"sdk", toolchain.selection}, {"declaration", {{"name", name}, {"provider", "port"}, {"build_key", key}}}};
        if (fs::exists(store.item_path(key))) {
            // The same inputs were built before: re-verify, never rebuild or replace.
            data["inventory_entries"] = store.verify(key, toolchain.sdk);
            data["already_published"] = true;
            return {0, "port", data, "port item already published and verified: " + key};
        }

        auto work = store.root() / "work";
        require(!fs::is_symlink(work), "port work directory must not be a symlink");
        fs::create_directories(work);
        auto scratch = work / (std::to_string(current_pid()) + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directory(scratch);
        try {
            auto pristine = scratch / "pristine";
            fs::create_directory(pristine);
            extract_archive(archive, pristine);
            request.source_tree = pristine;
            request.job = scratch / "job";
            auto result = run_port_build(request);
            auto published = store.publish(request, result, inputs);
            data["already_published"] = published.already_published;
            data["tests"] = result.record.at("tests");
            data["libraries"] = published.manifest.value("libraries", Json::array());
        } catch (const Error& error) {
            // The job directory stays for inspection; only the extracted source copy is useless without it.
            throw Error(std::string(error.what()) + " (job kept at " + scratch.string() + ")", error.code);
        }
        std::error_code ignored;
        fs::remove_all(scratch, ignored);
        return {0, "port", data, "port item published: " + key};
    }
    throw Error(usage, 2);
}

}  // namespace crtcli
