#pragma once
#include "core.h"
#include <map>
namespace crtcli {
struct Toolchain {
    fs::path sdk, contract_path, cc, cxx, linker, resource;
    Json contract, selection, local;
    static Toolchain configure(const std::map<std::string, std::string>& options);
    static Toolchain load(const fs::path& root);
    void invoke(const std::string& operation, bool cpp, const std::vector<std::string>& inputs,
                const fs::path& output, const std::vector<std::string>& extra, const std::vector<std::string>& libraries,
                Json& commands) const;
};
}
