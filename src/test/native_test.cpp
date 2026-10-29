#include <crtcli/core.h>
#include <crt/platform.h>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
using namespace crtcli;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }
Json invoke(const std::string& cli, std::vector<std::string> args, bool success = true) {
    args.insert(args.begin(), "--json"); args.insert(args.begin(), cli);
    auto child = execute(args, 120, false);
    auto reply = Json::parse(child.output);
    check((child.exit_code == 0) == success, "expected command exit: " + args.at(2));
    check(reply.at("ok").get<bool>() == success && reply.at("schema") == "crt-cli.bootstrap-result", "JSON outcome");
    return reply.at("data");
}
}
int main(int argc, char** argv) {
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
        check(sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA256 vector");
        invoke(cli, {"--help"}); invoke(cli, {"--version"});
        auto doctor = invoke(cli, {"doctor", "--bootstrap"});
        check(doctor.at("checks").at("cxx_allocation_owner") == "pass", "CLI owns its C++ allocation functions");
        check(doctor.at("checks").at("executable_integrity") == "not_verified", "honest integrity result");
        check(doctor.at("checks").at("https_network") == "not_run", "honest network result");
        check(doctor.at("runtime_maps").at("status") == "collected", "PAL image enumeration");
        check(fs::path(doctor.at("sdk_store").get<std::string>()).filename() == "sdks", "common SDK store basename");
        for (const auto& args : std::vector<std::vector<std::string>>{{"upgrade"}, {"doctor"}, {"--version", "extra"}, {"--json"}, {"new"}, {"build", "--bad"}, {"configure", "--project"}})
            invoke(cli, args, false);
        // The temporary root comes from the environment (TMPDIR, then TEMP); a literal /tmp does not exist everywhere.
        const char* temporary_root = std::getenv("TMPDIR") ? std::getenv("TMPDIR") : std::getenv("TEMP") ? std::getenv("TEMP") : "/tmp";
        std::string temporary = (fs::path(temporary_root) / "crt-cli-native-XXXXXX").string();
        auto location = mkdtemp(temporary.data());
        require(location != nullptr, "mkdtemp failed");
        fixture = fs::path(location);
        // Windows narrow paths are ANSI code-page bytes and the imported libc++ rejects non-ASCII narrow<->wide conversion
        // ("locale not supported"), so the Hangul part of the hostile name is exercised on the other hosts only. The Windows
        // loader also splits its DLL search path at ';', so a program importing a DLL beside it cannot start from a directory
        // whose name contains one: the semicolon is exercised on the other hosts only.
        const bool windows_host = std::string(crt_get_platform_info()->os) == "windows";
        auto hostile = fixture / (windows_host ? "space @ $(touch never-execute)" : "space 한글 @ ; $(touch never-execute)");
        fs::create_directories(hostile);
        write_text(fixture / "duplicate.json", "{\"x\":1,\"x\":2}");
        bool rejected = false;
        try { read_json(fixture / "duplicate.json"); } catch (const Error&) { rejected = true; }
        check(rejected, "duplicate JSON keys");
        auto timeout = execute({fs::canonical(argv[0]).string(), "--sleeper"}, 1, false);
        check(timeout.timed_out && timeout.exit_code == 124, "owned process timeout");
        auto flood = execute({fs::canonical(argv[0]).string(), "--flood"}, 30, false);
        check(flood.exit_code == 0 && flood.output_truncated && flood.output.size() == 8 * 1024 * 1024, "bounded stdout capture without pipe deadlock");
        auto local = read_json(project_root / ".crt/local.json");
        auto lock = read_json(project_root / "crt.lock");
        for (const auto& language : {"cxx", "c"}) {
            auto root = hostile / (std::string("hello-") + language);
            auto created = invoke(cli, {"new", root.string(), "--language", language});
            check(created.at("sdk_acquired") == false && !fs::exists(root / ".crt") && !fs::exists(root / "out"), "source-only creation");
            std::string suffix = std::string(language) == "c" ? "c" : "cpp";
            check(fs::exists(root / ("src/test/greeting_test." + suffix)) && fs::exists(root / ("src/main/greeting." + suffix)), "native multi-source scaffold");
            check(fs::is_regular_file(root / "include/greeting.h") && !fs::exists(root / "src/main/greeting.h"), "headers belong to include");
            invoke(cli, {"new", root.string()}, false);
            invoke(cli, {"build", "--project", root.string()}, false);
            std::vector<std::string> configure{"configure", "--project", root.string(), "--sdk-root", local.at("sdk_root"), "--sdk-contract", local.at("sdk_contract"), "--contract-sha256", lock.at("selection").at("contract_sha256"), "--cc", local.at("cc"), "--cxx", local.at("cxx"), "--linker", local.at("linker")};
            configure.insert(configure.end(), {"--ar", local.at("ar")});
            if (lock.at("selection").value("allow_dirty_sdk", false)) configure.insert(configure.end(), {"--allow-dirty-sdk", "true"});
            auto wrong = configure; wrong[8] = std::string(64, '0');
            invoke(cli, wrong, false);
            auto configured = invoke(cli, configure);
            check(configured.at("selection") == lock.at("selection"), "exact shared SDK selection");
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
            check(read_json(root / "crt.lock") == bad_lock, "failed build preserves lock");
        }
        for (const auto& language : {"c", "cxx"}) {
            for (const auto& type : {"static-library", "shared-library", "executable"}) {
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
            }
        }
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
        if (lock.at("selection").value("allow_dirty_sdk", false)) configure.insert(configure.end(), {"--allow-dirty-sdk", "true"});
        invoke(cli, configure); invoke(cli, {"test", "--project", root.string()});
        check(fs::path(read_json(root / ".crt/local.json").at("sdk_root").get<std::string>()) == fs::canonical(installed), "SDK selected under common store");
        configure[4] = "../escape"; invoke(cli, configure, false);
        if (old_home.empty()) unsetenv("HOME"); else setenv("HOME", old_home.c_str(), 1);
        if (old_profile.empty()) unsetenv("USERPROFILE"); else setenv("USERPROFILE", old_profile.c_str(), 1);
        check(!fs::exists(hostile / "never-execute"), "shell metacharacters stay literal");
        fs::remove_all(fixture);
        std::printf("native tests: %zu checks passed\n", checks);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s (fixture retained at %s)\n", error.what(), fixture.string().c_str());
        return 1;
    }
}
