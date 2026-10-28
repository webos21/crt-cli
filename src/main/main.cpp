#include "core.h"
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
        if (args.empty() || args == std::vector<std::string>{"--help"}) return print_reply({0, "help", {{"usage", "crt-cli [--json] --help|--version|doctor --bootstrap|new|configure|build|test|run"}}, "crt-cli: --help | --version | doctor --bootstrap | new <dir> [--language c|cxx] | configure/build/test/run [--project <dir>]"}, json);
        command = args.front();
        if (args == std::vector<std::string>{"--version"}) return print_reply({0, "version", {{"version", "0.1.0-dev"}}, "crt-cli 0.1.0-dev"}, json);
        if (args == std::vector<std::string>{"doctor", "--bootstrap"}) return print_reply(bootstrap_doctor(), json);
        if (!std::set<std::string>{"new", "configure", "build", "test", "run"}.count(command)) throw Error("unsupported command or arguments; use --help", 2);
        return print_reply(workflow(args), json);
    } catch (const Error& error) {
        return print_reply({error.code, command, {{"code", "failure"}, {"message", error.what()}}, error.what()}, json);
    } catch (const std::exception& error) {
        return print_reply({1, command, {{"code", "invalid_input"}, {"message", error.what()}}, error.what()}, json);
    }
}
