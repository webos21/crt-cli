#include <crtcli/host_tools.h>
#include <crtcli/publisher.h>
#include <algorithm>
#include <map>
#include <set>
namespace crtcli {
namespace {
void resource_check(Reply& reply, const std::map<std::string, std::string>& opts) {
    require(opts.count("--resources") && opts.count("--resources-sha256"), "self resources require --resources and an explicit --resources-sha256 pin");
    auto root = fs::canonical(opts.at("--resources"));
    auto path = root / "manifest.json";
    require(!fs::is_symlink(path) && fs::is_regular_file(path) && fs::file_size(path) <= 16 * 1024 * 1024, "invalid bootstrap resources manifest");
    auto pin = opts.at("--resources-sha256");
    require(pin.size() == 64 && file_sha256(path) == pin, "bootstrap resources manifest SHA-256 mismatch");
    auto manifest = read_json(path);
    require(manifest.value("schema", "") == "crt-cli.bootstrap-resources" && manifest.value("format", 0) == 1 &&
            manifest.at("target") == native_target(), "unsupported bootstrap resources contract/target");
    auto expected = manifest.at("inventory");
    require(expected.is_array() && !expected.empty() && expected.size() <= 16384, "invalid bootstrap resource inventory");
    for (const auto& entry : expected)
        require(entry.is_array() && entry.size() == 3 && entry.at(1) == "file", "bootstrap resources require regular files");
    require(manifest.at("executable") == (native_target().at("os") == "windows" ? "crt-cli.exe" : "crt-cli"), "unsupported bootstrap executable name");
    const auto& trust = manifest.at("trust");
    require((trust.at("mode") == "explicit-at-fetch" && trust.at("ca_file").is_null()) ||
            (trust.at("mode") == "explicit-ca-file" && trust.at("ca_file") == "share/trust/ca.pem" &&
             fs::is_regular_file(inside(root, trust.at("ca_file")))), "unsupported bootstrap trust data");
    auto actual = inventory(root);
    actual.erase(std::remove_if(actual.begin(), actual.end(), [](const Json& e) { return e.at(0) == "manifest.json"; }), actual.end());
    require(actual == expected, "bootstrap resource/executable inventory mismatch");
    auto executable = inside(root, manifest.at("executable"));
    auto running = fs::path(reply.data.at("allocation_images").at("executable").get<std::string>());
    require(file_sha256(running) == file_sha256(executable), "running executable differs from bootstrap resource pin");
    require(opts.count("--publisher-key") == opts.count("--signature"), "self resources require both publisher key and signature");
    std::string authenticated = "not_verified";
    if (opts.count("--publisher-key")) {
        PublisherSignature signature(opts.at("--publisher-key"), opts.at("--signature"));
        require(signature.verify(file_sha256(path)), "bootstrap resource publisher signature invalid");
        authenticated = "signature_verified";
    }
    reply.data["checks"]["executable_integrity"] = "pinned-file-match";
    reply.data["checks"]["resources"] = "pass";
    reply.data["resources"] = {{"manifest_sha256", pin}, {"publisher_authentication", authenticated}, {"trust", manifest.at("trust")}};
}
}
Reply doctor(const std::vector<std::string>& args) {
    if (args == std::vector<std::string>{"doctor", "--bootstrap"}) return bootstrap_doctor();
    const std::set<std::string> allowed{"--scope", "--project", "--host-tools", "--host-tools-sha256", "--resources", "--resources-sha256", "--publisher-key", "--signature"};
    std::map<std::string, std::string> opts;
    for (size_t i = 1; i < args.size(); i += 2) {
        require(i + 1 < args.size() && allowed.count(args[i]), "unsupported/missing doctor option");
        require(opts.emplace(args[i], args[i + 1]).second, "duplicate doctor option");
    }
    auto scope = opts.count("--scope") ? opts.at("--scope") : "self";
    if (scope == "self") {
        for (const auto& key : {"--project", "--host-tools", "--host-tools-sha256"}) require(!opts.count(key), "self doctor does not require project or host tools");
        auto result = bootstrap_doctor();
        result.data["scope"] = "self";
        if (opts.size() > opts.count("--scope")) resource_check(result, opts);
        result.message = result.data.dump(2);
        return result;
    }
    for (const auto& key : {"--resources", "--resources-sha256", "--publisher-key", "--signature"}) require(!opts.count(key), "resource pins belong to self doctor");
    if (scope == "project") {
        require(!opts.count("--host-tools") && !opts.count("--host-tools-sha256"), "project doctor does not select host tools");
        std::vector<std::string> request{"env", "--profile", "app-build"};
        if (opts.count("--project")) { request.push_back("--project"); request.push_back(opts.at("--project")); }
        auto result = environment_report(request);
        result.command = "doctor";
        result.data["scope"] = "project";
        result.data["checks"] = {{"manifest_sdk_tool_binding", "pass"}, {"compile_link_run", "not_run"}};
        result.message = result.data.dump(2);
        return result;
    }
    require(scope == "environment", "unsupported doctor scope");
    require(opts.count("--host-tools") && opts.count("--host-tools-sha256"), "environment doctor requires an explicit host-tools selection");
    require(!opts.count("--project"), "environment doctor is SDK-independent; use project scope for SDK binding");
    auto tools = HostTools::load(opts.at("--host-tools"), opts.at("--host-tools-sha256"));
    Json data{{"scope", "environment"}, {"build_host", native_target()}, {"host_tools_identity", tools.identity()},
              {"checks", {{"inventory_aliases_native_tools", "pass"}, {"execution", "not_run"}}}};
    return {0, "doctor", data, data.dump(2)};
}
}
