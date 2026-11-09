// Project integration of a published port (D3 step 5): a project chooses its zlib provider explicitly, and the proof that a
// build consumed the port is the path evidence of the link and of runtime resolution, not a content comparison (the SDK's
// zlib is produced by the same recipe). Environment-gated like port-store-test:
//   CRT_CLI_TEST_PORT_PROJECT, CRT_CLI_TEST_PORT_HOST_TOOLS, CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256, CRT_CLI_TEST_PORT_ARCHIVE
#include <crtcli/archive.h>
#include <crtcli/port_store.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
using namespace crtcli;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }
std::string getenv_or_empty(const char* name) { const char* v = std::getenv(name); return v ? v : ""; }
std::string error_of(const std::function<void()>& action) { try { action(); } catch (const Error& e) { return e.what(); } return {}; }

// Prints every distinct mapped file whose name contains "libz": the loader's own answer to "which libz is in this process".
const char* kMapsSource = R"(#include <stdio.h>
#include <string.h>
#include <zlib.h>
int main(void) {
  unsigned char in[4096], out[8192];
  uLongf n = sizeof(out);
  memset(in, 'a', sizeof(in));
  if (compress2(out, &n, in, sizeof(in), 6) != Z_OK) return 1;
  FILE* f = fopen("/proc/self/maps", "r");
  char line[4096], last[4096] = "";
  while (f && fgets(line, sizeof(line), f)) {
    char* path = strchr(line, '/');
    if (!path || !strstr(path, "libz")) continue;
    if (strcmp(path, last) != 0) { printf("maps: %s", path); strcpy(last, path); }
  }
  printf("zlib_maps: ok version=%s\n", zlibVersion());
  return 0;
}
)";

const char* kCxxSource = R"(#include <zlib.h>
#include <cstdio>
#include <string>
#include <vector>
int main() {
  std::string text(10000, 'x');
  std::vector<unsigned char> packed(compressBound(text.size()));
  uLongf size = packed.size();
  if (compress(packed.data(), &size, reinterpret_cast<const unsigned char*>(text.data()), text.size()) != Z_OK) return 1;
  std::string back(text.size(), '\0');
  uLongf back_size = back.size();
  if (uncompress(reinterpret_cast<unsigned char*>(&back[0]), &back_size, packed.data(), size) != Z_OK || back != text) return 1;
  std::printf("zlib_cxx: ok\n");
  return 0;
}
)";

std::string manifest_with(const std::string& targets) {
    return std::string("{\"manifest_format\":1,\"package\":{\"name\":\"zlib-consumer\",\"version\":\"0.1.0\"},"
                       "\"sdk\":{\"profile\":\"dev\",\"version\":\"*\",\"target\":\"native\",\"storage\":\"shared-immutable\"},"
                       "\"targets\":[") + targets + "]}\n";
}

Json workflow_in(const fs::path& project, const std::string& command, const std::vector<std::string>& more = {}) {
    std::vector<std::string> args{command, "--project", project.string()};
    args.insert(args.end(), more.begin(), more.end());
    auto reply = workflow(args);
    check(reply.code == 0, command + " succeeded");
    return reply.data;
}

fs::path make_project(const fs::path& scratch, const std::string& name, const fs::path& bound, const fs::path& sdk, const std::string& targets,
                      const fs::path& roundtrip) {
    auto project = scratch / name;
    fs::create_directories(project / "src");
    fs::create_directories(project / ".crt");
    fs::copy_file(bound / "crt.lock", project / "crt.lock");
    fs::copy_file(bound / ".crt/local.json", project / ".crt/local.json");
    write_text(project / "crt.json", manifest_with(targets));
    fs::copy_file(roundtrip, project / "src/zlib_roundtrip.c");
    write_text(project / "src/zlib_maps.c", kMapsSource);
    write_text(project / "src/zlib_cxx.cpp", kCxxSource);
    (void)sdk;
    return project;
}

