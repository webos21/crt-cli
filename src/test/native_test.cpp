#include "core.h"
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
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
        check(doctor.at("checks").at("executable_integrity") == "not_verified", "honest integrity result");
        check(doctor.at("checks").at("https_network") == "not_run", "honest network result");
        for (const auto& args : std::vector<std::vector<std::string>>{{"upgrade"}, {"doctor"}, {"--version", "extra"}, {"--json"}, {"new"}, {"build", "--bad"}, {"configure", "--project"}})
            invoke(cli, args, false);
        char temporary[] = "/tmp/crt-cli-native-XXXXXX";
        auto location = mkdtemp(temporary);
        require(location != nullptr, "mkdtemp failed");
        fixture = fs::path(location);
        auto hostile = fixture / "space 한글 @ ; $(touch never-execute)";
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
            invoke(cli, {"new", root.string()}, false);
            invoke(cli, {"build", "--project", root.string()}, false);
            std::vector<std::string> configure{"configure", "--project", root.string(), "--sdk-root", local.at("sdk_root"), "--sdk-contract", local.at("sdk_contract"), "--contract-sha256", lock.at("selection").at("contract_sha256"), "--cc", local.at("cc"), "--cxx", local.at("cxx"), "--linker", local.at("linker")};
            auto wrong = configure; wrong[8] = std::string(64, '0');
            invoke(cli, wrong, false);
            auto configured = invoke(cli, configure);
            check(configured.at("selection") == lock.at("selection"), "exact shared SDK selection");
            if (std::string(language) == "cxx") {
                auto provider = read_json(fs::path(local.at("sdk_contract").get<std::string>()));
                for (const auto& change : {"format", "target", "producer"}) {
                    auto bad = provider;
                    if (std::string(change) == "format") bad["format"] = 99;
                    else if (std::string(change) == "target") bad["target"]["arch"] = "aarch64";
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
                check(command.at(0) == local.at("cc") || command.at(0) == local.at("cxx"), "only direct native compiler roles");
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
            broken = manifest; broken["application"]["sources"] = {"../outside.c"};
            write_text(root / "crt.json", broken.dump()); invoke(cli, {"build", "--project", root.string()}, false);
            write_text(root / "crt.json", manifest.dump());
            auto bad_lock = read_json(root / "crt.lock"); bad_lock["selection"]["sdk_content_sha256"] = std::string(64, '0');
            write_text(root / "crt.lock", bad_lock.dump()); invoke(cli, {"build", "--project", root.string()}, false);
            check(read_json(root / "crt.lock") == bad_lock, "failed build preserves lock");
        }
        check(!fs::exists(hostile / "never-execute"), "shell metacharacters stay literal");
        fs::remove_all(fixture);
        std::printf("native tests: %zu checks passed\n", checks);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s (fixture retained at %s)\n", error.what(), fixture.string().c_str());
        return 1;
    }
}
