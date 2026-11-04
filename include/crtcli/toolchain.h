#pragma once
#include <crtcli/core.h>
#include <map>
namespace crtcli {
struct Toolchain {
    fs::path sdk, contract_path, cc, cxx, linker, archiver, resource;
    Json contract, selection, local;
    // Maps the local, uncommitted `.sdk.json` (format 0) onto the `configure` option names.
    static std::map<std::string, std::string> read_sdk_config(const fs::path& file);
    // `--sdk <ref>` selects an SDK installed in the shared store by directory name (`<version>-<content12>`) or by a
    // content-identity prefix of at least 8 hex digits; `--sdk-root`/`--sdk-id` remain local handoff selections.
    static Toolchain configure(const std::map<std::string, std::string>& options);
    // The store directory of the only installed dev SDK for the native target whose version matches `version` ("*" matches
    // any). Throws, listing the candidates, when there are none or several: a project never follows a moving default.
    static std::string select_store_sdk(const std::string& version);
    static Toolchain load(const fs::path& root);
    // Returns false when the up-to-date output of an identical earlier command was reused.
    bool invoke(const std::string& operation, bool cpp, const std::vector<std::string>& inputs,
                const fs::path& output, const std::vector<std::string>& extra, const std::vector<std::string>& libraries,
                Json& commands, const std::string& mode = "executable-static") const;
};
}