std::string run_output(const fs::path& exe, const fs::path& sdk) {
    return execute({exe.string()}, 60, false, false, {(sdk / "bin").string(), (sdk / "lib").string()}).output;
}

// The last link command of `target` from a build receipt, with the artifact path swapped so re-running it does not touch the build output.
std::vector<std::string> link_command_for(const Json& receipt, const std::string& artifact, const std::string& replacement) {
    std::vector<std::string> found;
    for (const auto& command : receipt.at("commands")) {
        bool links = false;
        for (const auto& arg : command) links |= arg.get<std::string>() == artifact;
        bool compile = false;
        for (const auto& arg : command) compile |= arg.get<std::string>() == "-c";
        if (links && !compile) { found.clear(); for (const auto& arg : command) found.push_back(arg.get<std::string>() == artifact ? replacement : arg.get<std::string>()); }
    }
    return found;
}
}  // namespace

int main() {
    const auto bound = getenv_or_empty("CRT_CLI_TEST_PORT_PROJECT"), bundle = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS"),
               pin = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256"), archive = getenv_or_empty("CRT_CLI_TEST_PORT_ARCHIVE");
    if (bound.empty() || bundle.empty() || pin.empty() || archive.empty()) {
        std::printf("port-project-test: skipped (CRT_CLI_TEST_PORT_* not set)\n");
        return 0;
    }
    std::string here = __FILE__;
    auto recipe = parse_port_recipe(read_text(fs::path(here).parent_path() / "data" / "zlib.recipe.json"), "linux");
    const char* temporary = std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp";
    std::string pattern = std::string(temporary) + "/crt-port-project-XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    require(mkdtemp(buffer.data()) != nullptr, "cannot create a scratch directory");
    fs::path scratch = buffer.data();
    // The port store is CRT_HOME/ports, found through the PAL's user home.
    auto home = scratch / "home";
    fs::create_directories(home);
    setenv("HOME", home.c_str(), 1);
    check(sdk_store_root() == home / ".crt/sdks", "the SDK store root follows HOME (so the port store is " + (home / ".crt/ports").string() + ")");

    auto toolchain = Toolchain::load(bound);
    auto tools = HostTools::load(bundle, pin);
    auto sdk_before = read_identity(toolchain.sdk).content_sha256;
    auto roundtrip = toolchain.sdk / "porting/tests/zlib_roundtrip.c";

    // ---- a published zlib item -------------------------------------------------------------------------------------------
    auto pristine = scratch / "pristine";
    fs::create_directory(pristine);
    check(file_sha256(archive) == recipe.source.sha256, "the supplied archive is the recipe's pinned source");
    extract_archive(archive, pristine);
    PortBuildRequest request;
    request.recipe = recipe;
    request.source_tree = pristine;
    request.test_root = toolchain.sdk;
    request.toolchain = &toolchain;
    request.tools = &tools;
    request.job = scratch / "job";
    auto inputs = port_key_inputs(request);
    auto built = run_port_build(request);
    PortStore store(home / ".crt/ports");
    auto published = store.publish(request, built, inputs);
    const auto key = published.build_key;
    const auto item = published.path;
    check(item == home / ".crt/ports/store" / key, "the item is in CRT_HOME/ports/store/<key>");

    auto port_entry = [&](const std::string& extra = "") { return "\"ports\":[{\"name\":\"zlib\",\"provider\":\"port\",\"build_key\":\"" + key + "\"}]" + extra; };
    const std::string sdk_entry = "\"ports\":[{\"name\":\"zlib\",\"provider\":\"sdk\"}],\"sdk_libraries\":[\"z\"]";

    // ---- consumer with provider "port" ------------------------------------------------------------------------------------
    auto port_targets =
        "{\"name\":\"static-roundtrip\",\"sources\":[\"src/zlib_roundtrip.c\"]," + port_entry() + "},"
        "{\"name\":\"shared-roundtrip\",\"runtime_linkage\":\"shared\",\"sources\":[\"src/zlib_roundtrip.c\"]," + port_entry() + "},"
        "{\"name\":\"shared-maps\",\"runtime_linkage\":\"shared\",\"sources\":[\"src/zlib_maps.c\"]," + port_entry() + "},"
        "{\"name\":\"shared-cxx\",\"runtime_linkage\":\"shared\",\"sources\":[\"src/zlib_cxx.cpp\"]," + port_entry() + "},"
        "{\"name\":\"static-cxx\",\"sources\":[\"src/zlib_cxx.cpp\"]," + port_entry() + "}";
    auto project = make_project(scratch, "port-project", bound, toolchain.sdk, port_targets, roundtrip);
    auto receipt = workflow_in(project, "build");
    const auto out = project / "out/native";
    check(receipt.at("ports").size() == 1 && receipt.at("ports")[0].at("provider") == "port" && receipt.at("ports")[0].at("build_key") == key &&
          receipt.at("ports")[0].at("path") == item.string(), "the build receipt records the provider, key and item");

    for (const char* name : {"static-roundtrip", "shared-roundtrip"}) {
        auto text = run_output(out / name, toolchain.sdk);
        check(text.find(recipe.tests[0].expect_stdout) != std::string::npos, std::string(name) + " prints the recipe's expected line");
    }
    check(run_output(out / "static-cxx", toolchain.sdk).find("zlib_cxx: ok") != std::string::npos, "the static C++ consumer round-trips");
    check(run_output(out / "shared-cxx", toolchain.sdk).find("zlib_cxx: ok") != std::string::npos, "the shared C++ consumer round-trips");

    // Static: the link evidence (a linker map from the same command line) names the item's libz.a and not the SDK's.
    {
        auto command = link_command_for(receipt, (out / "static-roundtrip").string(), (scratch / "static-roundtrip.relinked").string());
        check(!command.empty(), "the static link command is in the receipt");
        auto map = scratch / "static.map";
        command.push_back("-Wl,-Map=" + map.string());
        auto result = execute(command, 120, false, true);
        check(result.exit_code == 0, "the static consumer relinks with a linker map (" + result.output + ")");
        auto text = read_text(map);
        check(text.find((item / "lib/libz.a").string()) != std::string::npos, "the link map names the item's libz.a");
        check(text.find((toolchain.sdk / "lib/libz.a").string()) == std::string::npos, "...and not the SDK's libz.a");
    }
    // Headers: the compiler's own dependency file shows which zlib.h each translation unit included.
    {
        std::string depfiles;
        for (const auto& entry : fs::directory_iterator(out))
            if (entry.path().extension() == ".d" && entry.path().filename().string().rfind("static-roundtrip", 0) == 0) depfiles += read_text(entry.path());
        check(depfiles.find((item / "include/zlib.h").string()) != std::string::npos, "the compiler included the item's zlib.h");
        check(depfiles.find((toolchain.sdk / "include/zlib.h").string()) == std::string::npos, "...and not the SDK's zlib.h");
    }
    // Shared: the process's own memory map (loader resolution), not a hash.
    {
        auto text = run_output(out / "shared-maps", toolchain.sdk);
        check(text.find("zlib_maps: ok") != std::string::npos, "the shared consumer runs");
        check(text.find((item / "lib/libz.so.1.3.1").string()) != std::string::npos, "the loader mapped the item's libz.so.1 (" + text + ")");
        check(text.find((toolchain.sdk / "lib").string()) == std::string::npos, "...and no libz from the SDK");
    }
    // The build is repeatable without rework.
    {
        auto again = workflow_in(project, "build");
        check(again.at("cache").at("executed") == 0, "an unchanged rebuild executes nothing");
        auto ran = workflow({"run", "--project", project.string(), "--target", "shared-roundtrip"});
        check(ran.code == 0, "run works for a port consumer");
    }

    // ---- control: provider "sdk" ----------------------------------------------------------------------------------------------
    {
        auto sdk_targets =
            "{\"name\":\"static-roundtrip\",\"sources\":[\"src/zlib_roundtrip.c\"]," + sdk_entry + "},"
            "{\"name\":\"shared-maps\",\"runtime_linkage\":\"shared\",\"sources\":[\"src/zlib_maps.c\"]," + sdk_entry + "}";
        auto control = make_project(scratch, "sdk-project", bound, toolchain.sdk, sdk_targets, roundtrip);
        auto control_receipt = workflow_in(control, "build");
        check(control_receipt.at("ports").size() == 1 && control_receipt.at("ports")[0].at("provider") == "sdk" && !control_receipt.at("ports")[0].contains("path"),
              "the receipt records the SDK provider");
        auto command = link_command_for(control_receipt, (control / "out/native/static-roundtrip").string(), (scratch / "control.relinked").string());
        auto map = scratch / "control.map";
        command.push_back("-Wl,-Map=" + map.string());
        check(execute(command, 120, false, true).exit_code == 0, "the control relinks with a map");
        auto text = read_text(map);
        check(text.find((toolchain.sdk / "lib/libz.a").string()) != std::string::npos && text.find((item / "lib").string()) == std::string::npos,
              "the control's map names the SDK's libz.a: the evidence discriminates between the providers");
        // Observed, not endorsed: a consumer linked to the SDK's *shared* libz carries only `$ORIGIN` as its runpath, so on this
        // Linux host the loader resolves libz.so.1 through the host's cache (/usr/lib...), i.e. not the SDK's. The control only
        // needs to show that the process maps no libz from the item.
        auto maps = run_output(control / "out/native/shared-maps", toolchain.sdk);
        check(maps.find("zlib_maps: ok") != std::string::npos && maps.find(item.string()) == std::string::npos,
              "the control's process maps no libz from the item: " + maps);
        std::printf("port-project-test: SDK-provider shared control resolved: %s", maps.c_str());
    }

    // ---- refusals ----------------------------------------------------------------------------------------------------------
    auto refuse = [&](const std::string& name, const std::string& targets, const std::string& expected) {
        auto refused = make_project(scratch, name, bound, toolchain.sdk, targets, roundtrip);
        auto message = error_of([&] { workflow({"build", "--project", refused.string()}); });
        check(message.find(expected) != std::string::npos, name + ": " + expected + " (got: " + message + ")");
        check(!fs::exists(refused / "out/native/refused"), name + ": nothing was built for the refused project");
    };
    refuse("both-providers", "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"]," + port_entry(",\"sdk_libraries\":[\"z\"]") + "}", "reaches both");
    refuse("both-via-dependency",
           "{\"name\":\"libz-user\",\"kind\":\"static-library\",\"sources\":[\"src/zlib_cxx.cpp\"],\"sdk_libraries\":[\"z\"]},"
           "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"],\"dependencies\":[\"libz-user\"]," + port_entry() + "}", "reaches both");
    refuse("two-providers",
           "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"]," + port_entry() + "},"
           "{\"name\":\"other\",\"sources\":[\"src/zlib_maps.c\"],\"ports\":[{\"name\":\"zlib\",\"provider\":\"sdk\"}]}", "one provider per dependency");
    refuse("two-keys",
           "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"]," + port_entry() + "},"
           "{\"name\":\"other\",\"sources\":[\"src/zlib_maps.c\"],\"ports\":[{\"name\":\"zlib\",\"provider\":\"port\",\"build_key\":\"" + std::string(64, 'a') + "\"}]}", "one provider per dependency");
    refuse("unknown-port", "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"],\"ports\":[{\"name\":\"nonesuch\",\"provider\":\"sdk\"}]}", "unknown port");
    refuse("bad-provider", "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"],\"ports\":[{\"name\":\"zlib\",\"provider\":\"system\"}]}", "provider must be");
    refuse("no-key", "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"],\"ports\":[{\"name\":\"zlib\",\"provider\":\"port\"}]}", "requires build_key");
    refuse("short-key", "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"],\"ports\":[{\"name\":\"zlib\",\"provider\":\"port\",\"build_key\":\"abc\"}]}", "requires build_key");
    refuse("sdk-with-key", "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"],\"ports\":[{\"name\":\"zlib\",\"provider\":\"sdk\",\"build_key\":\"" + key + "\"}]}", "takes no build_key");
    refuse("unknown-key", "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"],\"ports\":[{\"name\":\"zlib\",\"provider\":\"port\",\"build_key\":\"" + std::string(64, 'b') + "\"}]}", "no such port store item");
    refuse("extra-field", "{\"name\":\"refused\",\"sources\":[\"src/zlib_roundtrip.c\"],\"ports\":[{\"name\":\"zlib\",\"provider\":\"sdk\",\"path\":\"/usr\"}]}", "unknown port declaration key");

    // A damaged item is refused at build time, not linked.
    {
        auto header = item / "include/zlib.h";
        auto original = read_text(header);
        write_text(header, original + "/* tampered */\n");
        auto damaged = make_project(scratch, "damaged", bound, toolchain.sdk, port_targets, roundtrip);
        auto message = error_of([&] { workflow({"build", "--project", damaged.string()}); });
        check(message.find("differ from its manifest inventory") != std::string::npos, "a changed item is refused by the build (" + message + ")");
        write_text(header, original);
        check(store.verify(key, toolchain.sdk) > 0, "the restored item verifies");
    }

    // ---- crt.lock pins -----------------------------------------------------------------------------------------------------
    {
        auto pinned = make_project(scratch, "pinned", bound, toolchain.sdk, port_targets, roundtrip);
        auto lock_path = pinned / "crt.lock";
        auto lock_text = [&] { return read_text(lock_path); };
        auto run_lock = [&](const fs::path& p) { return port_lock(std::vector<std::string>{"port", "lock", "--project", p.string()}); };
        auto first = workflow_in(pinned, "build");
        check(first.at("ports")[0].at("pinned") == false, "without pins the receipt says the port is not pinned");
        check(!read_json(lock_path).contains("ports"), "...and the lock carries no pins");

        auto locked = run_lock(pinned);
        check(locked.code == 0 && locked.data.at("pins").size() == 1, "port lock pins the one provider-'port' dependency");
        auto pin = read_json(lock_path).at("ports")[0];
        check(pin.at("name") == "zlib" && pin.at("build_key") == key && pin.at("manifest_sha256") == file_sha256(item / "manifest.json") &&
              pin.at("sdk_content_sha256") == toolchain.selection.at("sdk_content_sha256"), "the pin names the key, the manifest digest and the SDK identity");
        check(read_json(lock_path).at("selection") == toolchain.selection, "the SDK selection in the lock is untouched");
        auto again = workflow_in(pinned, "build");
        check(again.at("ports")[0].at("pinned") == true && again.at("cache").at("executed") == 0, "a pinned build is accepted and reuses its outputs");
        auto before = lock_text();
        check(run_lock(pinned).code == 0 && lock_text() == before, "locking again is idempotent");

        // The configure that keeps the same SDK keeps the pins; one that selects another drops them and says so.
        auto configured = workflow_in(pinned, "configure");
        check(configured.at("port_pins") == "kept" && read_json(lock_path).contains("ports"), "configure keeps pins for the same SDK");
        check(lock_text() == before, "...and rewrites the lock byte-identically");

        auto refused_build = [&](const std::string& expected, const std::string& why) {
            auto message = error_of([&] { workflow({"build", "--project", pinned.string()}); });
            check(message.find(expected) != std::string::npos, why + " (got: " + message + ")");
        };
        auto edit = [&](const std::function<void(Json&)>& change) { auto lock = read_json(lock_path); change(lock); write_text(lock_path, lock.dump(2) + "\n"); };
        edit([](Json& lock) { lock["ports"][0]["manifest_sha256"] = std::string(64, '0'); });
        refused_build("differs from the crt.lock pin", "a changed manifest digest is refused");
        write_text(lock_path, before);
        edit([](Json& lock) { lock["ports"][0]["build_key"] = std::string(64, 'c'); });
        refused_build("crt.lock pins port 'zlib' to key", "a pin for another key is refused");
        write_text(lock_path, before);
        edit([](Json& lock) { lock["ports"][0]["sdk_content_sha256"] = std::string(64, 'd'); });
        refused_build("for another SDK", "a pin for another SDK is refused");
        write_text(lock_path, before);
        edit([](Json& lock) { lock["ports"] = Json::array(); });
        refused_build("does not pin it", "an empty pin list does not silently unpin");
        write_text(lock_path, before);
        edit([](Json& lock) { auto extra = lock["ports"][0]; extra["name"] = "other"; lock["ports"].push_back(extra); });
        refused_build("no target declares", "a pin for an undeclared port is refused");
        write_text(lock_path, before);
        edit([](Json& lock) { lock["ports"].push_back(lock["ports"][0]); });
        refused_build("twice", "a duplicated pin is refused");
        write_text(lock_path, before);
        edit([](Json& lock) { lock["ports"][0]["extra"] = 1; });
        refused_build("must be an object with name", "an unknown pin field is refused");
        write_text(lock_path, before);
        edit([](Json& lock) { lock["ports"] = "zlib"; });
        refused_build("must be an array", "a malformed pin list is refused");
        write_text(lock_path, before);

        // A consistently rewritten item (same key, different bytes) passes the item's own verification but not the pin.
        {
            auto manifest_path = item / "manifest.json";
            auto original = read_text(manifest_path);
            auto manifest = Json::parse(original);
            manifest["note"] = "rewritten";
            write_text(manifest_path, manifest.dump(2) + "\n");
            refused_build("differs from the crt.lock pin", "a rewritten item manifest is caught by the pin, not only by verify");
            write_text(manifest_path, original);
        }
        check(workflow_in(pinned, "build").at("ports")[0].at("pinned") == true, "the restored pin is accepted again");

        // Pins follow the SDK identity: a lock whose pins name another SDK is dropped by configure, visibly.
        edit([](Json& lock) { lock["ports"][0]["sdk_content_sha256"] = std::string(64, 'e'); });
        auto dropped = workflow_in(pinned, "configure");
        check(dropped.at("port_pins").get<std::string>().rfind("dropped", 0) == 0 && !read_json(lock_path).contains("ports"), "configure drops pins for another SDK and says so");

        // A project with no provider-'port' dependency has nothing to pin and an existing pin list is removed.
        edit([&](Json& lock) { lock["ports"] = Json::array({pin}); });
        auto sdk_only = make_project(scratch, "pin-sdk-only", bound, toolchain.sdk,
            "{\"name\":\"sdk-only\",\"sources\":[\"src/zlib_roundtrip.c\"],\"ports\":[{\"name\":\"zlib\",\"provider\":\"sdk\"}],\"sdk_libraries\":[\"z\"]}", roundtrip);
        edit([&](Json& lock) { (void)lock; });
        write_text(sdk_only / "crt.lock", [&] { auto l = read_json(sdk_only / "crt.lock"); l["ports"] = Json::array({pin}); return l.dump(2) + "\n"; }());
        auto message = error_of([&] { workflow({"build", "--project", sdk_only.string()}); });
        check(message.find("no target declares") != std::string::npos, "a pin in a project that uses the SDK's zlib is refused (" + message + ")");
        check(run_lock(sdk_only).data.at("pins").empty() && !read_json(sdk_only / "crt.lock").contains("ports"), "port lock removes pins the project no longer declares");
        check(workflow_in(sdk_only, "build").at("ports")[0].at("provider") == "sdk", "...and the project builds again");
        check(!error_of([&] { port_lock(std::vector<std::string>{"port", "lock", "--bogus"}); }).empty(), "port lock refuses unknown arguments");
    }

    check(read_identity(toolchain.sdk).content_sha256 == sdk_before, "the SDK is byte-identical after the whole run");
    std::error_code ignored;
    if (!std::getenv("KEEP")) fs::remove_all(scratch, ignored);
    std::printf("port-project-test: %zu checks passed\n", checks);
    return 0;
}
