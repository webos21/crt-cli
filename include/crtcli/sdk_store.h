#pragma once
// The shared, immutable SDK store: `$USER_HOME/.crt/sdks` (docs/design/crt_cli.md section 3).
//
// An installed SDK is a directory whose name is derived from its content identity and never changes afterwards.
// Installation is a transaction: verify the archive, extract into private staging on the same file system, validate,
// then publish with one rename. Failure at any point leaves the store exactly as it was, plus recoverable leftovers
// that the next operation removes. Nothing here reads or writes a project: selection is explicit and project locks are
// never migrated.
//
//   <root>/
//     <version>-<content12>/       published SDK (read-only by convention, never modified)
//     .staging/<txn>/              extraction in progress; txn.json records the owner process
//     .locks/<content12>.lock/     per-identity lock (a directory: creation is atomic on every host)
#include "crtcli/archive.h"
#include "crtcli/core.h"
#include <optional>
#include <string>
#include <vector>

namespace crtcli {

struct SdkIdentity {
    std::string version;          // the SDK's VERSION file
    std::string content_sha256;   // sha256 of the compact inventory JSON, the same value the toolchain contract carries
    std::string directory() const { return version + "-" + content_sha256.substr(0, 12); }
    Json to_json() const { return {{"version", version}, {"content_sha256", content_sha256}, {"directory", directory()}}; }
};

// The `install` object of a CRT release-manifest asset (crt docs/design/sdk_development_contract.md, install metadata
// format 1). It bounds extraction before it starts and is compared with the extracted tree afterwards.
struct InstallMetadata {
    std::string archive_format;               // "tar.xz" or "zip"
    std::string root;                         // the single top-level directory the archive extracts to
    std::string version, stage, profile;
    Json target;
    std::string content_sha256;
    uint64_t entries = 0, files = 0, directories = 0, symlinks = 0;
    uint64_t uncompressed_bytes = 0, max_file_bytes = 0, max_path_bytes = 0, max_depth = 0;
    static InstallMetadata from_json(const Json& value);   // throws Error on a missing or malformed field
};

struct InstallRequest {
    fs::path archive;
    std::string archive_sha256;               // required: the digest the caller trusts for these bytes
    std::string expected_content_sha256;      // optional: refuse unless the extracted SDK has exactly this identity
    ArchiveLimits limits;
    // When present: limits are tightened to the metadata, and the extracted tree must match it exactly (format, root,
    // counts, byte total, identity, version, stage, profile, target). expected_content_sha256 is set from it.
    std::optional<InstallMetadata> metadata;
};

struct InstallResult {
    SdkIdentity identity;
    fs::path path;
    bool already_installed = false;           // the identical SDK was published before; nothing was changed
    ExtractReport extracted;
    Json to_json() const;
};

struct StoreEntry {
    std::string directory;        // <version>-<content12>; `verify` proves the content behind it
    std::string version;
    std::string content12;        // the first 12 hex digits of the content identity, read from the directory name
    fs::path path;
    Json target;                              // {"os","arch"} from the SDK manifest
    std::string stage;
    std::string profile;
};

class SdkStore {
public:
    explicit SdkStore(fs::path root);
    const fs::path& root() const { return root_; }

    // Installs the archive. Throws Error on any failure; on failure no directory named like an SDK has been added.
    InstallResult install(const InstallRequest& request);

    // Published SDKs, sorted by directory name. Staging and lock entries are not SDKs and are never listed.
    std::vector<StoreEntry> list() const;

    // Re-reads an installed SDK and proves its content still matches the identity in its directory name.
    // Returns the number of files checked; throws Error when anything differs, is missing or was added.
    uint64_t verify(const std::string& directory) const;

    // Removes leftovers of interrupted installations (staging directories whose owner is gone, stale locks).
    // Returns what was removed. Called by install() first, and available on its own.
    std::vector<std::string> recover();

    // Upper bound, in seconds, a lock held by a live process is waited for before install() gives up.
    unsigned lock_wait_seconds = 0;

private:
    fs::path root_;
};

// Reads the identity of an extracted (or installed) SDK directory: its VERSION and the content hash of its inventory.
SdkIdentity read_identity(const fs::path& sdk);

// What an SDK must be for the CLI to use it at all: the checks `configure` makes later, made at installation so a
// store never holds an SDK that cannot be selected. Returns the reasons it is not acceptable (empty when it is).
std::vector<std::string> sdk_problems(const fs::path& sdk);

// `sdk install|list|verify|recover`.
Reply sdk_command(const std::vector<std::string>& args);

}  // namespace crtcli
