#include <crtcli/core.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <unistd.h>
using namespace crtcli;
namespace {
size_t checks = 0;
Json calls = Json::array();
void check(bool condition, const std::string& message) { require(condition, "editor test failed: " + message); ++checks; }
Json call(const std::string& cli, std::vector<std::string> args, int status = 0, const std::string& code = "") {
    auto original = args;
    args.insert(args.begin(), "--json"); args.insert(args.begin(), cli);
    auto child = execute(args, 300, false);
    auto reply = Json::parse(child.output);
    calls.push_back({{"args", original}, {"exit", child.exit_code}, {"reply", reply}});
    check(child.exit_code == status, "exit status: " + original.front() + " " + reply.dump());
    check(reply.size() == 5 && reply.at("schema") == "crt-cli.bootstrap-result" && reply.at("format") == 1 &&
          reply.at("command").is_string() && reply.at("data").is_object() && reply.at("ok") == (status == 0), "final envelope");
    if (!code.empty()) check(reply.at("data").at("code") == code, "stable editor error code");
    return reply.at("data");
}
std::vector<std::string> binding(const fs::path& project, const fs::path& parent) {
    auto local = read_json(parent / ".crt/local.json");
    auto selection = read_json(parent / "crt.lock").at("selection");
    std::vector<std::string> args{"configure", "--project", project.string()};
    if (selection.value("source", "") == "store") args.insert(args.end(), {"--sdk", selection.at("sdk_directory")});
    else args.insert(args.end(), {"--sdk-root", local.at("sdk_root")});
    if (selection.value("contract_source", "") != "store")
        args.insert(args.end(), {"--sdk-contract", local.at("sdk_contract"), "--contract-sha256", selection.at("contract_sha256")});
    for (const auto& role : {"cc", "cxx", "linker", "ar"}) args.insert(args.end(), {std::string("--") + role, local.at(role)});
    if (local.contains("os_sdk")) args.insert(args.end(), {"--os-sdk", local.at("os_sdk")});
    if (selection.value("allow_dirty_sdk", false)) args.insert(args.end(), {"--allow-dirty-sdk", "true"});
    return args;
}
}

