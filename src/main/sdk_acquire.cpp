#include <crtcli/sdk_acquire.h>
#include <crtcli/sdk_store.h>
#include <chrono>
#include <cctype>
#include <set>
#include <unistd.h>

namespace crtcli {
namespace {
void keys(const Json& value, std::initializer_list<const char*> allowed) {
    require(value.is_object(), "SDK source fields must be objects");
    std::set<std::string> names;
    for (auto name : allowed) names.insert(name);
    for (const auto& field : value.items()) require(names.count(field.key()), "unknown SDK source field: " + field.key());
}
std::string text(const Json& value, const char* key) {
    require(value.contains(key) && value.at(key).is_string(), std::string("SDK source requires ") + key);
    auto result = value.at(key).get<std::string>();
    require(!result.empty() && result.find('\0') == std::string::npos, std::string("empty or invalid SDK source ") + key);
    return result;
}
std::string pin(const Json& value, const char* key) {
    auto result = text(value, key);
    require(result.size() == 64 && result.find_first_not_of("0123456789abcdef") == std::string::npos,
            std::string("SDK source ") + key + " must be 64 lowercase hex digits");
    return result;
}
uint64_t size(const Json& value, const char* key, uint64_t maximum) {
    require(value.contains(key) && value.at(key).is_number_unsigned(), std::string("SDK source requires an unsigned ") + key);
    const auto result = value.at(key).get<uint64_t>();
    require(result >= 1 && result <= maximum, std::string("SDK source ") + key + " exceeds its supported size limit");
    return result;
}
std::string url(const Json& value, const char* key) {
    auto result = text(value, key);
    require(result.rfind("https://", 0) == 0, "SDK source URLs require HTTPS");
    const auto host = result.substr(8, result.find('/', 8) - 8);
    require(result.rfind("https://", 0) == 0 && !host.empty() && host.find('@') == std::string::npos &&
            result.find_first_of("?#\r\n\t ") == std::string::npos,
            "SDK source URLs require HTTPS without credentials, query or fragment");
    return result;
}
void filename(const std::string& name) {
    require(!name.empty() && name.size() <= 200 && name[0] != '.' && name.find_first_of("/\\:") == std::string::npos,
            "SDK asset name must be a plain file name");
    for (unsigned char c : name) require(std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == '+', "invalid SDK asset name");
    require(name != "txn.json" && name != "release-manifest.json", "SDK asset uses a reserved acquisition name");
}
Json asset(const Json& manifest, const std::string& name, uint64_t maximum) {
    require(manifest.contains("assets") && manifest.at("assets").is_array(), "release manifest requires assets");
    Json result;
    for (const auto& entry : manifest.at("assets")) {
        require(entry.is_object(), "invalid release manifest asset");
        if (entry.value("name", "") == name) { require(result.is_null(), "release manifest lists " + name + " twice"); result = entry; }
    }
    require(!result.is_null(), "release manifest has no asset named " + name);
    pin(result, "sha256"); size(result, "size", maximum);
    return result;
}
}

Json acquire_sdk(const fs::path& source, const fs::path& root, const std::string& version) {
    require(fs::is_regular_file(source) && fs::file_size(source) <= 65536, "SDK source must be a regular file of at most 64 KiB");
    const auto input = read_json(source);
    keys(input, {"schema", "format", "ca_file", "ca_sha256", "manifest", "archive", "contract_url", "sdk", "timeout_seconds"});
    require(input.at("schema") == "crt-cli.sdk-source" && input.at("format") == 0, "unsupported SDK source schema/format");
    const auto& meta = input.at("manifest"); keys(meta, {"url", "sha256", "size"});
    const auto& archive = input.at("archive"); keys(archive, {"name", "url"});
    const auto& sdk = input.at("sdk"); keys(sdk, {"version", "content_sha256", "contract_sha256"});
    const auto manifest_url = url(meta, "url"), manifest_pin = pin(meta, "sha256");
    const auto manifest_size = size(meta, "size", 1u << 20);
    const auto archive_url = url(archive, "url"), contract_url = url(input, "contract_url");
    const auto archive_name = text(archive, "name"); filename(archive_name);
    const auto content_pin = pin(sdk, "content_sha256"), contract_pin = pin(sdk, "contract_sha256");
    const auto sdk_version = text(sdk, "version");
    require(sdk_version.size() <= 64 && std::isalnum(static_cast<unsigned char>(sdk_version[0])), "invalid SDK source version");
    for (unsigned char c : sdk_version) require(std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == '+', "invalid SDK source version");
    require(version == "*" || version == sdk_version, "project SDK version differs from the pinned SDK source");
    const fs::path ca = text(input, "ca_file");
    const auto ca_pin = pin(input, "ca_sha256");
    auto check_ca = [&] {
        require(ca.is_absolute() && fs::is_regular_file(ca) && fs::file_size(ca) <= (1u << 20) && file_sha256(ca) == ca_pin,
                "SDK source CA must be an absolute regular file matching ca_sha256 (at most 1 MiB)");
    };
    check_ca();
    const auto timeout = input.contains("timeout_seconds") ? size(input, "timeout_seconds", 3600) : 60;
    require(root.is_absolute() && !fs::is_symlink(root) && !fs::is_symlink(root / ".staging"), "SDK acquisition store/staging must be absolute, non-symlink directories");
    const auto directory = sdk_version + "-" + content_pin.substr(0, 12);
    SdkStore store(root);
    Json provenance{{"source_sha256", file_sha256(source)}, {"manifest_sha256", manifest_pin}, {"ca_sha256", ca_pin},
                    {"integrity", "explicit-pinned-inputs"}, {"publisher_authentication", "not_verified"}, {"downloads", Json::array()}};
    if (fs::exists(fs::symlink_status(root / directory))) {
        require(!fs::is_symlink(root / directory) && fs::is_directory(root / directory), "installed SDK must be a real directory");
        store.verify(directory);
        const auto receipt = store.verified_receipt(directory);
        require(receipt.at("sdk").at("content_sha256") == content_pin && receipt.at("sdk").at("version") == sdk_version &&
                !receipt.at("contract").is_null() && receipt.at("contract").at("sha256") == contract_pin,
                "installed SDK does not match the pinned SDK source identity/contract");
        require(sdk_problems(root / directory).empty(), "installed SDK source is not compatible with this native host");
        return {{"identity", receipt.at("sdk")}, {"path", (root / directory).string()}, {"already_installed", true}, {"acquisition", provenance}};
    }

    fs::create_directories(root / ".staging");
    store.recover();
    const auto pending = root / ".staging" / ("fetch-" + std::to_string(getpid()) + "-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    require(fs::create_directory(pending), "cannot create SDK acquisition staging");
    struct Cleanup { fs::path path; ~Cleanup() { std::error_code ignored; fs::remove_all(path, ignored); } } cleanup{pending};
    write_text(pending / "txn.json", Json{{"pid", getpid()}, {"state", "fetching"}, {"source_sha256", provenance.at("source_sha256")}}.dump() + "\n");
    auto download = [&](const std::string& address, const std::string& name, const std::string& digest, uint64_t bytes) {
        check_ca();
        auto result = fetch({"fetch", "--url", address, "--ca-file", ca.string(), "--output", (pending / name).string(),
                             "--sha256", digest, "--size", std::to_string(bytes), "--timeout", std::to_string(timeout)});
        require(result.code == 0, "SDK acquisition fetch failed (" + result.data.value("code", "failure") + "): " + result.data.value("message", "transfer failed"));
        provenance["downloads"].push_back({{"url", address}, {"sha256", digest}, {"size", bytes}});
    };
    download(manifest_url, "release-manifest.json", manifest_pin, manifest_size);
    auto manifest = read_json(pending / "release-manifest.json");
    require(manifest.at("format") == 1 && manifest.at("version") == sdk_version, "release manifest format/version differs from SDK source");
    auto package = asset(manifest, archive_name, 1ull << 31);
    const auto metadata = InstallMetadata::from_json(package.at("install"));
    require(metadata.content_sha256 == content_pin && metadata.version == sdk_version && metadata.stage == "02-cxx" &&
            metadata.profile == "dev" && metadata.target == native_target(), "release manifest does not match the pinned native Dev SDK");
    const auto contract_name = text(package, "contract"); filename(contract_name);
    require(contract_name != archive_name, "archive and contract must be distinct assets");
    auto policy = asset(manifest, contract_name, 16u << 20);
    require(pin(policy, "sha256") == contract_pin, "release manifest contract differs from the pinned SDK source");
    download(contract_url, contract_name, contract_pin, size(policy, "size", 16u << 20));
    download(archive_url, archive_name, pin(package, "sha256"), size(package, "size", 1ull << 31));
    // The ordinary installer remains the authoritative release/archive/contract
    // validator and transaction owner. This is a native call, never a subprocess.
    auto installed = sdk_command({"sdk", "install", "--archive", (pending / archive_name).string(),
        "--release-manifest", (pending / "release-manifest.json").string(), "--release-manifest-sha256", manifest_pin, "--store", root.string()});
    require(installed.code == 0, "SDK acquisition installation failed");
    installed.data["acquisition"] = provenance;
    return installed.data;
}
}
