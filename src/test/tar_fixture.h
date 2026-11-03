#pragma once
// Hand-built tar.xz fixtures shared by the archive and SDK-store tests: the product only decodes, tests encode with liblzma.
#include <crtcli/core.h>
#include <lzma.h>
#include <zlib.h>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace tarfix {
using namespace crtcli;

struct Entry {
    std::string name;
    char type = '0';
    std::string data;        // file bytes
    std::string link;        // link target
    unsigned mode = 0644;
    bool gnu_long_name = false;
    bool pax_path = false;
};

void put_octal(char* field, size_t size, uint64_t value) {
    std::snprintf(field, size, "%0*llo", static_cast<int>(size - 1), static_cast<unsigned long long>(value));
}

std::string header_block(const Entry& entry, const std::string& name_field, uint64_t size, char type) {
    std::string block(512, '\0');
    std::memcpy(&block[0], name_field.data(), std::min<size_t>(name_field.size(), 100));
    put_octal(&block[100], 8, entry.mode);
    put_octal(&block[108], 8, 0);
    put_octal(&block[116], 8, 0);
    put_octal(&block[124], 12, size);
    put_octal(&block[136], 12, 0);
    std::memset(&block[148], ' ', 8);
    block[156] = type;
    std::memcpy(&block[157], entry.link.data(), std::min<size_t>(entry.link.size(), 100));
    std::memcpy(&block[257], "ustar\0" "00", 8);
    unsigned sum = 0;
    for (unsigned char c : block) sum += c;
    std::snprintf(&block[148], 8, "%06o", sum);
    block[154] = '\0';
    block[155] = ' ';
    return block;
}

std::string padded(const std::string& data) { return data + std::string((512 - data.size() % 512) % 512, '\0'); }

std::string pax_record(const std::string& key, const std::string& value) {
    std::string body = " " + key + "=" + value + "\n";
    size_t length = body.size() + 1;
    while (std::to_string(length).size() + body.size() != length) length = std::to_string(length).size() + body.size();
    return std::to_string(length) + body;
}

std::string tar_entry(const Entry& entry) {
    std::string out;
    if (entry.gnu_long_name) {
        Entry meta; meta.name = "././@LongLink";
        out += header_block(meta, "././@LongLink", entry.name.size() + 1, 'L') + padded(entry.name + std::string(1, '\0'));
    }
    if (entry.pax_path) {
        auto record = pax_record("path", entry.name);
        Entry meta;
        out += header_block(meta, "PaxHeader", record.size(), 'x') + padded(record);
    }
    std::string field = (entry.gnu_long_name || entry.pax_path) ? entry.name.substr(0, 90) : entry.name;
    uint64_t size = (entry.type == '0' || entry.type == '7') ? entry.data.size() : 0;
    out += header_block(entry, field, size, entry.type);
    if (size) out += padded(entry.data);
    return out;
}

std::string tar_of(const std::vector<Entry>& entries, bool end_marker = true) {
    std::string out;
    for (const auto& entry : entries) out += tar_entry(entry);
    if (end_marker) out += std::string(1024, '\0');
    return out;
}

std::string xz_of(const std::string& tar) {
    std::string out(tar.size() + tar.size() / 2 + 4096, '\0');
    lzma_stream stream = LZMA_STREAM_INIT;
    require(lzma_easy_encoder(&stream, 6, LZMA_CHECK_CRC64) == LZMA_OK, "xz encoder");
    stream.next_in = reinterpret_cast<const uint8_t*>(tar.data());
    stream.avail_in = tar.size();
    stream.next_out = reinterpret_cast<uint8_t*>(out.data());
    stream.avail_out = out.size();
    require(lzma_code(&stream, LZMA_FINISH) == LZMA_STREAM_END, "xz encode");
    out.resize(out.size() - stream.avail_out);
    lzma_end(&stream);
    return out;
}


inline void put16(std::string& out, uint32_t v) { out.push_back(char(v & 255)); out.push_back(char((v >> 8) & 255)); }
inline void put32(std::string& out, uint32_t v) { put16(out, v & 0xFFFF); put16(out, v >> 16); }

