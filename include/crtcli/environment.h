#pragma once
#include <crtcli/core.h>
namespace crtcli {
struct ChildEnvironment {
    std::vector<std::string> values;
    Json fixed, inherited_names;
};
// Diagnostic output exposes computed values and inherited names only, never inherited values.
ChildEnvironment child_environment(bool host_tool = false, const std::vector<std::string>& runtime_path = {});
}
