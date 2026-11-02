#include <crtcli/host_tools.h>
#include <crtcli/toolchain.h>
#include <algorithm>
#include <map>
#include <set>
#include <unistd.h>
namespace crtcli {
namespace {
bool hex_pin(const std::string& value) {
    return value.size() == 64 && std::all_of(value.begin(), value.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}
std::map<std::string, std::string> options(const std::vector<std::string>& args, bool run) {
    const std::set<std::string> common{"--host-tools", "--host-tools-sha256", "--work-root", "--project", "--profile"};
    std::map<std::string, std::string> result;
    for (size_t i = 1; i < args.size(); i += 2) {
        require(i + 1 < args.size() && (common.count(args[i]) || (run && (args[i] == "--script" || args[i] == "--directory" || args[i] == "--timeout"))), "unsupported/missing port environment option");
        require(result.emplace(args[i], args[i + 1]).second, "duplicate port environment option");
    }
    for (const auto& name : {"--host-tools", "--host-tools-sha256", "--work-root"}) require(result.count(name), std::string("port-build requires ") + name);
    if (result.count("--profile")) require(result.at("--profile") == "port-build", "shell/host-tools require profile port-build");
    return result;
}
struct Context {
    HostTools tools;
    fs::path work, sdk;
    Json data;
    ChildEnvironment child;
    explicit Context(const std::map<std::string, std::string>& opts)
        : tools(HostTools::load(opts.at("--host-tools"), opts.at("--host-tools-sha256"))),
          work(fs::absolute(opts.at("--work-root")).lexically_normal()),
          data{{"environment_format", 0}, {"profile", "port-build"}, {"build_host", native_target()},
               {"scope", "child-process"}, {"host_tools_identity", tools.identity()}, {"work_root", work.string()}} {
        require(!fs::exists(work) && !fs::is_symlink(work), "work-root must be a new job path");
        // Resolve the existing parent before checking ownership, including directory links.
        work = fs::canonical(work.parent_path()) / work.filename();
        data["work_root"] = work.string();
        auto relative = work.lexically_relative(tools.root);
        require(relative.empty() || *relative.begin() == "..", "job must be outside immutable host tools");
        if (opts.count("--project")) {
            auto project = fs::canonical(opts.at("--project"));
            auto toolchain = Toolchain::load(project);
            sdk = toolchain.sdk;
            auto relative_sdk = work.lexically_relative(sdk);
            require(relative_sdk.empty() || *relative_sdk.begin() == "..", "job must be outside immutable SDK");
            data["project"] = project.string();
            data["selection"] = toolchain.selection;
        }
        child = tools.environment(work, sdk);
        data["fixed"] = child.fixed;
        data["inherited_names"] = child.inherited_names;
        data["other_inheritance"] = "none";
        data["interactive"] = false;
    }
};
}
HostTools HostTools::load(const fs::path& requested, const std::string& pin) {
    require(hex_pin(pin), "host-tools manifest SHA-256 must be lowercase hex");
    auto root = fs::canonical(requested);
    auto file = root / "manifest.json";
    require(!fs::is_symlink(file) && fs::is_regular_file(file) && fs::file_size(file) <= 16 * 1024 * 1024, "invalid host-tools manifest file");
    require(file_sha256(file) == pin, "host-tools manifest SHA-256 mismatch");
    auto manifest = read_json(file);
    require(manifest.value("schema", "") == "crt.experimental.host-tools" && manifest.value("format", 0) == 1 &&
            manifest.value("promotion", "") == "local-only", "unsupported host-tools contract");
    require(manifest.at("host") == native_target(), "host tools do not match the execution host");
    auto expected = manifest.at("inventory");
    require(expected.is_array() && expected.size() <= 16384 && !expected.empty(), "invalid host-tools inventory");
    require(sha256(expected.dump()) == manifest.at("content_sha256"), "host-tools content identity mismatch");
    auto actual = inventory(root);
    actual.erase(std::remove_if(actual.begin(), actual.end(), [](const Json& e) { return e.at(0) == "manifest.json"; }), actual.end());
    require(actual == expected, "host-tools payload/alias inventory mismatch");
    const auto& roles = manifest.at("tools");
    require(roles.is_object() && roles.size() == 4, "invalid host-tools roles");
    for (const auto& role : {"shell", "toybox", "awk", "make"}) {
        auto path = inside(root, roles.at(role));
        native_executable(path);
        require(access(path.string().c_str(), X_OK) == 0, "host tool is not executable: " + path.string());
    }
    std::set<std::string> executables;
    for (auto it = roles.begin(); it != roles.end(); ++it) executables.insert(it.value().get<std::string>());
    for (const auto& alias : manifest.at("aliases")) {
        auto path = inside(root, alias.at("path"));
        auto role = alias.at("role").get<std::string>();
        require(roles.contains(role), "unknown host-tools alias role");
        require((fs::is_symlink(root / alias.at("path").get<std::string>()) ? "symlink" : "file") == alias.at("kind"), "host-tools alias kind mismatch");
        require(file_sha256(path) == file_sha256(inside(root, roles.at(role))), "host-tools alias dispatcher mismatch");
        executables.insert(alias.at("path").get<std::string>());
    }
    require(Json(executables) == manifest.at("executable_files"), "host-tools executable inventory mismatch");
    for (const auto& relative : executables)
        require(access(inside(root, relative).string().c_str(), X_OK) == 0, "host-tools alias is not executable: " + relative);
    const auto& layout = manifest.at("rootfs");
    require(layout.at("layout") == Json::array({"system", "bin", "usr"}) &&
            layout.at("writable") == Json::array({"tmp", "data/local", "data/local/tmp"}), "unsupported host-tools rootfs layout");
    auto suffix = native_target().at("os") == "windows" ? ".exe" : "";
    for (const auto& role : {"shell", "make", "awk"}) {
        std::string name = std::string(role) == "shell" ? "sh" : role;
        require(layout.at(role) == "/system/bin/" + name + suffix, "unsupported host-tools virtual entry point");
        require(file_sha256(inside(root, "system/bin/" + name + suffix)) == file_sha256(inside(root, roles.at(role))), "virtual alias does not match selected host tool");
    }
    return {root, manifest, pin};
}
Json HostTools::identity() const {
    return {{"host", manifest.at("host")}, {"version", manifest.at("version")},
            {"manifest_sha256", manifest_sha256}, {"content_sha256", manifest.at("content_sha256")},
            {"assembler", manifest.at("assembler")}, {"promotion", "local-only"}};
}
ChildEnvironment HostTools::environment(const fs::path& work, const fs::path& sdk) const {
    auto result = child_environment();
    auto path = (root / "system/bin").string();
    auto separator = native_target().at("os") == "windows" ? ';' : ':';
    require(path.find(separator) == std::string::npos, "host-tools path contains the PATH delimiter");
    result.fixed["PATH"] = path;
    result.fixed["CRT_ROOTFS"] = (work / "rootfs").string();
    result.fixed["TMPDIR"] = (work / "rootfs/tmp").string();
    result.fixed["TMP"] = result.fixed["TMPDIR"];
    result.fixed["TEMP"] = result.fixed["TMPDIR"];
    result.fixed["SHELL"] = manifest.at("rootfs").at("shell");
    result.fixed["CONFIG_SHELL"] = result.fixed["SHELL"];
    result.fixed["MAKE"] = manifest.at("rootfs").at("make");
    result.fixed["AWK"] = manifest.at("rootfs").at("awk");
    result.fixed["MAKEFLAGS"] = "SHELL=" + result.fixed.at("SHELL").get<std::string>();
    if (!sdk.empty()) result.fixed["CRT_SYSROOT"] = sdk.string();
    std::map<std::string, std::string> values;
    for (const auto& item : result.values) { auto eq = item.find('='); values[item.substr(0, eq)] = item.substr(eq + 1); }
    for (auto it = result.fixed.begin(); it != result.fixed.end(); ++it) values[it.key()] = it.value().get<std::string>();
    result.inherited_names.erase(std::remove(result.inherited_names.begin(), result.inherited_names.end(), Json("TMPDIR")), result.inherited_names.end());
    result.values.clear();
    for (const auto& item : values) result.values.push_back(item.first + "=" + item.second);
    return result;
}
void HostTools::create_job(const fs::path& work) const {
    require(fs::is_directory(work.parent_path()) && fs::create_directory(work), "cannot create a new host-tools job directory");
    try {
        fs::create_directory(work / "rootfs");
        for (const auto& name : {"system", "bin", "usr"}) fs::create_directory_symlink(root / name, work / "rootfs" / name);
        for (const auto& name : {"tmp", "data/local", "data/local/tmp"}) fs::create_directories(work / "rootfs" / name);
    } catch (...) {
        fs::remove_all(work);
        throw Error("cannot create real job-rootfs links; Windows requires Developer Mode; no substitute aliases are created");
    }
}
Reply host_environment(const std::vector<std::string>& args) {
    Context context(options(args, false));
    return {0, "env", context.data, context.data.dump(2)};
}
Reply shell(const std::vector<std::string>& args) {
    auto opts = options(args, true);
    require(opts.count("--script"), "shell requires an explicit non-interactive --script");
    auto script = fs::canonical(opts.at("--script"));
    require(fs::is_regular_file(script), "shell script must be a file");
    auto directory = fs::canonical(opts.count("--directory") ? fs::path(opts.at("--directory")) : fs::current_path());
    require(fs::is_directory(directory), "shell directory must exist");
    unsigned timeout = 180;
    if (opts.count("--timeout")) {
        const auto& text = opts.at("--timeout");
        require(!text.empty() && text.size() <= 4 && std::all_of(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; }), "invalid shell timeout");
        timeout = static_cast<unsigned>(std::stoul(text));
        require(timeout >= 1 && timeout <= 3600, "shell timeout must be 1..3600 seconds");
    }
    Context context(opts);
    context.tools.create_job(context.work);
    auto child = execute_child({inside(context.tools.root, context.tools.manifest.at("tools").at("shell")).string(), script.string()}, context.child, directory, timeout, true);
    context.data["stdout"] = child.output;
    context.data["output_truncated"] = child.output_truncated;
    context.data["timed_out"] = child.timed_out;
    context.data["exit_code"] = child.exit_code;
    return {child.exit_code, "shell", context.data, child.output};
}
}
