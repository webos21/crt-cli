#include <crtcli/toolchain.h>
#include <crtcli/sdk_store.h>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <set>
namespace crtcli {
namespace {
std::string selected(const std::map<std::string, std::string>& options, const std::string& name) {
    auto found = options.find(name);
    require(found != options.end(), "missing " + name + "; supply an explicit local SDK/toolchain (automatic SDK acquisition is pending)");
    return found->second;
}
void replace(std::string& value, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos) { value.replace(pos, from.size(), to); pos += to.size(); }
}

// A directory published by `sdk install`: `<version>-<content12>`. Anything else in the store is a local handoff copy.
bool published_name(const StoreEntry& entry) {
    const auto& name = entry.directory;
    if (entry.version.empty() || name.size() != entry.version.size() + 13 || name.compare(0, entry.version.size() + 1, entry.version + "-") != 0) return false;
    for (unsigned char c : entry.content12) if (!(std::isdigit(c) || (c >= 'a' && c <= 'f'))) return false;
    return entry.content12.size() == 12;
}
// Where this host keeps the OS SDK that a portable toolchain contract (@OS_SDK@) refers to.
fs::path discover_os_sdk(const std::string& kind) {
    if (kind == "macos-xcode-sdk") {
        auto query = execute({"/usr/bin/xcrun", "--sdk", "macosx", "--show-sdk-path"}, 10, false, true);
        require(query.exit_code == 0, "cannot locate the macOS SDK (xcrun --show-sdk-path failed); pass --os-sdk <directory>");
        while (!query.output.empty() && (query.output.back() == '\n' || query.output.back() == '\r')) query.output.pop_back();
        return fs::canonical(query.output);
    }
    if (kind == "windows-um-import-libraries") {
        const char* given = std::getenv("CRT_WINDOWS_SDK_LIBPATH");
        require(given && *given && fs::is_regular_file(fs::path(given) / "kernel32.lib"),
                "set CRT_WINDOWS_SDK_LIBPATH to the Windows SDK um/<arch> import-library directory, or pass --os-sdk <directory>");
        return fs::canonical(given);
    }
    throw Error("unsupported OS SDK kind in the toolchain contract: " + kind + "; pass --os-sdk <directory>");
}
std::string describe(const std::vector<StoreEntry>& entries) {
    std::string text;
    for (const auto& entry : entries) text += "\n  " + entry.directory;
    return text.empty() ? " (none)" : text;
}
fs::path store_sdk(const std::string& reference) {
    require(!reference.empty(), "--sdk must not be empty");
    const bool prefix = reference.size() >= 8 && reference.size() <= 64 && reference.find_first_not_of("0123456789abcdef") == std::string::npos;
    std::vector<StoreEntry> published, matches;
    for (const auto& entry : SdkStore(sdk_store_root()).list()) if (published_name(entry)) published.push_back(entry);
    for (const auto& entry : published) {
        if (entry.directory == reference) { matches.push_back(entry); continue; }
        if (!prefix || entry.content12.compare(0, std::min<size_t>(12, reference.size()), reference, 0, 12) != 0) continue;
        // Directory names carry only 12 digits. Longer references must also
        // match the installation-time full identity; never truncate a pin.
        if (reference.size() > 12) {
            auto receipt = SdkStore(sdk_store_root()).verified_receipt(entry.directory);
            const std::string content = receipt.at("sdk").at("content_sha256");
            if (content.compare(0, reference.size(), reference) != 0) continue;
        }
        matches.push_back(entry);
    }
    require(!matches.empty(), "no installed SDK matches '" + reference + "'; installed:" + describe(published));
    require(matches.size() == 1, "'" + reference + "' matches several installed SDKs:" + describe(matches));
    return matches.front().path;
}

bool under(const fs::path& path, const fs::path& directory) {
    auto text = path.lexically_normal().generic_string(), prefix = directory.lexically_normal().generic_string();
    if (!prefix.empty() && prefix.back() != '/') prefix += '/';
    return text.compare(0, prefix.size(), prefix) == 0;
}
// Makefile-style dependency file as written by -MD: "target: dep dep \\\n dep", with "\\ ", "\\#" and "$$" escapes.
std::vector<std::string> parse_depfile(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;
    bool open = false;
    auto finish = [&] { if (open) tokens.push_back(current); current.clear(); open = false; };
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\\' && i + 1 < text.size() && (text[i + 1] == '\n' || (text[i + 1] == '\r' && i + 2 < text.size() && text[i + 2] == '\n'))) {
            finish();
            i += text[i + 1] == '\r' ? 2 : 1;
        } else if (c == '\\' && i + 1 < text.size() && (text[i + 1] == ' ' || text[i + 1] == '#')) { current += text[++i]; open = true; }
        else if (c == '$' && i + 1 < text.size() && text[i + 1] == '$') { current += '$'; ++i; open = true; }
        else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') finish();
        else { current += c; open = true; }
    }
    finish();
    if (!tokens.empty()) tokens.erase(tokens.begin()); // the "target:" token
    return tokens;
}
// A running executable cannot be deleted or overwritten on Windows (the CLI rebuilding itself) but it can be renamed:
// move it aside under a unique name and remove the leftovers of earlier builds once nothing uses them.
void retire_output(const fs::path& output) {
    std::error_code ignored;
    const std::string prefix = output.filename().string() + ".old-";
    for (const auto& entry : fs::directory_iterator(output.parent_path(), ignored))
        if (entry.path().filename().string().compare(0, prefix.size(), prefix) == 0) fs::remove(entry.path(), ignored);
    std::error_code error;
    fs::remove(output, error);
    if (!error) return;
    auto aside = fs::path(output.string() + ".old-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::rename(output, aside, error);
    require(!error, "cannot replace build output (in use?): " + output.string());
}
bool unchanged(const fs::path& key_file, const fs::path& output, const std::string& command) {
    if (!fs::is_regular_file(output) || !fs::is_regular_file(key_file)) return false;
    try {
        auto key = read_json(key_file);
        if (key.at("command") != command) return false;
        for (const auto& item : key.at("inputs").items()) {
            if (!fs::is_regular_file(item.key()) || file_sha256(item.key()) != item.value()) return false;
        }
        return true;
    } catch (const std::exception&) { return false; }
}
Json inspect_sdk(const fs::path& sdk, const Json& contract, bool allow_dirty) {
    require(contract.at("schema") == "crt.experimental.native-toolchain" && contract.at("format") == 1, "unsupported native toolchain contract");
    require(contract.at("target") == native_target() && contract.at("policy") == "native-targets-exceptions-rtti", "unsupported target/policy");
    auto actual = inventory(sdk);
    require(actual == contract.at("sdk_inventory") && sha256(actual.dump()) == contract.at("sdk_content_sha256"), "SDK content mismatch");
    auto manifest = read_json(sdk / "manifest.json");
    auto profile = read_json(sdk / "share/crt/contracts/development.json");
    require(manifest.at("format") == 1 && manifest.at("stage") == "02-cxx", "unsupported SDK manifest/stage");
    require(profile.at("format") == 1 && profile.at("profile") == "dev", "unsupported SDK profile");
    for (const auto& capability : {"crt-platform-queries-1", "json-sha2-1"}) {
        const auto& list = profile.at("capabilities");
        require(std::find(list.begin(), list.end(), Json(capability)) != list.end(), "missing SDK capability: " + std::string(capability));
    }
    require(manifest.at("target") == contract.at("target") && profile.at("target") == contract.at("target"), "SDK target mismatch");
    auto commit = profile.at("producer_source_commit");
    require(commit == contract.at("producer_source_commit") && commit == manifest.at("source_commit") && commit == profile.at("base_source_commit") && commit == profile.at("compile_provenance").at("source_commit"), "SDK provenance mismatch");
    for (const auto& key : {"producer_working_tree_dirty", "base_source_commit", "compile_provenance"})
        require(profile.at(key) == contract.at("provenance").at(key), "SDK/provider provenance mismatch");
    require(allow_dirty || (!profile.at("producer_working_tree_dirty").get<bool>() && !profile.at("compile_provenance").at("working_tree_dirty").get<bool>()), "dirty SDK requires explicit --allow-dirty-sdk true for an unpromoted local experiment");
    for (const auto& item : profile.at("runtime_sha256").items())
        require(file_sha256(inside(sdk, item.key())) == item.value(), "SDK runtime digest mismatch");
    require(profile.at("tls_trust").at("mode") == "explicit-ca-file", "unsupported TLS trust policy");
    return {{"profile", "dev"}, {"target", contract.at("target")}, {"version", profile.at("version")}, {"producer_source_commit", commit}, {"sdk_content_sha256", contract.at("sdk_content_sha256")}, {"provenance", contract.at("provenance")}, {"allow_dirty_sdk", allow_dirty}};
}
}
std::map<std::string, std::string> Toolchain::read_sdk_config(const fs::path& file) {
    require(fs::is_regular_file(file), "SDK config is not a regular file: " + file.string());
    const std::map<std::string, std::string> roles{{"sdk_root", "--sdk-root"}, {"sdk_id", "--sdk-id"}, {"sdk", "--sdk"}, {"sdk_source", "--sdk-source"}, {"os_sdk", "--os-sdk"}, {"sdk_contract", "--sdk-contract"}, {"contract_sha256", "--contract-sha256"},
        {"cc", "--cc"}, {"cxx", "--cxx"}, {"linker", "--linker"}, {"ar", "--ar"}};
    const std::set<std::string> absolute_paths{"sdk_root", "sdk_source", "os_sdk", "sdk_contract", "cc", "cxx", "linker", "ar"};
    auto config = read_json(file);
    require(config.is_object() && config.contains("format") && config.at("format") == 0, "unsupported SDK config format (expected 0)");
    std::map<std::string, std::string> options;
    for (const auto& item : config.items()) {
        const auto& key = item.key();
        if (key == "format") continue;
        if (key == "seed" || key == "origin") { // informational: not a configure input
            require(item.value().is_string(), "SDK config " + key + " must be a string");
            continue;
        }
        if (key == "allow_dirty_sdk") {
            require(item.value().is_boolean(), "SDK config allow_dirty_sdk must be a boolean");
            if (item.value().get<bool>()) options["--allow-dirty-sdk"] = "true";
            continue;
        }
        auto role = roles.find(key);
        require(role != roles.end(), "unknown SDK config key: " + key);
        require(item.value().is_string() && !item.value().get<std::string>().empty(), "SDK config " + key + " must be a non-empty string");
        auto value = item.value().get<std::string>();
        require(!absolute_paths.count(key) || fs::path(value).is_absolute(), "SDK config " + key + " must be an absolute path");
        options[role->second] = value;
    }
    return options;
}
Toolchain Toolchain::configure(const std::map<std::string, std::string>& options) {
    Toolchain result;
    require(options.count("--sdk-root") + options.count("--sdk-id") + options.count("--sdk") <= 1, "choose one of --sdk, --sdk-id or --sdk-root");
    const bool from_store = options.count("--sdk") != 0;
    result.sdk = from_store ? fs::canonical(store_sdk(options.at("--sdk")))
        : options.count("--sdk-id") ? inside(sdk_store_root(), options.at("--sdk-id")) : fs::canonical(selected(options, "--sdk-root"));
    Json installation;
    if (from_store) installation = SdkStore(sdk_store_root()).verified_receipt(result.sdk.filename().string());
    // A store SDK installed from a release carries its toolchain contract beside it; an explicit contract still overrides.
    const bool stored_contract = from_store && !options.count("--sdk-contract");
    std::string hash;
    if (stored_contract) {
        result.contract_path = SdkStore(sdk_store_root()).contract_path(result.sdk.filename().string());
        require(fs::is_regular_file(result.contract_path) && !fs::is_symlink(result.contract_path),
                "the installed SDK has no stored toolchain contract; install it from a release that ships one or pass --sdk-contract");
        result.contract_path = fs::canonical(result.contract_path);
        require(!installation.at("contract").is_null(), "the installation receipt names no stored toolchain contract");
        hash = installation.at("contract").at("sha256");
        require(!options.count("--contract-sha256") || options.at("--contract-sha256") == hash, "toolchain contract digest mismatch");
    } else {
        result.contract_path = fs::canonical(selected(options, "--sdk-contract"));
        hash = selected(options, "--contract-sha256");
        require(hash.size() == 64 && file_sha256(result.contract_path) == hash, "toolchain contract digest mismatch");
        if (from_store && !installation.at("contract").is_null())
            require(hash == installation.at("contract").at("sha256"), "explicit contract differs from the immutable installation receipt");
    }
    result.contract = read_json(result.contract_path);
    bool allow_dirty = options.count("--allow-dirty-sdk") && options.at("--allow-dirty-sdk") == "true";
    require(!options.count("--allow-dirty-sdk") || allow_dirty, "--allow-dirty-sdk accepts only true");
    result.selection = inspect_sdk(result.sdk, result.contract, allow_dirty);
    if (from_store) {
        // The directory name is derived from the content identity; a renamed or substituted directory is not that SDK.
        const std::string expected = result.selection.at("version").get<std::string>() + "-" + result.selection.at("sdk_content_sha256").get<std::string>().substr(0, 12);
        require(result.sdk.filename().string() == expected, "installed SDK directory name does not match its content identity: " + result.sdk.filename().string());
        require(installation.at("sdk").at("content_sha256") == result.selection.at("sdk_content_sha256") &&
                installation.at("sdk").at("version") == result.selection.at("version"), "SDK installation receipt identity mismatch");
        result.selection["source"] = "store";
        result.selection["sdk_directory"] = expected;
    } else result.selection["source"] = "handoff";
    result.selection["contract_sha256"] = hash;
    result.selection["contract_source"] = stored_contract ? "store" : "explicit";
    if (result.contract.contains("os_sdk")) {
        // The contract is host-independent: the OS SDK it refers to is this machine's, found again here and never locked.
        const std::string kind = result.contract.at("os_sdk").at("kind");
        fs::path location = options.count("--os-sdk") ? fs::path(options.at("--os-sdk")) : discover_os_sdk(kind);
        require(location.is_absolute() && fs::is_directory(location), "the OS SDK must be an absolute directory: " + location.string());
        result.os_sdk = location.generic_string();
        result.selection["os_sdk"] = Json{{"kind", kind}};
    } else require(!options.count("--os-sdk"), "--os-sdk given, but the toolchain contract needs no OS SDK");
    result.cc = fs::canonical(selected(options, "--cc"));
    result.cxx = fs::canonical(selected(options, "--cxx"));
    // Linkers select their driver from argv[0]; preserve the selected name.
    result.linker = fs::absolute(selected(options, "--linker")).lexically_normal();
    result.archiver = fs::absolute(selected(options, "--ar")).lexically_normal();
    for (const auto& tool : {result.cc, result.cxx, result.linker, result.archiver}) native_executable(tool);
    auto query = execute({result.cxx.string(), "-print-resource-dir"}, 10, false, true);
    require(query.exit_code == 0, "Clang resource directory query failed");
    while (!query.output.empty() && (query.output.back() == '\n' || query.output.back() == '\r')) query.output.pop_back();
    result.resource = fs::canonical(query.output);
    result.selection["tools"] = Json{{"cc", file_sha256(result.cc)}, {"cxx", file_sha256(result.cxx)}, {"linker", file_sha256(result.linker)}, {"ar", file_sha256(result.archiver)}, {"resource_content_sha256", sha256(inventory(result.resource).dump())}};
    result.local = {{"sdk_root", result.sdk.string()}, {"sdk_contract", result.contract_path.string()}, {"cc", result.cc.string()}, {"cxx", result.cxx.string()}, {"linker", result.linker.string()}, {"ar", result.archiver.string()}, {"resource_dir", result.resource.string()}};
    // A store SDK is found again through its lock identity; only handoff selections keep a machine path. A stored contract
    // is found beside the SDK, and the OS SDK location belongs to this machine alone.
    if (from_store) result.local.erase("sdk_root");
    if (stored_contract) result.local.erase("sdk_contract");
    if (!result.os_sdk.empty()) result.local["os_sdk"] = result.os_sdk;
    return result;
}
std::string Toolchain::select_store_sdk(const std::string& version) {
    std::vector<StoreEntry> published, candidates;
    const Json target = native_target();
    for (const auto& entry : SdkStore(sdk_store_root()).list()) if (published_name(entry)) published.push_back(entry);
    for (const auto& entry : published)
        if (entry.profile == "dev" && entry.stage == "02-cxx" && entry.target == target && (version == "*" || entry.version == version)) candidates.push_back(entry);
    require(!candidates.empty(), "no installed SDK for this host (profile dev, version " + version + "); install one with `sdk install` or select with --sdk; installed:" + describe(published));
    require(candidates.size() == 1, "several installed SDKs fit (version " + version + "); choose one with --sdk:" + describe(candidates));
    return candidates.front().directory;
}
Toolchain Toolchain::load(const fs::path& root) {
    require(fs::exists(root / ".crt/local.json"), "project has no local SDK selection; run configure with explicit local inputs (SDK acquisition is pending)");
    auto local = read_json(root / ".crt/local.json");
    auto lock = read_json(root / "crt.lock");
    require(lock.at("lock_format") != 0, "project lock format 0 is no longer read and is never migrated implicitly; run configure again");
    require(lock.at("lock_format") == 1 && lock.at("state") == "local-experiment", "unsupported/unresolved project lock");
    // Verify selected tool bytes before even a resource-directory query.
    for (const auto& role : {"cc", "cxx", "linker", "ar"})
        require(file_sha256(fs::path(local.at(role).get<std::string>())) == lock.at("selection").at("tools").at(role), "locked external tool digest mismatch");
    const auto& locked = lock.at("selection");
    const bool from_store = locked.value("source", "") == "store";
    const bool stored_contract = locked.value("contract_source", "explicit") == "store";
    std::map<std::string, std::string> options{{"--contract-sha256", lock.at("selection").at("contract_sha256")}, {"--cc", local.at("cc")}, {"--cxx", local.at("cxx")}, {"--linker", local.at("linker")}};
    options["--ar"] = local.at("ar");
    if (!stored_contract) options["--sdk-contract"] = local.at("sdk_contract");
    if (local.contains("os_sdk")) options["--os-sdk"] = local.at("os_sdk");
    // A store selection is found again by its locked identity, never by a path remembered on this machine.
    if (from_store) options["--sdk"] = locked.at("sdk_directory"); else options["--sdk-root"] = local.at("sdk_root");
    if (lock.at("selection").value("allow_dirty_sdk", false)) options["--allow-dirty-sdk"] = "true";
    auto result = configure(options);
    require(result.selection == lock.at("selection"), "locked SDK/toolchain identity mismatch");
    return result;
}
bool Toolchain::invoke(const std::string& operation, bool cpp, const std::vector<std::string>& inputs, const fs::path& output, const std::vector<std::string>& extra, const std::vector<std::string>& libraries, Json& commands, const std::string& mode) const {
    std::vector<std::string> argv{(operation == "archive" ? archiver : cpp ? cxx : cc).string()};
    const auto& recipe = contract.at("modes").at(mode);
    for (const auto& value : recipe.at(operation == "archive" ? "archive" : operation + (cpp ? "_cxx" : "_c"))) {
        std::string arg = value;
        if (arg == "@OBJECTS@") argv.insert(argv.end(), inputs.begin(), inputs.end());
        else if (arg == "@LIBRARIES@") argv.insert(argv.end(), libraries.begin(), libraries.end());
        else if (arg == "@EXTRA@") argv.insert(argv.end(), extra.begin(), extra.end());
        else {
            std::string check = arg;
            for (const auto& known : {"@SDK@", "@OS_SDK@", "@RESOURCE@", "@OUTPUT@", "@OUTPUT_NAME@", "@SOURCE@"}) replace(check, known, "");
            require(check.find('@') == std::string::npos, "unresolved toolchain placeholder: " + arg);
            replace(arg, "@SDK@", sdk.string());
            replace(arg, "@OS_SDK@", os_sdk);
            replace(arg, "@RESOURCE@", resource.string());
            replace(arg, "@OUTPUT@", output.string());
            replace(arg, "@OUTPUT_NAME@", output.filename().string());
            if (operation == "compile") replace(arg, "@SOURCE@", inputs.at(0));
            argv.push_back(arg);
        }
    }
    if (operation == "link") argv.push_back("--ld-path=" + linker.string());
    // The reuse key covers the command line, the selected SDK/tool identity and the content of every input outside the SDK.
    auto key_file = fs::path(output.string() + ".key"), depfile = fs::path(output.string() + ".d");
    std::string command = sha256(selection.dump() + std::string(1, '\0') + [&] { std::string joined; for (const auto& arg : argv) joined += arg + '\0'; return joined; }());
    require(!fs::is_symlink(key_file) && !fs::is_symlink(depfile), "build cache files must not be symlinks");
    if (unchanged(key_file, output, command)) return false;
    retire_output(output);
    fs::remove(key_file);
    fs::remove(depfile);
    auto run = argv;
    if (operation == "compile") run.insert(run.begin() + 1, {"-MD", "-MF", depfile.string()});
    commands.push_back(run);
    auto child = execute(run, 180, true, true);
    require(child.exit_code == 0, "compiler/linker failed (exit " + std::to_string(child.exit_code) + ")");
    require(fs::is_regular_file(output), "compiler did not produce output");
    std::vector<std::string> dependencies = inputs;
    if (operation == "compile") dependencies = parse_depfile(read_text(depfile));
    else dependencies.insert(dependencies.end(), libraries.begin(), libraries.end());
    Json recorded = Json::object();
    for (const auto& dependency : dependencies) {
        require_native_utf8(dependency, "a dependency path reported by the compiler");
        fs::path path = fs::absolute(dependency);
        if (under(path, sdk) || under(path, resource)) continue; // covered by the verified SDK/tool identity
        recorded[path.string()] = file_sha256(path);
    }
    write_text(key_file, Json{{"command", command}, {"inputs", recorded}}.dump(2) + "\n");
    return true;
}
}
