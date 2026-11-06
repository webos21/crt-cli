#include <crtcli/core.h>
#include <crtcli/host_tools.h>
#include <crtcli/sdk_store.h>
#include <set>
int main(int argc, char** argv) {
    using namespace crtcli;
    bool json = false; std::string command = "invalid";
    try {
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--json") { require(!json, "duplicate --json"); json = true; }
            else args.emplace_back(argv[i]);
        }
        if (args.empty() || args == std::vector<std::string>{"--help"}) return print_reply({0, "help", {{"usage", "crt-cli [--json] --help|--version|doctor|shell|fetch|sdk|env|new|configure|build|test|run"}},
            "crt-cli [--json]\n"
            "  --help | --version\n"
            "  doctor [--scope self] [--resources <dir> --resources-sha256 <hex>] [--publisher-key <pem> --signature <file>]\n"
            "  doctor --scope project [--project <dir>]\n"
            "  doctor --scope environment --host-tools <dir> --host-tools-sha256 <hex>\n"
            "  env --profile control|app-build|app-run [--project <dir>]\n"
            "  env --profile port-build --host-tools <dir> --host-tools-sha256 <hex> --work-root <new-dir> [--project <dir>]\n"
            "  shell --host-tools <dir> --host-tools-sha256 <hex> --work-root <new-dir> --script <file> [--directory <dir>] [--timeout <seconds>] [--project <dir>]\n"
            "  fetch --url <https-url> --ca-file <pem> --output <file> --sha256 <hex> --size <bytes> [--publisher-key <pem> --signature <file>]\n"
            "  sdk install --archive <file> --archive-sha256 <hex> [--content-sha256 <hex>] [--store <dir>] [--lock-wait <seconds>]\n"
            "  sdk install --archive <file> --release-manifest <file> --release-manifest-sha256 <hex> [--store <dir>] [--lock-wait <seconds>]\n"
            "  sdk repair <sdk-directory> --archive <file> --release-manifest <file> --release-manifest-sha256 <hex> [--store <dir>]\n"
            "  sdk list|recover [--store <dir>]; sdk verify <sdk-directory> [--store <dir>]\n"
            "  new <dir> [--language c|cxx]\n"
            "  configure/build/test/run [--project <dir>]\n"
            "  First build: declare --cc <clang> --cxx <clang++> --linker <lld> --ar <llvm-ar> or local .sdk.json.\n"
            "  Existing bindings require explicit configure to change SDK/tools."}, json);
        check_arguments(args);
        command = args.front();
        if (args == std::vector<std::string>{"--version"}) return print_reply({0, "version", {{"version", "0.1.0-dev"}}, "crt-cli 0.1.0-dev"}, json);
        if (command == "doctor") return print_reply(doctor(args), json);
        if (command == "shell") return print_reply(shell(args), json);
        if (command == "fetch") return print_reply(fetch(args), json);
        if (command == "sdk") return print_reply(sdk_command(args), json);
        if (command == "env") return print_reply(environment_report(args), json);
        if (!std::set<std::string>{"new", "configure", "build", "test", "run"}.count(command)) throw Error("unsupported command or arguments; use --help", 2);
        return print_reply(workflow(args), json);
    } catch (const Error& error) {
        return print_reply({error.code, command, {{"code", "failure"}, {"message", error.what()}}, error.what()}, json);
    } catch (const std::exception& error) {
        return print_reply({1, command, {{"code", "invalid_input"}, {"message", error.what()}}, error.what()}, json);
    }
}
