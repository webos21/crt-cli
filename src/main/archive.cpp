#include "crtcli/archive.h"
#include <lzma.h>
#include <zlib.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <set>

namespace crtcli {
namespace {

std::string lowered(std::string value) {
    for (auto& c : value) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}

std::vector<std::string> split(const std::string& path) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= path.size()) {
        size_t slash = path.find('/', start);
        if (slash == std::string::npos) slash = path.size();
        parts.push_back(path.substr(start, slash - start));
        start = slash + 1;
    }
    return parts;
}

// CON, PRN, AUX, NUL, COM1-9, LPT1-9, with or without an extension ("nul.txt" is the NUL device on Windows).
bool windows_reserved(const std::string& component) {
    auto stem = lowered(component.substr(0, component.find('.')));
    while (!stem.empty() && stem.back() == ' ') stem.pop_back();
    if (stem == "con" || stem == "prn" || stem == "aux" || stem == "nul" || stem == "conin$" || stem == "conout$") return true;
    return stem.size() == 4 && (stem.compare(0, 3, "com") == 0 || stem.compare(0, 3, "lpt") == 0) && stem[3] >= '1' && stem[3] <= '9';
}

}  // namespace

std::string archive_path_problem(const std::string& path, const ArchiveLimits& limits) {
    if (path.empty()) return "empty path";
    if (path.size() > limits.path_bytes) return "path longer than " + std::to_string(limits.path_bytes) + " bytes";
    if (!valid_utf8(path)) return "path is not valid UTF-8";
    for (unsigned char c : path) {
        if (c == 0) return "path contains NUL";
        if (c < 0x20 || c == 0x7f) return "path contains a control character";
        if (c == '\\') return "path contains a backslash";
    }
    if (path[0] == '/') return "absolute path";
    if (path.size() >= 2 && path[1] == ':' && std::isalpha(static_cast<unsigned char>(path[0]))) return "drive-letter path";
    auto parts = split(path);
    // A single trailing slash names a directory; anything else empty is a "//" or a leading slash.
    if (!parts.empty() && parts.back().empty()) parts.pop_back();
    if (parts.empty()) return "empty path";
    if (parts.size() > limits.depth) return "path deeper than " + std::to_string(limits.depth) + " components";
    for (const auto& part : parts) {
        if (part.empty()) return "empty path component";
        if (part == ".") return "'.' path component";
        if (part == "..") return "'..' path component";
        if (part.find(':') != std::string::npos) return "path component contains ':'";
        for (char c : part) if (c == '<' || c == '>' || c == '"' || c == '|' || c == '?' || c == '*') return "path component contains a Windows-reserved character";
        if (part.back() == '.' || part.back() == ' ') return "path component ends in '.' or a space";
        if (windows_reserved(part)) return "Windows reserved device name: " + part;
    }
    return {};
}

std::string archive_symlink_problem(const std::string& link, const std::string& target) {
    if (target.empty()) return "empty symlink target";
    if (!valid_utf8(target)) return "symlink target is not valid UTF-8";
    for (unsigned char c : target) {
        if (c == 0 || c < 0x20 || c == 0x7f) return "symlink target contains a control character";
        if (c == '\\') return "symlink target contains a backslash";
    }
    if (target[0] == '/') return "absolute symlink target";
    if (target.size() >= 2 && target[1] == ':' && std::isalpha(static_cast<unsigned char>(target[0]))) return "drive-letter symlink target";
    // Resolve lexically from the link's own directory and require the result to stay inside the extraction root.
    auto base = split(link);
    base.pop_back();
    std::vector<std::string> stack = base;
    for (const auto& part : split(target)) {
        if (part.empty() || part == ".") continue;
        if (part == "..") {
            if (stack.empty()) return "symlink target escapes the extraction root";
            stack.pop_back();
        } else stack.push_back(part);
    }
    if (stack.empty()) return "symlink target is the extraction root";
    return {};
}

