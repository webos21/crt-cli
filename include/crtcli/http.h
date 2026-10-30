#pragma once
#include <string>
namespace crtcli {
// Initializes libcurl and the Mbed TLS backend; kept free of C++ standard-library path types (see crt.json).
bool http_tls_probe();

struct HttpsDownload {
    std::string url;          // https only; no credentials in the authority
    std::string ca_file;      // explicit PEM trust input; no default store is consulted
    std::string output_path;  // staging file, created here and left for the caller to verify or remove
    unsigned long long max_bytes = 0;
    unsigned timeout_seconds = 60;
};
struct HttpsOutcome {
    bool ok = false;
    std::string kind;    // "", "tls", "interrupted", "timeout", "network", "http_status", "too_large", "transfer", "invalid_input"
    std::string detail;  // curl's description; never the request headers or credentials
    long http_status = 0;
    long curl_code = 0;
    long tls_verify_result = -1;
    unsigned long long bytes = 0;
    std::string backend;  // TLS library reported by libcurl
};
// One verified HTTPS GET into request.output_path. Peer and host verification are always on, redirects are not followed
// and the response is capped at max_bytes.
HttpsOutcome https_download(const HttpsDownload& request);
}
