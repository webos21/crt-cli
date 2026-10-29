#include <crtcli/core.h>
#include <crt/platform.h>
#include <mbedtls/sha256.h>
#include <cstdio>
#include <fstream>
#include <set>
#include <algorithm>
namespace crtcli {
void require(bool condition, const std::string& message) { if (!condition) throw Error(message); }
std::string read_text(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    require(bool(input), "cannot read: " + path.string());
    std::string result((std::istreambuf_iterator<char>(input)), {});
    require(!input.bad(), "read failed: " + path.string());
    return result;
}
void write_text(const fs::path& path, const std::string& value) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
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
    std::ifstream input(path, std::ios::binary);
    require(bool(input), "cannot hash: " + path.string());
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    int error = mbedtls_sha256_starts(&hash, 0);
    char buffer[65536];
    while (input && error == 0) {
        input.read(buffer, sizeof(buffer));
        error = mbedtls_sha256_update(&hash, reinterpret_cast<const unsigned char*>(buffer), input.gcount());
    }
    unsigned char digest[32];
    if (error == 0) error = mbedtls_sha256_finish(&hash, digest);
    mbedtls_sha256_free(&hash);
    require(!input.bad() && error == 0, "hash read failed: " + path.string());
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
int print_reply(const Reply& reply, bool json) {
    if (json) std::printf("%s\n", Json{{"schema", "crt-cli.bootstrap-result"}, {"format", 1},
        {"command", reply.command}, {"ok", reply.code == 0}, {"data", reply.data}}.dump().c_str());
    else std::fprintf(reply.code ? stderr : stdout, "%s\n", reply.message.c_str());
    return reply.code;
}
}
