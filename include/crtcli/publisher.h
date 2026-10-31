#pragma once
#include <crtcli/core.h>
#include <memory>
namespace crtcli {
// Experimental caller-selected trust: no downloaded key is trusted implicitly.
// Verifies an RSA-PSS/SHA-256 signature over the exact downloaded bytes.
class PublisherSignature {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    PublisherSignature(const fs::path& public_key, const fs::path& signature);
    ~PublisherSignature();
    bool verify(const std::string& sha256_hex) const;
    std::string key_sha256() const;
};
}
