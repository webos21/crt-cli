#include <crtcli/toolchain.h>
#include <set>
#include <map>
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
    require(manifest.at("manifest_format") == 0 || manifest.at("manifest_format") == 1, "unsupported project manifest format");
    require(identifier(manifest.at("package").at("name")), "invalid package name");
    auto sdk = manifest.at("sdk");
    require(sdk.at("profile") == "dev" && sdk.at("target") == "native" && sdk.at("storage") == "shared-immutable", "unsupported SDK request");
    require(manifest.contains("targets") != manifest.contains("application"), "declare targets or legacy application, not both");
    return manifest;
}

void match_request(const Json& manifest, const Toolchain& toolchain) {
    auto version = manifest.at("sdk").at("version").get<std::string>();
    require(version == "*" || version == toolchain.selection.at("version"), "project SDK version mismatch");
}

std::string kind(const Json& target) { return target.value("kind", "executable"); }
std::string linkage(const Json& target) {
    return target.value("runtime_linkage", kind(target) == "shared-library" ? "shared" : "static");
}
std::string mode(const Json& target) {
    return kind(target) == "executable" ? "executable-" + linkage(target) : kind(target);
}

struct Builder {
    fs::path root, build;
    Json manifest, consumer_options, commands = Json::array(), artifacts = Json::object();
    size_t executed = 0, reused = 0;
    const Toolchain& toolchain;
    std::map<std::string, Json> targets;
    std::map<std::string, fs::path> outputs;
    std::map<std::string, bool> cpp;
    std::set<std::string> visiting;

    Builder(fs::path source, fs::path directory, Json declaration, const Toolchain& tools)
        : root(std::move(source)), build(std::move(directory)), manifest(std::move(declaration)), toolchain(tools) {
        consumer_options = read_json(toolchain.sdk / "share/crt/contracts/development.json").at("consumer_compile_options");
        auto list = manifest.contains("targets") ? manifest.at("targets") : Json::array({manifest.at("application")});
        require(list.is_array() && !list.empty(), "project targets are required");
        for (const auto& target : list) add(target, false);
        for (const auto& target : manifest.value("tests", Json::array())) add(target, true);
        for (const auto& entry : targets) {
            for (const auto& value : entry.second.value("dependencies", Json::array())) {
                std::string name = value;
                require(targets.count(name) && kind(targets.at(name)) != "executable", "dependency must name a library target: " + name);
                require(!(linkage(entry.second) == "static" && kind(entry.second) == "executable" && kind(targets.at(name)) == "shared-library"), "a shared-library consumer must select shared runtime linkage");
            }
        }
    }

    void add(const Json& target, bool test) {
        std::string name = target.at("name");
        require(identifier(name) && targets.emplace(name, target).second, "invalid or duplicate target name: " + name);
        require(kind(target) == "executable" || kind(target) == "static-library" || kind(target) == "shared-library", "unsupported target kind");
        require(!test || kind(target) == "executable", "test target must be executable");
        require(linkage(target) == "static" || linkage(target) == "shared", "unsupported runtime linkage");
        require(kind(target) != "shared-library" || linkage(target) == "shared", "shared libraries require shared runtime linkage");
        require(target.at("sources").is_array() && !target.at("sources").empty(), "target requires a source array");
    }

    fs::path output(const std::string& name) const {
        const auto& target = targets.at(name);
        const auto& names = toolchain.contract.at("artifacts");
        std::string file = name;
        if (kind(target) == "executable") file += names.at("executable_suffix").get<std::string>();
        else {
            std::string prefix = kind(target) == "static-library" ? "static_library_" : "shared_library_";
            file = names.at(prefix + "prefix").get<std::string>() + name + names.at(prefix + "suffix").get<std::string>();
        }
        require(fs::path(file).filename() == file, "invalid SDK artifact naming policy");
        require(!fs::is_symlink(root / "out/native" / file), "build output must not be a symlink");
        return inside(root, "out/native/" + file);
    }

    void sdk_libraries(const Json& target, const std::string& selected_linkage, std::vector<std::string>& libraries) const {
        std::string selected = target.value("sdk_library_linkage", selected_linkage);
        require(selected == "static" || selected == "shared", "unsupported SDK library linkage");
        for (const auto& value : target.value("sdk_libraries", Json::array())) {
            std::string name = value;
            require(identifier(name), "invalid SDK library name");
            auto library = toolchain.contract.at("libraries").at(name).at(selected).get<std::string>();
            libraries.push_back(inside(toolchain.sdk, library).string());
        }
    }

