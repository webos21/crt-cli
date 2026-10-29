#include <crtcli/toolchain.h>
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
Toolchain Toolchain::configure(const std::map<std::string, std::string>& options) {
    Toolchain result;
    require(!(options.count("--sdk-root") && options.count("--sdk-id")), "choose --sdk-root or --sdk-id, not both");
    result.sdk = options.count("--sdk-id") ? inside(sdk_store_root(), options.at("--sdk-id")) : fs::canonical(selected(options, "--sdk-root"));
    result.contract_path = fs::canonical(selected(options, "--sdk-contract"));
    auto hash = selected(options, "--contract-sha256");
    require(hash.size() == 64 && file_sha256(result.contract_path) == hash, "toolchain contract digest mismatch");
    result.contract = read_json(result.contract_path);
    bool allow_dirty = options.count("--allow-dirty-sdk") && options.at("--allow-dirty-sdk") == "true";
    require(!options.count("--allow-dirty-sdk") || allow_dirty, "--allow-dirty-sdk accepts only true");
    result.selection = inspect_sdk(result.sdk, result.contract, allow_dirty);
    result.selection["contract_sha256"] = hash;
    result.cc = fs::canonical(selected(options, "--cc"));
    result.cxx = fs::canonical(selected(options, "--cxx"));
    // Linkers select their driver from argv[0]; preserve the selected name.
    result.linker = fs::absolute(selected(options, "--linker")).lexically_normal();
    result.archiver = fs::absolute(selected(options, "--ar")).lexically_normal();
    for (const auto& tool : {result.cc, result.cxx, result.linker, result.archiver}) native_executable(tool);
    auto query = execute({result.cxx.string(), "-print-resource-dir"}, 10, false);
    require(query.exit_code == 0, "Clang resource directory query failed");
    while (!query.output.empty() && (query.output.back() == '\n' || query.output.back() == '\r')) query.output.pop_back();
    result.resource = fs::canonical(query.output);
    result.selection["tools"] = Json{{"cc", file_sha256(result.cc)}, {"cxx", file_sha256(result.cxx)}, {"linker", file_sha256(result.linker)}, {"ar", file_sha256(result.archiver)}, {"resource_content_sha256", sha256(inventory(result.resource).dump())}};
    result.local = {{"sdk_root", result.sdk.string()}, {"sdk_contract", result.contract_path.string()}, {"cc", result.cc.string()}, {"cxx", result.cxx.string()}, {"linker", result.linker.string()}, {"ar", result.archiver.string()}, {"resource_dir", result.resource.string()}};
    return result;
}
Toolchain Toolchain::load(const fs::path& root) {
    require(fs::exists(root / ".crt/local.json"), "project has no local SDK selection; run configure with explicit local inputs (SDK acquisition is pending)");
    auto local = read_json(root / ".crt/local.json");
    auto lock = read_json(root / "crt.lock");
    require(lock.at("lock_format") == 0 && lock.at("state") == "local-experiment", "unsupported/unresolved project lock");
    // Verify selected tool bytes before even a resource-directory query.
    for (const auto& role : {"cc", "cxx", "linker", "ar"})
        require(file_sha256(fs::path(local.at(role).get<std::string>())) == lock.at("selection").at("tools").at(role), "locked external tool digest mismatch");
    std::map<std::string, std::string> options{{"--sdk-root", local.at("sdk_root")}, {"--sdk-contract", local.at("sdk_contract")}, {"--contract-sha256", lock.at("selection").at("contract_sha256")}, {"--cc", local.at("cc")}, {"--cxx", local.at("cxx")}, {"--linker", local.at("linker")}};
    options["--ar"] = local.at("ar");
    if (lock.at("selection").value("allow_dirty_sdk", false)) options["--allow-dirty-sdk"] = "true";
    auto result = configure(options);
    require(result.selection == lock.at("selection"), "locked SDK/toolchain identity mismatch");
    return result;
}
void Toolchain::invoke(const std::string& operation, bool cpp, const std::vector<std::string>& inputs, const fs::path& output, const std::vector<std::string>& extra, const std::vector<std::string>& libraries, Json& commands, const std::string& mode) const {
    std::vector<std::string> argv{(operation == "archive" ? archiver : cpp ? cxx : cc).string()};
    const auto& recipe = contract.at("modes").at(mode);
    for (const auto& value : recipe.at(operation == "archive" ? "archive" : operation + (cpp ? "_cxx" : "_c"))) {
        std::string arg = value;
        if (arg == "@OBJECTS@") argv.insert(argv.end(), inputs.begin(), inputs.end());
        else if (arg == "@LIBRARIES@") argv.insert(argv.end(), libraries.begin(), libraries.end());
        else if (arg == "@EXTRA@") argv.insert(argv.end(), extra.begin(), extra.end());
        else {
            std::string check = arg;
            for (const auto& known : {"@SDK@", "@RESOURCE@", "@OUTPUT@", "@OUTPUT_NAME@", "@SOURCE@"}) replace(check, known, "");
            require(check.find('@') == std::string::npos, "unresolved toolchain placeholder: " + arg);
            replace(arg, "@SDK@", sdk.string());
            replace(arg, "@RESOURCE@", resource.string());
            replace(arg, "@OUTPUT@", output.string());
            replace(arg, "@OUTPUT_NAME@", output.filename().string());
            if (operation == "compile") replace(arg, "@SOURCE@", inputs.at(0));
            argv.push_back(arg);
        }
    }
    if (operation == "link") argv.push_back("--ld-path=" + linker.string());
    commands.push_back(argv);
    auto child = execute(argv);
    require(child.exit_code == 0, "compiler/linker failed (exit " + std::to_string(child.exit_code) + ")");
    require(fs::is_regular_file(output), "compiler did not produce output");
}
}
