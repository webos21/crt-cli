#include <crtcli/http.h>
#include <curl/curl.h>
#include <mbedtls/ssl.h>
#include <mbedtls/version.h>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <set>
namespace crtcli {
bool http_tls_probe() {
    bool curl = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
    const curl_version_info_data* http = curl ? curl_version_info(CURLVERSION_NOW) : nullptr;
    bool tls = http && (http->features & CURL_VERSION_SSL) && http->ssl_version
               && std::strstr(http->ssl_version, "mbedTLS") && mbedtls_version_get_number() != 0;
    mbedtls_ssl_context context;
    mbedtls_ssl_init(&context); mbedtls_ssl_free(&context);
    if (curl) curl_global_cleanup();
    return tls;
}

namespace {
struct Sink { std::FILE* file; unsigned long long limit; unsigned long long written; bool exceeded; bool failed; };
size_t write_body(char* data, size_t size, size_t count, void* user) {
    auto* sink = static_cast<Sink*>(user);
    size_t bytes = size * count;
    if (bytes > sink->limit - sink->written) { sink->exceeded = true; return 0; }
    size_t done = std::fwrite(data, 1, bytes, sink->file);
    sink->written += done;
    if (done != bytes) sink->failed = true;
    return done;
}
std::string classify(CURLcode code) {
    switch (code) {
        case CURLE_PEER_FAILED_VERIFICATION: case CURLE_SSL_CACERT_BADFILE: case CURLE_SSL_CERTPROBLEM:
        case CURLE_SSL_CONNECT_ERROR: case CURLE_SSL_ISSUER_ERROR: case CURLE_SSL_CIPHER: case CURLE_SSL_ENGINE_NOTFOUND:
            return "tls";
        case CURLE_PARTIAL_FILE: case CURLE_RECV_ERROR: case CURLE_GOT_NOTHING: case CURLE_SEND_ERROR: return "interrupted";
        case CURLE_OPERATION_TIMEDOUT: return "timeout";
        case CURLE_COULDNT_CONNECT: case CURLE_COULDNT_RESOLVE_HOST: case CURLE_COULDNT_RESOLVE_PROXY: return "network";
        case CURLE_HTTP_RETURNED_ERROR: return "http_status";
        case CURLE_FILESIZE_EXCEEDED: return "too_large";
        default: return "transfer";
    }
}
}

const std::vector<std::string>& github_release_redirect_authorities() {
    static const std::vector<std::string> hosts{"release-assets.githubusercontent.com", "objects.githubusercontent.com",
                                                "github-releases.githubusercontent.com"};
    return hosts;
}

namespace {
struct Headers { std::string location; unsigned locations = 0; };
size_t read_header(char* data, size_t size, size_t count, void* user) {
    auto* headers = static_cast<Headers*>(user);
    std::string line(data, size * count);
    static const char name[] = "location:";
    if (line.size() > sizeof(name) - 1) {
        bool match = true;
        for (size_t i = 0; i < sizeof(name) - 1; ++i) if (std::tolower(static_cast<unsigned char>(line[i])) != name[i]) { match = false; break; }
        if (match) {
            auto value = line.substr(sizeof(name) - 1);
            auto first = value.find_first_not_of(" \t"), last = value.find_last_not_of(" \t\r\n");
            headers->location = first == std::string::npos ? std::string() : value.substr(first, last - first + 1);
            ++headers->locations;
        }
    }
    return size * count;
}
struct Hop { bool ok = false; std::string url, authority, reason; };
// Resolves a Location against the current URL and applies the redirect policy; the result never leaves this function
// in a message (the URL of a signed storage redirect carries a credential-like query).
Hop resolve_redirect(const std::string& current, const std::string& location, const HttpsDownload& request) {
    Hop hop;
    if (location.empty() || location.size() > 8192) { hop.reason = "missing or oversized Location"; return hop; }
    CURLU* url = curl_url();
    if (!url) { hop.reason = "cannot parse Location"; return hop; }
    struct Free { CURLU* url; ~Free() { curl_url_cleanup(url); } } release{url};
    auto field = [&](CURLUPart part, unsigned flags, std::string& out) {
        char* text = nullptr;
        CURLUcode code = curl_url_get(url, part, &text, flags);
        if (code == CURLUE_OK && text) out = text;
        if (text) curl_free(text);
        return code == CURLUE_OK;
    };
    if (curl_url_set(url, CURLUPART_URL, current.c_str(), 0) != CURLUE_OK ||
        curl_url_set(url, CURLUPART_URL, location.c_str(), 0) != CURLUE_OK) { hop.reason = "malformed Location"; return hop; }
    std::string scheme, host, port, user, password, fragment, text;
    field(CURLUPART_SCHEME, 0, scheme); field(CURLUPART_HOST, 0, host);
    if (scheme != "https") { hop.reason = "redirect is not HTTPS"; return hop; }
    if (field(CURLUPART_USER, 0, user) || field(CURLUPART_PASSWORD, 0, password)) { hop.reason = "redirect carries credentials"; return hop; }
    if (field(CURLUPART_FRAGMENT, 0, fragment)) { hop.reason = "redirect carries a fragment"; return hop; }
    for (char& c : host) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (host.empty() || host.find_first_of("[]%") != std::string::npos) { hop.reason = "unsupported redirect host"; return hop; }
    hop.authority = host;
    if (field(CURLUPART_PORT, CURLU_NO_DEFAULT_PORT, port)) hop.authority += ":" + port;
    auto authority_of = [&](const std::string& address) {
        CURLU* other = curl_url();
        std::string out;
        if (other && curl_url_set(other, CURLUPART_URL, address.c_str(), 0) == CURLUE_OK) {
            char* h = nullptr; char* p = nullptr;
            if (curl_url_get(other, CURLUPART_HOST, &h, 0) == CURLUE_OK && h) {
                out = h;
                for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                if (curl_url_get(other, CURLUPART_PORT, &p, CURLU_NO_DEFAULT_PORT) == CURLUE_OK && p) out += std::string(":") + p;
            }
            if (h) curl_free(h);
            if (p) curl_free(p);
        }
        if (other) curl_url_cleanup(other);
        return out;
    };
    bool allowed = hop.authority == authority_of(current);
    for (const auto& item : request.redirect_authorities) if (hop.authority == item) allowed = true;
    if (!allowed) { hop.reason = "redirect target " + hop.authority + " is not an allowed authority"; return hop; }
    if (!field(CURLUPART_URL, 0, text)) { hop.reason = "malformed Location"; return hop; }
    hop.url = text; hop.ok = true;
    return hop;
}
}

HttpsOutcome https_download(const HttpsDownload& request) {
    HttpsOutcome outcome;
    auto invalid = [&](const char* detail) { outcome.kind = "invalid_input"; outcome.detail = detail; return outcome; };
    if (request.url.compare(0, 8, "https://") != 0) return invalid("only https:// URLs are accepted");
    if (request.ca_file.empty() || request.output_path.empty() || request.max_bytes == 0) return invalid("missing trust input, output or size cap");
    if (request.max_redirects > 5) return invalid("at most 5 redirects are supported");
    if (request.max_redirects != 0 && request.redirect_authorities.empty()) return invalid("redirects require explicit allowed authorities");
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) { outcome.kind = "transfer"; outcome.detail = "libcurl initialization failed"; return outcome; }
    struct Global { ~Global() { curl_global_cleanup(); } } global;
    const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
    if (info && info->ssl_version) outcome.backend = info->ssl_version;
    const auto started = std::chrono::steady_clock::now();
    std::string current = request.url;
    std::set<std::string> visited{current};
    for (unsigned hop = 0;; ++hop) {
        auto elapsed = static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - started).count());
        if (elapsed >= request.timeout_seconds) { outcome.kind = "timeout"; outcome.detail = "the transfer exceeded its overall timeout"; return outcome; }
        const long budget = static_cast<long>(request.timeout_seconds - elapsed);
        std::FILE* file = std::fopen(request.output_path.c_str(), "wb");  // every hop starts from an empty staging file
        if (!file) { outcome.kind = "transfer"; outcome.detail = "cannot create the staging file"; return outcome; }
        Sink sink{file, request.max_bytes, 0, false, false};
        Headers headers;
        CURL* easy = curl_easy_init();
        char error[CURL_ERROR_SIZE] = {};
        CURLcode code = CURLE_FAILED_INIT;
        long status = 0;
        if (easy) {
            curl_easy_setopt(easy, CURLOPT_URL, current.c_str());
            curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, "https");
            curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 0L);
            curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1L);
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(easy, CURLOPT_SSLVERSION, static_cast<long>(CURL_SSLVERSION_TLSv1_2));
            curl_easy_setopt(easy, CURLOPT_CAINFO, request.ca_file.c_str());
            curl_easy_setopt(easy, CURLOPT_PROXY, "");
            curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
            curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, budget);
            curl_easy_setopt(easy, CURLOPT_TIMEOUT, budget);
            curl_easy_setopt(easy, CURLOPT_MAXFILESIZE_LARGE, static_cast<curl_off_t>(request.max_bytes));
            curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_body);
            curl_easy_setopt(easy, CURLOPT_WRITEDATA, &sink);
            curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, read_header);
            curl_easy_setopt(easy, CURLOPT_HEADERDATA, &headers);
            curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error);
            code = curl_easy_perform(easy);
            curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
            curl_easy_getinfo(easy, CURLINFO_SSL_VERIFYRESULT, &outcome.tls_verify_result);
            curl_easy_cleanup(easy);
        }
        bool closed = std::fclose(file) == 0;
        outcome.http_status = status;
        outcome.curl_code = static_cast<long>(code);
        outcome.bytes = sink.written;
        outcome.detail = error[0] ? error : curl_easy_strerror(code);
        if (sink.exceeded) { outcome.kind = "too_large"; outcome.detail = "response exceeds the declared size"; return outcome; }
        if (code != CURLE_OK) { outcome.kind = classify(code); return outcome; }
        if (sink.failed || !closed) { outcome.kind = "transfer"; outcome.detail = "writing the staging file failed"; return outcome; }
        const bool redirect = status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
        if (redirect && request.max_redirects != 0) {
            if (hop >= request.max_redirects) { outcome.kind = "redirect"; outcome.detail = "too many redirects"; return outcome; }
            if (headers.locations != 1) { outcome.kind = "redirect"; outcome.detail = "a redirect needs exactly one Location"; return outcome; }
            auto next = resolve_redirect(current, headers.location, request);
            if (!next.ok) { outcome.kind = "redirect"; outcome.detail = next.reason; return outcome; }
            if (!visited.insert(next.url).second) { outcome.kind = "redirect"; outcome.detail = "redirect loop"; return outcome; }
            outcome.redirects = hop + 1;
            outcome.redirect_chain.push_back(next.authority);
            current = next.url;
            continue;
        }
        if (status != 200) { outcome.kind = "http_status"; outcome.detail = "unexpected HTTP status"; return outcome; }
        outcome.ok = true; outcome.kind.clear(); outcome.detail.clear();
        return outcome;
    }
}
}
