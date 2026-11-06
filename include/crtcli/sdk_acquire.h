#pragma once
#include <crtcli/core.h>

namespace crtcli {
// Explicit consumer-owned format-0 source: fixed HTTPS metadata/assets, CA,
// SDK identity and compiler-contract pin. No channels or default publisher.
Json acquire_sdk(const fs::path& source, const fs::path& store, const std::string& version = "*");
}
