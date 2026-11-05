// SDK store transactions on real tar.xz archives: install, identity, immutability, recovery, locking, no partial
// publication. The archives are built by the test (tar_fixture.h); nothing here touches the user's real store.
#include "tar_fixture.h"
#include <crtcli/sdk_store.h>
#include <cstdlib>
#include <functional>
#include <unistd.h>
using namespace crtcli;
using namespace tarfix;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }

std::vector<Entry> sdk_entries(const std::string& prefix = "", const std::string& stage = "02-cxx", Json target = native_target(), const std::string& marker = "a") {
    Json manifest{{"format", 1}, {"stage", stage}, {"target", target}};
    Json profile{{"format", 1}, {"profile", "dev"}, {"target", target}};
    std::vector<Entry> entries;
    if (!prefix.empty()) entries.push_back(make_dir(prefix));
    auto at = [&](const std::string& name) { return prefix.empty() ? name : prefix + "/" + name; };
    entries.push_back(make_file(at("VERSION"), "1.2.3\n"));
    entries.push_back(make_file(at("manifest.json"), manifest.dump()));
    entries.push_back(make_dir(at("share")));
    entries.push_back(make_dir(at("share/crt")));
    entries.push_back(make_dir(at("share/crt/contracts")));
    entries.push_back(make_file(at("share/crt/contracts/development.json"), profile.dump()));
    entries.push_back(make_dir(at("lib")));
    entries.push_back(make_file(at("lib/libx.a"), "archive " + marker));
    entries.push_back(make_file(at("bin-tool"), "#!/bin/sh\n", 0755));
    return entries;
}

std::string failure(SdkStore& store, const InstallRequest& request) {
    try { store.install(request); } catch (const Error& error) { return error.what(); }
    return {};
}

size_t directories_in(const fs::path& path) {
    size_t count = 0;
    if (fs::is_directory(path)) for (const auto& entry : fs::directory_iterator(path)) { (void)entry; ++count; }
    return count;
}
}  // namespace

