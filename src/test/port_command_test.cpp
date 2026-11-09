// `crt-cli port build|list|verify|recover` (D3): the command builds the SDK-shipped zlib recipe into the port store and
// reports the declaration a project needs. Environment-gated like the other port tests:
//   CRT_CLI_TEST_PORT_PROJECT, CRT_CLI_TEST_PORT_HOST_TOOLS, CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256, CRT_CLI_TEST_PORT_ARCHIVE
#include <crtcli/port_store.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
using namespace crtcli;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }
std::string getenv_or_empty(const char* name) { const char* v = std::getenv(name); return v ? v : ""; }
std::string error_of(const std::function<void()>& action) { try { action(); } catch (const Error& e) { return e.what(); } return {}; }
}  // namespace

int main() {
    const auto project = getenv_or_empty("CRT_CLI_TEST_PORT_PROJECT"), bundle = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS"),
               pin = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256"), archive = getenv_or_empty("CRT_CLI_TEST_PORT_ARCHIVE");
    if (project.empty() || bundle.empty() || pin.empty() || archive.empty()) {
        std::printf("port-command-test: skipped (CRT_CLI_TEST_PORT_* not set)\n");
        return 0;
    }
    const char* temporary = std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp";
    std::string pattern = std::string(temporary) + "/crt-port-command-XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    require(mkdtemp(buffer.data()) != nullptr, "cannot create a scratch directory");
    fs::path scratch = buffer.data();
    auto home = scratch / "home";
    fs::create_directories(home);
    setenv("HOME", home.c_str(), 1);
    auto ports = home / ".crt/ports";
    auto toolchain = Toolchain::load(project);
    auto sdk_before = inventory(toolchain.sdk);

    auto build = [&](std::vector<std::string> more) {
        std::vector<std::string> args{"port", "build", "zlib", "--archive", archive, "--host-tools", bundle, "--host-tools-sha256", pin, "--project", project};
        args.insert(args.end(), more.begin(), more.end());
        return port_command(args);
    };
    // (a braced list of exactly two string literals would be taken for an iterator range, so callers spell the vector out)
    auto build_error = [&](std::vector<std::string> args) { return error_of([&] { port_command(args); }); };

    // ---- refusals leave no store, no work directory -----------------------------------------------------------------------
    check(build_error(std::vector<std::string>{"port"}).find("usage:") == 0, "no action prints the usage");
    check(build_error(std::vector<std::string>{"port", "build"}).find("usage:") == 0, "build without a name prints the usage");
    check(build_error({"port", "build", "zlib", "--archive", archive}).find("requires --host-tools") != std::string::npos, "missing host tools are named");
    check(build_error({"port", "build", "../zlib", "--archive", archive, "--host-tools", bundle, "--host-tools-sha256", pin}).find("invalid port name") != std::string::npos, "a path-like port name is refused");
    check(build_error({"port", "build", "nosuchport", "--archive", archive, "--host-tools", bundle, "--host-tools-sha256", pin, "--project", project}).find("no recipe for 'nosuchport'") != std::string::npos, "an unknown port is refused");
    check(build_error({"port", "build", "zlib", "--archive", archive, "--host-tools", bundle, "--host-tools-sha256", "ABC", "--project", project}).find("64 lowercase hex") != std::string::npos, "a bad host-tools pin is refused");
    check(build_error({"port", "build", "zlib", "--bogus", "x"}).find("usage:") == 0, "an unknown option prints the usage");
    check(build_error({"port", "build", "zlib", "--archive", archive, "--archive", archive}).find("duplicate port option") != std::string::npos, "a duplicate option is refused");
    check(build_error({"port", "build", "zlib", "--archive", archive, "--host-tools", bundle, "--host-tools-sha256", pin, "--project", project, "--jobs", "0"}).find("--jobs must be 1..256") != std::string::npos, "--jobs 0 is refused");
    {
        auto other = scratch / "other.tar.gz";
        std::ofstream(other) << "not the pinned source";
        auto message = build_error({"port", "build", "zlib", "--archive", other.string(), "--host-tools", bundle, "--host-tools-sha256", pin, "--project", project});
        check(message.find("not the recipe's pinned source") != std::string::npos, "an archive other than the pinned source is refused (" + message + ")");
    }
    check(!fs::exists(ports), "refused builds created no port store");
    check(port_command(std::vector<std::string>{"port", "list"}).data.at("items").empty(), "list on an empty store is empty");

    // ---- a failing build keeps its job directory and publishes nothing ---------------------------------------------------
    {
        auto message = build_error({"port", "build", "zlib", "--archive", archive, "--host-tools", bundle, "--host-tools-sha256", pin, "--project", project, "--timeout", "1"});
        check(message.find("(job kept at ") != std::string::npos, "a failed build names its kept job directory (" + message.substr(0, 200) + ")");
        check(port_command(std::vector<std::string>{"port", "list"}).data.at("items").empty(), "...and published nothing");
        std::error_code ignored;
        fs::remove_all(ports / "work", ignored);
    }

    // ---- a real build ----------------------------------------------------------------------------------------------------
    auto reply = build({"--jobs", "4"});
    check(reply.code == 0 && reply.data.at("already_published") == false, "the build published a new item");
    auto key = reply.data.at("build_key").get<std::string>();
    check(key.size() == 64, "the reply carries the build key");
    check(reply.data.at("declaration") == (Json{{"name", "zlib"}, {"provider", "port"}, {"build_key", key}}), "the reply holds the project declaration");
    check(reply.data.at("tests").size() == 2 && reply.data.at("tests")[0].at("passed") == true, "both recipe tests passed");
    check(reply.data.at("libraries").size() >= 1, "the audit reported the item's libraries");
    check(fs::is_directory(ports / "store" / key) && fs::is_regular_file(ports / "store" / key / "manifest.json"), "the item is in the store");
    check(fs::is_directory(ports / "work") && fs::is_empty(ports / "work"), "the successful build removed its work directory");
    check(reply.message.find(key) != std::string::npos, "the text output names the key");

    // ---- the same inputs are not rebuilt ---------------------------------------------------------------------------------
    auto manifest_before = file_sha256(ports / "store" / key / "manifest.json");
    auto again = build({});
    check(again.data.at("already_published") == true && again.data.at("build_key") == key, "an identical request finds the item");
    check(file_sha256(ports / "store" / key / "manifest.json") == manifest_before, "...and does not touch it");
    check(fs::is_empty(ports / "work"), "...and builds nothing");

    // ---- list / verify / recover -----------------------------------------------------------------------------------------
    auto listed = port_command(std::vector<std::string>{"port", "list"}).data.at("items");
    check(listed.size() == 1 && listed[0] == key, "list names the item");
    check(port_command({"port", "verify", key, "--project", project}).data.at("inventory_entries").get<uint64_t>() > 5, "verify re-checks the inventory");
    check(build_error({"port", "verify", "xyz", "--project", project}).find("64 lowercase hex") != std::string::npos, "verify refuses a malformed key");
    check(!build_error({"port", "verify", std::string(64, 'a'), "--project", project}).empty(), "verify refuses a key that is not in the store");
    check(port_command(std::vector<std::string>{"port", "recover"}).data.at("recovered").empty(), "recover on a clean store removes nothing");
    {
        std::ofstream(ports / "store" / key / "include/extra.h") << "tamper";
        check(!build_error({"port", "verify", key, "--project", project}).empty(), "verify detects an added file");
        check(!build_error({"port", "build", "zlib", "--archive", archive, "--host-tools", bundle, "--host-tools-sha256", pin, "--project", project}).empty(), "a damaged item is reported, not rebuilt over");
    }
    check(inventory(toolchain.sdk) == sdk_before, "the SDK is unchanged");

    std::error_code ignored;
    fs::remove_all(scratch, ignored);
    std::printf("port-command-test: %zu checks passed\n", checks);
    return 0;
}
