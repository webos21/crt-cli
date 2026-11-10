#include "crtcli/sdk_store.h"
#include <crt/platform.h>
#include <algorithm>
#include <chrono>
#include <csignal>
#include <thread>
#include <unistd.h>
#include <crtcli/store_lock.h>

namespace crtcli {
namespace {

bool is_hex(const std::string& value, size_t length) {
    return value.size() == length && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c) || (c >= 'a' && c <= 'f'); });
}

std::string lower(std::string value) {
    for (auto& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

std::string trimmed(std::string value) {
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ')) value.pop_back();
    return value;
}

// The version string becomes part of a directory name, so it is held to a conservative alphabet.
void check_version(const std::string& version) {
    require(!version.empty() && version.size() <= 64, "SDK VERSION is empty or too long");
    for (unsigned char c : version)
        require(std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == '+', "SDK VERSION contains a character that cannot name a directory");
    require(version[0] != '.' && version[0] != '-', "SDK VERSION may not start with '.' or '-'");
}

Json read_optional_json(const fs::path& path) {
    try { return read_json(path); } catch (const std::exception&) { return Json::object(); }
}

}  // namespace

InstallMetadata InstallMetadata::from_json(const Json& value) {
    require(value.is_object() && value.value("format", 0) == 1, "unsupported install metadata (format 1 expected)");
    InstallMetadata data;
    auto text = [&](const char* key) {
        require(value.contains(key) && value[key].is_string() && !value[key].get<std::string>().empty(), std::string("install metadata lacks ") + key);
        return value[key].get<std::string>();
    };
    auto number = [&](const char* key) {
        require(value.contains(key) && value[key].is_number_integer() && (value[key].is_number_unsigned() || value[key].get<int64_t>() >= 0), std::string("install metadata lacks a non-negative integer ") + key);
        return value[key].get<uint64_t>();
    };
    data.archive_format = text("archive_format");
    require(data.archive_format == "tar.xz" || data.archive_format == "zip", "install metadata names an unknown archive format");
    data.root = text("root");
    require(archive_path_problem(data.root).empty() && data.root.find('/') == std::string::npos, "install metadata root is not a single directory name");
    data.version = text("version"); data.stage = text("stage");
    data.profile = value.contains("profile") && value["profile"].is_string() ? value["profile"].get<std::string>() : "";
    require(value.contains("target") && value["target"].is_object(), "install metadata lacks target");
    data.target = value["target"];
    data.content_sha256 = text("content_sha256");
    require(is_hex(data.content_sha256, 64), "install metadata content_sha256 is not 64 lowercase hex digits");
    data.entries = number("entries"); data.files = number("files"); data.directories = number("directories"); data.symlinks = number("symlinks");
    require(data.entries == data.files + data.directories + data.symlinks, "install metadata entry counts are inconsistent");
    data.uncompressed_bytes = number("uncompressed_bytes"); data.max_file_bytes = number("max_file_bytes");
    data.max_path_bytes = number("max_path_bytes"); data.max_depth = number("max_depth");
    return data;
}

Json InstallResult::to_json() const {
    return {{"identity", identity.to_json()}, {"path", path.string()}, {"already_installed", already_installed},
            {"contract_stored", contract_stored}, {"receipt_stored", receipt_stored}, {"extracted", extracted.to_json()}};
}

SdkStore::SdkStore(fs::path root) : root_(std::move(root)) {}

SdkIdentity read_identity(const fs::path& sdk) {
    SdkIdentity identity;
    identity.version = trimmed(read_text(sdk / "VERSION"));
    check_version(identity.version);
    // The identity is the hash of the compact inventory, byte for byte what the toolchain contract records.
    identity.content_sha256 = sha256(inventory(sdk).dump());
    return identity;
}

std::vector<std::string> sdk_problems(const fs::path& sdk) {
    std::vector<std::string> problems;
    auto need = [&](const char* relative) {
        if (!fs::is_regular_file(sdk / relative)) { problems.push_back(std::string("missing ") + relative); return false; }
        return true;
    };
    bool has_manifest = need("manifest.json"), has_profile = need("share/crt/contracts/development.json");
    need("VERSION");
    if (has_manifest) {
        try {
            auto manifest = read_json(sdk / "manifest.json");
            if (manifest.value("format", 0) != 1) problems.push_back("unsupported manifest format");
            if (manifest.value("stage", "") != "02-cxx") problems.push_back("unsupported SDK stage (the Development profile is 02-cxx)");
            if (manifest.value("target", Json::object()) != native_target())
                problems.push_back("SDK target " + manifest.value("target", Json::object()).dump() + " is not this host " + native_target().dump());
        } catch (const std::exception& error) { problems.push_back(std::string("unreadable manifest.json: ") + error.what()); }
    }
    if (has_profile) {
        try {
            auto profile = read_json(sdk / "share/crt/contracts/development.json");
            if (profile.value("format", 0) != 1 || profile.value("profile", "") != "dev") problems.push_back("unsupported SDK profile");
            if (profile.value("target", Json::object()) != native_target()) problems.push_back("profile target differs from this host");
        } catch (const std::exception& error) { problems.push_back(std::string("unreadable development.json: ") + error.what()); }
    }
    return problems;
}

std::vector<std::string> SdkStore::recover() {
    std::vector<std::string> removed;
    std::error_code error;
    for (const char* area : {".staging", ".locks"}) {
        auto directory = root_ / area;
        if (!fs::is_directory(directory)) continue;
        for (const auto& entry : fs::directory_iterator(directory, error)) {
            long owner = 0;
            auto record = std::string(area) == ".staging" ? entry.path() / "txn.json" : entry.path() / "owner";
            try {
                if (std::string(area) == ".staging") owner = read_json(record).value("pid", 0L);
                else owner = std::stol(trimmed(read_text(record)));
            } catch (const std::exception&) { owner = 0; }
            if (process_alive(owner)) continue;
            std::error_code ignored;
            fs::remove_all(entry.path(), ignored);
            removed.push_back(std::string(area) + "/" + entry.path().filename().string());
        }
    }
    // PID-owned contract/receipt writes are removable only after their owner has exited.
    for (const auto& area : {".contracts", ".receipts"}) {
        auto contracts = root_ / area;
        require(!fs::is_symlink(contracts), "store metadata directory must not be a symlink: " + contracts.string());
        if (fs::is_directory(contracts)) {
            for (const auto& entry : fs::directory_iterator(contracts, error)) {
                auto name = entry.path().filename().string();
                auto marker = name.rfind(".json.tmp-");
                if (marker == std::string::npos) continue;
                long owner = 0;
                try { owner = std::stol(name.substr(marker + 10)); } catch (const std::exception&) { owner = 0; }
                if (process_alive(owner)) continue;
                std::error_code ignored;
                fs::remove(entry.path(), ignored);
                removed.push_back(std::string(area) + "/" + name);
            }
        }
    }
    return removed;
}

namespace {
// A portable contract the store may keep: the right kind of document, bound to the SDK identity, naming no machine path.
Json checked_contract(const fs::path& file, const std::string& expected_sha256) {
    require(is_hex(lower(expected_sha256), 64), "a toolchain contract SHA-256 is required (64 hex digits)");
    require(fs::is_regular_file(file) && !fs::is_symlink(file), "toolchain contract not found: " + file.string());
    require(fs::file_size(file) <= 16 * 1024 * 1024, "toolchain contract exceeds 16 MiB");
    require(file_sha256(file) == lower(expected_sha256), "toolchain contract SHA-256 mismatch");
    auto contract = read_json(file);
    require(contract.is_object() && contract.value("schema", "") == "crt.experimental.native-toolchain" && contract.value("format", 0) == 1,
            "unsupported toolchain contract");
    require(!contract.contains("external_os_sdk"), "the toolchain contract names a machine path (not portable)");
    require(contract.contains("sdk_content_sha256") && contract.at("sdk_content_sha256").is_string(), "the toolchain contract has no SDK identity");
    return contract;
}

void check_directory_name(const std::string& directory) {
    require(!directory.empty() && directory.find('/') == std::string::npos && directory.find('\\') == std::string::npos &&
            directory != "." && directory != ".." && directory[0] != '.', "not an SDK directory name: " + directory);
}

void check_metadata_area(const fs::path& root, const char* name) {
    require(!fs::is_symlink(root / name), "store metadata directory must not be a symlink: " + (root / name).string());
}

Json read_receipt(const fs::path& root, const std::string& directory) {
    check_directory_name(directory);
    check_metadata_area(root, ".receipts");
    const auto file = root / ".receipts" / (directory + ".json");
    require(fs::is_regular_file(file) && !fs::is_symlink(file),
            "missing or untrusted SDK installation receipt; use sdk repair with a pinned release manifest: " + directory);
    require(fs::file_size(file) <= 65536, "SDK installation receipt exceeds 64 KiB");
    Json receipt;
    try {
        receipt = read_json(file);
        require(receipt.is_object() && receipt.value("schema", "") == "crt-cli.sdk-install-receipt" && receipt.value("format", 0) == 1,
                "unsupported SDK installation receipt");
        const auto& sdk = receipt.at("sdk");
        std::string version = sdk.at("version"), content = sdk.at("content_sha256");
        check_version(version);
        require(is_hex(content, 64) && SdkIdentity{version, content}.directory() == directory && sdk.at("directory") == directory,
                "SDK installation receipt identity mismatch");
        const auto& contract = receipt.at("contract");
        require(contract.is_null() || (contract.is_object() && contract.size() == 1 && contract.contains("sha256") &&
                contract.at("sha256").is_string() && is_hex(contract.at("sha256").get<std::string>(), 64)),
                "invalid SDK installation receipt contract pin");
    } catch (const std::exception& error) { throw Error("invalid SDK installation receipt: " + std::string(error.what())); }
    return receipt;
}

// Only the final rename publishes metadata; a crashed writer leaves a PID-owned temporary file for recover.
void publish_file(const fs::path& destination, const std::string& bytes, const std::string& expected_sha256 = {}) {
    const auto parent = destination.parent_path();
    require(!fs::is_symlink(parent), "store metadata directory must not be a symlink: " + parent.string());
    fs::create_directories(parent);
    auto pending = fs::path(destination.string() + ".tmp-" + std::to_string(current_pid()) + "-" +
                            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    require(!fs::exists(pending) && !fs::is_symlink(pending), "metadata temporary file already exists");
    struct Remove { fs::path p; ~Remove() { std::error_code ignored; fs::remove(p, ignored); } } cleanup{pending};
    write_text(pending, bytes);
    require(expected_sha256.empty() || file_sha256(pending) == expected_sha256, "metadata differs from the verified input");
    fs::rename(pending, destination);
}
}  // namespace

Json SdkStore::verified_receipt(const std::string& directory) const {
    auto receipt = read_receipt(root_, directory);
    check_metadata_area(root_, ".contracts");
    const auto stored = contract_path(directory);
    const auto& pin = receipt.at("contract");
    if (pin.is_null()) {
        require(!fs::exists(stored) && !fs::is_symlink(stored), "unexpected stored contract: installation receipt declares none");
    } else {
        require(fs::is_regular_file(stored) && !fs::is_symlink(stored),
                "the installed SDK has no stored toolchain contract (or it is not a regular file); use sdk repair");
        require(fs::file_size(stored) <= 16 * 1024 * 1024, "toolchain contract exceeds 16 MiB");
        require(file_sha256(stored) == pin.at("sha256"), "toolchain contract digest mismatch: stored bytes differ from the installation receipt; use sdk repair");
        auto contract = checked_contract(stored, pin.at("sha256"));
        require(contract.at("sdk_content_sha256") == receipt.at("sdk").at("content_sha256"), "the stored contract belongs to another SDK");
    }
    return receipt;
}

InstallResult SdkStore::install(const InstallRequest& request) {
    require(is_hex(lower(request.archive_sha256), 64), "an archive SHA-256 is required (64 hex digits)");
    require(request.expected_content_sha256.empty() || is_hex(lower(request.expected_content_sha256), 64),
            "the expected content identity must be 64 hex digits");
    require(fs::is_regular_file(request.archive), "archive not found: " + request.archive.string());
    const bool repairing = !request.repair_directory.empty();
    if (repairing) {
        check_directory_name(request.repair_directory);
        require(request.metadata.has_value(), "sdk repair requires verified release install metadata");
    }
    // A bad contract is refused before anything is extracted.
    Json contract;
    const bool with_contract = !request.contract.empty();
    if (with_contract) contract = checked_contract(request.contract, request.contract_sha256);

    // 1. The archive is verified before anything is created.
    auto actual = file_sha256(request.archive);
    require(actual == lower(request.archive_sha256), "archive SHA-256 mismatch: expected " + lower(request.archive_sha256) + ", got " + actual);

    std::error_code error;
    fs::create_directories(root_, error);
    require(!error, "cannot create the SDK store " + root_.string() + ": " + error.message());
    fs::create_directories(root_ / ".staging", error);
    fs::create_directories(root_ / ".locks", error);
    require(!error, "cannot create the SDK store bookkeeping: " + error.message());

    // 2. Leftovers of interrupted installations never accumulate and never block this one.
    recover();

    // 3. Private staging on the same file system as the store, so publication is one rename.
    auto transaction = std::to_string(current_pid()) + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto staging = root_ / ".staging" / transaction;
    fs::create_directories(staging / "sdk", error);
    require(!error, "cannot create staging " + staging.string() + ": " + error.message());
    write_text(staging / "txn.json", Json{{"pid", current_pid()}, {"archive_sha256", actual}, {"state", "extracting"}}.dump(2) + "\n");
    struct Cleanup {
        fs::path path;
        ~Cleanup() { std::error_code ignored; fs::remove_all(path, ignored); }
    } cleanup{staging};

    InstallResult result;
    auto limits = request.limits;
    if (request.metadata) {
        // The producer promised this tree: nothing larger or deeper is even started.
        const auto& promised = *request.metadata;
        limits.entries = std::min(limits.entries, promised.entries);
        limits.total_bytes = std::min(limits.total_bytes, promised.uncompressed_bytes);
        limits.file_bytes = std::min(limits.file_bytes, promised.max_file_bytes);
        limits.path_bytes = std::min<uint64_t>(limits.path_bytes, promised.max_path_bytes);
        limits.depth = std::min<uint64_t>(limits.depth, promised.max_depth);
    }
    // 4. Extraction, identity, acceptability. Any failure leaves only the staging directory, which Cleanup removes.
    result.extracted = extract_archive(request.archive, staging / "sdk", limits);
    // An archive may wrap everything in one top-level directory (crt-<v>-<os>-<arch>-dev/). Look through exactly one.
    fs::path sdk = staging / "sdk";
    if (!fs::exists(sdk / "manifest.json")) {
        std::vector<fs::path> children;
        for (const auto& entry : fs::directory_iterator(sdk)) children.push_back(entry.path());
        if (children.size() == 1 && fs::is_directory(children[0]) && !fs::is_symlink(children[0])) sdk = children[0];
    }
    if (request.metadata) {
        const auto& promised = *request.metadata;
        const auto& got = result.extracted;
        require(got.format == promised.archive_format, "archive format " + got.format + " differs from the release metadata (" + promised.archive_format + ")");
        require(sdk != staging / "sdk" && sdk.filename().string() == promised.root, "archive does not extract to the promised root '" + promised.root + "/'");
        auto count = [](const char* what, uint64_t actual, uint64_t expected) {
            require(actual == expected, std::string("extracted ") + what + " " + std::to_string(actual) + " differ from the release metadata (" + std::to_string(expected) + ")");
        };
        count("files", got.files, promised.files); count("directories", got.directories, promised.directories);
        count("symbolic links", got.symlinks, promised.symlinks); count("bytes", got.bytes, promised.uncompressed_bytes);
    }
    auto problems = sdk_problems(sdk);
    if (!problems.empty()) {
        std::string joined;
        for (const auto& problem : problems) joined += (joined.empty() ? "" : "; ") + problem;
        throw Error("archive is not an installable SDK: " + joined);
    }
    result.identity = read_identity(sdk);
    if (!request.expected_content_sha256.empty())
        require(result.identity.content_sha256 == lower(request.expected_content_sha256),
                "SDK content identity mismatch: expected " + lower(request.expected_content_sha256) + ", got " + result.identity.content_sha256);

    if (request.metadata) {
        const auto& promised = *request.metadata;
        auto manifest = read_json(sdk / "manifest.json");
        auto profile = read_optional_json(sdk / "share/crt/contracts/development.json").value("profile", "");
        require(result.identity.content_sha256 == promised.content_sha256, "SDK content identity mismatch: release metadata says " + promised.content_sha256 + ", got " + result.identity.content_sha256);
        require(result.identity.version == promised.version, "SDK VERSION " + result.identity.version + " differs from the release metadata (" + promised.version + ")");
        require(manifest.value("stage", "") == promised.stage && profile == promised.profile && manifest.value("target", Json::object()) == promised.target,
                "SDK stage/profile/target differ from the release metadata");
    }
    if (with_contract)
        require(contract.at("sdk_content_sha256") == result.identity.content_sha256,
                "the toolchain contract belongs to SDK " + contract.at("sdk_content_sha256").get<std::string>() + ", not " + result.identity.content_sha256);
    // Serialize payload and metadata publication on the same SDK identity.
    Lock lock(root_ / ".locks" / (result.identity.content_sha256.substr(0, 12) + ".lock"), lock_wait_seconds);
    result.path = root_ / result.identity.directory();
    check_metadata_area(root_, ".contracts"); check_metadata_area(root_, ".receipts");
    if (repairing) require(request.repair_directory == result.identity.directory(), "repair archive does not match the requested SDK identity");
    Json wanted{{"schema", "crt-cli.sdk-install-receipt"}, {"format", 1}, {"sdk", result.identity.to_json()},
                {"contract", with_contract ? Json{{"sha256", lower(request.contract_sha256)}} : Json(nullptr)}};
    const auto stored = contract_path(result.identity.directory());
    const auto receipt = receipt_path(result.identity.directory());
    if (fs::exists(result.path) || fs::is_symlink(result.path)) {
        require(fs::is_directory(result.path) && !fs::is_symlink(result.path), "installed SDK must be a real directory");
        auto present = read_identity(result.path);
        require(present.to_json() == result.identity.to_json(),
                "an SDK with this directory name but different content is installed; installed SDKs are immutable");
        result.already_installed = true;
        if (repairing) {
            // Repair cannot update a known pin or downgrade a required contract. A missing legacy receipt is established
            // only from the verified release inputs, never by hashing the current stored contract.
            if (fs::exists(receipt) || fs::is_symlink(receipt)) {
                auto prior = read_receipt(root_, result.identity.directory());
                require(prior == wanted || (prior.at("sdk") == wanted.at("sdk") && prior.at("contract").is_null() && with_contract),
                        "repair cannot change an existing installation receipt; SDK policy is immutable");
            }
            if (!with_contract) require(!fs::exists(stored) && !fs::is_symlink(stored), "repair release names no contract but one is stored");
        } else {
            auto prior = verified_receipt(result.identity.directory());
            require(prior.at("sdk") == result.identity.to_json(), "SDK installation receipt identity mismatch");
            if (!with_contract) return result; // Do not downgrade an installation that already requires a contract.
            if (!prior.at("contract").is_null()) {
                require(prior == wanted, "a different toolchain contract is stored for this SDK; stored contracts are immutable");
                return result;
            }
        }
    } else {
        require(!repairing, "sdk repair requires an existing installed SDK");
        require(!fs::exists(receipt) && !fs::is_symlink(receipt) && !fs::exists(stored) && !fs::is_symlink(stored),
                "orphan SDK metadata exists; do not replace its identity silently");
        write_text(staging / "txn.json", Json{{"pid", current_pid()}, {"archive_sha256", actual}, {"state", "publishing"}}.dump(2) + "\n");
        fs::rename(sdk, result.path);
    }
    // The receipt is the final commit record. Between renames an interrupted installation fails closed and needs
    // explicit repair from the original release, rather than accepting a missing/changed policy as a new baseline.
    if (with_contract) {
        publish_file(stored, read_text(request.contract), lower(request.contract_sha256));
        result.contract_stored = true;
    }
    publish_file(receipt, wanted.dump(2) + "\n");
    result.receipt_stored = true;
    return result;
}

std::vector<StoreEntry> SdkStore::list() const {
    std::vector<StoreEntry> entries;
    if (!fs::is_directory(root_)) return entries;
    std::vector<fs::path> directories;
    for (const auto& entry : fs::directory_iterator(root_)) {
        auto name = entry.path().filename().string();
        if (name.empty() || name[0] == '.' || !entry.is_directory() || entry.is_symlink()) continue;
        directories.push_back(entry.path());
    }
    std::sort(directories.begin(), directories.end());
    for (const auto& path : directories) {
        StoreEntry item;
        item.path = path;
        item.directory = path.filename().string();
        try {
            item.version = trimmed(read_text(path / "VERSION"));
            // The content hash is not recomputed here (that is `verify`); the directory name records its first 48 bits.
            item.content12 = item.directory.size() > 12 ? item.directory.substr(item.directory.size() - 12) : item.directory;
            auto manifest = read_optional_json(path / "manifest.json");
            item.target = manifest.value("target", Json::object());
            item.stage = manifest.value("stage", "");
            item.profile = read_optional_json(path / "share/crt/contracts/development.json").value("profile", "");
            item.has_contract = fs::is_regular_file(contract_path(item.directory));
        } catch (const std::exception&) {}
        entries.push_back(item);
    }
    return entries;
}

uint64_t SdkStore::verify(const std::string& directory) const {
    require(!directory.empty() && directory.find('/') == std::string::npos && directory.find('\\') == std::string::npos &&
            directory != "." && directory != ".." && directory[0] != '.', "not an SDK directory name: " + directory);
    auto path = root_ / directory;
    require(fs::is_directory(path) && !fs::is_symlink(path), "no such installed SDK: " + directory);
    auto identity = read_identity(path);
    require(identity.directory() == directory,
            "installed SDK content no longer matches its identity: directory " + directory + ", content " + identity.directory());
    auto receipt = verified_receipt(directory);
    require(receipt.at("sdk") == identity.to_json(), "SDK installation receipt identity mismatch");
    return inventory(path).size();
}

}  // namespace crtcli