Json ExtractReport::to_json() const {
    return {{"format", format}, {"entries", entries}, {"files", files}, {"directories", directories},
            {"symlinks", symlinks}, {"bytes", bytes}};
}

std::string detect_archive_format(const fs::path& archive) {
    std::ifstream in(archive.string(), std::ios::binary);
    require(in.good(), "cannot open archive: " + archive.string());
    unsigned char head[6] = {0};
    in.read(reinterpret_cast<char*>(head), sizeof(head));
    const unsigned char xz[6] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
    if (in.gcount() == 6 && std::memcmp(head, xz, 6) == 0) return "xz";
    if (in.gcount() >= 4 && head[0] == 'P' && head[1] == 'K' && (head[2] == 3 || head[2] == 5) && (head[3] == 4 || head[3] == 6)) return "zip";
    throw Error("unsupported archive format (expected xz or zip): " + archive.string());
}

namespace {

// ---- extraction target: owns the "what has been created so far" bookkeeping -----------------------------------

class Sink {
public:
    Sink(const fs::path& root, const ArchiveLimits& limits) : root_(fs::canonical(root)), limits_(limits) {}

    void begin_entry() {
        require(++report.entries <= limits_.entries, "archive has more than " + std::to_string(limits_.entries) + " entries");
    }

    // Records an entry name and enforces, for the entry and every directory above it:
    //   - names are unique ignoring case ("a/B" after "a/b" is a collision, whatever the host's file system does);
    //   - a component that is a file or a symlink is never a directory for a later entry;
    //   - a file or symlink name is used once; a directory may be named again (tar lists parents, and "a/" after "a/x").
    void claim(const std::string& path, bool directory) {
        auto clean = path;
        while (clean.size() > 1 && clean.back() == '/') clean.pop_back();
        auto parts = split(clean);
        std::string prefix;
        for (size_t i = 0; i < parts.size(); ++i) {
            prefix += (i ? "/" : "") + parts[i];
            const bool last = i + 1 == parts.size();
            const bool is_directory = !last || directory;
            const auto key = lowered(prefix);
            auto found = names_.find(key);
            if (found == names_.end()) {
                names_.emplace(key, Name{prefix, is_directory});
                continue;
            }
            const Name& seen = found->second;
            require(seen.spelling == prefix, "case-colliding names: " + seen.spelling + " and " + prefix);
            if (!last) {
                require(seen.directory, "entry below a file or symbolic link: " + prefix);
            } else if (directory) {
                require(seen.directory, "name used as both file and directory: " + prefix);
            } else {
                throw Error("duplicate entry: " + prefix);
            }
        }
    }

