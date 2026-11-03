#include "crtcli/sdk_store.h"
#include <cctype>
#include <map>
#include <set>

namespace crtcli {
namespace {

const char* usage =
    "usage: sdk install --archive <file> --archive-sha256 <hex> [--content-sha256 <hex>] [--store <dir>] [--lock-wait <seconds>]\n"
    "       sdk install --archive <file> --release-manifest <file> --release-manifest-sha256 <hex> [--store <dir>] [--lock-wait <seconds>]\n"
    "       sdk list [--store <dir>]\n"
    "       sdk verify <sdk-directory> [--store <dir>]\n"
    "       sdk recover [--store <dir>]";

bool lower_hex(const std::string& value, size_t length) {
    if (value.size() != length) return false;
    for (unsigned char c : value) if (!(std::isdigit(c) || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

// "--name value" pairs after the subcommand (and after an optional single positional argument).
std::map<std::string, std::string> options(const std::vector<std::string>& args, size_t first, const std::set<std::string>& known) {
    std::map<std::string, std::string> result;
    for (size_t i = first; i < args.size(); i += 2) {
        if (i + 1 >= args.size() || !known.count(args[i])) throw Error(usage, 2);
        if (!result.emplace(args[i], args[i + 1]).second) throw Error("duplicate sdk option: " + args[i], 2);
    }
    return result;
}

fs::path store_path(const std::map<std::string, std::string>& given) {
    if (!given.count("--store")) return sdk_store_root();
    fs::path path(given.at("--store"));
    // A relative store would silently move with the working directory, and an installed SDK must have one address.
    require(path.is_absolute(), "--store must be an absolute path");
    require_native_utf8(given.at("--store"), "--store");
    return path;
}

}  // namespace

Reply sdk_command(const std::vector<std::string>& args) {
    if (args.size() < 2) throw Error(usage, 2);
    const std::string& action = args[1];

    if (action == "install") {
        auto given = options(args, 2, {"--archive", "--archive-sha256", "--content-sha256", "--release-manifest", "--release-manifest-sha256", "--store", "--lock-wait"});
        if (!given.count("--archive")) throw Error(usage, 2);
        const bool by_manifest = given.count("--release-manifest") || given.count("--release-manifest-sha256");
        InstallRequest request;
        request.archive = given.at("--archive");
        if (by_manifest) {
            // The release manifest is the trusted input (its digest is pinned by the caller): it names the archive digest,
            // size and install metadata, so none of those may also be given separately.
            if (!given.count("--release-manifest") || !given.count("--release-manifest-sha256") || given.count("--archive-sha256") || given.count("--content-sha256")) throw Error(usage, 2);
            if (!lower_hex(given.at("--release-manifest-sha256"), 64)) throw Error("--release-manifest-sha256 must be 64 lowercase hex digits", 2);
            const fs::path manifest_path = given.at("--release-manifest");
            require(fs::is_regular_file(manifest_path), "release manifest not found: " + manifest_path.string());
            require(file_sha256(manifest_path) == given.at("--release-manifest-sha256"), "release manifest SHA-256 mismatch");
            auto manifest = read_json(manifest_path);
            const auto name = request.archive.filename().string();
            const Json* found = nullptr;
            const Json assets = manifest.value("assets", Json::array());
            for (const auto& asset : assets)
                if (asset.is_object() && asset.value("name", "") == name) { require(!found, "release manifest lists " + name + " twice"); found = &asset; }
            require(found != nullptr, "release manifest has no asset named " + name);
            require(found->contains("install"), "release manifest asset " + name + " has no install metadata (not an installable SDK archive)");
            require(found->contains("sha256") && found->at("sha256").is_string() && lower_hex(found->at("sha256").get<std::string>(), 64), "release manifest asset has no valid sha256");
            require(fs::is_regular_file(request.archive), "archive not found: " + request.archive.string());
            require(found->contains("size") && found->at("size").is_number_unsigned() && fs::file_size(request.archive) == found->at("size").get<uint64_t>(),
                    "archive size differs from the release manifest");
            request.archive_sha256 = found->at("sha256").get<std::string>();
            request.metadata = InstallMetadata::from_json(found->at("install"));
            request.expected_content_sha256 = request.metadata->content_sha256;
        } else {
            if (!given.count("--archive-sha256")) throw Error(usage, 2);
            if (!lower_hex(given.at("--archive-sha256"), 64)) throw Error("--archive-sha256 must be 64 lowercase hex digits", 2);
            if (given.count("--content-sha256") && !lower_hex(given.at("--content-sha256"), 64)) throw Error("--content-sha256 must be 64 lowercase hex digits", 2);
            request.archive_sha256 = given.at("--archive-sha256");
            if (given.count("--content-sha256")) request.expected_content_sha256 = given.at("--content-sha256");
        }
        SdkStore store(store_path(given));
        if (given.count("--lock-wait")) {
            unsigned long long seconds = 0;
            for (unsigned char c : given.at("--lock-wait")) if (!std::isdigit(c)) throw Error("--lock-wait must be a number of seconds", 2);
            seconds = std::stoull(given.at("--lock-wait"));
            if (seconds > 3600) throw Error("--lock-wait must be at most 3600 seconds", 2);
            store.lock_wait_seconds = static_cast<unsigned>(seconds);
        }
        auto result = store.install(request);
        return {0, "sdk", result.to_json(),
                (result.already_installed ? "already installed: " : "installed: ") + result.identity.directory()};
    }

    if (action == "list") {
        auto given = options(args, 2, {"--store"});
        SdkStore store(store_path(given));
        Json items = Json::array();
        for (const auto& entry : store.list())
            items.push_back({{"directory", entry.directory}, {"version", entry.version}, {"content12", entry.content12},
                             {"path", entry.path.string()}, {"target", entry.target}, {"stage", entry.stage}, {"profile", entry.profile}});
        return {0, "sdk", {{"store", store.root().string()}, {"sdks", items}}, std::to_string(items.size()) + " SDK(s) installed"};
    }

    if (action == "verify") {
        if (args.size() < 3) throw Error(usage, 2);
        auto given = options(args, 3, {"--store"});
        SdkStore store(store_path(given));
        auto files = store.verify(args[2]);
        return {0, "sdk", {{"directory", args[2]}, {"files", files}, {"verified", true}}, "verified: " + args[2]};
    }

    if (action == "recover") {
        auto given = options(args, 2, {"--store"});
        SdkStore store(store_path(given));
        auto removed = store.recover();
        Json items = Json::array();
        for (const auto& item : removed) items.push_back(item);
        return {0, "sdk", {{"removed", items}}, std::to_string(removed.size()) + " leftover(s) removed"};
    }

    throw Error(usage, 2);
}

}  // namespace crtcli
