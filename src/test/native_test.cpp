#include <crtcli/core.h>
#include <crtcli/environment.h>
#include <crt/platform.h>
#include <algorithm>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <spawn.h>
#include <cstdlib>
#include <map>
#include <atomic>
#include <chrono>
#include <exception>
#include <thread>
#include <mutex>
using namespace crtcli;
namespace {
std::atomic<size_t> checks{0};
thread_local size_t worker_checks = 0;
std::mutex phase_mutex;
std::vector<Json> phases;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; ++worker_checks; }
struct NativePhase {
    std::string name;
    size_t before = worker_checks;
    int exceptions = std::uncaught_exceptions();
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    explicit NativePhase(std::string value) : name(std::move(value)) {}
    ~NativePhase() {
        Json data{{"name", name}, {"checks", worker_checks - before},
                  {"seconds", std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count()},
                  {"completed", std::uncaught_exceptions() == exceptions}};
        std::lock_guard<std::mutex> guard(phase_mutex);
        phases.push_back(std::move(data));
    }
};
void report_phases() {
    for (const auto& phase : phases) std::fprintf(stderr, "native-phase: %s\n", phase.dump().c_str());
}
// Only independent roots run here. Environment and store mutation stay on the joining thread outside this function.
template<class Work> void parallel_cases(const std::string& group, const std::vector<std::string>& cases, unsigned jobs, Work work) {
    std::atomic<size_t> next{0};
    std::vector<std::exception_ptr> errors(cases.size());
    auto worker = [&]() {
        for (;;) {
            size_t index = next++;
            if (index >= cases.size()) return;
            try { NativePhase phase(group + "/" + cases[index]); work(cases[index]); }
            catch (...) { errors[index] = std::current_exception(); }
        }
    };
    std::vector<std::thread> threads;
    try {
        for (unsigned i = 1; i < std::min<size_t>(jobs, cases.size()); ++i) threads.emplace_back(worker);
    } catch (...) {
        for (auto& thread : threads) thread.join();
        throw;
    }
    worker();
    for (auto& thread : threads) thread.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
}
void portable_vendor_paths(Json& value, const std::string& location) {
    if (value.is_array() || value.is_object()) {
        for (auto& child : value) portable_vendor_paths(child, location);
    } else if (value.is_string()) {
        auto text = value.get<std::string>();
        size_t position = 0;
        while ((position = text.find(location, position)) != std::string::npos) {
            text.replace(position, location.size(), "@OS_SDK@"); position += 8;
        }
        value = text;
    }
}
Json invoke(const std::string& cli, std::vector<std::string> args, bool success = true) {
    args.insert(args.begin(), "--json"); args.insert(args.begin(), cli);
    auto child = execute(args, 120, false);
    auto reply = Json::parse(child.output);
    check((child.exit_code == 0) == success, "expected command exit: " + args.at(2) + " (exit " + std::to_string(child.exit_code) +
          ", " + reply.at("data").value("message", std::string("no failure diagnostic")) + ")");
    check(reply.at("ok").get<bool>() == success && reply.at("schema") == "crt-cli.bootstrap-result", "JSON outcome");
    return reply.at("data");
}
}
int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--environment-probe") {
        Json values = Json::object();
        for (char** entry = environ; *entry; ++entry) {
            std::string value(*entry);
            auto equals = value.find('=');
            if (equals != std::string::npos) values[value.substr(0, equals)] = value.substr(equals + 1);
        }
        std::printf("%s\n", values.dump().c_str());
        return 0;
    }
    if (argc == 3 && std::string(argv[1]) == "--tree-child") { sleep(2); write_text(argv[2], "descendant ran\n"); return 0; }
    if (argc == 3 && (std::string(argv[1]) == "--tree-parent" || std::string(argv[1]) == "--tree-exit")) {
        char* child_args[] = {argv[0], const_cast<char*>("--tree-child"), argv[2], nullptr};
        pid_t child;
        if (posix_spawn(&child, argv[0], nullptr, nullptr, child_args, environ) != 0) return 17;
        if (std::string(argv[1]) == "--tree-parent") sleep(10);
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--cwd-probe") { std::printf("%s", fs::current_path().string().c_str()); return 0; }
    if (argc == 2 && std::string(argv[1]) == "--sleeper") { sleep(10); return 0; }
    if (argc == 2 && std::string(argv[1]) == "--flood") {
        std::string block(65536, 'x');
        for (int i = 0; i < 160; ++i) std::fwrite(block.data(), 1, block.size(), stdout);
        return 0;
    }
    fs::path fixture;
    try {
        require(argc == 3, "test needs CLI executable and source project");
        std::string cli = fs::canonical(argv[1]).string();
        fs::path project_root = fs::canonical(argv[2]);
        unsigned jobs = 1;
        const auto settings_path = project_root / ".crt/native-test-settings.json";
        if (fs::exists(settings_path)) {
            auto settings = read_json(settings_path);
            require(settings.is_object() && settings.size() == 2 && settings.at("format") == 0 &&
                    settings.at("jobs").is_number_integer() && settings.at("jobs") >= 1 && settings.at("jobs") <= 4,
                    "native test settings require format 0 and jobs between 1 and 4");
            jobs = settings.at("jobs").get<unsigned>();
        }
        {
        NativePhase phase("startup");
        check(sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA256 vector");
        auto raw = [](std::initializer_list<int> values) { std::string bytes; for (int value : values) bytes += static_cast<char>(value); return bytes; };
        check(valid_utf8("") && valid_utf8("space 한글 é") && valid_utf8(raw({0xf0, 0x9f, 0x98, 0x80})) && !valid_utf8(raw({0xff})) && !valid_utf8(raw({0xc0, 0x80})) &&
              !valid_utf8(raw({0xed, 0xa0, 0x80})) && !valid_utf8(raw({0xf4, 0x90, 0x80, 0x80})) && !valid_utf8(raw({0xe2, 0x82})) && !valid_utf8(raw({0x80})), "strict UTF-8 validation");
        check(std::string(crt_get_narrow_encoding()) == "utf-8", "the test program runs with UTF-8 narrow strings");
        invoke(cli, {"--help"}); invoke(cli, {"--version"});
        auto doctor = invoke(cli, {"doctor", "--bootstrap"});
        check(doctor.at("checks").at("cxx_allocation_owner") == "pass", "CLI owns its C++ allocation functions");
        check(doctor.at("checks").at("executable_integrity") == "not_verified", "honest integrity result");
        check(doctor.at("checks").at("narrow_encoding") == "pass" && doctor.at("narrow_encoding") == "utf-8", "UTF-8 narrow strings");
        check(doctor.at("checks").at("https_network") == "not_run", "honest network result");
        check(doctor.at("runtime_maps").at("status") == "collected", "PAL image enumeration");
        check(fs::path(doctor.at("sdk_store").get<std::string>()).filename() == "sdks", "common SDK store basename");
        check(invoke(cli, {"doctor"}).at("scope") == "self", "default SDK-free self doctor");
        for (const auto& args : std::vector<std::vector<std::string>>{{"upgrade"}, {"doctor", "--scope", "unknown"}, {"doctor", "--scope", "environment"}, {"--version", "extra"}, {"--json"}, {"new"}, {"build", "--bad"}, {"configure", "--project"}})
            invoke(cli, args, false);
        // fetch validates every input before any network use: plain HTTP, URL credentials, a malformed digest or size and
        // a missing trust file are all rejected without a connection attempt.
        const std::string zero_digest(64, '0');
        const std::vector<std::string> fetch_base{"fetch", "--url", "https://localhost:9/m.json", "--ca-file", cli, "--output", cli + ".fetched", "--sha256", zero_digest, "--size", "1"};
        auto with = [&](const std::string& option, const std::string& value) {
            auto args = fetch_base;
            for (size_t i = 1; i + 1 < args.size(); i += 2) if (args[i] == option) args[i + 1] = value;
            return args;
        };
        for (const auto& args : {std::vector<std::string>{"fetch"}, with("--url", "http://localhost:9/m.json"), with("--url", "https://user:secret@localhost:9/m.json"),
                                 with("--url", "https:///m.json"), with("--sha256", std::string(64, 'A')), with("--sha256", "abc"), with("--size", "0"),
                                 with("--size", "-1"), with("--ca-file", cli + ".missing"), with("--output", cli)})
            invoke(cli, args, false);
        // Redirect options are validated before any connection too: both lists, an unknown policy, non-canonical authorities,
        // the default port spelled out, wildcards, empty entries and hop counts outside 1..5 or without an allow-list.
        for (const auto& extra : std::vector<std::vector<std::string>>{
                 {"--redirect-policy", "github-release", "--redirect-hosts", "a.example"}, {"--redirect-policy", "other"},
                 {"--redirect-hosts", "Upper.example"}, {"--redirect-hosts", "a.example:443"}, {"--redirect-hosts", "*.example"},
                 {"--redirect-hosts", "a.example,,b.example"}, {"--redirect-hosts", "a.example:0"}, {"--max-redirects", "2"},
                 {"--redirect-hosts", "a.example", "--max-redirects", "0"}, {"--redirect-hosts", "a.example", "--max-redirects", "6"}}) {
            auto args = fetch_base;
            args.insert(args.end(), extra.begin(), extra.end());
            invoke(cli, args, false);
        }
        check(!fs::exists(cli + ".fetched"), "a rejected fetch publishes nothing");
        for (const auto& option : {"--publisher-key", "--signature"}) {
            auto args = fetch_base;
            args.insert(args.end(), {option, cli});
            invoke(cli, args, false);
        }
        auto invalid_key = fetch_base;
        invalid_key.insert(invalid_key.end(), {"--publisher-key", cli, "--signature", cli});
        invoke(cli, invalid_key, false);
        }
        // The temporary root comes from the environment (TMPDIR, then TEMP); a literal /tmp does not exist everywhere.
        const char* temporary_root = std::getenv("TMPDIR") ? std::getenv("TMPDIR") : std::getenv("TEMP") ? std::getenv("TEMP") : "/tmp";
        std::string temporary = (fs::path(temporary_root) / "crt-cli-native-XXXXXX").string();
        auto location = mkdtemp(temporary.data());
        require(location != nullptr, "mkdtemp failed");
        fixture = fs::path(location);
        // Windows narrow strings are UTF-8 because the SDK's link policy embeds the active-code-page manifest (doctor reports it);
        // the Windows loader also splits its DLL search path at ';', so a program importing a DLL beside it cannot start from a
        // directory whose name contains one: the semicolon is exercised on the other hosts only.
        const bool windows_host = std::string(crt_get_platform_info()->os) == "windows";
        auto hostile = fixture / (windows_host ? "space 한글 é @ $(touch never-execute)" : "space 한글 @ ; $(touch never-execute)");
        fs::create_directories(hostile);
        {
        NativePhase phase("process");
        // Compare the actual spawned environment with the shared builder while the parent carries hostile inputs.
        std::map<std::string, Json> saved;
        for (const auto& name : {"CRT_CLI_TEST_SECRET", "CPATH", "LIBRARY_PATH", "LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH"}) {
            const char* old = std::getenv(name);
            saved[name] = old ? Json(old) : Json(nullptr);
            require(setenv(name, "must-not-reach-child", 1) == 0, "set environment sentinel");
        }
        for (bool host_tool : {false, true}) {
            // A CRT child must use CRT descriptor propagation on Windows; the native-host hint is for vendor tools.
            if (windows_host && host_tool) {
                check(child_environment(true).fixed.at("CRT_SPAWN_NATIVE_WINDOWS") == "1", "native Windows host-tool spawn hint");
                continue;
            }
            auto expected = child_environment(host_tool);
            auto child = execute({fs::canonical(argv[0]).string(), "--environment-probe"}, 30, false, host_tool);
            Json actual = Json::parse(child.output), wanted = Json::object();
            for (const auto& value : expected.values) {
                auto equals = value.find('=');
                wanted[value.substr(0, equals)] = value.substr(equals + 1);
            }
            // On Windows the OS itself adds PROCESSOR_ARCHITECTURE to a new process whose environment lacks it (measured on
            // Windows/arm64 with a kernel32-only program that includes no CRT, so it is not the CLI's or CRT's doing). Only
            // that one name is tolerated, and only with the value for this architecture; anything else is still an error.
            if (windows_host && actual.is_object() && actual.contains("PROCESSOR_ARCHITECTURE")) {
                const std::string architecture = crt_get_platform_info()->arch;
                check(actual.at("PROCESSOR_ARCHITECTURE") == (architecture == "aarch64" ? "ARM64" : architecture == "x86_64" ? "AMD64" : "?") &&
                      !wanted.contains("PROCESSOR_ARCHITECTURE"), "OS-added PROCESSOR_ARCHITECTURE matches the host architecture");
                actual.erase("PROCESSOR_ARCHITECTURE");
            }
            check(child.exit_code == 0 && actual == wanted, "spawn environment exactly matches shared builder");
            check(actual.at("PATH") == "/nonexistent" && actual.at("LC_ALL") == "C", "fixed PATH and locale");
            for (const auto& entry : saved) check(!actual.contains(entry.first), "injection and credential sentinel removed");
        }
        auto environment = invoke(cli, {"env", "--profile", "control"});
        check(environment.at("environment_format") == 0 && environment.at("fixed") == child_environment().fixed &&
              environment.at("inherited_names") == child_environment().inherited_names && !environment.contains("selection"), "SDK-free control diagnostic parity");
        check(environment.dump().find("must-not-reach-child") == std::string::npos, "diagnostic does not expose ambient values");
        for (const auto& entry : saved) {
            if (entry.second.is_null()) unsetenv(entry.first.c_str());
            else setenv(entry.first.c_str(), entry.second.get<std::string>().c_str(), 1);
        }
        for (const auto& args : std::vector<std::vector<std::string>>{{"env"}, {"env", "--profile"}, {"env", "--profile", "unknown"},
             {"env", "--profile", "port-build"}, {"env", "--profile", "control", "--project", fixture.string()},
             {"env", "--profile", "control", "--profile", "app-run"}, {"env", "--profile", "control", "--bad", "x"}})
            invoke(cli, args, false);
        write_text(fixture / "duplicate.json", "{\"x\":1,\"x\":2}");
        bool rejected = false;
        try { read_json(fixture / "duplicate.json"); } catch (const Error&) { rejected = true; }
        check(rejected, "duplicate JSON keys");
        auto timeout = execute({fs::canonical(argv[0]).string(), "--sleeper"}, 1, false);
        check(timeout.timed_out && timeout.exit_code == 124, "owned process timeout");
        auto marker = fixture / "descendant-marker";
        auto control = execute({fs::canonical(argv[0]).string(), "--tree-parent", marker.string()}, 4, false);
        check(control.timed_out && fs::exists(marker), "descendant control actually executes before a later timeout");
        fs::remove(marker);
        auto tree = execute({fs::canonical(argv[0]).string(), "--tree-parent", marker.string()}, 1, false);
        sleep(3);
        check(tree.timed_out && tree.exit_code == 124 && !fs::exists(marker), "timeout kills owned descendants before delayed writes");
        auto completed = execute({fs::canonical(argv[0]).string(), "--tree-exit", marker.string()}, 10, false);
        sleep(3);
        check(completed.exit_code == 0 && !completed.timed_out && !fs::exists(marker), "normal completion also stops owned background descendants");
        auto parent_cwd = fs::current_path();
        auto changed_cwd = execute_child({fs::canonical(argv[0]).string(), "--cwd-probe"}, child_environment(), hostile, 30, false);
        check(changed_cwd.exit_code == 0 && changed_cwd.output == fs::canonical(hostile).string() && fs::current_path() == parent_cwd,
              "child cwd handles spaced Unicode paths without changing parent cwd");
        auto flood = execute({fs::canonical(argv[0]).string(), "--flood"}, 30, false);
        check(flood.exit_code == 0 && flood.output_truncated && flood.output.size() == 8 * 1024 * 1024, "bounded stdout capture without pipe deadlock");
        }
        const auto local = read_json(project_root / ".crt/local.json");
        const auto lock = read_json(project_root / "crt.lock");
        parallel_cases("projects", {"cxx", "c"}, jobs, [&](const std::string& language) {
            auto root = hostile / (std::string("hello-") + language);
            auto created = invoke(cli, {"new", root.string(), "--language", language});
            check(created.at("sdk_acquired") == false && !fs::exists(root / ".crt") && !fs::exists(root / "out"), "source-only creation");
            std::string suffix = std::string(language) == "c" ? "c" : "cpp";
            check(fs::exists(root / ("src/test/greeting_test." + suffix)) && fs::exists(root / ("src/main/greeting." + suffix)), "native multi-source scaffold");
            check(fs::is_regular_file(root / "include/greeting.h") && !fs::exists(root / "src/main/greeting.h"), "headers belong to include");
            invoke(cli, {"new", root.string()}, false);
            invoke(cli, {"build", "--project", root.string()}, false);
            invoke(cli, {"env", "--profile", "app-run", "--project", root.string()}, false);
            std::vector<std::string> configure{"configure", "--project", root.string(), "--sdk-root", local.at("sdk_root"), "--sdk-contract", local.at("sdk_contract"), "--contract-sha256", lock.at("selection").at("contract_sha256"), "--cc", local.at("cc"), "--cxx", local.at("cxx"), "--linker", local.at("linker")};
            configure.insert(configure.end(), {"--ar", local.at("ar")});
            if (local.contains("os_sdk")) configure.insert(configure.end(), {"--os-sdk", local.at("os_sdk")});
            if (lock.at("selection").value("allow_dirty_sdk", false)) configure.insert(configure.end(), {"--allow-dirty-sdk", "true"});
            auto wrong = configure; wrong[8] = std::string(64, '0');
            invoke(cli, wrong, false);
            auto configured = invoke(cli, configure);
            check(configured.at("selection") == lock.at("selection"), "exact shared SDK selection");
            auto before_lock = read_text(root / "crt.lock"), before_local = read_text(root / ".crt/local.json");
            check(invoke(cli, {"doctor", "--scope", "project", "--project", root.string()}).at("checks").at("manifest_sdk_tool_binding") == "pass", "read-only project doctor");
            for (const auto& profile : {"app-build", "app-run"}) {
                auto report = invoke(cli, {"env", "--profile", profile, "--project", root.string()});
                auto runtime = std::string(profile) == "app-run" ? std::vector<std::string>{(fs::path(local.at("sdk_root").get<std::string>()) / "bin").string(),
                    (fs::path(local.at("sdk_root").get<std::string>()) / "lib").string()} : std::vector<std::string>{};
                check(report.at("fixed") == child_environment(std::string(profile) == "app-build", runtime).fixed, "project environment matches actual execution policy");
                check(report.at("selection") == configured.at("selection") && report.at("build_host") == native_target(), "exact SDK and separate host identity");
                check(report.contains("executables") == (std::string(profile) == "app-build"), "compiler roles limited to build profile");
            }
            const auto after_lock = read_text(root / "crt.lock"), after_local = read_text(root / ".crt/local.json");
            const bool output_exists = fs::exists(root / "out");
            check(after_lock == before_lock && after_local == before_local && !output_exists,
                  "environment diagnostics preserve binding and create no build output: " + root.string() +
                  " lock_changed=" + std::to_string(after_lock != before_lock) + " local_changed=" + std::to_string(after_local != before_local) +
                  " output_exists=" + std::to_string(output_exists) + " lock_bytes=" + std::to_string(before_lock.size()) + "/" + std::to_string(after_lock.size()) +
                  " local_bytes=" + std::to_string(before_local.size()) + "/" + std::to_string(after_local.size()));
            if (std::string(language) == "cxx") {
                auto provider = read_json(fs::path(local.at("sdk_contract").get<std::string>()));
                for (const auto& change : {"format", "target", "producer"}) {
                    auto bad = provider;
                    if (std::string(change) == "format") bad["format"] = 99;
                    else if (std::string(change) == "target") bad["target"]["arch"] = provider.at("target").at("arch") == "aarch64" ? "x86_64" : "aarch64";
                    else bad["producer_source_commit"] = std::string(40, '0');
                    auto path = fixture / "bad-provider.json";
                    write_text(path, bad.dump());
                    auto rejected = configure;
                    rejected[6] = path.string(); rejected[8] = file_sha256(path);
                    invoke(cli, rejected, false);
                }
                auto script = fixture / "fake-compiler";
                write_text(script, "#!/bin/sh\ntouch never-execute\n");
                auto rejected = configure; rejected[10] = script.string();
                invoke(cli, rejected, false);
                auto fake_sdk = fixture / "incomplete-sdk";
                fs::create_directories(fake_sdk);
                write_text(fake_sdk / "manifest.json", "{}\n");
                rejected = configure; rejected[4] = fake_sdk.string();
                invoke(cli, rejected, false);
            }
            auto tested = invoke(cli, {"test", "--project", root.string()});
            check(tested.at("outcomes").size() == 1 && tested.at("outcomes").at(0).at("exit_code") == 0, "generated native test passes");
            invoke(cli, {"run", "--project", root.string()});
            for (const auto& command : tested.at("commands")) {
                check(command.at(0) == local.at("cc") || command.at(0) == local.at("cxx") || command.at(0) == local.at("ar"), "only direct native compiler/archive roles");
                for (const auto& arg : command) check(arg.get<std::string>().find("tools/crt-") == std::string::npos, "no SDK wrappers");
            }
            auto test_source = root / ("src/test/greeting_test." + suffix);
            auto original = read_text(test_source);
            write_text(test_source, "int main(void) { return 7; }\n");
            auto failed = invoke(cli, {"test", "--project", root.string()}, false);
            check(failed.at("outcomes").at(0).at("exit_code") == 7, "C/C++ test failure retained");
            write_text(test_source, original);
            auto manifest = read_json(root / "crt.json");
            auto broken = manifest; broken["manifest_format"] = 99;
            write_text(root / "crt.json", broken.dump()); invoke(cli, {"build", "--project", root.string()}, false);
            broken = manifest; broken["sdk"]["profile"] = "full";
            write_text(root / "crt.json", broken.dump()); invoke(cli, {"build", "--project", root.string()}, false);
            broken = manifest; broken["sdk"]["version"] = "not-the-selected-sdk";
            write_text(root / "crt.json", broken.dump()); invoke(cli, {"build", "--project", root.string()}, false);
            broken = manifest; broken["targets"][0]["sources"] = {"../outside.c"};
            write_text(root / "crt.json", broken.dump()); invoke(cli, {"build", "--project", root.string()}, false);
            write_text(root / "crt.json", manifest.dump());
            auto bad_lock = read_json(root / "crt.lock"); bad_lock["selection"]["sdk_content_sha256"] = std::string(64, '0');
            write_text(root / "crt.lock", bad_lock.dump()); invoke(cli, {"build", "--project", root.string()}, false);
            invoke(cli, {"env", "--profile", "app-run", "--project", root.string()}, false);
            check(read_json(root / "crt.lock") == bad_lock, "failed build preserves lock");
        });
        parallel_cases("variants", {"c/static-library", "c/shared-library", "c/executable",
                                   "cxx/static-library", "cxx/shared-library", "cxx/executable"}, jobs, [&](const std::string& value) {
            const auto slash = value.find('/');
            const auto language = value.substr(0, slash), type = value.substr(slash + 1);
                auto root = hostile / (std::string("variant-") + language + "-" + type);
                std::vector<std::string> create{"new", root.string(), "--language", language, "--kind", type};
                if (std::string(type) == "executable") create.insert(create.end(), {"--runtime-linkage", "shared"});
                invoke(cli, create);
                fs::create_directories(root / ".crt");
                write_text(root / ".crt/local.json", local.dump());
                write_text(root / "crt.lock", lock.dump());
                auto built = invoke(cli, {"build", "--project", root.string(), "--target", root.filename().string()});
                auto artifact = built.at("artifacts").at(root.filename().string());
                check(artifact.at("kind") == type && fs::file_size(artifact.at("path").get<std::string>()) > 0, "typed target artifact");
                auto again = invoke(cli, {"build", "--project", root.string(), "--target", root.filename().string()});
                check(again.at("cache").at("executed") == 0 && again.at("cache").at("reused") > 0 && again.at("commands").empty(), "unchanged rebuild reuses every output");
                auto declared = read_json(root / "crt.json").at("targets").at(0).at("sources").at(0).get<std::string>();
                auto edited = root / declared;
                auto before_edit = read_text(edited);
                // A real definition, not a comment: a target that emits no source checksum would keep the object bytes unchanged.
                write_text(edited, before_edit + "\nint crt_cli_cache_probe(void) { return 7; }\n");
                auto changed = invoke(cli, {"build", "--project", root.string(), "--target", root.filename().string()});
                check(changed.at("cache").at("executed") >= 2 && !changed.at("commands").empty(), "edited source recompiles and relinks");
                write_text(edited, before_edit);
                invoke(cli, {"build", "--project", root.string(), "--target", root.filename().string()});
                auto header = root / "include/greeting.h";
                auto header_text = read_text(header);
                write_text(header, header_text + "\n/* header probe */\n");
                auto header_changed = invoke(cli, {"build", "--project", root.string(), "--target", root.filename().string()});
                // A comment leaves the object bytes unchanged, so the archive/link step may legitimately be reused.
                check(header_changed.at("cache").at("executed") >= 1 && !header_changed.at("commands").empty(), "edited header recompiles its dependents");
                write_text(header, header_text);
                auto tested = invoke(cli, {"test", "--project", root.string()});
                check(tested.at("outcomes").at(0).at("exit_code") == 0, "typed target link/run consumer");
                if (std::string(type) != "executable") invoke(cli, {"run", "--project", root.string()}, false);
                else invoke(cli, {"run", "--project", root.string(), "--target", root.filename().string()});
                invoke(cli, {"build", "--project", root.string(), "--target", "missing"}, false);
                auto manifest = read_json(root / "crt.json");
                auto bad = manifest;
                bad["targets"][0]["kind"] = "not-a-kind";
                write_text(root / "crt.json", bad.dump()); invoke(cli, {"build", "--project", root.string()}, false);
                if (std::string(type) == "static-library") {
                    bad = manifest;
                    auto second = bad["targets"][0]; second["name"] = "cycle";
                    second["dependencies"] = {root.filename().string()};
                    bad["targets"][0]["dependencies"] = {"cycle"};
                    bad["targets"].push_back(second);
                    write_text(root / "crt.json", bad.dump()); invoke(cli, {"build", "--project", root.string()}, false);
                }
                write_text(root / "crt.json", manifest.dump());
        });
        {
        NativePhase phase("selection");
        // The local, uncommitted .sdk.json supplies configure defaults; explicit flags override it per key.
        Json sdk_config{{"format", 0}, {"origin", "native test"}, {"seed", "/not/used/by/configure"}, {"sdk_root", local.at("sdk_root")}, {"sdk_contract", local.at("sdk_contract")},
            {"contract_sha256", lock.at("selection").at("contract_sha256")}, {"cc", local.at("cc")}, {"cxx", local.at("cxx")}, {"linker", local.at("linker")}, {"ar", local.at("ar")},
            {"allow_dirty_sdk", lock.at("selection").value("allow_dirty_sdk", false)}};
        if (local.contains("os_sdk")) sdk_config["os_sdk"] = local.at("os_sdk");
        auto config_root = hostile / "config-project";
        invoke(cli, {"new", config_root.string()});
        write_text(config_root / ".sdk.json", sdk_config.dump(2));
        check(read_text(config_root / ".gitignore").find("/.sdk.json") != std::string::npos, "generated project ignores .sdk.json");
        auto from_config = invoke(cli, {"configure", "--project", config_root.string()});
        check(from_config.at("selection") == lock.at("selection"), "configure defaults come from .sdk.json");
        invoke(cli, {"test", "--project", config_root.string()});
        invoke(cli, {"configure", "--project", config_root.string(), "--contract-sha256", std::string(64, '0')}, false);
        auto elsewhere = fixture / "elsewhere.sdk.json";
        write_text(elsewhere, sdk_config.dump());
        fs::remove(config_root / ".sdk.json");
        invoke(cli, {"configure", "--project", config_root.string()}); // no config: the existing binding is re-verified
        auto unbound_root = hostile / "unbound-project";
        invoke(cli, {"new", unbound_root.string()});
        invoke(cli, {"configure", "--project", unbound_root.string()}, false); // neither .sdk.json nor flags nor binding
        auto from_path = invoke(cli, {"configure", "--project", config_root.string(), "--sdk-config", elsewhere.string()});
        check(from_path.at("selection") == lock.at("selection"), "--sdk-config selects another file");
        for (const auto& change : {"format", "unknown", "relative", "typed", "hash"}) {
            auto bad = sdk_config;
            if (std::string(change) == "format") bad["format"] = 99;
            else if (std::string(change) == "unknown") bad["sdk_roots"] = "/x";
            else if (std::string(change) == "relative") bad["cc"] = "clang";
            else if (std::string(change) == "typed") bad["allow_dirty_sdk"] = "true";
            else bad["contract_sha256"] = std::string(64, '0');
            write_text(elsewhere, bad.dump());
            invoke(cli, {"configure", "--project", config_root.string(), "--sdk-config", elsewhere.string()}, false);
        }
        invoke(cli, {"configure", "--project", config_root.string(), "--sdk-config", (fixture / "missing.sdk.json").string()}, false);
        // The SDK store is a user-home contract, not an OS-specific data root.
        auto home = hostile / "User Home";
        const std::string old_home = std::getenv("HOME") ? std::getenv("HOME") : "";
        const std::string old_profile = std::getenv("USERPROFILE") ? std::getenv("USERPROFILE") : "";
        setenv("HOME", home.string().c_str(), 1); setenv("USERPROFILE", home.string().c_str(), 1);
        check(sdk_store_root() == home / ".crt/sdks", "identical user-home SDK location");
        auto installed = sdk_store_root() / "local-candidate";
        fs::create_directories(installed.parent_path());
        fs::copy(fs::path(local.at("sdk_root").get<std::string>()), installed, fs::copy_options::recursive | fs::copy_options::copy_symlinks);
        auto root = hostile / "store-project";
        invoke(cli, {"new", root.string()});
        std::vector<std::string> configure{"configure", "--project", root.string(), "--sdk-id", "local-candidate", "--sdk-contract", local.at("sdk_contract"), "--contract-sha256", lock.at("selection").at("contract_sha256"), "--cc", local.at("cc"), "--cxx", local.at("cxx"), "--linker", local.at("linker"), "--ar", local.at("ar")};
        if (local.contains("os_sdk")) configure.insert(configure.end(), {"--os-sdk", local.at("os_sdk")});
        if (lock.at("selection").value("allow_dirty_sdk", false)) configure.insert(configure.end(), {"--allow-dirty-sdk", "true"});
        invoke(cli, configure); invoke(cli, {"test", "--project", root.string()});
        check(fs::path(read_json(root / ".crt/local.json").at("sdk_root").get<std::string>()) == fs::canonical(installed), "SDK selected under common store");
        configure[4] = "../escape"; invoke(cli, configure, false);
        // A flag-selected store id replaces the SDK root named by .sdk.json instead of conflicting with it.
        write_text(root / ".sdk.json", sdk_config.dump());
        invoke(cli, {"configure", "--project", root.string(), "--sdk-id", "local-candidate"});
        check(fs::path(read_json(root / ".crt/local.json").at("sdk_root").get<std::string>()) == fs::canonical(installed), "--sdk-id overrides .sdk.json sdk_root");
        // A published store SDK is selected by name or identity prefix, locked by identity alone (lock format 1) and found
        // again through the store; ambiguity, renamed directories and a missing SDK are refused rather than guessed.
        const std::string version = lock.at("selection").at("version"), content = lock.at("selection").at("sdk_content_sha256");
        const std::string identity = version + "-" + content.substr(0, 12);
        const auto published = sdk_store_root() / identity;
        fs::copy(installed, published, fs::copy_options::recursive | fs::copy_options::copy_symlinks);
        auto select = [&](const std::string& flag, const std::string& value) { auto args = configure; args[3] = flag; args[4] = value; return args; };
        invoke(cli, select("--sdk", identity), false); // An arbitrary legacy copy is not an installation-time policy pin.
        // Trusted synthetic store fixtures exercise selection independently of the archive installer tests.
        auto fixture_receipt = [&](const std::string& hash) {
            auto path = sdk_store_root() / ".receipts" / (identity + ".json"); fs::create_directories(path.parent_path());
            write_text(path, Json{{"schema", "crt-cli.sdk-install-receipt"}, {"format", 1},
                {"sdk", {{"version", version}, {"content_sha256", content}, {"directory", identity}}},
                {"contract", hash.empty() ? Json(nullptr) : Json{{"sha256", hash}}}}.dump(2) + "\n");
        };
        fixture_receipt("");
        invoke(cli, select("--sdk", identity));
        auto stored = read_json(root / "crt.lock");
        check(stored.at("lock_format") == 1 && stored.at("selection").at("source") == "store" && stored.at("selection").at("sdk_directory") == identity, "store selection is locked by identity");
        check(!read_json(root / ".crt/local.json").contains("sdk_root"), "a store selection keeps no machine path");
        invoke(cli, {"test", "--project", root.string()});
        invoke(cli, select("--sdk", content.substr(0, 10)));
        check(read_json(root / "crt.lock").at("selection").at("sdk_directory") == identity, "identity prefix selects the same SDK");
        const auto identity_lock = read_text(root / "crt.lock"), identity_local = read_text(root / ".crt/local.json");
        for (size_t length : {13, 32, 64}) {
            invoke(cli, select("--sdk", content.substr(0, length)));
            auto wrong = content.substr(0, length); wrong.back() = wrong.back() == '0' ? '1' : '0';
            auto rejected = invoke(cli, select("--sdk", wrong), false);
            check(rejected.at("message").get<std::string>().find("no installed SDK matches") != std::string::npos,
                  "all supplied identity digits must match");
            check(read_text(root / "crt.lock") == identity_lock && read_text(root / ".crt/local.json") == identity_local,
                  "identity mismatch preserves both binding files");
        }
        fs::remove(root / ".sdk.json");                                           // its sdk_root would name the SDK
        auto automatic = select("--sdk", identity); automatic.erase(automatic.begin() + 3, automatic.begin() + 5);
        invoke(cli, automatic);
        check(read_json(root / "crt.lock").at("selection").at("sdk_directory") == identity, "the only fitting SDK is selected automatically");
        invoke(cli, select("--sdk", "local-candidate"), false);                  // a handoff copy is not a published SDK
        invoke(cli, select("--sdk", content.substr(0, 7)), false);               // too short to identify
        invoke(cli, select("--sdk", std::string(64, 'a')), false);               // nothing matches
        auto both = select("--sdk", identity); both.insert(both.end(), {"--sdk-id", "local-candidate"});
        invoke(cli, both, false);
        // First build binds only an unresolved project, after successful compilation.
        auto first = hostile / "first-build";
        invoke(cli, {"new", first.string()});
        const auto unresolved = read_text(first / "crt.lock");
        auto first_args = select("--sdk", identity); first_args[0] = "build"; first_args[2] = first.string();
        invoke(cli, {"build", "--project", first.string()}, false);
        check(read_text(first / "crt.lock") == unresolved && !fs::exists(first / ".crt/local.json"), "missing first-build tools preserve unresolved state");
        const auto main_source = read_text(first / "src/main/main.cpp");
        write_text(first / "src/main/main.cpp", "#error deliberate first-build compilation failure\n");
        invoke(cli, first_args, false);
        check(read_text(first / "crt.lock") == unresolved && !fs::exists(first / ".crt/local.json"), "failed first build does not publish pins");
        write_text(first / "src/main/main.cpp", main_source);
        invoke(cli, first_args);
        check(read_json(first / "crt.lock").at("selection").at("sdk_directory") == identity, "successful first build pins the installed SDK");
        const auto first_lock = read_text(first / "crt.lock"), first_local = read_text(first / ".crt/local.json");
        invoke(cli, {"test", "--project", first.string()});
        invoke(cli, {"run", "--project", first.string()});
        invoke(cli, first_args, false); // Reselection remains an explicit configure operation.
        check(read_text(first / "crt.lock") == first_lock && read_text(first / ".crt/local.json") == first_local, "bound build refuses configuration flags without changing pins");
        fs::remove(first / ".crt/local.json");
        invoke(cli, first_args, false);
        check(read_text(first / "crt.lock") == first_lock && !fs::exists(first / ".crt/local.json"), "resolved lock is never rebound after local-state loss");
        write_text(first / ".crt/local.json", first_local);
        auto defaults_first = hostile / "first-build-defaults";
        invoke(cli, {"new", defaults_first.string()});
        auto first_config = sdk_config; first_config.erase("sdk_root"); first_config["sdk"] = identity;
        write_text(defaults_first / ".sdk.json", first_config.dump());
        invoke(cli, {"build", "--project", defaults_first.string()});
        check(read_json(defaults_first / "crt.lock").at("selection").at("sdk_directory") == identity, "plain first build reads declared .sdk.json tools");
        const auto copy = sdk_store_root() / (version + "-aaaaaaaaaaaa");
        fs::copy(published, copy, fs::copy_options::recursive | fs::copy_options::copy_symlinks);
        invoke(cli, automatic, false);                                           // two fit: never pick one silently
        check(read_text(root / "crt.lock") == identity_lock && read_text(root / ".crt/local.json") == identity_local,
              "ambiguous automatic selection preserves both binding files");
        invoke(cli, select("--sdk", identity));                                  // an explicit identity resolves ambiguity
        invoke(cli, select("--sdk", copy.filename().string()), false);           // the name does not match the content
        fs::remove_all(copy);
        invoke(cli, select("--sdk", identity));
        const auto lock_text = read_text(root / "crt.lock");
        auto legacy = read_json(root / "crt.lock"); legacy["lock_format"] = 0; write_text(root / "crt.lock", legacy.dump());
        invoke(cli, {"build", "--project", root.string()}, false);               // format 0 is refused, not migrated
        check(read_json(root / "crt.lock").at("lock_format") == 0, "a refused lock is left untouched");
        write_text(root / "crt.lock", lock_text);
        const auto away = sdk_store_root() / "moved-away";
        fs::rename(published, away);
        invoke(cli, {"build", "--project", root.string()}, false);               // the locked SDK is no longer installed
        fs::rename(away, published);
        invoke(cli, {"build", "--project", root.string()});
        // A toolchain contract stored beside the SDK replaces --sdk-contract; the lock records its origin and digest, no path.
        const auto stored_contract = sdk_store_root() / ".contracts" / (identity + ".json");
        fs::create_directories(stored_contract.parent_path());
        auto stored_policy = read_json(local.at("sdk_contract").get<std::string>());
        const auto vendor_sdk = local.value("os_sdk", stored_policy.value("external_os_sdk", std::string()));
        if (stored_policy.contains("external_os_sdk")) {
            // A developer policy names this machine's vendor SDK; it cannot be
            // installed as portable policy. Pin a separate synthetic fixture.
            const auto location = stored_policy.at("external_os_sdk").get<std::string>();
            check(!location.empty(), "a declared vendor SDK has a location");
            stored_policy.erase("external_os_sdk");
            stored_policy["os_sdk"] = {{"kind", "test-only"}};
            portable_vendor_paths(stored_policy, location);
        }
        write_text(stored_contract, stored_policy.dump());
        fixture_receipt(file_sha256(stored_contract));
        std::vector<std::string> stored_args{"configure", "--project", root.string(), "--sdk", identity, "--cc", local.at("cc"), "--cxx", local.at("cxx"), "--linker", local.at("linker"), "--ar", local.at("ar")};
        if (!vendor_sdk.empty()) stored_args.insert(stored_args.end(), {"--os-sdk", vendor_sdk});
        if (lock.at("selection").value("allow_dirty_sdk", false)) stored_args.insert(stored_args.end(), {"--allow-dirty-sdk", "true"});
        invoke(cli, stored_args);
        check(read_json(root / "crt.lock").at("selection").at("contract_source") == "store", "the stored contract is used");
        check(!read_json(root / ".crt/local.json").contains("sdk_contract") && !read_json(root / ".crt/local.json").contains("sdk_root"), "a stored contract keeps no path");
        check(read_json(root / "crt.lock").at("selection").at("contract_sha256") == file_sha256(stored_contract), "the lock records the contract digest");
        invoke(cli, {"test", "--project", root.string()});
        auto bad_digest = stored_args; bad_digest.insert(bad_digest.end(), {"--contract-sha256", std::string(64, '0')});
        invoke(cli, bad_digest, false);
        const auto saved_contract = read_text(stored_contract);
        const auto pinned_lock = read_text(root / "crt.lock"), pinned_local = read_text(root / ".crt/local.json");
        auto plausible = Json::parse(saved_contract); plausible["changed_policy"] = true;
        write_text(stored_contract, plausible.dump());
        invoke(cli, stored_args, false); // A valid contract for this SDK must not be adopted as a fresh pin.
        check(read_text(root / "crt.lock") == pinned_lock && read_text(root / ".crt/local.json") == pinned_local,
              "receipt pin rejection preserves both project binding files");
        write_text(stored_contract, "{}");
        invoke(cli, stored_args, false);                                         // a tampered stored contract is not the SDK's
        fs::remove(stored_contract);
        invoke(cli, stored_args, false);                                         // no contract stored: say so, do not guess
        write_text(stored_contract, saved_contract);
        invoke(cli, stored_args);
        auto os_args = stored_args;
        auto declared_os = std::find(os_args.begin(), os_args.end(), "--os-sdk");
        if (declared_os == os_args.end()) os_args.insert(os_args.end(), {"--os-sdk", home.string()});
        else *(declared_os + 1) = (home / "missing-vendor-sdk").string();
        invoke(cli, os_args, false); // No OS SDK is needed, or the required explicit SDK is absent.

        // A portable contract (@OS_SDK@) is completed on this host: the location is local state, never locked.
        auto derived = read_json(stored_contract);
        derived["os_sdk"] = Json{{"kind", "test-only"}};
        for (const char* recipe : {"compile_c", "compile_cxx"}) derived["modes"]["executable-static"][recipe].push_back("-DCRT_OS_SDK=@OS_SDK@");
        write_text(stored_contract, derived.dump());
        fixture_receipt(sha256(derived.dump())); // A separately approved synthetic installation, not implicit re-pinning.
        const auto os_sdk = hostile / "OS SDK \xed\x95\x9c";
        fs::create_directories(os_sdk);
        // Keep the declared vendor inputs usable through the synthetic SDK root;
        // an empty directory is sufficient only for a policy with no vendor SDK.
        if (!vendor_sdk.empty()) {
            for (const auto& entry : fs::directory_iterator(vendor_sdk)) {
                if (entry.is_directory()) fs::create_directory_symlink(entry.path(), os_sdk / entry.path().filename());
                else fs::create_symlink(entry.path(), os_sdk / entry.path().filename());
            }
        }
        auto unknown_os = stored_args;
        declared_os = std::find(unknown_os.begin(), unknown_os.end(), "--os-sdk");
        if (declared_os != unknown_os.end()) unknown_os.erase(declared_os, declared_os + 2);
        invoke(cli, unknown_os, false);                                         // an unknown kind cannot be discovered
        auto with_os = unknown_os; with_os.insert(with_os.end(), {"--os-sdk", os_sdk.string()});
        invoke(cli, with_os);
        invoke(cli, {"build", "--project", root.string()});
        check(read_json(root / ".crt/local.json").at("os_sdk") == os_sdk.generic_string(), "the OS SDK stays in local state");
        check(read_text(root / "crt.lock").find(os_sdk.generic_string()) == std::string::npos, "the lock names no machine path");
        check(read_json(root / "crt.lock").at("selection").at("os_sdk").at("kind") == "test-only", "the lock records the OS SDK kind");
        check(read_text(root / "out/native/last-build.json").find("-DCRT_OS_SDK=" + os_sdk.generic_string()) != std::string::npos, "@OS_SDK@ is replaced in the compile command");
        fs::remove_all(os_sdk);
        invoke(cli, {"build", "--project", root.string()}, false);               // the recorded OS SDK is gone
        write_text(stored_contract, saved_contract);
        if (old_home.empty()) unsetenv("HOME"); else setenv("HOME", old_home.c_str(), 1);
        if (old_profile.empty()) unsetenv("USERPROFILE"); else setenv("USERPROFILE", old_profile.c_str(), 1);
        check(!fs::exists(hostile / "never-execute"), "shell metacharacters stay literal");
        fs::remove_all(fixture);
        }
        report_phases();
        std::printf("native tests: %zu checks passed\n", checks.load());
        return 0;
    } catch (const std::exception& error) {
        report_phases();
        std::fprintf(stderr, "%s (fixture retained at %s)\n", error.what(), fixture.string().c_str());
        return 1;
    }
}
