#pragma once
#include <crtcli/core.h>
#include <map>
namespace crtcli {
struct Toolchain {
    fs::path sdk, contract_path, cc, cxx, linker, archiver, resource;
    Json contract, selection, local;
    static Toolchain configure(const std::map<std::string, std::string>& options);
    static Toolchain load(const fs::path& root);
    // Returns false when the up-to-date output of an identical earlier command was reused.
    bool invoke(const std::string& operation, bool cpp, const std::vector<std::string>& inputs,
                const fs::path& output, const std::vector<std::string>& extra, const std::vector<std::string>& libraries,
                Json& commands, const std::string& mode = "executable-static") const;
};
}
