#include <crtcli/http.h>
#include <curl/curl.h>
#include <mbedtls/ssl.h>
#include <mbedtls/version.h>
#include <cstdio>
#include <cstring>
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

HttpsOutcome https_download(const HttpsDownload& request) {
    HttpsOutcome outcome;
    auto invalid = [&](const char* detail) { outcome.kind = "invalid_input"; outcome.detail = detail; return outcome; };
    if (request.url.compare(0, 8, "https://") != 0) return invalid("only https:// URLs are accepted");
    if (request.ca_file.empty() || request.output_path.empty() || request.max_bytes == 0) return invalid("missing trust input, output or size cap");
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) { outcome.kind = "transfer"; outcome.detail = "libcurl initialization failed"; return outcome; }
    const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
    if (info && info->ssl_version) outcome.backend = info->ssl_version;
    std::FILE* file = std::fopen(request.output_path.c_str(), "wb");
    if (!file) { curl_global_cleanup(); outcome.kind = "transfer"; outcome.detail = "cannot create the staging file"; return outcome; }
    Sink sink{file, request.max_bytes, 0, false, false};
    CURL* easy = curl_easy_init();
    char error[CURL_ERROR_SIZE] = {};
    CURLcode code = CURLE_FAILED_INIT;
    if (easy) {
        curl_easy_setopt(easy, CURLOPT_URL, request.url.c_str());
        curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, "https");
        curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(easy, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(easy, CURLOPT_SSLVERSION, static_cast<long>(CURL_SSLVERSION_TLSv1_2));
        curl_easy_setopt(easy, CURLOPT_CAINFO, request.ca_file.c_str());
        curl_easy_setopt(easy, CURLOPT_PROXY, "");
        curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, static_cast<long>(request.timeout_seconds));
        curl_easy_setopt(easy, CURLOPT_TIMEOUT, static_cast<long>(request.timeout_seconds));
        curl_easy_setopt(easy, CURLOPT_MAXFILESIZE_LARGE, static_cast<curl_off_t>(request.max_bytes));
        curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_body);
        curl_easy_setopt(easy, CURLOPT_WRITEDATA, &sink);
        curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error);
        code = curl_easy_perform(easy);
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &outcome.http_status);
        curl_easy_getinfo(easy, CURLINFO_SSL_VERIFYRESULT, &outcome.tls_verify_result);
        curl_easy_cleanup(easy);
    }
    bool closed = std::fclose(file) == 0;
    curl_global_cleanup();
    outcome.curl_code = static_cast<long>(code);
    outcome.bytes = sink.written;
    outcome.detail = error[0] ? error : curl_easy_strerror(code);
    if (sink.exceeded) { outcome.kind = "too_large"; outcome.detail = "response exceeds the declared size"; return outcome; }
    if (code != CURLE_OK) { outcome.kind = classify(code); return outcome; }
    if (sink.failed || !closed) { outcome.kind = "transfer"; outcome.detail = "writing the staging file failed"; return outcome; }
    if (outcome.http_status != 200) { outcome.kind = "http_status"; outcome.detail = "unexpected HTTP status"; return outcome; }
    outcome.ok = true; outcome.kind.clear(); outcome.detail.clear();
    return outcome;
}
}
