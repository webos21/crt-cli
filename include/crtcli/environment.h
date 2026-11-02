#pragma once
#include <crtcli/core.h>
namespace crtcli {
struct ChildEnvironment {
    std::vector<std::string> values;
    Json fixed, inherited_names;
};
// Diagnostic output exposes computed values and inherited names only, never inherited values.
ChildEnvironment child_environment(bool host_tool = false, const std::vector<std::string>& runtime_path = {});
// All process profiles share one spawn/timeout/output path. Working directory
// is applied in the child, without changing the CLI's current directory.
ProcessResult execute_child(const std::vector<std::string>& argv, const ChildEnvironment& environment,
                            const fs::path& directory = {}, unsigned timeout_seconds = 180, bool relay = true);
}
