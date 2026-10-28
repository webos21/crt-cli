#include "toolchain.h"
#include <set>
#include <cctype>
namespace crtcli {
namespace {
bool identifier(const std::string& name) {
    if (name.empty()) return false;
    for (unsigned char c : name) if (!std::isalnum(c) && c != '_' && c != '-' && c != '+') return false;
    return true;
}
Json project(const fs::path& root) {
    auto manifest = read_json(root / "crt.json");
    require(manifest.at("manifest_format") == 0, "unsupported project manifest format");
    require(identifier(manifest.at("package").at("name")), "invalid package name");
    auto sdk = manifest.at("sdk");
    require(sdk.at("profile") == "dev" && sdk.at("target") == "native" && sdk.at("storage") == "shared-immutable", "unsupported SDK request");
    require(manifest.at("application").at("sources").is_array() && !manifest.at("application").at("sources").empty(), "application sources are required");
    return manifest;
}
void match_request(const Json& manifest, const Toolchain& toolchain) {
    auto version = manifest.at("sdk").at("version").get<std::string>();
    require(version == "*" || version == toolchain.selection.at("version"), "project SDK version mismatch");
}
fs::path build_target(const fs::path& root, const Json& manifest, const Json& target, const Toolchain& toolchain, const fs::path& build, Json& commands) {
    std::string name = target.at("name");
    require(identifier(name), "invalid target name");
    require(target.at("sources").is_array() && !target.at("sources").empty(), "target requires a source array");
    auto output = inside(root, "out/native/" + name);
    require(!fs::is_symlink(build / name), "build output must not be a symlink");
    fs::remove(output);
    std::vector<std::string> objects, libraries;
    bool cpp = false;
    std::set<std::string> seen;
    for (const auto& source : target.at("sources")) {
        auto file = inside(root, source.get<std::string>());
        require(fs::is_regular_file(file), "missing source: " + file.string());
        require(seen.insert(file.string()).second, "duplicate source: " + file.string());
        auto extension = file.extension().string();
        bool is_cpp = extension == ".cpp" || extension == ".cc" || extension == ".cxx";
        require(is_cpp || extension == ".c", "unsupported C/C++ source extension");
        cpp |= is_cpp;
        auto object = build / (name + "-" + sha256(file.string()).substr(0, 16) + ".o");
        require(!fs::is_symlink(object), "object output must not be a symlink");
        std::vector<std::string> flags{is_cpp ? "-std=c++17" : "-std=c99", "-O1", "-g", "-I" + (build / "generated").string()};
        for (const auto& dir : manifest.value("include_dirs", Json::array())) flags.push_back("-I" + inside(root, dir).string());
        for (const auto& option : read_json(toolchain.sdk / "share/crt/contracts/development.json").at("consumer_compile_options")) {
            auto text = option.get<std::string>();
            require(text.rfind("-U", 0) == 0, "unsupported SDK consumer option");
            flags.push_back(text);
        }
        toolchain.invoke("compile", is_cpp, {file.string()}, object, flags, {}, commands);
        objects.push_back(object.string());
    }
    for (const auto& value : target.value("sdk_libraries", Json::array())) {
        std::string name = value;
        require(identifier(name), "invalid SDK library name");
        auto library = toolchain.sdk / "lib" / ("lib" + name + ".a");
        require(fs::is_regular_file(library), "missing SDK static library: " + name);
        libraries.push_back(library.string());
    }
    toolchain.invoke("link", cpp, objects, output, {}, libraries, commands);
    return output;
}
Reply create(const std::vector<std::string>& args) {
    require(args.size() == 2 || (args.size() == 4 && args[2] == "--language" && (args[3] == "c" || args[3] == "cxx")), "usage: new <directory> [--language c|cxx]");
    auto root = fs::absolute(args[1]).lexically_normal();
    auto name = root.filename().string();
    require(identifier(name), "project directory basename must be a package identifier");
    require(!fs::exists(root), "new destination already exists");
    bool cpp = args.size() == 2 || args[3] == "cxx";
    std::string suffix = cpp ? "cpp" : "c";
    Json manifest{{"manifest_format", 0}, {"package", {{"name", name}, {"version", "0.1.0"}}}, {"sdk", {{"profile", "dev"}, {"version", "*"}, {"target", "native"}, {"storage", "shared-immutable"}}}, {"include_dirs", {"src/main"}}, {"application", {{"name", name}, {"sources", {"src/main/main." + suffix, "src/main/greeting." + suffix}}}}, {"tests", {{{"name", "greeting-test"}, {"sources", {"src/test/greeting_test." + suffix, "src/main/greeting." + suffix}}}}}};
    fs::create_directories(root / "src/main"); fs::create_directories(root / "src/test");
    write_text(root / "crt.json", manifest.dump(2) + "\n");
    write_text(root / "crt.lock", Json{{"lock_format", 0}, {"state", "unresolved"}}.dump(2) + "\n");
    write_text(root / ".gitignore", "/out/\n/.crt/\n");
    write_text(root / "src/main/greeting.h", "#pragma once\nconst char* greeting(void);\n");
    write_text(root / ("src/main/greeting." + suffix), "#include \"greeting.h\"\nconst char* greeting(void) { return \"Hello from CRT\"; }\n");
    write_text(root / ("src/main/main." + suffix), "#include \"greeting.h\"\n#include <stdio.h>\nint main(void) { puts(greeting()); return 0; }\n");
    write_text(root / ("src/test/greeting_test." + suffix), "#include \"greeting.h\"\n#include <string.h>\nint main(void) { return strcmp(greeting(), \"Hello from CRT\") != 0; }\n");
    return {0, "new", {{"path", root.string()}, {"sdk_acquired", false}}, "created " + root.string()};
}
}
Reply workflow(const std::vector<std::string>& args) {
    require(!args.empty(), "missing workflow command");
    if (args[0] == "new") return create(args);
    require(args[0] == "configure" || args[0] == "build" || args[0] == "test" || args[0] == "run", "unsupported command or arguments");
    fs::path root = fs::current_path();
    std::map<std::string, std::string> options;
    const std::set<std::string> configure_options{"--sdk-root", "--sdk-contract", "--contract-sha256", "--cc", "--cxx", "--linker"};
    for (size_t i = 1; i < args.size(); i += 2) {
        require(i + 1 < args.size() && (args[i] == "--project" || (args[0] == "configure" && configure_options.count(args[i]))), "unsupported/missing workflow option");
        require(options.emplace(args[i], args[i + 1]).second, "duplicate workflow option");
    }
    if (options.count("--project")) root = options.at("--project");
    root = fs::canonical(root);
    auto manifest = project(root);
    std::set<std::string> target_names{manifest.at("application").at("name")};
    for (const auto& test : manifest.value("tests", Json::array()))
        require(target_names.insert(test.at("name").get<std::string>()).second, "duplicate application/test target name");
    bool explicit_selection = args[0] == "configure" && options.size() > options.count("--project");
    auto toolchain = explicit_selection ? Toolchain::configure(options) : Toolchain::load(root);
    match_request(manifest, toolchain);
    if (args[0] == "configure") {
        require(!fs::is_symlink(root / ".crt") && !fs::is_symlink(root / ".crt/local.json") && !fs::is_symlink(root / "crt.lock"), "project binding outputs must not be symlinks");
        fs::create_directories(inside(root, ".crt"));
        write_text(inside(root, ".crt/local.json"), toolchain.local.dump(2) + "\n");
        write_text(inside(root, "crt.lock"), Json{{"lock_format", 0}, {"state", "local-experiment"}, {"selection", toolchain.selection}}.dump(2) + "\n");
        return {0, "configure", {{"selection", toolchain.selection}, {"promotion", "none"}}, "configured local SDK experiment"};
    }
    auto build = inside(root, "out/native");
    require(!fs::is_symlink(root / "out") && !fs::is_symlink(root / "out/native") && !fs::is_symlink(build / "generated") &&
            !fs::is_symlink(build / "generated/bootstrap_config.h") && !fs::is_symlink(build / "last-build.json"), "build outputs must not be symlinks");
    fs::create_directories(build / "generated");
    auto selection = toolchain.selection;
    write_text(build / "generated/bootstrap_config.h", "#pragma once\n#define CRT_CLI_VERSION \"0.1.0-dev\"\n#define CRT_CLI_TARGET_OS \"linux\"\n#define CRT_CLI_TARGET_ARCH \"x86_64\"\n#define CRT_CLI_SDK_VERSION " + selection.at("version").dump() + "\n#define CRT_CLI_SDK_COMMIT " + selection.at("producer_source_commit").dump() + "\n#define CRT_CLI_SDK_CONTENT " + selection.at("sdk_content_sha256").dump() + "\n");
    Json commands = Json::array();
    auto application = build_target(root, manifest, manifest.at("application"), toolchain, build, commands);
    Json outcomes = Json::array(); int code = 0;
    if (args[0] == "test") {
        require(manifest.contains("tests") && !manifest.at("tests").empty(), "project declares no tests");
        for (const auto& target : manifest.at("tests")) {
            auto test = build_target(root, manifest, target, toolchain, build, commands);
            unsigned timeout = target.value("timeout_seconds", 180u);
            require(timeout >= 1 && timeout <= 3600, "test timeout_seconds must be between 1 and 3600");
            auto child = execute({test.string(), application.string(), root.string()}, timeout);
            outcomes.push_back({{"name", target.at("name")}, {"exit_code", child.exit_code}, {"timed_out", child.timed_out}, {"stdout_truncated", child.output_truncated}});
            if (child.exit_code) code = 1;
        }
    } else if (args[0] == "run") {
        auto child = execute({application.string()});
        outcomes.push_back({{"exit_code", child.exit_code}, {"timed_out", child.timed_out}, {"stdout_truncated", child.output_truncated}});
        code = child.exit_code ? 1 : 0;
    }
    Json receipt{{"selection", selection}, {"commands", commands}, {"outcomes", outcomes}, {"executable", application.string()}, {"executable_sha256", file_sha256(application)}};
    write_text(build / "last-build.json", receipt.dump(2) + "\n");
    return {code, args[0], receipt, code ? "child workflow failed" : args[0] + " completed"};
}
}
