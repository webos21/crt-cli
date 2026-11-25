#include <crtcli/editor.h>
#include <chrono>
#include <cerrno>
#include <fcntl.h>
#include <map>
#include <set>
#include <unistd.h>

namespace crtcli {
namespace {
[[noreturn]] void fail(const std::string& code, const std::string& text, int status = 1) {
    throw Error(text, status, code);
}

bool under(const fs::path& path, const fs::path& root) {
    auto relative = path.lexically_relative(root);
    return !relative.empty() && *relative.begin() != "..";
}

void publish_database(const fs::path& requested, const fs::path& root, const Json& info, const Json& entries) {
    // Canonicalize the parent, preserving the final name so a symlink cannot
    // redirect a write. Export never creates directories or project bindings.
    if (requested.filename().empty() || !fs::is_directory(requested.parent_path()))
        fail("output_error", "output parent must be an existing directory");
    auto output = fs::canonical(requested.parent_path()) / requested.filename();
    if (fs::is_symlink(output) || output.extension() != ".json" ||
            output == root / "crt.json" || output == root / ".sdk.json" ||
            under(output, root / ".crt") || under(output, root / "src") || under(output, root / "include") ||
            under(output, root / "out/native") || under(output, fs::path(info.at("sdk").at("root").get<std::string>())) ||
            under(output, fs::path(info.at("tools").at("resource").get<std::string>())) || under(output, sdk_store_root().parent_path()))
        fail("output_conflict", "compile database output must not replace project, SDK, store or build inputs");
    for (const auto& entry : entries)
        if (output == fs::path(entry.at("file").get<std::string>())) fail("output_conflict", "output is a project source");
    if (fs::exists(output)) {
        if (!fs::is_regular_file(output)) fail("output_conflict", "output is not a regular file");
        try {
            const auto previous = read_json(output);
            require(previous.is_array(), "not a compilation database");
            for (const auto& entry : previous)
                require(entry.is_object() && entry.contains("file") && entry.contains("directory") &&
                        (entry.contains("arguments") || entry.contains("command")), "not a compilation database");
        } catch (const std::exception&) {
            fail("output_conflict", "existing output is not a compilation database; choose another file");
        }
    }
    const auto temporary = fs::path(output.string() + ".tmp-" + std::to_string(getpid()) + "-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    int descriptor = open(temporary.string().c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (descriptor < 0) fail("output_error", "cannot stage compile database output");
    struct Cleanup {
        fs::path path;
        int& descriptor;
        ~Cleanup() { if (descriptor >= 0) close(descriptor); std::error_code ignored; fs::remove(path, ignored); }
    } cleanup{temporary, descriptor};
    const auto bytes = entries.dump(2) + "\n";
    size_t written = 0;
    while (written < bytes.size()) {
        auto count = write(descriptor, bytes.data() + written, bytes.size() - written);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) fail("output_error", "cannot write compile database output");
        written += static_cast<size_t>(count);
    }
    const auto closed = close(descriptor); descriptor = -1;
    if (closed != 0) fail("output_error", "cannot close compile database output");
    if (file_sha256(root / "crt.json") != info.at("project").at("manifest_sha256") ||
            file_sha256(root / "crt.lock") != info.at("lock").at("sha256"))
        fail("project_incompatible", "project inputs changed during export; retry");
    std::error_code error;
    fs::rename(temporary, output, error);
    if (error) fail("output_error", "cannot publish compile database: " + error.message());
}
}

Reply editor_command(const std::vector<std::string>& args) {
    const auto command = args.front();
    const bool capabilities = command == "capabilities";
    const bool exporting = command == "export";
    if (!capabilities && (args.size() < 2 || args[1] != (exporting ? "compile-commands" : "info")))
        fail("unsupported_command", "expected export compile-commands or project info", 2);
    std::map<std::string, std::string> options;
    for (size_t i = capabilities ? 1 : 2; i < args.size(); i += 2) {
        const auto& name = args[i];
        if (i + 1 >= args.size() || (name != "--api-version" && (capabilities ||
                (name != "--project" && (!exporting || name != "--output")))))
            fail("invalid_arguments", "unsupported or missing editor API option: " + name, 2);
        if (!options.emplace(name, args[i + 1]).second) fail("invalid_arguments", "duplicate option: " + name, 2);
    }
    if (options.count("--api-version") && options.at("--api-version") != "1")
        fail("unsupported_api_version", "editor API version is unsupported; discover capabilities and select version 1", 2);
    if (capabilities) {
        Json data{{"schema", "crt-cli.capabilities"}, {"format", 1}, {"api_versions", {1}}, {"version", "0.1.0-dev"},
            {"result", {{"schema", "crt-cli.bootstrap-result"}, {"format", 1}, {"stdout", "single-final-object"},
                        {"child_output", "stderr"}, {"job_events", false}}},
            {"commands", {
                {"project.info", {{"api_version", 1}, {"schema", "crt-cli.project-info"}, {"format", 1}, {"read_only", true}}},
                {"export.compile-commands", {{"api_version", 1}, {"schema", "crt-cli.compile-commands-export"}, {"format", 1},
                    {"database_format", "clang-json-compilation-database"}, {"arguments", true}, {"builds", false}}},
                {"sdk.list", {{"envelope_format", 1}, {"collection", "sdks"}}},
                {"port.list", {{"envelope_format", 1}, {"collection", "items"}}}}}};
        return {0, command, data, data.dump(2)};
    }
    if (exporting && !options.count("--output")) fail("invalid_arguments", "export compile-commands requires --output <file>", 2);
    std::error_code error;
    auto root = fs::canonical(options.count("--project") ? fs::path(options.at("--project")) : fs::current_path(), error);
    if (error || !fs::is_directory(root) || !fs::is_regular_file(root / "crt.json"))
        fail("project_not_found", "project directory must contain crt.json");
    auto info = inspect_project(root);
    auto entries = info.value("compile_commands", Json::array());
    info.erase("compile_commands");
    if (!exporting) return {0, command, info, info.dump(2)};
    if (info.at("state") != "ready") {
        const auto& diagnostic = info.at("diagnostics").at(0);
        fail(diagnostic.at("code"), diagnostic.at("message"));
    }
    auto output = fs::absolute(options.at("--output"));
    publish_database(output, root, info, entries);
    Json data{{"schema", "crt-cli.compile-commands-export"}, {"format", 1}, {"api_version", 1},
              {"output", output.string()}, {"entries", entries.size()},
              {"manifest_sha256", info.at("project").at("manifest_sha256")},
              {"lock_sha256", info.at("lock").at("sha256")}, {"selection", info.at("selection")}};
    return {0, command, data, "exported " + std::to_string(entries.size()) + " compile commands"};
}
}