    void dependencies(const Json& target, const std::string& selected_linkage, std::vector<std::string>& libraries, std::set<std::string>& seen) {
        for (const auto& value : target.value("dependencies", Json::array())) {
            std::string name = value;
            if (!seen.insert(name).second) continue;
            require(selected_linkage == "shared" || kind(targets.at(name)) != "shared-library", "transitive shared dependency requires shared runtime linkage");
            libraries.push_back(outputs.at(name).string());
            dependencies(targets.at(name), selected_linkage, libraries, seen);
            sdk_libraries(targets.at(name), selected_linkage, libraries);
            seen.erase(name); // Repeat diamond dependencies after each dependent archive.
        }
    }

    fs::path build_one(const std::string& name) {
        require(targets.count(name), "unknown target: " + name);
        if (outputs.count(name)) return outputs.at(name);
        require(visiting.insert(name).second, "target dependency cycle: " + name);
        const auto& target = targets.at(name);
        bool uses_cpp = false;
        for (const auto& value : target.value("dependencies", Json::array())) {
            std::string dependency = value;
            build_one(dependency);
            uses_cpp |= cpp.at(dependency);
        }
        auto destination = output(name);
        require(!fs::is_symlink(build / destination.filename()), "build output must not be a symlink");
        std::vector<std::string> objects, libraries;
        std::set<std::string> seen;
        for (const auto& source : target.at("sources")) {
            auto file = inside(root, source.get<std::string>());
            require(fs::is_regular_file(file), "missing source: " + file.string());
            require(seen.insert(file.string()).second, "duplicate source: " + file.string());
            auto extension = file.extension().string();
            bool is_cpp = extension == ".cpp" || extension == ".cc" || extension == ".cxx";
            require(is_cpp || extension == ".c", "unsupported C/C++ source extension");
            uses_cpp |= is_cpp;
            auto object = build / (name + "-" + sha256(file.string()).substr(0, 16) + ".o");
            require(!fs::is_symlink(object), "object output must not be a symlink");
            std::vector<std::string> flags{is_cpp ? "-std=c++17" : "-std=c99", "-O1", "-g", "-I" + (build / "generated").string()};
            for (const auto& dir : manifest.value("include_dirs", Json::array())) flags.push_back("-I" + inside(root, dir).string());
            for (const auto& dir : target.value("include_dirs", Json::array())) flags.push_back("-I" + inside(root, dir).string());
            // The SDK's consumer options (Windows: -U_WIN32 ... so the curl headers take their POSIX branch) change how libc++
            // sees the target, so they apply only to the translation units that declare they need them, never to <filesystem> users.
            bool wants_options = false;
            for (const auto& listed : target.value("sdk_consumer_option_sources", Json::array())) wants_options |= listed == source;
            for (const auto& option : consumer_options) {
                if (!wants_options) break;
                auto text = option.get<std::string>();
                require(text.rfind("-U", 0) == 0, "unsupported SDK consumer option");
                flags.push_back(text);
            }
            (toolchain.invoke("compile", is_cpp, {file.string()}, object, flags, {}, commands, mode(target)) ? executed : reused)++;
            objects.push_back(object.string());
        }
        std::set<std::string> linked;
        dependencies(target, linkage(target), libraries, linked);
        sdk_libraries(target, linkage(target), libraries);
        (toolchain.invoke(kind(target) == "static-library" ? "archive" : "link", uses_cpp, objects, destination, {}, libraries, commands, mode(target)) ? executed : reused)++;
        outputs[name] = destination;
        cpp[name] = uses_cpp;
        visiting.erase(name);
        artifacts[name] = {{"path", destination.string()}, {"kind", kind(target)}, {"runtime_linkage", linkage(target)}, {"sha256", file_sha256(destination)}};
        return destination;
    }
};

