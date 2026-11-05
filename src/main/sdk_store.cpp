#include "crtcli/sdk_store.h"
#include <crt/platform.h>
#include <algorithm>
#include <chrono>
#include <csignal>
#include <thread>
#include <unistd.h>

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

// A process is alive when signal 0 reaches it. Another user's process counts as alive (EPERM): a lock must never be
// stolen from a process that may still be running.
bool process_alive(long pid) {
    if (pid <= 0) return false;
    if (kill(static_cast<pid_t>(pid), 0) == 0) return true;
    return errno == EPERM;
}

long current_pid() { return static_cast<long>(getpid()); }

// The version string becomes part of a directory name, so it is held to a conservative alphabet.
void check_version(const std::string& version) {
    require(!version.empty() && version.size() <= 64, "SDK VERSION is empty or too long");
    for (unsigned char c : version)
        require(std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == '+', "SDK VERSION contains a character that cannot name a directory");
    require(version[0] != '.' && version[0] != '-', "SDK VERSION may not start with '.' or '-'");
}

class Lock {
public:
    Lock(const fs::path& directory, unsigned wait_seconds) : directory_(directory) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(wait_seconds);
        for (;;) {
            std::error_code error;
            if (fs::create_directory(directory_, error)) {
                write_text(directory_ / "owner", std::to_string(current_pid()) + "\n");
                held_ = true;
                return;
            }
            require(!error, "cannot create lock " + directory_.string() + ": " + error.message());
            if (steal_if_dead()) continue;
            if (std::chrono::steady_clock::now() >= deadline)
                throw Error("another installation of this SDK is in progress (lock " + directory_.string() + ")");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    ~Lock() { release(); }
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    void release() {
        if (!held_) return;
        std::error_code ignored;
        fs::remove_all(directory_, ignored);
        held_ = false;
    }
private:
    bool steal_if_dead() {
        long owner = 0;
        try { owner = std::stol(trimmed(read_text(directory_ / "owner"))); } catch (const std::exception&) { owner = 0; }
        if (process_alive(owner)) return false;
        // The owner is gone: remove the lock, then let the loop retry the atomic create. Two takers race on that create,
        // and exactly one of them wins it.
        std::error_code ignored;
        fs::remove_all(directory_, ignored);
        return true;
    }
    fs::path directory_;
    bool held_ = false;
};

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
            {"contract_stored", contract_stored}, {"extracted", extracted.to_json()}};
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
    // A contract is first written as `<directory>.json.tmp-<pid>` and renamed once its SDK is published.
    auto contracts = root_ / ".contracts";
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
            removed.push_back(".contracts/" + name);
        }
    }
    return removed;
}

namespace {
// A portable contract the store may keep: the right kind of document, bound to the SDK identity, naming no machine path.
Json checked_contract(const fs::path& file, const std::string& expected_sha256) {
    require(is_hex(lower(expected_sha256), 64), "a toolchain contract SHA-256 is required (64 hex digits)");
    require(fs::is_regular_file(file) && !fs::is_symlink(file), "toolchain contract not found: " + file.string());
    require(file_sha256(file) == lower(expected_sha256), "toolchain contract SHA-256 mismatch");
    auto contract = read_json(file);
    require(contract.is_object() && contract.value("schema", "") == "crt.experimental.native-toolchain" && contract.value("format", 0) == 1,
            "unsupported toolchain contract");
    require(!contract.contains("external_os_sdk"), "the toolchain contract names a machine path (not portable)");
    require(contract.contains("sdk_content_sha256") && contract.at("sdk_content_sha256").is_string(), "the toolchain contract has no SDK identity");
    return contract;
}
}  // namespace