int main() {
    const char* temporary = std::getenv("TMPDIR");
    fs::path base = fs::path(temporary ? temporary : "/tmp") / ("crt-cli-sdk-store-" + std::to_string(getpid()));
    fs::remove_all(base);
    fs::create_directories(base);
    Work work(base);
    try {
        auto bytes_of = [&](const std::vector<Entry>& entries) { return work.archive(xz_of(tar_of(entries))); };
        auto request_for = [&](const fs::path& archive) {
            InstallRequest request; request.archive = archive; request.archive_sha256 = file_sha256(archive); return request;
        };
        SdkStore store(base / "store");

        // First installation.
        auto flat = bytes_of(sdk_entries());
        auto first = store.install(request_for(flat));
        check(!first.already_installed, "first install reports a new SDK");
        check(first.identity.version == "1.2.3", "version from VERSION");
        check(first.identity.content_sha256.size() == 64, "full content identity");
        check(first.path == store.root() / first.identity.directory(), "published under its identity");
        check(fs::is_regular_file(first.path / "lib/libx.a"), "files present after publication");
        check(store.list().size() == 1 && store.list()[0].directory == first.identity.directory(), "list shows exactly the SDK");
        check(store.list()[0].profile == "dev" && store.list()[0].stage == "02-cxx", "list reads manifest and profile");
        check(store.verify(first.identity.directory()) >= 4, "verify counts files");
        check(directories_in(store.root() / ".staging") == 0, "staging is empty after success");
        check(directories_in(store.root() / ".locks") == 0, "locks are released after success");

        // Same content again, including inside a wrapper directory: nothing changes.
        auto again = store.install(request_for(flat));
        check(again.already_installed && again.path == first.path, "identical archive is idempotent");
        auto wrapped = store.install(request_for(bytes_of(sdk_entries("crt-1.2.3-dev"))));
        check(wrapped.already_installed && wrapped.identity.content_sha256 == first.identity.content_sha256, "wrapper directory does not change identity");
        check(store.list().size() == 1, "still one SDK");

        // The same SDK as a zip is the same identity (format is not part of it).
        auto zipped = work.archive(zip_of(sdk_entries()));
        auto from_zip = store.install(request_for(zipped));
        check(from_zip.already_installed && from_zip.identity.content_sha256 == first.identity.content_sha256, "zip and tar.xz of one SDK share an identity");
        check(from_zip.extracted.format == "zip", "zip format reported");

        // Expected identities.
        auto request = request_for(flat);
        request.expected_content_sha256 = first.identity.content_sha256;
        check(failure(store, request).empty(), "matching content identity accepted");
        request.expected_content_sha256 = std::string(64, '0');
        check(failure(store, request).find("content identity mismatch") != std::string::npos, "wrong content identity refused");
        request = request_for(flat);
        request.archive_sha256 = std::string(64, '1');
        check(failure(store, request).find("archive SHA-256 mismatch") != std::string::npos, "wrong archive digest refused");

        // Unacceptable SDKs never reach the store.
        auto other = request_for(bytes_of(sdk_entries("", "02-cxx", Json{{"os", "nowhere"}, {"arch", "none"}}, "b")));
        check(failure(store, other).find("is not this host") != std::string::npos, "foreign target refused");
        auto stage = request_for(bytes_of(sdk_entries("", "01-c", native_target(), "c")));
        check(failure(store, stage).find("stage") != std::string::npos, "wrong stage refused");
        auto entries = sdk_entries("", "02-cxx", native_target(), "d");
        entries.erase(entries.begin());
        check(failure(store, request_for(bytes_of(entries))).find("missing VERSION") != std::string::npos, "missing VERSION refused");
        auto hostile = sdk_entries("", "02-cxx", native_target(), "e");
        hostile.push_back(make_file("../escape", "x"));
        check(failure(store, request_for(bytes_of(hostile))).find("..") != std::string::npos, "unsafe archive refused");
        check(failure(store, InstallRequest{work.root / "absent.tar.xz", std::string(64, '0'), "", {}}).find("not found") != std::string::npos, "missing archive refused");
        check(store.list().size() == 1, "failed installs published nothing");
        check(directories_in(store.root() / ".staging") == 0, "failed installs leave no staging");
        check(directories_in(store.root() / ".locks") == 0, "failed installs leave no locks");

        // A second, different SDK coexists.
        auto changed = store.install(request_for(bytes_of(sdk_entries("", "02-cxx", native_target(), "z"))));
        check(!changed.already_installed && changed.path != first.path, "different content gets its own directory");
        check(store.list().size() == 2, "two SDKs listed");

        // Immutability: a damaged installation is reported and never silently replaced.
        write_text(first.path / "lib/libx.a", "tampered");
        std::string verify_error;
        try { store.verify(first.identity.directory()); } catch (const Error& error) { verify_error = error.what(); }
        check(verify_error.find("no longer matches") != std::string::npos, "verify detects a modified file");
        check(failure(store, request_for(flat)).find("immutable") != std::string::npos, "damaged install is not overwritten");

        // Install metadata: the producer's promise about the tree, enforced before and after extraction.
        {
            SdkStore meta_store(base / "meta-store");
            auto wrapped_sdk = bytes_of(sdk_entries("dev", "02-cxx", native_target(), "m"));
            auto probe_dir = work.fresh();
            auto probe_report = extract_archive(wrapped_sdk, probe_dir);
            auto probe_identity = read_identity(probe_dir / "dev");
            auto manifest_json = read_json(probe_dir / "dev/manifest.json");
            Json promise{{"format", 1}, {"archive_format", "tar.xz"}, {"root", "dev"}, {"version", "1.2.3"}, {"stage", "02-cxx"}, {"profile", "dev"},
                         {"target", native_target()}, {"content_sha256", probe_identity.content_sha256}, {"entries", probe_report.entries},
                         {"files", probe_report.files}, {"directories", probe_report.directories}, {"symlinks", probe_report.symlinks},
                         {"uncompressed_bytes", probe_report.bytes}, {"max_file_bytes", 400}, {"max_path_bytes", 64}, {"max_depth", 6}};
            auto with = [&](const std::function<void(Json&)>& change) {
                Json copy = promise; change(copy);
                InstallRequest r = request_for(wrapped_sdk);
                try { r.metadata = InstallMetadata::from_json(copy); } catch (const Error& error) { return std::string(error.what()); }
                r.expected_content_sha256 = r.metadata->content_sha256;
                return failure(meta_store, r);
            };
            auto expect = [&](const std::string& why, const std::function<void(Json&)>& change, const std::string& text) {
                auto message = with(change);
                check(message.find(text) != std::string::npos, why + " (got: " + message + ")");
                check(meta_store.list().empty(), why + ": nothing published");
            };
            expect("more files than promised", [](Json& j) { j["files"] = j["files"].get<uint64_t>() + 1; j["entries"] = j["entries"].get<uint64_t>() + 1; }, "differ from the release metadata");
            expect("fewer bytes than promised", [](Json& j) { j["uncompressed_bytes"] = j["uncompressed_bytes"].get<uint64_t>() - 1; }, "total size limit");
            expect("entry limit", [](Json& j) { j["entries"] = 3; j["files"] = 1; j["directories"] = 1; j["symlinks"] = 1; }, "more than 3 entries");
            expect("file limit", [](Json& j) { j["max_file_bytes"] = 4; }, "per-file limit");
            expect("path limit", [](Json& j) { j["max_path_bytes"] = 10; }, "path");
            expect("depth limit", [](Json& j) { j["max_depth"] = 1; }, "path");
            expect("wrong root", [](Json& j) { j["root"] = "other"; }, "promised root");
            expect("wrong format", [](Json& j) { j["archive_format"] = "zip"; }, "archive format");
            expect("wrong identity", [](Json& j) { j["content_sha256"] = std::string(64, 'a'); }, "content identity mismatch");
            expect("wrong version", [](Json& j) { j["version"] = "9.9.9"; }, "VERSION");
            expect("wrong profile", [](Json& j) { j["profile"] = "full"; }, "differ from the release metadata");
            expect("inconsistent counts", [](Json& j) { j["entries"] = 1; }, "inconsistent");
            expect("missing field", [](Json& j) { j.erase("max_depth"); }, "max_depth");
            expect("unsupported format", [](Json& j) { j["format"] = 2; }, "unsupported install metadata");
            check(with([](Json&) {}).empty(), "exact metadata installs");
            check(meta_store.list().size() == 1, "installed under metadata");
        }

        // Recovery and locking. Process ids: ours is alive; a number beyond any pid_max is not.
        const std::string dead = "999999999";
        auto fresh_archive = request_for(bytes_of(sdk_entries("", "02-cxx", native_target(), "y")));
        fs::create_directories(store.root() / ".staging" / "old");
        write_text(store.root() / ".staging" / "old" / "txn.json", Json{{"pid", 999999999L}}.dump());
        fs::create_directories(store.root() / ".staging" / "live");
        write_text(store.root() / ".staging" / "live" / "txn.json", Json{{"pid", static_cast<long>(getpid())}}.dump());
        auto removed = store.recover();
        check(removed.size() == 1 && removed[0] == ".staging/old", "recover removes only the dead transaction");
        check(fs::exists(store.root() / ".staging" / "live"), "a live transaction is kept");
        fs::remove_all(store.root() / ".staging" / "live");

        // The identity of the next archive, to place a lock where install will look for it.
        auto probe = work.fresh();
        extract_archive(fresh_archive.archive, probe);
        auto lock = store.root() / ".locks" / (read_identity(probe).content_sha256.substr(0, 12) + ".lock");
        fs::create_directories(lock);
        write_text(lock / "owner", std::to_string(getpid()) + "\n");
        check(failure(store, fresh_archive).find("in progress") != std::string::npos, "a live lock blocks installation");
        check(store.list().size() == 2, "a blocked install published nothing");
        fs::remove_all(lock);
        fs::create_directories(lock);
        write_text(lock / "owner", dead + "\n");
        auto stolen = store.install(fresh_archive);
        check(!stolen.already_installed && store.list().size() == 3, "a dead owner's lock is taken over");
        check(directories_in(store.root() / ".locks") == 0, "the lock is released");

        // A release's toolchain contract is kept beside the SDK it belongs to and is as immutable as that SDK.
        {
            SdkStore contract_store(base / "contract-store");
            auto identity_of = [&](const fs::path& archive) { auto probe = work.fresh(); extract_archive(archive, probe); return read_identity(probe).content_sha256; };
            auto write_contract = [&](const std::string& name, const std::string& identity, const std::function<void(Json&)>& change) {
                Json document{{"schema", "crt.experimental.native-toolchain"}, {"format", 1}, {"sdk_content_sha256", identity}};
                change(document);
                auto file = base / name;
                write_text(file, document.dump(2) + "\n");
                return file;
            };
            auto install_with = [&](const fs::path& archive, const fs::path& contract) {
                auto request = request_for(archive);
                if (!contract.empty()) { request.contract = contract; request.contract_sha256 = file_sha256(contract); }
                return request;
            };
            auto leftovers = [&] { size_t count = 0; if (fs::is_directory(contract_store.root() / ".contracts")) for (const auto& e : fs::directory_iterator(contract_store.root() / ".contracts")) { (void)e; ++count; } return count; };

            auto k_archive = bytes_of(sdk_entries("", "02-cxx", native_target(), "k"));
            const auto k_identity = identity_of(k_archive);
            auto good = write_contract("good-contract.json", k_identity, [](Json&) {});
            auto stored = contract_store.install(install_with(k_archive, good));
            check(!stored.already_installed && stored.contract_stored, "a contract is stored with a new SDK");
            check(fs::is_regular_file(contract_store.contract_path(stored.identity.directory())) &&
                  file_sha256(contract_store.contract_path(stored.identity.directory())) == file_sha256(good), "the stored contract is the verified file");
            check(contract_store.list().size() == 1 && contract_store.list()[0].has_contract, "list reports the stored contract");
            check(stored.to_json().at("contract_stored") == true, "the result reports it");

            auto again_k = contract_store.install(install_with(k_archive, good));
            check(again_k.already_installed && !again_k.contract_stored, "the same contract again changes nothing");
            auto other_k = write_contract("other-contract.json", k_identity, [](Json& d) { d["policy"] = "different"; });
            check(failure(contract_store, install_with(k_archive, other_k)).find("immutable") != std::string::npos, "a different contract for an installed SDK is refused");
            check(contract_store.install(install_with(k_archive, fs::path())).already_installed, "installing without a contract is still fine");
            check(file_sha256(contract_store.contract_path(stored.identity.directory())) == file_sha256(good), "the stored contract is unchanged");

            // Refusals publish nothing and leave nothing in the contract store.
            auto m_archive = bytes_of(sdk_entries("", "02-cxx", native_target(), "m"));
            const auto m_identity = identity_of(m_archive);
            check(failure(contract_store, install_with(m_archive, good)).find("belongs to SDK") != std::string::npos, "a contract of another SDK is refused");
            auto machine = write_contract("machine-contract.json", m_identity, [](Json& d) { d["external_os_sdk"] = "/Applications/Xcode.app/SDK"; });
            check(failure(contract_store, install_with(m_archive, machine)).find("machine path") != std::string::npos, "a host-specific contract is refused");
            auto wrong_schema = write_contract("schema-contract.json", m_identity, [](Json& d) { d["format"] = 2; });
            check(failure(contract_store, install_with(m_archive, wrong_schema)).find("unsupported toolchain contract") != std::string::npos, "an unknown contract format is refused");
            auto bad_digest = install_with(m_archive, write_contract("digest-contract.json", m_identity, [](Json&) {}));
            bad_digest.contract_sha256 = std::string(64, '0');
            check(failure(contract_store, bad_digest).find("SHA-256 mismatch") != std::string::npos, "a wrong contract digest is refused");
            check(contract_store.list().size() == 1 && leftovers() == 1, "refused contracts left no SDK and no file");

            // A contract may be added to an SDK installed without one.
            auto plain = contract_store.install(install_with(m_archive, fs::path()));
            check(!plain.already_installed && !plain.contract_stored && !contract_store.list()[1].has_contract, "an SDK without a contract installs");
            auto added = contract_store.install(install_with(m_archive, write_contract("m-contract.json", m_identity, [](Json&) {})));
            check(added.already_installed && added.contract_stored && contract_store.list()[1].has_contract, "a contract is added to an installed SDK");

            // Interrupted writes: only the dead owner's temporary file is a leftover.
            auto pending = [&](const std::string& owner) { return contract_store.root() / ".contracts" / ("x.json.tmp-" + owner); };
            write_text(pending("999999999"), "{}"); write_text(pending(std::to_string(getpid())), "{}");
            auto swept = contract_store.recover();
            check(swept.size() == 1 && swept[0] == ".contracts/x.json.tmp-999999999", "recover removes only a dead owner's contract file");
            check(fs::exists(pending(std::to_string(getpid()))), "a live owner's contract file is kept");
            fs::remove(pending(std::to_string(getpid())));
        }
    } catch (const Error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        fs::remove_all(base);
        return 1;
    }
    fs::remove_all(base);
    std::printf("sdk store test: %zu checks passed\n", checks);
    return 0;
}
