#pragma once
// Executor for a parsed source-port recipe (D3 step 3): configure -> post-configure patch -> make -> make install -> the
// recipe's compile-run tests, all inside one new job directory and through the pinned host-tools bundle. See
// docs/design/source_port_adapter.md. Linux hosts only for now; every other host is refused rather than approximated.
//
// The executor does not fetch, audit or publish anything: the caller supplies an already verified pristine source tree,
// the result stays in the job directory, and promotion to a store item is a later step.
#include "crtcli/host_tools.h"
#include "crtcli/port_recipe.h"
#include "crtcli/toolchain.h"

namespace crtcli {

struct PortBuildRequest {
    PortRecipe recipe;                 // from parse_port_recipe (already digest-gated)
    fs::path source_tree;              // directory that contains <recipe.source.source_dir>; copied, never modified
    fs::path test_root;                // directory that contains the recipe's test sources (the SDK root); each must match
                                       // the SHA-256 the adapter table pins
    const Toolchain* toolchain = nullptr;
    const HostTools* tools = nullptr;
    fs::path job;                      // must not exist; created here, outside the host tools and the SDK
    unsigned jobs = 0;                 // make -j; 0 chooses min(hardware threads, 8)
    unsigned step_timeout_seconds = 900;
};

struct PortBuildResult {
    fs::path prefix;                   // <job>/prefix, the install result
    fs::path logs;                     // <job>/logs, one file per step with stdout and stderr merged
    Json record;                       // steps, patches, environment, test results and the prefix inventory
};

// Throws Error naming the failing step and its log file. The job directory is kept on failure for inspection.
PortBuildResult run_port_build(const PortBuildRequest& request);

// The llvm-ranlib that sits beside the archiver (llvm-ar-21 -> llvm-ranlib-21); throws when there is none.
fs::path port_ranlib_for(const fs::path& archiver);

// The environment a build declares and therefore keys on: the executor's fixed flag defaults and the recipe's own ops,
// with nothing that names a job directory. (Part of the store build key.)
Json port_declared_environment(const PortRecipe& recipe);

// Exposed for tests: the environment the build steps receive (names and values, inherited values omitted by the record).
std::map<std::string, std::string> port_build_environment(const PortBuildRequest& request, const fs::path& prefix);

}  // namespace crtcli
