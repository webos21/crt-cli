#include <crtcli/environment.h>
#include <crt/platform.h>
#include <cstdlib>
namespace crtcli {
ChildEnvironment child_environment(bool host_tool, const std::vector<std::string>& runtime_path) {
    const bool windows = std::string(crt_get_platform_info()->os) == "windows";
    std::string search = "/nonexistent";
    if (windows && !runtime_path.empty()) {
        search.clear();
        for (const auto& directory : runtime_path) {
            require(fs::path(directory).is_absolute() && directory.find(';') == std::string::npos &&
                    directory.find('\0') == std::string::npos, "runtime search directory must be absolute and contain no PATH separator");
            require_native_utf8(directory, "runtime search directory");
            search += (search.empty() ? "" : ";") + directory;
        }
    }
    ChildEnvironment result{{}, {{"PATH", search}, {"LC_ALL", "C"}}, Json::array()};
    if (host_tool && windows) result.fixed["CRT_SPAWN_NATIVE_WINDOWS"] = "1";
    for (auto it = result.fixed.begin(); it != result.fixed.end(); ++it)
        result.values.push_back(it.key() + "=" + it.value().get<std::string>());
    for (const auto& key : {"HOME", "USERPROFILE", "TMPDIR", "SystemRoot"}) {
        if (const char* value = std::getenv(key)) {
            result.values.push_back(std::string(key) + "=" + value);
            result.inherited_names.push_back(key);
        }
    }
    return result;
}
}
