#include "bootstrap_config.h"
#include "core.h"
#include <curl/curl.h>
#include <mbedtls/ssl.h>
#include <mbedtls/version.h>
#include <zlib.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <set>
#include <string>
#include <vector>

namespace {
std::string quote(const std::string& value) {
    std::string result = "\"";
    const char hex[] = "0123456789abcdef";
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') { result += '\\'; result += static_cast<char>(c); }
        else if (c < 32) {
            result += "\\u00"; result += hex[c >> 4]; result += hex[c & 15];
        } else result += static_cast<char>(c);
    }
    return result + '"';
}

std::string runtime_maps() {
#if defined(__linux__)
    FILE* stream = std::fopen("/proc/self/maps", "r");
    if (!stream) return "{\"status\":\"unavailable\",\"images\":[]}";
    std::set<std::string> images;
    char line[8192];
    while (std::fgets(line, sizeof(line), stream)) {
        const char* path = std::strchr(line, '/');
        if (path) {
            std::string name(path);
            while (!name.empty() && (name.back() == '\n' || name.back() == '\r')) name.pop_back();
            images.insert(name);
        }
    }
    bool complete = !std::ferror(stream);
    std::fclose(stream);
    std::string data = "{\"status\":" + quote(complete ? "collected" : "unavailable") + ",\"images\":[";
    bool first = true;
    for (const auto& image : images) { if (!first) data += ','; data += quote(image); first = false; }
    return data + "]}";
#else
    return "{\"status\":\"not_implemented\",\"images\":[]}";
#endif
}

} // namespace
namespace crtcli {
Reply bootstrap_doctor() {
    bool cxx = false;
    try { throw std::runtime_error("CRT bootstrap"); }
    catch (const std::exception& error) { cxx = std::strcmp(error.what(), "CRT bootstrap") == 0; }
    const unsigned char input[] = "crt-cli-bootstrap";
    unsigned char compressed[128] = {}, restored[128] = {};
    uLongf compressed_size = sizeof(compressed), restored_size = sizeof(restored);
    bool zlib = compress2(compressed, &compressed_size, input, sizeof(input), Z_DEFAULT_COMPRESSION) == Z_OK;
    zlib = zlib && uncompress(restored, &restored_size, compressed, compressed_size) == Z_OK;
    zlib = zlib && restored_size == sizeof(input) && std::memcmp(restored, input, sizeof(input)) == 0;
    bool curl = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
    const curl_version_info_data* http = curl ? curl_version_info(CURLVERSION_NOW) : nullptr;
    bool tls = http && (http->features & CURL_VERSION_SSL) && http->ssl_version
               && std::strstr(http->ssl_version, "mbedTLS") && mbedtls_version_get_number() != 0;
    mbedtls_ssl_context context;
    mbedtls_ssl_init(&context); mbedtls_ssl_free(&context);
    if (curl) curl_global_cleanup();
    bool ok = cxx && zlib && tls;
    Json data{{"scope", "bootstrap"}, {"target", {{"os", CRT_CLI_TARGET_OS}, {"arch", CRT_CLI_TARGET_ARCH}}},
              {"sdk_build_input", {{"version", CRT_CLI_SDK_VERSION}, {"source_commit", CRT_CLI_SDK_COMMIT}, {"content_sha256", CRT_CLI_SDK_CONTENT}}},
              {"checks", {{"cxx_runtime", cxx ? "pass" : "fail"}, {"zlib", zlib ? "pass" : "fail"}, {"http_tls_linkage", tls ? "pass" : "fail"}, {"executable_integrity", "not_verified"}, {"https_network", "not_run"}}},
              {"runtime_maps", Json::parse(runtime_maps())}};
    return {ok ? 0 : 1, "doctor", data, ok ? "bootstrap C++/zlib/HTTP/TLS pass; integrity not verified; HTTPS not run" : "bootstrap checks failed"};
}
}