    fs::path target(const std::string& path) const { return root_ / fs::path(path); }
    const fs::path& root() const { return root_; }
    const ArchiveLimits& limits() const { return limits_; }
    ExtractReport report;

private:
    fs::path root_;
    ArchiveLimits limits_;
    struct Name { std::string spelling; bool directory; };
    std::map<std::string, Name> names_;
};

void make_parent(const fs::path& file) {
    std::error_code ec;
    fs::create_directories(file.parent_path(), ec);
    require(!ec, "cannot create directory " + file.parent_path().string() + ": " + ec.message());
}

// ---- file/symlink/directory creation, shared by every archive format -----------------------------------------

void put_directory(Sink& sink, const std::string& path) {
    sink.begin_entry();
    sink.claim(path, true);
    std::error_code ec;
    fs::create_directories(sink.target(path), ec);
    require(!ec, "cannot create directory " + path + ": " + ec.message());
    ++sink.report.directories;
}

void put_symlink(Sink& sink, const std::string& path, const std::string& target) {
    sink.begin_entry();
    auto problem = archive_symlink_problem(path, target);
    require(problem.empty(), "unsafe symlink " + path + " -> " + target + ": " + problem);
    sink.claim(path, false);
    make_parent(sink.target(path));
    std::error_code ec;
    fs::create_symlink(target, sink.target(path), ec);
    require(!ec, "cannot create symlink " + path + ": " + ec.message());
    ++sink.report.symlinks;
}

class FileWriter {
public:
    FileWriter(Sink& sink, const std::string& path, uint64_t size, bool executable) : sink_(sink), path_(path), expected_(size) {
        sink.begin_entry();
        require(size <= sink.limits().file_bytes, "file larger than the per-file limit: " + path);
        require(sink.report.bytes + size <= sink.limits().total_bytes, "archive exceeds the total size limit");
        sink.claim(path, false);
        make_parent(sink.target(path));
        file_ = std::fopen(sink.target(path).string().c_str(), "wb");
        require(file_ != nullptr, "cannot create file: " + path);
        executable_ = executable;
    }
    ~FileWriter() { if (file_) std::fclose(file_); }
    void write(const char* data, size_t size) {
        written_ += size;
        require(written_ <= expected_, "file data exceeds its declared size: " + path_);
        require(std::fwrite(data, 1, size, file_) == size, "cannot write file: " + path_);
    }
    void finish() {
        require(written_ == expected_, "file data shorter than its declared size: " + path_);
        require(std::fclose(file_) == 0, "cannot close file: " + path_);
        file_ = nullptr;
        std::error_code ec;
        fs::permissions(sink_.target(path_), executable_ ? (fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec)
                                                         : (fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read | fs::perms::others_read), ec);
        sink_.report.bytes += written_;
        ++sink_.report.files;
    }
private:
    Sink& sink_;
    std::string path_;
    uint64_t expected_;
    uint64_t written_ = 0;
    bool executable_ = false;
    std::FILE* file_ = nullptr;
};

// ---- a pull-style byte stream over the decompressed archive ---------------------------------------------------

class ByteSource {
public:
    virtual ~ByteSource() = default;
    // Fills up to `size` bytes; returns the number read (0 at end of stream).
    virtual size_t read(char* out, size_t size) = 0;
    uint64_t consumed_in = 0;   // compressed bytes read from the file
    uint64_t produced = 0;      // decompressed bytes handed out
};

class XzSource : public ByteSource {
public:
    XzSource(const fs::path& path, const ArchiveLimits& limits) : in_(path.string(), std::ios::binary), limits_(limits) {
        require(in_.good(), "cannot open archive: " + path.string());
        lzma_stream init = LZMA_STREAM_INIT;
        stream_ = init;
        // memlimit is generous (a 64 MiB dictionary needs ~65 MiB); a hostile header cannot force more than this.
        auto rc = lzma_stream_decoder(&stream_, 1ull << 30, LZMA_CONCATENATED);
        require(rc == LZMA_OK, "cannot initialize the xz decoder");
        ok_ = true;
    }
    ~XzSource() { if (ok_) lzma_end(&stream_); }
    size_t read(char* out, size_t size) override {
        stream_.next_out = reinterpret_cast<uint8_t*>(out);
        stream_.avail_out = size;
        while (stream_.avail_out > 0 && !done_) {
            if (stream_.avail_in == 0) {
                in_.read(inbuf_, sizeof(inbuf_));
                stream_.next_in = reinterpret_cast<const uint8_t*>(inbuf_);
                stream_.avail_in = static_cast<size_t>(in_.gcount());
                consumed_in += stream_.avail_in;
                if (stream_.avail_in == 0) finishing_ = true;
            }
            auto rc = lzma_code(&stream_, finishing_ ? LZMA_FINISH : LZMA_RUN);
            if (rc == LZMA_STREAM_END) { done_ = true; break; }
            if (rc == LZMA_BUF_ERROR && finishing_) throw Error("truncated xz archive");
            if (rc != LZMA_OK && rc != LZMA_BUF_ERROR) throw Error("corrupt xz archive (liblzma error " + std::to_string(rc) + ")");
        }
        size_t produced_now = size - stream_.avail_out;
        produced += produced_now;
        require(consumed_in == 0 || produced / std::max<uint64_t>(consumed_in, 1) <= limits_.compression_ratio || produced < (1ull << 20),
                "decompression ratio beyond the limit (decompression bomb)");
        return produced_now;
    }
private:
    std::ifstream in_;
    ArchiveLimits limits_;
    lzma_stream stream_;
    char inbuf_[1 << 16];
    bool ok_ = false;
    bool done_ = false;
    bool finishing_ = false;
};

// ---- tar -------------------------------------------------------------------------------------------------------

uint64_t octal(const char* field, size_t size) {
    if (static_cast<unsigned char>(field[0]) & 0x80) {  // GNU base-256 for large values
        uint64_t value = static_cast<unsigned char>(field[0]) & 0x7f;
        for (size_t i = 1; i < size; ++i) value = (value << 8) | static_cast<unsigned char>(field[i]);
        return value;
    }
    uint64_t value = 0;
    size_t i = 0;
    while (i < size && (field[i] == ' ' || field[i] == '\0')) ++i;
    for (; i < size && field[i] >= '0' && field[i] <= '7'; ++i) value = value * 8 + static_cast<uint64_t>(field[i] - '0');
    return value;
}

std::string field_string(const char* field, size_t size) {
    size_t length = 0;
    while (length < size && field[length] != '\0') ++length;
    return std::string(field, length);
}

void read_exact(ByteSource& source, char* out, size_t size, const char* what) {
    size_t have = 0;
    while (have < size) {
        auto got = source.read(out + have, size - have);
        require(got > 0, std::string("truncated tar archive in ") + what);
        have += got;
    }
}

void skip(ByteSource& source, uint64_t size, const char* what) {
    char scratch[1 << 14];
    while (size > 0) {
        auto chunk = static_cast<size_t>(std::min<uint64_t>(size, sizeof(scratch)));
        read_exact(source, scratch, chunk, what);
        size -= chunk;
    }
}

std::string read_block_text(ByteSource& source, uint64_t size, const char* what) {
    require(size <= (1u << 20), std::string("oversized tar metadata record: ") + what);
    std::string value(static_cast<size_t>(size), '\0');
    if (size) read_exact(source, value.data(), value.size(), what);
    skip(source, (512 - size % 512) % 512, what);
    return value;
}

// pax records: "<length> <key>=<value>\n".
std::map<std::string, std::string> parse_pax(const std::string& text) {
    std::map<std::string, std::string> records;
    size_t at = 0;
    while (at < text.size()) {
        size_t space = text.find(' ', at);
        require(space != std::string::npos, "malformed pax record");
        uint64_t length = 0;
        for (size_t i = at; i < space; ++i) {
            require(text[i] >= '0' && text[i] <= '9', "malformed pax record length");
            length = length * 10 + static_cast<uint64_t>(text[i] - '0');
        }
        require(length > space - at + 1 && at + length <= text.size() && text[at + length - 1] == '\n', "malformed pax record");
        auto body = text.substr(space + 1, at + length - 1 - (space + 1));
        auto equals = body.find('=');
        require(equals != std::string::npos, "malformed pax record");
        records[body.substr(0, equals)] = body.substr(equals + 1);
        at += length;
    }
    return records;
}

void extract_tar(ByteSource& source, Sink& sink) {
    std::string long_name, long_link;
    std::map<std::string, std::string> pax_next, pax_global;
    char header[512];
    int zero_blocks = 0;
    for (;;) {
        size_t got = source.read(header, sizeof(header));
        if (got == 0) break;                       // a stream without the end marker is tolerated at a record boundary
        if (got < sizeof(header)) read_exact(source, header + got, sizeof(header) - got, "a header");
        bool zero = std::all_of(header, header + sizeof(header), [](char c) { return c == 0; });
        if (zero) { if (++zero_blocks >= 2) break; continue; }
        zero_blocks = 0;
        // Checksum: unsigned sum of the header with the checksum field read as spaces.
        uint64_t sum = 0;
        for (size_t i = 0; i < sizeof(header); ++i) sum += (i >= 148 && i < 156) ? 32u : static_cast<unsigned char>(header[i]);
        require(octal(header + 148, 8) == sum, "tar header checksum mismatch");

        char type = header[156] ? header[156] : '0';
        uint64_t size = octal(header + 124, 12);
        auto mode = octal(header + 100, 8);
        if (type == 'L') { long_name = field_string(read_block_text(source, size, "a GNU long name").data(), size); continue; }
        if (type == 'K') { long_link = field_string(read_block_text(source, size, "a GNU long link").data(), size); continue; }
        if (type == 'x') { pax_next = parse_pax(read_block_text(source, size, "a pax header")); continue; }
        if (type == 'g') { for (auto& r : parse_pax(read_block_text(source, size, "a global pax header"))) pax_global[r.first] = r.second; continue; }

        std::string name = field_string(header, 100);
        std::string prefix = field_string(header + 345, 155);
        if (!prefix.empty() && std::memcmp(header + 257, "ustar", 5) == 0) name = prefix + "/" + name;
        std::string link = field_string(header + 157, 100);
        auto pick = [&](const char* key, std::string current) {
            if (pax_next.count(key)) return pax_next[key];
            if (pax_global.count(key)) return pax_global[key];
            return current;
        };
        if (!long_name.empty()) name = long_name;
        if (!long_link.empty()) link = long_link;
        name = pick("path", name);
        link = pick("linkpath", link);
        if (pax_next.count("size")) size = std::stoull(pax_next["size"]);
        long_name.clear(); long_link.clear(); pax_next.clear();

        auto problem = archive_path_problem(name, sink.limits());
        require(problem.empty(), "unsafe archive path '" + name + "': " + problem);
        bool directory = type == '5' || (type == '0' && name.back() == '/');
        if (directory) {
            put_directory(sink, name);
            skip(source, size + (512 - size % 512) % 512, "a directory entry");
            continue;
        }
        if (type == '2') {
            put_symlink(sink, name, link);
            continue;
        }
        if (type == '0' || type == '7') {
            FileWriter writer(sink, name, size, (mode & 0100) != 0);
            char buffer[1 << 16];
            uint64_t left = size;
            while (left > 0) {
                auto chunk = static_cast<size_t>(std::min<uint64_t>(left, sizeof(buffer)));
                read_exact(source, buffer, chunk, "file data");
                writer.write(buffer, chunk);
                left -= chunk;
            }
            skip(source, (512 - size % 512) % 512, "file padding");
            writer.finish();
            continue;
        }
        throw Error(std::string("unsupported tar entry type '") + type + "' (hard links, devices, FIFOs and sockets are rejected): " + name);
    }
}


// ---- zip: central directory driven, stored and deflate only --------------------------------------------------
//
// The central directory is the authority (local headers must agree with it). Not supported, and refused with a
// reason: zip64, multi-disk archives, encryption, and methods other than stored/deflate. Entry types come from the
// Unix mode in the external attributes when the archive was made on Unix (host 3); other hosts only mark directories
// with a trailing slash.

uint32_t le16(const unsigned char* p) { return static_cast<uint32_t>(p[0] | (p[1] << 8)); }
uint32_t le32(const unsigned char* p) { return le16(p) | (le16(p + 2) << 16); }

void extract_zip(const fs::path& path, Sink& sink) {
    std::ifstream in(path.string(), std::ios::binary);
    require(in.good(), "cannot open archive: " + path.string());
    in.seekg(0, std::ios::end);
    const uint64_t file_size = static_cast<uint64_t>(in.tellg());
    require(file_size >= 22, "truncated zip archive");
    auto read_at = [&](uint64_t offset, void* out, size_t size, const char* what) {
        require(offset + size <= file_size, std::string("truncated zip archive (") + what + ")");
        in.clear();
        in.seekg(static_cast<std::streamoff>(offset));
        in.read(static_cast<char*>(out), static_cast<std::streamsize>(size));
        require(static_cast<size_t>(in.gcount()) == size, std::string("cannot read zip archive (") + what + ")");
    };

    // End of central directory: the last 22+comment bytes; scan back for the signature.
    const uint64_t tail_size = std::min<uint64_t>(file_size, 22 + 65535);
    std::vector<unsigned char> tail(tail_size);
    read_at(file_size - tail_size, tail.data(), tail.size(), "end record");
    size_t eocd = std::string::npos;
    for (size_t i = tail.size() - 22 + 1; i-- > 0;) {
        if (le32(&tail[i]) == 0x06054b50 && i + 22 + le16(&tail[i + 20]) == tail.size()) { eocd = i; break; }
    }
    require(eocd != std::string::npos, "not a zip archive: no end-of-central-directory record");
    const unsigned char* end = &tail[eocd];
    require(le16(end + 4) == 0 && le16(end + 6) == 0, "multi-disk zip archives are not supported");
    uint64_t count = le16(end + 10), directory_size = le32(end + 12), directory_offset = le32(end + 16);
    require(le16(end + 8) == count, "inconsistent zip entry counts");
    require(count != 0xFFFF && directory_size != 0xFFFFFFFFull && directory_offset != 0xFFFFFFFFull, "zip64 archives are not supported");
    require(count <= sink.limits().entries, "archive has more than " + std::to_string(sink.limits().entries) + " entries");
    require(directory_offset + directory_size <= file_size - tail_size + eocd, "zip central directory lies outside the archive");

    std::vector<unsigned char> directory(directory_size);
    read_at(directory_offset, directory.data(), directory.size(), "central directory");
    size_t at = 0;
    for (uint64_t index = 0; index < count; ++index) {
        require(at + 46 <= directory.size() && le32(&directory[at]) == 0x02014b50, "corrupt zip central directory");
        const unsigned char* h = &directory[at];
        const uint32_t made_by = le16(h + 4), flags = le16(h + 8), method = le16(h + 10), crc = le32(h + 16);
        const uint64_t packed = le32(h + 20), size = le32(h + 24), local = le32(h + 42);
        const size_t name_length = le16(h + 28), extra_length = le16(h + 30), comment_length = le16(h + 32);
        const uint32_t external = le32(h + 38);
        require(at + 46 + name_length + extra_length + comment_length <= directory.size(), "corrupt zip central directory");
        std::string name(reinterpret_cast<const char*>(h + 46), name_length);
        at += 46 + name_length + extra_length + comment_length;

        require(packed != 0xFFFFFFFFull && size != 0xFFFFFFFFull && local != 0xFFFFFFFFull && le16(h + 34) != 0xFFFF, "zip64 entries are not supported: " + name);
        require((flags & 1) == 0 && (flags & 0x40) == 0, "encrypted zip entries are not supported: " + name);
        require(method == 0 || method == 8, "unsupported zip compression method " + std::to_string(method) + ": " + name);
        auto problem = archive_path_problem(name, sink.limits());
        require(problem.empty(), "unsafe archive path '" + name + "': " + problem);

        const uint32_t unix_mode = (made_by >> 8) == 3 ? (external >> 16) : 0;
        const uint32_t kind = unix_mode & 0170000;
        const bool directory_entry = name.back() == '/' || kind == 0040000;
        if (directory_entry) {
            require(size == 0, "zip directory entry with data: " + name);
            put_directory(sink, name);
            continue;
        }
        require(kind == 0 || kind == 0100000 || kind == 0120000,
                "unsupported zip entry type (devices, FIFOs and sockets are rejected): " + name);

        // Local header: must agree with the central directory; the data follows its variable parts.
        unsigned char l[30];
        read_at(local, l, sizeof(l), "local header");
        require(le32(l) == 0x04034b50, "corrupt zip local header: " + name);
        const size_t local_name = le16(l + 26), local_extra = le16(l + 28);
        std::string name_here(local_name, '\0');
        read_at(local + 30, name_here.data(), local_name, "local name");
        require(name_here == name, "zip local header disagrees with the central directory: " + name);
        const uint64_t data_offset = local + 30 + local_name + local_extra;
        require(data_offset + packed <= directory_offset, "zip entry data lies outside the archive: " + name);
        require(method != 0 || packed == size, "stored zip entry with inconsistent sizes: " + name);

        const bool symlink = kind == 0120000;
        std::string link_text;
        std::unique_ptr<FileWriter> writer;
        if (symlink) {
            require(size <= 4096, "symbolic link target too long: " + name);
        } else {
            writer = std::make_unique<FileWriter>(sink, name, size, (unix_mode & 0100) != 0);
        }
        uint32_t actual_crc = crc32(0, nullptr, 0);
        auto emit = [&](const unsigned char* data, size_t length) {
            actual_crc = crc32(actual_crc, data, static_cast<uInt>(length));
            if (symlink) link_text.append(reinterpret_cast<const char*>(data), length);
            else writer->write(reinterpret_cast<const char*>(data), length);
        };
        std::vector<unsigned char> in_buffer(1 << 16), out_buffer(1 << 16);
        uint64_t left = packed, produced = 0;
        z_stream z{};
        if (method == 8) require(inflateInit2(&z, -15) == Z_OK, "cannot initialize the deflate decoder");
        struct Guard { z_stream* z; bool on; ~Guard() { if (on) inflateEnd(z); } } guard{&z, method == 8};
        bool ended = method == 0;
        while (left > 0 || !ended) {
            if (method == 0) {
                auto chunk = static_cast<size_t>(std::min<uint64_t>(left, in_buffer.size()));
                read_at(data_offset + (packed - left), in_buffer.data(), chunk, "stored data");
                emit(in_buffer.data(), chunk);
                produced += chunk; left -= chunk;
                continue;
            }
            if (z.avail_in == 0) {
                require(left > 0, "truncated deflate stream: " + name);
                auto chunk = static_cast<size_t>(std::min<uint64_t>(left, in_buffer.size()));
                read_at(data_offset + (packed - left), in_buffer.data(), chunk, "compressed data");
                z.next_in = in_buffer.data(); z.avail_in = static_cast<uInt>(chunk); left -= chunk;
            }
            z.next_out = out_buffer.data(); z.avail_out = static_cast<uInt>(out_buffer.size());
            int rc = inflate(&z, Z_NO_FLUSH);
            require(rc == Z_OK || rc == Z_STREAM_END, "corrupt deflate stream: " + name);
            size_t got = out_buffer.size() - z.avail_out;
            produced += got;
            require(produced <= size, "zip entry data exceeds its declared size: " + name);
            require(produced < (1ull << 20) || produced / std::max<uint64_t>(packed, 1) <= sink.limits().compression_ratio,
                    "decompression ratio beyond the limit (decompression bomb): " + name);
            emit(out_buffer.data(), got);
            if (rc == Z_STREAM_END) { ended = true; break; }
        }
        require(produced == size, "zip entry shorter than its declared size: " + name);
        require(actual_crc == crc, "zip entry CRC-32 mismatch: " + name);
        if (symlink) put_symlink(sink, name, link_text);
        else writer->finish();
    }
    require(at == directory.size(), "trailing bytes in the zip central directory");
}

}  // namespace

ExtractReport extract_archive(const fs::path& archive, const fs::path& destination, const ArchiveLimits& limits) {
    require(fs::is_directory(destination), "extraction destination is not a directory: " + destination.string());
    require(fs::is_empty(destination), "extraction destination is not empty: " + destination.string());
    auto format = detect_archive_format(archive);
    Sink sink(destination, limits);
    if (format == "xz") {
        XzSource source(archive, limits);
        extract_tar(source, sink);
        sink.report.format = "tar.xz";
    } else {
        extract_zip(archive, sink);
        sink.report.format = "zip";
    }
    return sink.report;
}

}  // namespace crtcli