InstallResult SdkStore::install(const InstallRequest& request) {
    require(is_hex(lower(request.archive_sha256), 64), "an archive SHA-256 is required (64 hex digits)");
    require(request.expected_content_sha256.empty() || is_hex(lower(request.expected_content_sha256), 64),
            "the expected content identity must be 64 hex digits");
    require(fs::is_regular_file(request.archive), "archive not found: " + request.archive.string());
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
    // 5.-7. Serialize on the identity; decide; publish.
    Lock lock(root_ / ".locks" / (result.identity.content_sha256.substr(0, 12) + ".lock"), lock_wait_seconds);
    result.path = root_ / result.identity.directory();
    if (fs::exists(result.path) || fs::is_symlink(result.path)) {
        // The name carries 48 bits of the identity. The same name must hold the same content: anything else is either a
        // damaged store or a collision, and an installed SDK is never replaced.
        auto present = read_identity(result.path);
        require(present.content_sha256 == result.identity.content_sha256,
                "an SDK with this directory name but different content is installed (" + present.content_sha256 + "); installed SDKs are immutable");
        result.already_installed = true;
        if (with_contract) {
            // An installed SDK's contract is as immutable as the SDK: it may be added, never changed.
            auto stored = contract_path(result.identity.directory());
            if (fs::exists(stored)) require(file_sha256(stored) == lower(request.contract_sha256), "a different toolchain contract is stored for this SDK; stored contracts are immutable");
            else {
                fs::create_directories(root_ / ".contracts", error);
                require(!error, "cannot create the contract store: " + error.message());
                auto pending = fs::path(stored.string() + ".tmp-" + std::to_string(current_pid()));
                write_text(pending, read_text(request.contract));
                require(file_sha256(pending) == lower(request.contract_sha256), "the stored toolchain contract differs from the verified file");
                fs::rename(pending, stored, error);
                require(!error, "cannot store the toolchain contract: " + error.message());
                result.contract_stored = true;
            }
        }
        return result;
    }
    fs::path pending;
    if (with_contract) {
        // Prepared before publication, renamed right after it: the pair is complete or the contract is a recoverable leftover.
        fs::create_directories(root_ / ".contracts", error);
        require(!error, "cannot create the contract store: " + error.message());
        pending = contract_path(result.identity.directory()).string() + ".tmp-" + std::to_string(current_pid());
        write_text(pending, read_text(request.contract));
                require(file_sha256(pending) == lower(request.contract_sha256), "the stored toolchain contract differs from the verified file");
    }
    write_text(staging / "txn.json", Json{{"pid", current_pid()}, {"archive_sha256", actual}, {"state", "publishing"}}.dump(2) + "\n");
    fs::rename(sdk, result.path, error);
    if (error) { std::error_code ignored; if (!pending.empty()) fs::remove(pending, ignored); }
    require(!error, "cannot publish the SDK: " + error.message());
    if (!pending.empty()) {
        fs::rename(pending, contract_path(result.identity.directory()), error);
        require(!error, "the SDK is installed but its toolchain contract could not be stored (install again to add it): " + error.message());
        result.contract_stored = true;
    }
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
    // The toolchain contract kept beside the SDK is part of what the lock trusts: it must still be a portable contract of this SDK.
    // Its digest is not known here (the project lock records it); this checks what the store itself can know.
    auto stored = contract_path(directory);
    if (fs::exists(stored) || fs::is_symlink(stored)) {
        Json contract;
        try {
            require(fs::is_regular_file(stored) && !fs::is_symlink(stored), "not a regular file");
            contract = read_json(stored);
        } catch (const std::exception& error) {
            throw Error("the stored toolchain contract of " + directory + " is unreadable: " + error.what());
        }
        require(contract.is_object() && contract.value("schema", "") == "crt.experimental.native-toolchain" && contract.value("format", 0) == 1,
                "the stored toolchain contract of " + directory + " is not a supported contract");
        require(!contract.contains("external_os_sdk"), "the stored toolchain contract of " + directory + " names a machine path (not portable)");
        require(contract.value("sdk_content_sha256", "") == identity.content_sha256,
                "the stored toolchain contract of " + directory + " belongs to another SDK");
    }
    return inventory(path).size();
}

}  // namespace crtcli
