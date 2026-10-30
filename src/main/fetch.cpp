#include <crtcli/core.h>
#include <crtcli/http.h>
#include <chrono>
#include <map>
#include <set>
namespace crtcli {
namespace {
bool lower_hex(const std::string& value, size_t length) {
    if (value.size() != length) return false;
    for (char c : value) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
bool decimal(const std::string& value, unsigned long long maximum, unsigned long long& result) {
    if (value.empty() || value.size() > 19) return false;
    result = 0;
    for (char c : value) { if (c < '0' || c > '9') return false; result = result * 10 + static_cast<unsigned>(c - '0'); }
    return result >= 1 && result <= maximum;
}
void discard(const fs::path& path) { std::error_code ignored; fs::remove(path, ignored); }
}

Reply fetch(const std::vector<std::string>& args) {
    const char* usage = "usage: fetch --url <https-url> --ca-file <pem> --output <file> --sha256 <hex> --size <bytes> [--timeout <seconds>]";
    std::map<std::string, std::string> options;
    const std::set<std::string> known{"--url", "--ca-file", "--output", "--sha256", "--size", "--timeout"};
    for (size_t i = 1; i < args.size(); i += 2) {
        if (i + 1 >= args.size() || !known.count(args[i])) throw Error(usage, 2);
        if (!options.emplace(args[i], args[i + 1]).second) throw Error("duplicate fetch option", 2);
    }
    for (const auto& name : {"--url", "--ca-file", "--output", "--sha256", "--size"}) if (!options.count(name)) throw Error(usage, 2);
    const std::string& url = options.at("--url");
    if (url.compare(0, 8, "https://") != 0) throw Error("fetch accepts https:// URLs only", 2);
    auto authority = url.substr(8, url.find_first_of("/?#", 8) - 8);
    if (authority.empty() || authority.find('@') != std::string::npos) throw Error("the URL must name a host and carry no credentials", 2);
    if (!lower_hex(options.at("--sha256"), 64)) throw Error("--sha256 must be 64 lowercase hex digits", 2);
    unsigned long long size = 0, timeout = 60;
    if (!decimal(options.at("--size"), 1ull << 31, size)) throw Error("--size must be between 1 and 2147483648 bytes", 2);
    if (options.count("--timeout") && !decimal(options.at("--timeout"), 3600, timeout)) throw Error("--timeout must be between 1 and 3600 seconds", 2);

    fs::path ca = fs::absolute(options.at("--ca-file")).lexically_normal();
    if (!fs::is_regular_file(ca)) throw Error("--ca-file must name an existing regular file: " + ca.string(), 2);
    fs::path output = fs::absolute(options.at("--output")).lexically_normal();
    if (!fs::is_directory(output.parent_path())) throw Error("the output directory must exist", 2);
    // A fetch never replaces anything: a previous usable file stays untouched, a failed candidate is never published.
    if (fs::exists(fs::symlink_status(output))) throw Error("the output already exists; fetch does not overwrite: " + output.string(), 2);
    fs::path staging = output;
    staging += ".part-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

    HttpsDownload request;
    request.url = url; request.ca_file = ca.string(); request.output_path = staging.string();
    request.max_bytes = size; request.timeout_seconds = static_cast<unsigned>(timeout);
    auto outcome = https_download(request);
    Json trust{{"verify_peer", true}, {"verify_host", true}, {"ca_file", ca.string()}, {"minimum_tls", "1.2"}, {"backend", outcome.backend}};
    auto failure = [&](const std::string& kind, const std::string& stage, const std::string& detail, Json extra) {
        discard(staging);
        Json data{{"code", kind}, {"stage", stage}, {"message", detail}, {"url", url}, {"output", output.string()},
                  {"published", false}, {"bytes", outcome.bytes}, {"trust", trust}};
        for (auto item = extra.begin(); item != extra.end(); ++item) data[item.key()] = item.value();
        return Reply{1, "fetch", data, "fetch failed (" + kind + "): " + detail};
    };
    if (outcome.kind == "invalid_input") { discard(staging); throw Error(outcome.detail, 2); }
    if (!outcome.ok) return failure(outcome.kind, "transfer", outcome.detail,
        {{"curl_code", outcome.curl_code}, {"http_status", outcome.http_status}, {"tls_verify_result", outcome.tls_verify_result}});
    auto actual_size = fs::file_size(staging);
    if (actual_size != size) return failure("size_mismatch", "verify", "downloaded size differs from the declared size", {{"expected_bytes", size}, {"actual_bytes", actual_size}});
    auto digest = file_sha256(staging);
    if (digest != options.at("--sha256")) return failure("digest_mismatch", "verify", "SHA-256 differs from the declared digest", {{"expected_sha256", options.at("--sha256")}, {"actual_sha256", digest}});
    if (fs::exists(fs::symlink_status(output))) return failure("output_exists", "publish", "the output appeared while downloading", Json::object());
    fs::rename(staging, output);
    Json data{{"url", url}, {"output", output.string()}, {"published", true}, {"bytes", actual_size}, {"sha256", digest},
              {"http_status", outcome.http_status}, {"trust", trust},
              {"integrity", "pinned-digest"}, {"publisher_authentication", "not_verified"}};
    return {0, "fetch", data, "fetched " + url + " (" + std::to_string(actual_size) + " bytes, sha256 " + digest + ")"};
}
}
