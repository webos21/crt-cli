#pragma once
#include <crtcli/environment.h>
namespace crtcli {
struct HostTools {
    fs::path root;
    Json manifest;
    std::string manifest_sha256;
    static HostTools load(const fs::path& root, const std::string& pin);
    ChildEnvironment environment(const fs::path& work, const fs::path& sdk = {}) const;
    Json identity() const;
    void create_job(const fs::path& work) const;
};
Reply host_environment(const std::vector<std::string>& args);
Reply shell(const std::vector<std::string>& args);
Reply doctor(const std::vector<std::string>& args);
}
