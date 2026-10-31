#include <crtcli/publisher.h>
#include <mbedtls/pk.h>
#include <fstream>
namespace crtcli {
namespace {
std::string bounded_file(const fs::path& path, size_t limit) {
    if (!fs::is_regular_file(path)) throw Error("publisher input must be a regular file: " + path.string(), 2);
    std::ifstream input(path.string(), std::ios::binary);
    if (!input) throw Error("cannot read publisher input: " + path.string(), 2);
    std::string bytes(limit + 1, '\0');
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    bytes.resize(static_cast<size_t>(input.gcount()));
    if (input.bad() || bytes.empty() || bytes.size() > limit) throw Error("empty, unreadable or oversized publisher input", 2);
    return bytes;
}
}
struct PublisherSignature::Impl {
    mbedtls_pk_context key;
    std::string signature;
    std::string fingerprint;
    Impl() { mbedtls_pk_init(&key); }
    ~Impl() { mbedtls_pk_free(&key); }
};
PublisherSignature::PublisherSignature(const fs::path& public_key, const fs::path& signature) : impl_(new Impl) {
    auto pem = bounded_file(public_key, 16384);
    if (pem.rfind("-----BEGIN PUBLIC KEY-----\n", 0) != 0 &&
        pem.rfind("-----BEGIN PUBLIC KEY-----\r\n", 0) != 0)
        throw Error("publisher key must be a PEM SubjectPublicKeyInfo public key", 2);
    if (mbedtls_pk_parse_public_key(&impl_->key, reinterpret_cast<const unsigned char*>(pem.c_str()), pem.size() + 1) != 0 ||
        mbedtls_pk_get_type(&impl_->key) != MBEDTLS_PK_RSA ||
        mbedtls_pk_get_bitlen(&impl_->key) < 2048 || mbedtls_pk_get_bitlen(&impl_->key) > 4096)
        throw Error("publisher key must be RSA with 2048..4096 bits", 2);
    impl_->signature = bounded_file(signature, 512);
    if (impl_->signature.size() != mbedtls_pk_get_len(&impl_->key)) throw Error("signature length does not match publisher key", 2);
    unsigned char der[1024];
    int size = mbedtls_pk_write_pubkey_der(&impl_->key, der, sizeof(der));
    require(size > 0, "cannot encode publisher public key");
    impl_->fingerprint = sha256(std::string(reinterpret_cast<char*>(der + sizeof(der) - size), static_cast<size_t>(size)));
}
PublisherSignature::~PublisherSignature() = default;
bool PublisherSignature::verify(const std::string& hex) const {
    require(hex.size() == 64, "invalid signature digest length");
    unsigned char hash[32];
    auto nibble = [](char c) -> unsigned { require((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'), "invalid signature digest"); return c <= '9' ? c - '0' : c - 'a' + 10; };
    for (size_t i = 0; i < sizeof(hash); ++i) hash[i] = static_cast<unsigned char>((nibble(hex[2*i]) << 4) | nibble(hex[2*i+1]));
    mbedtls_pk_rsassa_pss_options options{MBEDTLS_MD_SHA256, 32};
    return mbedtls_pk_verify_ext(MBEDTLS_PK_RSASSA_PSS, &options, &impl_->key, MBEDTLS_MD_SHA256, hash, sizeof(hash),
        reinterpret_cast<const unsigned char*>(impl_->signature.data()), impl_->signature.size()) == 0;
}
std::string PublisherSignature::key_sha256() const { return impl_->fingerprint; }
}
