#pragma once
#include <crtcli/core.h>
namespace crtcli {
// Uses the native project's planner without building or publishing a binding.
Json inspect_project(const fs::path& root);
Reply editor_command(const std::vector<std::string>& args);
}