Reply create(const std::vector<std::string>& args) {
    require(args.size() >= 2, "usage: new <directory> [--language c|cxx] [--kind executable|static-library|shared-library] [--runtime-linkage static|shared]");
    std::map<std::string, std::string> options;
    for (size_t i = 2; i < args.size(); i += 2) {
        require(i + 1 < args.size() && (args[i] == "--language" || args[i] == "--kind" || args[i] == "--runtime-linkage"), "unsupported/missing new option");
        require(options.emplace(args[i], args[i + 1]).second, "duplicate new option");
    }
    std::string language = options.count("--language") ? options.at("--language") : "cxx";
    std::string type = options.count("--kind") ? options.at("--kind") : "executable";
    std::string runtime = options.count("--runtime-linkage") ? options.at("--runtime-linkage") : type == "shared-library" ? "shared" : "static";
    require(language == "c" || language == "cxx", "unsupported language");
    require(type == "executable" || type == "static-library" || type == "shared-library", "unsupported target kind");
    require((runtime == "static" || runtime == "shared") && (type != "shared-library" || runtime == "shared"), "unsupported kind/runtime linkage");
    auto root = fs::absolute(args[1]).lexically_normal();
    auto name = root.filename().string();
    require(identifier(name), "project directory basename must be a package identifier");
    require(!fs::exists(root), "new destination already exists");
    std::string suffix = language == "cxx" ? "cpp" : "c";
    Json sources = Json::array({"src/main/greeting." + suffix});
    if (type == "executable") sources.push_back("src/main/main." + suffix);
    Json target{{"name", name}, {"kind", type}, {"runtime_linkage", runtime}, {"sources", sources}};
    Json test{{"name", "greeting-test"}, {"kind", "executable"}, {"runtime_linkage", runtime}, {"sources", {"src/test/greeting_test." + suffix}}};
    if (type == "executable") test["sources"].push_back("src/main/greeting." + suffix);
    else test["dependencies"] = {name};
    Json manifest{{"manifest_format", 1}, {"package", {{"name", name}, {"version", "0.1.0"}}},
        {"sdk", {{"profile", "dev"}, {"version", "*"}, {"target", "native"}, {"storage", "shared-immutable"}}},
        {"include_dirs", {"include"}}, {"targets", {target}}, {"tests", {test}}};
    fs::create_directories(root / "src/main"); fs::create_directories(root / "src/test"); fs::create_directories(root / "include");
    write_text(root / "crt.json", manifest.dump(2) + "\n");
    write_text(root / "crt.lock", Json{{"lock_format", 0}, {"state", "unresolved"}}.dump(2) + "\n");
    write_text(root / ".gitignore", "/out/\n/.crt/\n");
    write_text(root / "include/greeting.h", "#pragma once\nconst char* greeting(void);\n");
    write_text(root / ("src/main/greeting." + suffix), "#include <greeting.h>\nconst char* greeting(void) { return \"Hello from CRT\"; }\n");
    if (type == "executable") write_text(root / ("src/main/main." + suffix), "#include <greeting.h>\n#include <stdio.h>\nint main(void) { puts(greeting()); return 0; }\n");
    write_text(root / ("src/test/greeting_test." + suffix), "#include <greeting.h>\n#include <string.h>\nint main(void) { return strcmp(greeting(), \"Hello from CRT\") != 0; }\n");
    return {0, "new", {{"path", root.string()}, {"sdk_acquired", false}}, "created " + root.string()};
}
}

