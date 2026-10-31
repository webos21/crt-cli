#include <crtcli/core.h>
#include <crt/platform.h>
#include <mbedtls/sha256.h>
#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <set>
#include <algorithm>
#include <unistd.h>
namespace crtcli {
void require(bool condition, const std::string& message) { if (!condition) throw Error(message); }
std::string read_text(const fs::path& path) {
    std::ifstream input(path.string(), std::ios::binary);
    require(bool(input), "cannot read: " + path.string());
    std::string result((std::istreambuf_iterator<char>(input)), {});
    require(!input.bad(), "read failed: " + path.string());
    return result;
}
void write_text(const fs::path& path, const std::string& value) {
    std::ofstream output(path.string(), std::ios::binary | std::ios::trunc);
    require(bool(output), "cannot write: " + path.string());
    output.write(value.data(), value.size());
    output.close();
    require(bool(output), "write failed: " + path.string());
}
Json read_json(const fs::path& path) {
    std::vector<std::set<std::string>> keys;
    auto check = [&](int, Json::parse_event_t event, Json& parsed) {
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        if (event == Json::parse_event_t::key)
            require(keys.back().insert(parsed.get<std::string>()).second, "duplicate JSON key: " + parsed.get<std::string>());
        if (event == Json::parse_event_t::object_end) keys.pop_back();
        return true;
    };
    return Json::parse(read_text(path), check);
}
namespace {
std::string hex_digest(const unsigned char* digest) {
    const char hex[] = "0123456789abcdef";
    std::string result;
    for (size_t i = 0; i < 32; ++i) { result += hex[digest[i] >> 4]; result += hex[digest[i] & 15]; }
    return result;
}
}
std::string sha256(const std::string& bytes) {
    unsigned char digest[32];
    require(mbedtls_sha256(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), digest, 0) == 0, "SHA256 failed");
    return hex_digest(digest);
}
std::string file_sha256(const fs::path& path) {
    // Descriptor reads, not stdio: CRT's fread copies byte by byte (about 70 MB/s on Windows), while read() runs at memory speed,
    // and the SDK identity check hashes hundreds of MB on every invocation.
    int descriptor = open(path.string().c_str(), O_RDONLY);
    require(descriptor >= 0, "cannot hash: " + path.string());
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    int error = mbedtls_sha256_starts(&hash, 0);
    static thread_local std::vector<unsigned char> buffer(1 << 20);
    bool failed = false;
    for (;;) {
        ssize_t count = read(descriptor, buffer.data(), buffer.size());
        if (count == 0) break;
        if (count < 0) { if (errno == EINTR) continue; failed = true; break; }
        error = mbedtls_sha256_update(&hash, buffer.data(), static_cast<size_t>(count));
        if (error != 0) break;
    }
    close(descriptor);
    unsigned char digest[32];
    if (error == 0 && !failed) error = mbedtls_sha256_finish(&hash, digest);
    mbedtls_sha256_free(&hash);
    require(!failed && error == 0, "hash read failed: " + path.string());
    return hex_digest(digest);
}
fs::path inside(const fs::path& root, const std::string& relative) {
    fs::path value(relative);
    require(!value.empty() && !value.is_absolute(), "expected relative path: " + relative);
    for (const auto& part : value) require(part != "..", "path traversal: " + relative);
    auto base = fs::canonical(root);
    auto result = fs::weakly_canonical(base / value);
    auto remainder = result.lexically_relative(base);
    require(!remainder.empty() && *remainder.begin() != "..", "path escapes root: " + relative);
    return result;
}
Json inventory(const fs::path& root) {
    auto base = fs::canonical(root);
    std::vector<fs::path> paths;
    for (const auto& entry : fs::recursive_directory_iterator(base))
        if (entry.is_symlink() || entry.is_regular_file()) paths.push_back(entry.path());
    std::sort(paths.begin(), paths.end());
    Json result = Json::array();
    for (const auto& path : paths) {
        auto relative = path.lexically_relative(base).generic_string();
        if (fs::is_symlink(path)) {
            inside(base, relative);
            result.push_back({relative, "symlink", fs::read_symlink(path).generic_string()});
        } else result.push_back({relative, "file", file_sha256(path)});
    }
    return result;
}
Json native_target() {
    const auto* platform = crt_get_platform_info();
    return {{"os", platform->os}, {"arch", platform->arch}};
}
fs::path sdk_store_root() {
    std::vector<char> home(65536);
    require(crt_get_user_home(home.data(), home.size()) == 0, "PAL user home is unavailable");
    return fs::path(home.data()) / ".crt/sdks";
}
void native_executable(const fs::path& path) {
    require(path.is_absolute() && fs::is_regular_file(path), "external tool must be an absolute native executable: " + path.string());
    require(crt_is_native_executable(path.string().c_str()) == 1, "script/interpreter or non-native tool is unsupported: " + path.string());
}
bool valid_utf8(const std::string& value) {
    for (size_t i = 0; i < value.size();) {
        unsigned char c = static_cast<unsigned char>(value[i]);
        size_t length = c < 0x80 ? 1 : (c >= 0xC2 && c <= 0xDF) ? 2 : (c >= 0xE0 && c <= 0xEF) ? 3 : (c >= 0xF0 && c <= 0xF4) ? 4 : 0;
        if (length == 0 || i + length > value.size()) return false;
        for (size_t k = 1; k < length; ++k) if ((static_cast<unsigned char>(value[i + k]) & 0xC0) != 0x80) return false;
        unsigned char second = length > 1 ? static_cast<unsigned char>(value[i + 1]) : 0;
        if ((c == 0xE0 && second < 0xA0) || (c == 0xED && second > 0x9F) || (c == 0xF0 && second < 0x90) || (c == 0xF4 && second > 0x8F)) return false;
        i += length;
    }
    return true;
}
void require_native_utf8(const std::string& value, const std::string& what) {
    if (std::string(crt_get_platform_info()->os) == "windows")
        require(valid_utf8(value), what + " is not valid UTF-8; Windows would silently replace the invalid bytes");
}
void check_arguments(const std::vector<std::string>& args) {
    const bool ansi_process = std::string(crt_get_narrow_encoding()) != "utf-8";
    for (const auto& argument : args) {
        bool ascii = true;
        for (unsigned char c : argument) if (c >= 0x80) ascii = false;
        if (ascii) continue;
        if (ansi_process) throw Error("this executable runs with the " + std::string(crt_get_narrow_encoding()) + " narrow-string encoding (it lacks the SDK's UTF-8 manifest); non-ASCII arguments are not supported", 2);
        try { require_native_utf8(argument, "an argument"); } catch (const Error& error) { throw Error(error.what(), 2); }
    }
}
int print_reply(const Reply& reply, bool json) {
    if (json) std::printf("%s\n", Json{{"schema", "crt-cli.bootstrap-result"}, {"format", 1},
        {"command", reply.command}, {"ok", reply.code == 0}, {"data", reply.data}}.dump(-1, ' ', false, Json::error_handler_t::replace).c_str());
    else std::fprintf(reply.code ? stderr : stdout, "%s\n", reply.message.c_str());
    return reply.code;
}
}
