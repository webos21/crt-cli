#pragma once
#include <string>
#include <vector>
namespace crtcli {
// Initializes libcurl and the Mbed TLS backend; kept free of C++ standard-library path types (see crt.json).
bool http_tls_probe();

struct HttpsDownload {
    std::string url;          // https only; no credentials in the authority
    std::string ca_file;      // explicit PEM trust input; no default store is consulted
    std::string output_path;  // staging file, created here and left for the caller to verify or remove
    unsigned long long max_bytes = 0;
    unsigned timeout_seconds = 60;
    // Redirects are refused unless the caller names the only authorities (host or host:port, lower case) a response may
    // send the transfer to. A request that names an authority may also be redirected within it. Every hop is re-checked
    // here (HTTPS, no credentials, no downgrade, allow-listed authority, at most max_redirects hops); curl never follows.
    unsigned max_redirects = 0;
    std::vector<std::string> redirect_authorities;
};
struct HttpsOutcome {
    bool ok = false;
    std::string kind;    // "", "tls", "interrupted", "timeout", "network", "http_status", "too_large", "transfer", "invalid_input", "redirect"
    std::string detail;  // curl's description; never the request headers or credentials
    long http_status = 0;
    long curl_code = 0;
    long tls_verify_result = -1;
    unsigned long long bytes = 0;
    std::string backend;  // TLS library reported by libcurl
    unsigned redirects = 0;                  // hops followed
    std::vector<std::string> redirect_chain;  // authorities only: never a path, query or token of a (signed) redirect URL
};
// The only built-in redirect authority set: GitHub's release-asset delivery hosts (a github.com release download answers
// with a redirect to one of these). It is a named, reviewed list, not a pattern; widen it only by a reviewed change.
const std::vector<std::string>& github_release_redirect_authorities();
// One verified HTTPS GET into request.output_path. Peer and host verification are always on at every hop, no credentials
// or cookies are ever sent, redirects follow only the explicit policy above and the response is capped at max_bytes.
HttpsOutcome https_download(const HttpsDownload& request);
}
