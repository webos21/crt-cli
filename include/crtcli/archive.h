#pragma once
// Safe extraction of SDK archives (tar.xz and zip) into a directory the caller owns.
//
// The archive is untrusted input. Every entry is validated before anything is written for it, and the first violation
// stops the extraction with an Error; the caller discards the (partially filled) destination. Nothing here decides what
// an SDK is or where it is stored: that belongs to the SDK store.
#include "crtcli/core.h"
#include <cstdint>
#include <string>
#include <vector>

namespace crtcli {

struct ArchiveLimits {
    uint64_t total_bytes = 8ull << 30;     // sum of extracted file sizes
    uint64_t entries = 200000;             // files, directories and symlinks
    uint64_t file_bytes = 2ull << 30;      // one file
    size_t path_bytes = 1024;              // one archive path
    size_t depth = 64;                     // components in one archive path
    uint64_t compression_ratio = 4096;     // extracted bytes per archive byte (decompression-bomb guard)
};

struct ExtractReport {
    std::string format;                    // "tar.xz" or "zip"
    uint64_t entries = 0;
    uint64_t files = 0;
    uint64_t directories = 0;
    uint64_t symlinks = 0;
    uint64_t bytes = 0;
    Json to_json() const;
};

// Format by content, never by file name: "xz" (FD 37 7A 58 5A 00) or "zip" ("PK").
std::string detect_archive_format(const fs::path& archive);

// Extracts into `destination`, which must be an existing empty directory. Rejected, before they are created:
//   - absolute paths, drive or UNC prefixes, backslashes, empty or "." / ".." components, NUL or control characters,
//     names that are not valid UTF-8, paths longer than limits.path_bytes or deeper than limits.depth;
//   - Windows reserved device names (CON, PRN, AUX, NUL, COM1-9, LPT1-9, with or without an extension) and names ending
//     in a dot or a space, on every host, so an archive extracts the same everywhere;
//   - two entries whose names differ only in case, or a file where a directory (or the reverse) was already extracted;
//   - hard links, devices, FIFOs and sockets; symbolic links whose target is absolute, contains "..", or resolves
//     outside the extraction root, and links that would be written through (an entry below a symlink);
//   - totals beyond the limits, including a decompression ratio beyond limits.compression_ratio.
// Regular files keep the owner execute bit; setuid/setgid/sticky bits and group/other write bits are dropped.
ExtractReport extract_archive(const fs::path& archive, const fs::path& destination, const ArchiveLimits& limits = {});

// The path rules alone, for tests and for callers that validate before they extract. Returns an empty string for an
// acceptable relative path, otherwise the reason it is rejected.
std::string archive_path_problem(const std::string& path, const ArchiveLimits& limits = {});
// The symlink rules alone: `link` is the entry path (already validated), `target` its link text.
std::string archive_symlink_problem(const std::string& link, const std::string& target);

}  // namespace crtcli
