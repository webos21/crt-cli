// Archive extraction: positive trees and one rejection per safety rule, on real tar.xz bytes. The product code only
// decodes; the test encodes with liblzma so no fixture bytes are checked in.
#include "tar_fixture.h"
#include <crtcli/archive.h>
#include <crt/platform.h>
#include <lzma.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <fstream>
using namespace crtcli;
using namespace tarfix;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }


// Returns the error text, or an empty string when the extraction succeeded.
std::string attempt(Work& work, const std::vector<Entry>& entries, const ArchiveLimits& limits = {}, bool end_marker = true) {
    auto destination = work.fresh();
    try { extract_archive(work.archive(xz_of(tar_of(entries, end_marker))), destination, limits); }
    catch (const Error& error) { return error.what(); }
    return {};
}

void rejects(Work& work, const std::string& why, const std::vector<Entry>& entries, const std::string& expect, const ArchiveLimits& limits = {}) {
    auto message = attempt(work, entries, limits);
    check(!message.empty(), "accepted: " + why);
    check(message.find(expect) != std::string::npos, why + " (got: " + message + ")");
}
}  // namespace

int main() {
    const char* temporary = std::getenv("TMPDIR");
    std::string pattern = std::string(temporary && *temporary ? temporary : "/tmp") + "/crt-cli-archive-XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    require(mkdtemp(buffer.data()) != nullptr, "cannot create the test work directory");
    Work work{fs::path(buffer.data())};

    // ---- path rules, as pure functions -------------------------------------------------------------------------
    for (const char* good : {"a", "a/b", "dir/", "lib/libc++.so.1", "share/한글 space/x", "a/.hidden", "a.b/c.d"})
        check(archive_path_problem(good).empty(), std::string("path accepted: ") + good);
    struct { const char* path; const char* why; } bad[] = {
        {"", "empty"}, {"/etc/passwd", "absolute"}, {"C:/x", "drive"}, {"c:x", "drive"}, {"a/../b", "'..'"}, {"..", "'..'"},
        {"./a", "'.'"}, {"a//b", "empty path component"}, {"a\\b", "backslash"}, {"a/CON", "reserved"}, {"nul.txt", "reserved"},
        {"a/com1", "reserved"}, {"lpt9.log", "reserved"}, {"a/b.", "ends in"}, {"a/b ", "ends in"}, {"a/b:stream", "':'"},
        {"a/b?c", "reserved character"}, {"a/b*", "reserved character"}, {"a/\x01", "control"}, {"a/\xff", "UTF-8"}};
    for (const auto& item : bad) check(archive_path_problem(item.path).find(item.why) != std::string::npos,
                                       std::string("path rejected (") + item.why + ")");
    check(!archive_path_problem(std::string(2000, 'a')).empty(), "overlong path rejected");
    check(!archive_path_problem("a/b/c/d", ArchiveLimits{.depth = 3}).empty(), "deep path rejected");
    check(archive_symlink_problem("lib/libz.so", "libz.so.1").empty(), "sibling symlink accepted");
    check(archive_symlink_problem("lib/a/b", "../c").empty(), "symlink up within the root accepted");
    check(!archive_symlink_problem("a", "/etc/passwd").empty(), "absolute symlink rejected");
    check(!archive_symlink_problem("a", "../x").empty(), "symlink above the root rejected");
    check(!archive_symlink_problem("lib/a", "../../x").empty(), "symlink above the root (deeper) rejected");
    check(!archive_symlink_problem("a", "C:/x").empty(), "drive symlink rejected");
    check(!archive_symlink_problem("a", "").empty(), "empty symlink rejected");

    // ---- a plain SDK-like tree -----------------------------------------------------------------------------------
    {
        auto destination = work.fresh();
        std::string long_name = "share/" + std::string(150, 'n') + "/file.txt";
        std::string pax_name = "include/" + std::string(140, 'p') + ".h";
        Entry gnu = make_file(long_name, "gnu"); gnu.gnu_long_name = true;
        Entry pax = make_file(pax_name, "pax"); pax.pax_path = true;
        auto report = extract_archive(work.archive(xz_of(tar_of({
            make_dir("lib/"), make_file("lib/libc.so", "SO"), make_link("lib/libc.so.1", "libc.so"), make_file("bin/tool", "#!tool", 0755),
            make_file("manifest.json", "{}"), gnu, pax, make_file("empty", ""), make_dir("tmp/")}))), destination);
        check(report.format == "tar.xz", "format reported");
        check(report.files == 6 && report.symlinks == 1 && report.directories == 2 && report.entries == 9, "entry counts: " + report.to_json().dump());
        check(read_text(destination / "lib/libc.so") == "SO", "file content");
        check(fs::is_symlink(destination / "lib/libc.so.1") && fs::read_symlink(destination / "lib/libc.so.1") == "libc.so", "relative symlink created");
        check(read_text(destination / long_name) == "gnu", "GNU long name");
        check(read_text(destination / pax_name) == "pax", "pax path");
        check(fs::file_size(destination / "empty") == 0, "empty file");
        check((fs::status(destination / "bin/tool").permissions() & fs::perms::owner_exec) != fs::perms::none, "execute bit kept");
        check((fs::status(destination / "lib/libc.so").permissions() & fs::perms::owner_exec) == fs::perms::none, "no execute bit invented");
        check((fs::status(destination / "bin/tool").permissions() & (fs::perms::group_write | fs::perms::others_write)) == fs::perms::none, "group/other write dropped");
    }
    // Implicit parent directories and a directory named twice are fine.
    check(attempt(work, {make_file("a/b/c.txt", "x"), make_dir("a/"), make_dir("a/b/")}).empty(), "implicit parents and repeated directories");

    // ---- rejections: one per rule -----------------------------------------------------------------------------------
    rejects(work, "absolute path", {make_file("/tmp/evil", "x")}, "absolute path");
    rejects(work, "dot-dot", {make_file("a/../../evil", "x")}, "'..'");
    rejects(work, "drive path", {make_file("C:/evil", "x")}, "drive-letter");
    rejects(work, "backslash path", {make_file("a\\evil", "x")}, "backslash");
    rejects(work, "reserved name", {make_file("lib/NUL", "x")}, "reserved device");
    rejects(work, "trailing dot", {make_file("lib/x.", "x")}, "ends in");
    rejects(work, "non-UTF-8 name", {make_file("lib/\xc3\x28", "x")}, "UTF-8");
    rejects(work, "case collision", {make_file("lib/Readme", "a"), make_file("lib/readme", "b")}, "case-colliding");
    rejects(work, "case collision in a directory", {make_file("Lib/a", "a"), make_file("lib/b", "b")}, "case-colliding");
    rejects(work, "duplicate file", {make_file("x", "a"), make_file("x", "b")}, "duplicate entry");
    rejects(work, "file then directory", {make_file("x", "a"), make_dir("x/")}, "both file and directory");
    rejects(work, "entry below a file", {make_file("x", "a"), make_file("x/y", "b")}, "below a file or symbolic link");
    rejects(work, "absolute symlink", {make_link("l", "/etc/passwd")}, "absolute symlink");
    rejects(work, "escaping symlink", {make_link("l", "../outside")}, "escapes");
    rejects(work, "escaping symlink from a subdirectory", {make_link("a/b/l", "../../../outside")}, "escapes");
    rejects(work, "write through a symlink", {make_link("l", "a"), make_file("l/evil", "x")}, "below a file or symbolic link");
    rejects(work, "hard link", {make_file("a", "x"), make_special("h", '1')}, "unsupported tar entry type '1'");
    rejects(work, "character device", {make_special("dev", '3')}, "unsupported tar entry type '3'");
    rejects(work, "block device", {make_special("dev", '4')}, "unsupported tar entry type '4'");
    rejects(work, "FIFO", {make_special("fifo", '6')}, "unsupported tar entry type '6'");
    rejects(work, "entry limit", {make_file("a", "1"), make_file("b", "2"), make_file("c", "3")}, "more than 2 entries", ArchiveLimits{.entries = 2});
    rejects(work, "per-file limit", {make_file("big", std::string(100, 'x'))}, "per-file limit", ArchiveLimits{.file_bytes = 50});
    rejects(work, "total limit", {make_file("a", std::string(60, 'x')), make_file("b", std::string(60, 'x'))}, "total size limit", ArchiveLimits{.total_bytes = 100});

    // ---- structural damage -----------------------------------------------------------------------------------------
    {
        auto tar = tar_of({make_file("a", std::string(2000, 'z'))});
        tar[10] ^= 1;  // corrupt the name: the checksum no longer matches
        auto destination = work.fresh();
        std::string message;
        try { extract_archive(work.archive(xz_of(tar)), destination); } catch (const Error& e) { message = e.what(); }
        check(message.find("checksum") != std::string::npos, "tar checksum enforced (" + message + ")");
        message.clear();
        auto cut = xz_of(tar_of({make_file("a", std::string(5000, 'z'))}));
        cut.resize(cut.size() / 2);
        try { extract_archive(work.archive(cut), work.fresh()); } catch (const Error& e) { message = e.what(); }
        check(!message.empty(), "truncated xz rejected (" + message + ")");
        message.clear();
        auto damaged = xz_of(tar_of({make_file("a", std::string(5000, 'z'))}));
        damaged[damaged.size() / 2] ^= 0x40;  // inside the compressed payload: the xz integrity check must catch it
        try { extract_archive(work.archive(damaged), work.fresh()); } catch (const Error& e) { message = e.what(); }
        check(!message.empty(), "corrupt xz payload rejected (" + message + ")");
        message.clear();
        try { extract_archive(work.archive("this is not an archive at all"), work.fresh()); } catch (const Error& e) { message = e.what(); }
        check(message.find("unsupported archive format") != std::string::npos, "unknown format rejected");
        message.clear();
        auto occupied = work.fresh();
        write_text(occupied / "x", "x");
        try { extract_archive(work.archive(xz_of(tar_of({make_file("a", "x")}))), occupied); } catch (const Error& e) { message = e.what(); }
        check(message.find("not empty") != std::string::npos, "non-empty destination rejected");
        // A tar cut inside a file's data (the end marker never arrives) is an error, not a short file.
        auto short_tar = tar_of({make_file("a", std::string(3000, 'q'))}).substr(0, 512 + 1000);
        message.clear();
        try { extract_archive(work.archive(xz_of(short_tar)), work.fresh()); } catch (const Error& e) { message = e.what(); }
        check(message.find("truncated tar") != std::string::npos, "tar cut inside file data rejected (" + message + ")");
    }
    // A highly compressible payload beyond the ratio limit is a decompression bomb.
    {
        std::string message;
        try { extract_archive(work.archive(xz_of(tar_of({make_file("zeros", std::string(40u << 20, '\0'))}))), work.fresh(),
                              ArchiveLimits{.compression_ratio = 16}); } catch (const Error& e) { message = e.what(); }
        check(message.find("decompression ratio") != std::string::npos, "decompression bomb rejected (" + message + ")");
    }

    // ---- zip ----
    {
        auto zip_error = [&](const std::string& bytes, const ArchiveLimits& limits = {}) {
            try { extract_archive(work.archive(bytes), work.fresh(), limits); } catch (const Error& e) { return std::string(e.what()); }
            return std::string();
        };
        auto zip_rejects = [&](const std::string& why, const std::string& bytes, const std::string& expect, const ArchiveLimits& limits = {}) {
            auto message = zip_error(bytes, limits);
            check(!message.empty(), "zip accepted: " + why);
            check(message.find(expect) != std::string::npos, "zip " + why + " (got: " + message + ")");
        };
        // Positive: deflate and stored, executable bit, symlink, nested directories, empty file, same report as tar.
        std::vector<Entry> tree = {make_dir("sdk"), make_file("sdk/a.txt", std::string(5000, 'a')), make_file("sdk/run", "#!/bin/sh\n", 0755),
                                   make_file("sdk/empty", ""), make_link("sdk/alias", "a.txt"), make_file("sdk/deep/er/file", "x")};
        for (bool deflate : {true, false}) {
            auto out = work.fresh();
            ZipOptions options; options.deflate = deflate;
            auto report = extract_archive(work.archive(zip_of(tree, options)), out);
            check(report.format == "zip" && report.files == 4 && report.symlinks == 1 && report.directories == 1, "zip report");
            check(read_text(out / "sdk/a.txt") == std::string(5000, 'a'), "zip content");
            check(fs::is_symlink(out / "sdk/alias") && fs::read_symlink(out / "sdk/alias") == "a.txt", "zip symlink kept");
            check((fs::status(out / "sdk/run").permissions() & fs::perms::owner_exec) != fs::perms::none, "zip exec bit kept");
            check((fs::status(out / "sdk/a.txt").permissions() & fs::perms::owner_exec) == fs::perms::none, "zip plain file not executable");
            check(fs::file_size(out / "sdk/empty") == 0, "zip empty file");
        }
        { ZipOptions dos; dos.host = 0; auto out = work.fresh();
          extract_archive(work.archive(zip_of({make_dir("d"), make_file("d/f", "dos")}, dos)), out);
          check(read_text(out / "d/f") == "dos", "zip made on DOS extracts as plain files"); }
        // Rules shared with tar, through the zip reader.
        zip_rejects("absolute path", zip_of({make_file("/etc/evil", "x")}), "absolute path");
        zip_rejects("dot-dot", zip_of({make_file("a/../../evil", "x")}), "'..'");
        zip_rejects("backslash", zip_of({make_file("a\\b", "x")}), "backslash");
        zip_rejects("drive path", zip_of({make_file("C:/evil", "x")}), "drive-letter");
        zip_rejects("reserved name", zip_of({make_file("sdk/aux.txt", "x")}), "reserved");
        zip_rejects("case collision", zip_of({make_file("a/B", "1"), make_file("a/b", "2")}), "case-colliding");
        zip_rejects("duplicate", zip_of({make_file("a", "1"), make_file("a", "2")}), "duplicate");
        zip_rejects("escaping symlink", zip_of({make_link("l", "../outside")}), "symlink");
        zip_rejects("absolute symlink", zip_of({make_link("l", "/etc")}), "symlink");
        zip_rejects("write through symlink", zip_of({make_link("l", "a"), make_file("l/x", "x")}), "below");
        { Entry device = make_special("dev", '3'); device.mode = 0;
          std::string bytes = zip_of({make_file("dev", "")}); // flip the unix type to a character device in the central directory
          auto at = bytes.rfind("PK\x01\x02"); uint32_t attrs = (020000u | 0666u) << 16; std::memcpy(&bytes[at + 38], &attrs, 4);
          zip_rejects("device node", bytes, "unsupported zip entry type"); }
        // Limits.
        zip_rejects("entry limit", zip_of({make_file("a", "1"), make_file("b", "1"), make_file("c", "1")}), "more than", ArchiveLimits{.entries = 2});
        zip_rejects("file limit", zip_of({make_file("a", std::string(100, 'x'))}), "per-file limit", ArchiveLimits{.file_bytes = 10});
        zip_rejects("total limit", zip_of({make_file("a", std::string(100, 'x')), make_file("b", std::string(100, 'x'))}), "total size limit", ArchiveLimits{.total_bytes = 150});
        zip_rejects("bomb", zip_of({make_file("zeros", std::string(40u << 20, '\0'))}), "decompression ratio", ArchiveLimits{.compression_ratio = 16});
        // Structure and integrity.
        { ZipOptions o; o.corrupt_crc = true; zip_rejects("bad CRC", zip_of({make_file("a", "payload")}, o), "CRC-32"); }
        { ZipOptions o; o.flags = 1; zip_rejects("encrypted", zip_of({make_file("a", "payload")}, o), "encrypted"); }
        { ZipOptions o; o.method_override = 12; zip_rejects("bzip2 method", zip_of({make_file("a", "payload")}, o), "compression method"); }
        { ZipOptions o; o.local_name_differs = true; zip_rejects("local/central mismatch", zip_of({make_file("a", "payload")}, o), "disagrees"); }
        { ZipOptions o; o.no_end_record = true; zip_rejects("no end record", zip_of({make_file("a", "payload")}, o), "end-of-central-directory"); }
        { auto bytes = zip_of({make_file("a", std::string(3000, 'q'))}); zip_rejects("truncated", bytes.substr(0, bytes.size() / 2), "end-of-central-directory"); }
        { auto bytes = zip_of({make_file("a", std::string(3000, 'q'))}, ZipOptions{.deflate = false}); bytes[40] ^= 0x55;
          zip_rejects("damaged stored data", bytes, "CRC-32"); }
        { auto bytes = zip_of({make_file("a", "0123456789abcdef")}); bytes[40] ^= 0xFF; bytes[41] ^= 0xFF;
          check(!zip_error(bytes).empty(), "damaged deflate data rejected"); }
    }

    fs::remove_all(work.root);
    std::printf("archive-test: %zu checks passed\n", checks);
    return 0;
}