Reply workflow(const std::vector<std::string>& args) {
    require(!args.empty(), "missing workflow command");
    if (args[0] == "new") return create(args);
    require(args[0] == "configure" || args[0] == "build" || args[0] == "test" || args[0] == "run", "unsupported command or arguments");
    fs::path root = fs::current_path();
    std::map<std::string, std::string> options;
    const std::set<std::string> configure_options{"--sdk-root", "--sdk-id", "--sdk-contract", "--contract-sha256", "--cc", "--cxx", "--linker", "--ar", "--allow-dirty-sdk"};
    for (size_t i = 1; i < args.size(); i += 2) {
        require(i + 1 < args.size() && (args[i] == "--project" || (args[0] != "configure" && args[i] == "--target") ||
            (args[0] == "configure" && configure_options.count(args[i]))), "unsupported/missing workflow option");
        require(options.emplace(args[i], args[i + 1]).second, "duplicate workflow option");
    }
    if (options.count("--project")) root = options.at("--project");
    root = fs::canonical(root);
    auto manifest = project(root);
    bool explicit_selection = args[0] == "configure" && options.size() > options.count("--project");
    auto toolchain = explicit_selection ? Toolchain::configure(options) : Toolchain::load(root);
    match_request(manifest, toolchain);
    Builder builder(root, inside(root, "out/native"), manifest, toolchain);
    if (args[0] == "configure") {
        require(!fs::is_symlink(root / ".crt") && !fs::is_symlink(root / ".crt/local.json") && !fs::is_symlink(root / "crt.lock"), "project binding outputs must not be symlinks");
        fs::create_directories(inside(root, ".crt"));
        write_text(inside(root, ".crt/local.json"), toolchain.local.dump(2) + "\n");
        write_text(inside(root, "crt.lock"), Json{{"lock_format", 0}, {"state", "local-experiment"}, {"selection", toolchain.selection}}.dump(2) + "\n");
        return {0, "configure", {{"selection", toolchain.selection}, {"promotion", "none"}}, "configured local SDK experiment"};
    }
    auto build = builder.build;
    require(!fs::is_symlink(root / "out") && !fs::is_symlink(root / "out/native") && !fs::is_symlink(build / "generated") &&
            !fs::is_symlink(build / "generated/bootstrap_config.h") && !fs::is_symlink(build / "last-build.json"), "build outputs must not be symlinks");
    fs::create_directories(build / "generated");
    auto selection = toolchain.selection;
    write_text(build / "generated/bootstrap_config.h", "#pragma once\n#define CRT_CLI_VERSION \"0.1.0-dev\"\n#define CRT_CLI_TARGET_OS " + selection.at("target").at("os").dump() + "\n#define CRT_CLI_TARGET_ARCH " + selection.at("target").at("arch").dump() + "\n#define CRT_CLI_SDK_VERSION " + selection.at("version").dump() + "\n#define CRT_CLI_SDK_COMMIT " + selection.at("producer_source_commit").dump() + "\n#define CRT_CLI_SDK_CONTENT " + selection.at("sdk_content_sha256").dump() + "\n");
    auto list = manifest.contains("targets") ? manifest.at("targets") : Json::array({manifest.at("application")});
    std::string primary;
    for (const auto& target : list) if (kind(target) == "executable" && primary.empty()) primary = target.at("name");
    std::string chosen = options.count("--target") ? options.at("--target") : primary;
    if (args[0] == "run") require(!chosen.empty() && builder.targets.count(chosen) && kind(builder.targets.at(chosen)) == "executable", "run requires an executable target");
    if (args[0] == "run" || (args[0] == "build" && options.count("--target"))) builder.build_one(chosen);
    else for (const auto& target : list) builder.build_one(target.at("name"));
    Json outcomes = Json::array(); int code = 0;
    // The SDK's runtime images (Windows DLLs live in bin/ and lib/) for programs linked against its shared runtime.
    const std::vector<std::string> runtime_path{(toolchain.sdk / "bin").string(), (toolchain.sdk / "lib").string()};
    if (args[0] == "test") {
        require(manifest.contains("tests") && !manifest.at("tests").empty(), "project declares no tests");
        for (const auto& target : manifest.at("tests")) {
            if (options.count("--target") && target.at("name") != chosen) continue;
            auto test = builder.build_one(target.at("name"));
            unsigned timeout = target.value("timeout_seconds", 180u);
            require(timeout >= 1 && timeout <= 3600, "test timeout_seconds must be between 1 and 3600");
            auto child = execute({test.string(), primary.empty() ? "" : builder.output(primary).string(), root.string()}, timeout, true, false, runtime_path);
            outcomes.push_back({{"name", target.at("name")}, {"exit_code", child.exit_code}, {"timed_out", child.timed_out}, {"stdout_truncated", child.output_truncated}});
            if (child.exit_code) code = 1;
        }
        require(!outcomes.empty(), "unknown test target");
    } else if (args[0] == "run") {
        auto child = execute({builder.output(chosen).string()}, 180, true, false, runtime_path);
        outcomes.push_back({{"exit_code", child.exit_code}, {"timed_out", child.timed_out}, {"stdout_truncated", child.output_truncated}});
        code = child.exit_code ? 1 : 0;
    }
    Json receipt{{"selection", selection}, {"commands", builder.commands}, {"cache", {{"executed", builder.executed}, {"reused", builder.reused}}}, {"outcomes", outcomes}, {"artifacts", builder.artifacts}};
    if (!primary.empty() && builder.outputs.count(primary)) {
        receipt["executable"] = builder.output(primary).string();
        receipt["executable_sha256"] = file_sha256(builder.output(primary));
    }
    write_text(build / "last-build.json", receipt.dump(2, ' ', false, Json::error_handler_t::replace) + "\n");
    return {code, args[0], receipt, code ? "child workflow failed" : args[0] + " completed"};
}
}