int main(int argc, char** argv) {
    require(argc == 3, "editor-test needs CLI and configured source project");
    const std::string cli = argv[1];
    const auto parent = fs::canonical(argv[2]);
    auto fixture = read_json(parent / "src/test/data/editor_api.json");
    auto capabilities = call(cli, {"capabilities", "--api-version", "1"});
    check(capabilities == fixture.at("final_envelopes").at(0).at("reply").at("data"), "capability wire fixture");
    for (const auto& wire : fixture.at("final_envelopes")) {
        const auto& reply = wire.at("reply");
        check(reply.size() == 5 && reply.at("schema") == fixture.at("envelope").at("schema") &&
              reply.at("format") == fixture.at("envelope").at("format") && reply.at("ok") == (wire.at("exit") == 0), "final-envelope consumer fixture");
    }
    check(capabilities.at("schema") == "crt-cli.capabilities" && capabilities.at("format") == 1 &&
          capabilities.at("api_versions") == Json::array({1}), "capability versions");
    check(capabilities.at("result") == Json({{"schema", "crt-cli.bootstrap-result"}, {"format", 1},
          {"stdout", "single-final-object"}, {"child_output", "stderr"}, {"job_events", false}}), "stream contract");
    for (const auto& name : fixture.at("capabilities")) check(capabilities.at("commands").contains(name), "named capability");
    for (const auto& error : fixture.at("errors")) call(cli, error.at("args").get<std::vector<std::string>>(), error.at("exit"), error.at("code"));
    call(cli, {"project", "info", "--project", (parent / "absent-project").string()}, 1, "project_not_found");
    const auto temporary = std::getenv("TMPDIR");
    const auto base = fs::absolute(fs::path(temporary ? temporary : "/tmp") / ("editor space 한글-" + std::to_string(getpid())));
    fs::create_directories(base);
    auto project = base / "probe";
    const auto database = base / "compile_commands.json";
    call(cli, {"new", project.string(), "--language", "cxx"});
    project = fs::canonical(project);
    auto snapshot = inventory(project);
    auto info = call(cli, {"project", "info", "--project", project.string()});
    check(info.at("state") == "unresolved" && info.at("selection").is_null() && info.at("executable").is_null(), "unresolved state");
    call(cli, {"export", "compile-commands", "--project", project.string(), "--output", database.string()}, 1, "project_unresolved");
    check(inventory(project) == snapshot && !fs::exists(database), "unresolved queries do not bind or export");

    write_text(project / "include/probe.h", "#pragma once\n#ifdef __cplusplus\nextern \"C\" {\n#endif\nint probe(void);\n#ifdef __cplusplus\n}\n#endif\n");
    write_text(project / "src/main/probe.c", "#include <probe.h>\n#include <zlib.h>\nint probe(void) { return ZLIB_VERNUM ? 0 : 1; }\n");
    write_text(project / "src/main/probe.cpp", "#include <filesystem>\nint cpp_probe() { return std::filesystem::path(\"a\").empty(); }\n");
    const std::string main_source = "#include <filesystem>\n#include <stdio.h>\n#include <probe.h>\nint main() { puts(\"editor child output\"); return probe() || std::filesystem::path(\"a\").empty(); }\n";
    write_text(project / "src/main/main.cpp", main_source);
    auto manifest = read_json(project / "crt.json");
    manifest.erase("application"); manifest.erase("tests");
    Json targets = Json::array();
    for (const auto& name : {"app-static", "app-shared", "archive", "module"}) {
        const bool app = std::string(name).find("app-") == 0;
        const std::string kind = app ? "executable" : std::string(name) == "archive" ? "static-library" : "shared-library";
        targets.push_back({{"name", name}, {"kind", kind}, {"runtime_linkage", std::string(name) == "app-static" || kind == "static-library" ? "static" : "shared"},
            {"sources", {"src/main/probe.c", app ? "src/main/main.cpp" : "src/main/probe.cpp"}}, {"sdk_libraries", {"z"}},
            // SDK library artifacts and CRT runtime linkage are independent. The
            // pinned Windows SDKs declare static zlib only; all four compile
            // modes still run with their requested static/shared CRT runtime.
            {"sdk_library_linkage", "static"}});
    }
    manifest["targets"] = targets;
    write_text(project / "crt.json", manifest.dump(2) + "\n");
    call(cli, binding(project, parent));
    snapshot = inventory(project);
    info = call(cli, {"project", "info", "--project", project.string()});
    check(info.at("state") == "ready" && info.at("targets").size() == 4, "ready before build: " + info.dump());
    check(info.at("lock").at("sha256") == file_sha256(project / "crt.lock") &&
          info.at("selection") == read_json(project / "crt.lock").at("selection"), "exact selected lock identity");
    for (const auto& target : info.at("targets")) check(target.at("exists") == false, "query does not build targets");
    auto exported = call(cli, {"export", "compile-commands", "--project", project.string(), "--output", database.string()});
    auto entries = read_json(database);
    check(entries.size() == 8 && exported.at("entries") == 8 && exported.at("lock_sha256") == file_sha256(project / "crt.lock"), "eight pinned compile entries");
    check(inventory(project) == snapshot, "queries/export leave project inventory byte-identical");
    for (const auto& entry : entries) {
        check(entry.size() == 4 && entry.at("directory") == project.string() && fs::path(entry.at("file").get<std::string>()).is_absolute(), "standard absolute compile entry");
        bool compile = false, standard = false;
        for (const auto& value : entry.at("arguments")) {
            const std::string arg = value;
            compile |= arg == "-c"; standard |= arg == "-std=c99" || arg == "-std=c++17";
            check(arg.find("crt0") == std::string::npos && arg.rfind("-Wl,", 0) != 0 && arg.rfind("--ld-path=", 0) != 0,
                  "no startup/link argv in compile database");
        }
        check(compile && standard, "compile language policy");
    }
    auto cold = call(cli, {"build", "--project", project.string()});
    size_t compiled = 0;
    for (const auto& command : cold.at("commands")) {
        if (std::find(command.begin(), command.end(), Json("-c")) == command.end()) continue;
        auto found = std::find_if(entries.begin(), entries.end(), [&](const Json& entry) { return entry.at("arguments") == command; });
        check(found != entries.end(), "export exactly equals actual SDK-expanded cold compiler argv"); ++compiled;
    }
    check(compiled == entries.size(), "every exported entry executed in cold build");
    call(cli, {"run", "--project", project.string(), "--target", "app-static"});
    call(cli, {"run", "--project", project.string(), "--target", "app-shared"});
    auto warm = call(cli, {"build", "--project", project.string()});
    check(warm.at("cache").at("executed") == 0 && warm.at("commands").empty(), "warm reuse");
    snapshot = inventory(project);
    const auto saved_database = read_text(database);
    call(cli, {"export", "compile-commands", "--project", project.string(), "--output", database.string()});
    check(read_text(database) == saved_database && inventory(project) == snapshot, "stable atomic re-export");
    // A missing SDK library is an incompatible plan, never permission to pick
    // another variant, rewrite a pin or replace the previous database.
    auto unavailable = manifest;
    unavailable["targets"][0]["sdk_libraries"] = Json::array({"editor-missing-sdk-library"});
    write_text(project / "crt.json", unavailable.dump()); snapshot = inventory(project);
    auto unsupported_library = call(cli, {"project", "info", "--project", project.string()});
    check(unsupported_library.at("state") == "incompatible" &&
          unsupported_library.at("diagnostics").at(0).at("message").get<std::string>().find("declares no library") != std::string::npos,
          "unavailable SDK library has an actionable diagnostic");
    call(cli, {"export", "compile-commands", "--project", project.string(), "--output", database.string()}, 1, "project_incompatible");
    check(inventory(project) == snapshot && read_text(database) == saved_database, "unavailable SDK library preserves project and database");
    write_text(project / "crt.json", manifest.dump(2) + "\n");
    auto policy = read_json(info.at("sdk").at("contract_path").get<std::string>());
    if (!policy.at("libraries").at("z").contains("shared")) {
        auto unsupported_variant = manifest;
        unsupported_variant["targets"][1]["sdk_library_linkage"] = "shared";
        write_text(project / "crt.json", unsupported_variant.dump()); snapshot = inventory(project);
        auto rejected = call(cli, {"project", "info", "--project", project.string()});
        check(rejected.at("state") == "incompatible" &&
              rejected.at("diagnostics").at(0).at("message").get<std::string>().find("no shared variant") != std::string::npos,
              "SDK without shared zlib refuses an explicit shared request");
        call(cli, {"export", "compile-commands", "--project", project.string(), "--output", database.string()}, 1, "project_incompatible");
        check(inventory(project) == snapshot && read_text(database) == saved_database, "unsupported variant does not fall back or rewrite");
        write_text(project / "crt.json", manifest.dump(2) + "\n");
    }
    const auto lock_bytes = read_text(project / "crt.lock");
    for (const auto& field : {"sdk_content_sha256", "contract_sha256"}) {
        auto lock = Json::parse(lock_bytes); lock["selection"][field] = std::string(64, '0');
        write_text(project / "crt.lock", lock.dump(2)); snapshot = inventory(project);
        auto stale = call(cli, {"project", "info", "--project", project.string()});
        check(stale.at("state") == "incompatible" && stale.at("selection").is_null(), "stale selected inputs are not advertised");
        call(cli, {"export", "compile-commands", "--project", project.string(), "--output", database.string()}, 1, "project_incompatible");
        check(inventory(project) == snapshot && read_text(database) == saved_database, "stale export preserves pins and previous database");
    }
    write_text(project / "crt.lock", lock_bytes);
    const auto source_bytes = read_text(project / "src/main/probe.c");
    fs::remove(project / "src/main/probe.c"); snapshot = inventory(project);
    check(call(cli, {"project", "info", "--project", project.string()}).at("state") == "incompatible", "missing source is diagnosed");
    call(cli, {"export", "compile-commands", "--project", project.string(), "--output", database.string()}, 1, "project_incompatible");
    check(inventory(project) == snapshot && read_text(database) == saved_database, "missing source does not rebuild or rewrite");
    write_text(project / "src/main/probe.c", source_bytes);
    auto unsupported = manifest; unsupported["manifest_format"] = 99;
    write_text(project / "crt.json", unsupported.dump()); snapshot = inventory(project);
    check(call(cli, {"project", "info", "--project", project.string()}).at("state") == "incompatible", "unsupported manifest format is diagnosed");
    call(cli, {"export", "compile-commands", "--project", project.string(), "--output", database.string()}, 1, "project_incompatible");
    check(inventory(project) == snapshot, "incompatible query preserves unknown formats");
    write_text(project / "crt.json", manifest.dump(2) + "\n");
    write_text(project / ".sdk.json", Json{{"format", 0}, {"sdk", std::string(64, '0')}, {"cc", "/absent/compiler"}}.dump());
    check(call(cli, {"project", "info", "--project", project.string()}).at("state") == "ready", "defaults cannot replace selected inputs");
    fs::remove(project / ".sdk.json");
    for (const auto& path : {project / "crt.json", project / ".crt/export.json", project / "src/export.json", base / "missing/db.json"})
        call(cli, {"export", "compile-commands", "--project", project.string(), "--output", path.string()}, 1,
             path.parent_path() == base / "missing" ? "output_error" : "output_conflict");
    const auto link = base / "symlink.json";
    fs::create_symlink(database, link);
    call(cli, {"export", "compile-commands", "--project", project.string(), "--output", link.string()}, 1, "output_conflict");
    const auto occupied = base / "occupied.json"; write_text(occupied, "preserve unrelated file");
    call(cli, {"export", "compile-commands", "--project", project.string(), "--output", occupied.string()}, 1, "output_conflict");
    check(read_text(occupied) == "preserve unrelated file" && read_text(database) == saved_database, "output failures preserve existing files");
    auto missing_port = manifest;
    missing_port["targets"][0]["sdk_libraries"] = Json::array();
    missing_port["targets"][0]["ports"] = Json::array({Json{{"name", "zlib"}, {"provider", "port"}, {"build_key", std::string(64, 'f')}}});
    const auto port_root = sdk_store_root().parent_path() / "ports";
    const bool had_ports = fs::exists(port_root);
    const auto prior_ports = had_ports ? inventory(port_root) : Json::array();
    write_text(project / "crt.json", missing_port.dump()); snapshot = inventory(project);
    call(cli, {"export", "compile-commands", "--project", project.string(), "--output", database.string()}, 1, "project_incompatible");
    check(inventory(project) == snapshot && fs::exists(port_root) == had_ports &&
          (!had_ports || inventory(port_root) == prior_ports), "missing ports are not built or acquired");
    write_text(project / "crt.json", manifest.dump(2) + "\n");
    write_text(project / "src/main/main.cpp", "#include <stdio.h>\nint main() { puts(\"child failure stdout\"); return 7; }\n");
    auto failed = call(cli, {"run", "--project", project.string(), "--target", "app-static"}, 1);
    check(failed.at("outcomes").at(0).at("exit_code") == 7, "child failure stays distinct from CLI errors and JSON stdout");
    check(call(cli, {"sdk", "list"}).at("sdks").is_array(), "existing SDK list contract");
    check(call(cli, {"port", "list"}).at("items").is_array(), "existing port list contract");
    const auto evidence = parent / "out/editor-test"; fs::create_directories(evidence);
    write_text(evidence / "result.json", Json{{"passed", true}, {"checks", checks}, {"calls", calls},
        {"project", project.string()}, {"compile_commands", entries}, {"cold", cold}, {"warm", warm}}.dump(2) + "\n");
    std::printf("editor-test: %zu checks passed\n", checks);
    return 0;
}