struct ZipOptions {
    bool deflate = true;          // false: stored
    uint32_t host = 3;            // "made by" host: 3 = Unix (modes honoured), 0 = DOS
    uint32_t flags = 0;
    uint32_t method_override = 0xFFFF;
    bool corrupt_crc = false;
    bool local_name_differs = false;
    bool no_end_record = false;
};

inline std::string raw_deflate(const std::string& data) {
    z_stream z{};
    require(deflateInit2(&z, 6, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) == Z_OK, "deflate init");
    std::string out(data.size() + data.size() / 100 + 64, '\0');
    z.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data())); z.avail_in = static_cast<uInt>(data.size());
    z.next_out = reinterpret_cast<Bytef*>(out.data()); z.avail_out = static_cast<uInt>(out.size());
    require(deflate(&z, Z_FINISH) == Z_STREAM_END, "deflate");
    out.resize(out.size() - z.avail_out);
    deflateEnd(&z);
    return out;
}

// Entry types as in the tar fixtures: '0' file (mode kept), '5' directory, '2' symlink (target in `link`).
inline std::string zip_of(const std::vector<Entry>& entries, const ZipOptions& options = {}) {
    std::string body, central;
    for (const auto& entry : entries) {
        std::string name = entry.name + (entry.type == '5' && (entry.name.empty() || entry.name.back() != '/') ? "/" : "");
        std::string data = entry.type == '2' ? entry.link : (entry.type == '5' ? std::string() : entry.data);
        bool compress = options.deflate && !data.empty();
        std::string packed = compress ? raw_deflate(data) : data;
        uint32_t crc = static_cast<uint32_t>(crc32(0, reinterpret_cast<const Bytef*>(data.data()), static_cast<uInt>(data.size())));
        if (options.corrupt_crc) crc ^= 1;
        uint32_t method = options.method_override != 0xFFFF ? options.method_override : (compress ? 8 : 0);
        uint32_t offset = static_cast<uint32_t>(body.size());
        std::string local_name = options.local_name_differs ? "X" + name : name;
        put32(body, 0x04034b50); put16(body, 20); put16(body, options.flags); put16(body, method); put32(body, 0);
        put32(body, crc); put32(body, packed.size()); put32(body, data.size());
        put16(body, local_name.size()); put16(body, 0); body += local_name; body += packed;
        uint32_t mode = entry.type == '5' ? (040000 | entry.mode) : entry.type == '2' ? (0120000 | entry.mode) : (0100000 | entry.mode);
        put32(central, 0x02014b50); put16(central, (options.host << 8) | 20); put16(central, 20); put16(central, options.flags);
        put16(central, method); put32(central, 0); put32(central, crc); put32(central, packed.size()); put32(central, data.size());
        put16(central, name.size()); put16(central, 0); put16(central, 0); put16(central, 0); put16(central, 0);
        put32(central, options.host == 3 ? (mode << 16) : 0); put32(central, offset); central += name;
    }
    std::string out = body;
    uint32_t directory_offset = static_cast<uint32_t>(out.size());
    out += central;
    if (!options.no_end_record) {
        put32(out, 0x06054b50); put16(out, 0); put16(out, 0); put16(out, entries.size()); put16(out, entries.size());
        put32(out, central.size()); put32(out, directory_offset); put16(out, 0);
    }
    return out;
}

struct Work {
    fs::path root;
    int next = 0;
    explicit Work(fs::path base) : root(std::move(base)) {}
    fs::path archive(const std::string& bytes) {
        auto path = root / ("a" + std::to_string(next++) + ".tar.xz");
        std::ofstream(path, std::ios::binary).write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return path;
    }
    fs::path fresh() {
        auto path = root / ("out" + std::to_string(next++));
        fs::create_directories(path);
        return path;
    }
};

Entry make_file(const std::string& name, const std::string& data, unsigned mode = 0644) { Entry e; e.name = name; e.data = data; e.mode = mode; return e; }
Entry make_dir(const std::string& name) { Entry e; e.name = name; e.type = '5'; e.mode = 0755; return e; }
Entry make_link(const std::string& name, const std::string& target) { Entry e; e.name = name; e.type = '2'; e.link = target; e.mode = 0777; return e; }
Entry make_special(const std::string& name, char type) { Entry e; e.name = name; e.type = type; return e; }

}  // namespace tarfix
