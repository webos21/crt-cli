// Recipe subset reader: the reviewed zlib recipe parses to the oracle's semantics on each OS, and every key, value and
// placeholder outside the subset is refused. Mutated recipes are parsed against a locally built adapter entry because the
// public entry point rightly rejects any file whose digest is not in the reviewed table.
#include <crtcli/port_recipe.h>
#include <cstdio>
#include <fstream>
using namespace crtcli;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }

std::string fixture() {
    for (const char* candidate : {"src/test/data/zlib.recipe.json", "../src/test/data/zlib.recipe.json"}) {
        std::ifstream in(candidate, std::ios::binary);
        if (in.good()) return std::string(std::istreambuf_iterator<char>(in), {});
    }
    std::string here = __FILE__;
    return read_text(fs::path(here).parent_path() / "data" / "zlib.recipe.json");
}

std::string error_of(const std::string& bytes, const std::string& os = "linux") {
    PortAdapter adapter = port_adapters().front();
    adapter.recipe_sha256 = sha256(bytes);
    try { parse_port_recipe_with(bytes, os, adapter); } catch (const Error& e) { return e.what(); }
    return {};
}
void rejects(const std::string& why, const Json& recipe, const std::string& expect, const std::string& os = "linux") {
    auto message = error_of(recipe.dump(), os);
    check(!message.empty(), "accepted: " + why);
    check(message.find(expect) != std::string::npos, why + " (got: " + message + ")");
}
}  // namespace

int main() {
    const auto bytes = fixture();
    const auto base = Json::parse(bytes);

    // ---- the reviewed recipe ---------------------------------------------------------------------------------------
    check(sha256(bytes) == port_adapter_for(sha256(bytes)).recipe_sha256, "fixture digest is in the reviewed table");
    check(port_adapters().size() == 1 && port_adapters()[0].crt_review == "approved", "adapter table contains the CRT-approved bounded zlib entry");
    const auto& adapter = port_adapters()[0];
    check(port_layout(adapter, "linux").shared_library == "lib/libz.so", "the Linux layout is unchanged");
    check(port_layout(adapter, "macos").shared_library == "lib/libz.dylib" &&
          port_layout(adapter, "macos").expected_names == std::vector<std::string>{"@rpath/libz.1.dylib"}, "the macOS layout declares its dylib and install name");
    check(port_executor_env(adapter, "linux").empty() && port_executor_make_args(adapter, "linux").empty(), "Linux has no added executor policy");
    check(port_executor_env(adapter, "macos").size() == 1 &&
          port_executor_make_args(adapter, "macos") == std::vector<std::string>{"AR=@AR@", "ARFLAGS=rc"}, "macOS declares the install-name and archiver policies");
    auto linux_recipe = parse_port_recipe(bytes, "linux");
    check(linux_recipe.name == "zlib" && linux_recipe.version == "1.3.1" && linux_recipe.target_os == "linux", "identity");
    check(linux_recipe.source.sha256 == "9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23" && linux_recipe.source.archive == "zlib-1.3.1.tar.gz"
          && linux_recipe.source.source_dir == "zlib-1.3.1" && linux_recipe.source.url == "https://zlib.net/fossils/zlib-1.3.1.tar.gz", "source");
    check(linux_recipe.source.sha1.size() == 40 && linux_recipe.source.md5.size() == 32, "informational digests kept");
    check(linux_recipe.dependencies == std::vector<std::string>{"make"}, "make is a dependency name; its role comes from the table");
    check(port_adapter_for(linux_recipe.recipe_sha256).host_tools == std::vector<std::string>{"make"} && port_adapter_for(linux_recipe.recipe_sha256).target_ports.empty(), "make is a host tool, no target ports");
    check(linux_recipe.configure_args.empty() && linux_recipe.make_args.empty(), "no extra arguments");
    check(linux_recipe.post_configure_patch.size() == 1 && linux_recipe.post_configure_patch[0].file == "Makefile"
          && linux_recipe.post_configure_patch[0].find == "prefix =@PORT_PREFIX_POSIX@", "post-configure patch of the generated Makefile");
    check(linux_recipe.tests.size() == 2 && linux_recipe.tests[0].name == "roundtrip-static" && linux_recipe.tests[1].name == "roundtrip-shared", "tests");
    check(linux_recipe.tests[0].link_args == std::vector<std::string>{"@PORT_PREFIX@/lib/libz.a"}, "static link argument");
    check(linux_recipe.tests[1].link_args == std::vector<std::string>{"@PORT_PREFIX@/lib/libz.so"}, "shared link argument is the Linux override");
    check(parse_port_recipe(bytes, "macos").tests[1].link_args == std::vector<std::string>{"@PORT_PREFIX@/lib/libz.dylib"}, "macOS override");
    check(parse_port_recipe(bytes, "windows").tests[1].link_args == std::vector<std::string>{"@PORT_PREFIX@/lib/libz.so"}, "Windows override keeps the POSIX spelling");
    check(parse_port_recipe(bytes, "macos").env.size() == 1, "macOS applies only the base env (no macOS override in the recipe)");

    // Environment semantics of the oracle, including its quirk: the base CFLAGS replaces, the Linux override then appends.
    {
        PortPlaceholders p{"/store/it em/prefix", "/sdk/tools/crt-cc"};
        std::map<std::string, std::string> env{{"CFLAGS", "-O2"}, {"CC", "/sdk/tools/crt-cc"}};
        apply_port_env(env, linux_recipe.env, p);
        check(env["CFLAGS"] == "-O2 -O2 -fPIC", "Linux CFLAGS: base replaces then override appends (oracle behavior): " + env["CFLAGS"]);
        check(env["LDSHARED"] == "/sdk/tools/crt-cc -shared -Wl,-soname,libz.so.1,--version-script,zlib.map,--undefined-version", "LDSHARED with @CC@");
        std::vector<PortEnvOp> overrides;
        for (const auto& op : linux_recipe.env) if (op.append_flags) overrides.push_back(op);
        check(overrides.size() == 2, "the Linux override carries CFLAGS and LDSHARED");
        std::map<std::string, std::string> empty{{"CFLAGS", ""}};
        apply_port_env(empty, overrides, p);
        check(empty["CFLAGS"] == "-O2 -fPIC", "an empty CFLAGS is replaced, not appended to");
        std::map<std::string, std::string> kept{{"CFLAGS", "-g"}, {"LDSHARED", "old"}};
        apply_port_env(kept, overrides, p);
        check(kept["CFLAGS"] == "-g -O2 -fPIC" && kept["LDSHARED"].rfind("/sdk/tools/crt-cc -shared", 0) == 0, "flag variables append, other variables replace");
        std::map<std::string, std::string> win;
        apply_port_env(win, parse_port_recipe(bytes, "windows").env, p);
        check(win["RANLIB"] == "true" && win["CFLAGS"].find("-U_MSC_VER") != std::string::npos, "Windows env");
        check(substitute_port_placeholders("prefix =@PORT_PREFIX_MAKE@", p) == "prefix =/store/it\\ em/prefix", "make prefix escapes spaces");
        check(substitute_port_placeholders("@PORT_PREFIX@/lib @PORT_PREFIX_POSIX@ user@host", p) == "/store/it em/prefix/lib /store/it em/prefix user@host", "prefix placeholders; a lone @ is text");
        bool refused = false;
        try { substitute_port_placeholders("@ROOT@/x", p); } catch (const Error&) { refused = true; }
        check(refused, "unsupported placeholder refused at substitution too");
    }

    // ---- the digest gate ------------------------------------------------------------------------------------------
    {
        auto mutated = base; mutated["version"] = "1.3.2";
        bool refused = false; std::string message;
        try { parse_port_recipe(mutated.dump(), "linux"); } catch (const Error& e) { refused = true; message = e.what(); }
        check(refused && message.find("not in the reviewed adapter table") != std::string::npos, "unreviewed recipe digest refused before parsing: " + message);
        std::string whitespace = bytes + "\n";
        refused = false;
        try { parse_port_recipe(whitespace, "linux"); } catch (const Error&) { refused = true; }
        check(refused, "even a whitespace change is a different, unreviewed file");
        refused = false;
        try { parse_port_recipe(bytes, "plan9"); } catch (const Error&) { refused = true; }
        check(refused, "unknown target OS refused");
        PortAdapter other = port_adapters().front(); other.recipe_sha256 = std::string(64, 'a');
        refused = false;
        try { parse_port_recipe_with(bytes, "linux", other); } catch (const Error&) { refused = true; }
        check(refused, "adapter entry for another digest refused");
    }
    check(error_of(bytes).empty(), "unmutated recipe parses against a locally built entry (control)");

    // ---- subset rules: one rejection each ---------------------------------------------------------------------------
    { auto r = base; r["extra"] = 1; rejects("unknown top-level key", r, "key 'extra'"); }
    { auto r = base; r["schema_version"] = 2; rejects("schema version 2", r, "schema_version"); }
    { auto r = base; r["schema_version"] = "1"; rejects("schema version as string", r, "schema_version"); }
    { auto r = base; r.erase("tests"); rejects("missing tests", r, "missing 'tests'"); }
    { auto r = base; r["tests"] = Json::array(); rejects("no tests", r, "without a test"); }
    { auto r = base; r["name"] = "../zlib"; rejects("path-like name", r, "simple package name"); }
    { auto r = base; r["name"] = "zlib2"; rejects("name differs from the entry", r, "differ from the reviewed"); }
    { auto r = base; r["source"]["mirror"] = "x"; rejects("unknown source key", r, "key 'mirror'"); }
    { auto r = base; r["source"]["url"] = "http://zlib.net/x.tar.gz"; rejects("plain http source", r, "https://"); }
    { auto r = base; r["source"]["url"] = "https://user:pw@zlib.net/x.tar.gz"; rejects("credentials in the URL", r, "https://"); }
    { auto r = base; r["source"]["archive"] = "../x.tar.gz"; rejects("archive with a path", r, "plain file name"); }
    { auto r = base; r["source"]["source_dir"] = "a/b"; rejects("source dir with a path", r, "plain directory"); }
    { auto r = base; r["source"]["sha256"] = "ABC"; rejects("short sha256", r, "64 lowercase"); }
    { auto r = base; auto s = r["source"]["sha256"].get<std::string>(); s[0] = 'A'; r["source"]["sha256"] = s; rejects("uppercase sha256", r, "64 lowercase"); }
    { auto r = base; r["source"].erase("sha256"); rejects("missing sha256", r, "missing 'sha256'"); }
    { auto r = base; r["source"]["sha1"] = "zz"; rejects("bad sha1", r, "40 lowercase"); }
    { auto r = base; r["dependencies"] = Json::array({"make", "perl"}); rejects("unclassified dependency", r, "classifies"); }
    { auto r = base; r["dependencies"] = Json::array(); rejects("missing dependency", r, "classifies"); }
    { auto r = base; r["dependencies"] = Json::array({"make", "make"}); rejects("duplicate dependency", r, "twice"); }
    { auto r = base; r["build"]["system"] = "cmake"; rejects("other build system", r, "only 'configure'"); }
    { auto r = base; r["build"]["skip_configure"] = true; rejects("unsupported build key", r, "key 'skip_configure'"); }
    { auto r = base; r["build"]["configure_args"] = Json::array({"--prefix=@ROOT@"}); rejects("@ROOT@ placeholder", r, "@ROOT@"); }
    { auto r = base; r["build"]["make_args"] = Json::array({"CC=@CXX@"}); rejects("@CXX@ placeholder", r, "@CXX@"); }
    { auto r = base; r["build"]["make_args"] = Json::array({"A\nB"}); rejects("control character in an argument", r, "control character"); }
    { auto r = base; r["build"]["make_args"] = Json::array({1}); rejects("non-string argument", r, "must be a string"); }
    { auto r = base; r["build"]["env"]["PATH"] = "/evil"; rejects("PATH from a recipe", r, "may not be set"); }
    { auto r = base; r["build"]["env"]["LD_PRELOAD"] = "/evil.so"; rejects("LD_PRELOAD from a recipe", r, "may not be set"); }
    { auto r = base; r["build"]["env"]["DYLD_INSERT_LIBRARIES"] = "x"; rejects("DYLD_ from a recipe", r, "may not be set"); }
    { auto r = base; r["build"]["env"]["lower"] = "x"; rejects("lower-case variable name", r, "[A-Z0-9_]"); }
    { auto r = base; r["build"]["env"]["CFLAGS"] = 5; rejects("non-string env value", r, "must be a string"); }
    { auto r = base; r["build"]["env"]["CFLAGS"] = "@WORK_ROOT@"; rejects("unsupported placeholder in env", r, "@WORK_ROOT@"); }
    { auto r = base; r["build"]["path_prepend"] = Json::array({"x"}); rejects("path_prepend", r, "key 'path_prepend'"); }
    { auto r = base; r["build"]["post_configure_patch"][0]["file"] = "../x"; rejects("patch outside the tree", r, "'..'"); }
    { auto r = base; r["build"]["post_configure_patch"][0]["file"] = "/etc/passwd"; rejects("absolute patch file", r, "absolute"); }
    { auto r = base; r["build"]["post_configure_patch"][0]["find"] = ""; rejects("empty patch find", r, "must not be empty"); }
    { auto r = base; r["build"]["post_configure_patch"][0].erase("reason"); rejects("patch without a reason", r, "missing 'reason'"); }
    { auto r = base; r["build"]["post_configure_patch"][0]["mode"] = "x"; rejects("unknown patch key", r, "key 'mode'"); }
    { auto r = base; r["build"]["post_configure_patch"][0]["replace"] = "@ROOT@"; rejects("unsupported placeholder in patch", r, "@ROOT@"); }
    { auto r = base; r["build"]["target_overrides"]["plan9"] = Json::object(); rejects("unknown override OS", r, "unknown OS 'plan9'"); }
    { auto r = base; r["build"]["target_overrides"]["linux"]["configure_args"] = Json::array({"--x"}); rejects("override with configure_args", r, "key 'configure_args'", "linux"); }
    { auto r = base; r["build"]["target_overrides"]["macos"]["patches"] = Json::array(); rejects("override of another OS is still validated", r, "key 'patches'", "linux"); }
    { auto r = base; r["tests"][0]["type"] = "script"; rejects("other test type", r, "only 'compile-run'"); }
    { auto r = base; r["tests"][0]["source"] = "porting/tests/evil.c"; rejects("unpinned test source", r, "is not a test source"); }
    { auto r = base; r["tests"][0]["source"] = "../evil.c"; rejects("test source outside the tree", r, "'..'"); }
    { auto r = base; r["tests"][1]["name"] = "roundtrip-static"; rejects("duplicate test name", r, "unique"); }
    { auto r = base; r["tests"][0]["cflags"] = Json::array({"-I@PORT_PREFIX@/include", "@SYSROOT@"}); rejects("unsupported placeholder in a test", r, "@SYSROOT@"); }
    { auto r = base; r["tests"][0]["env"] = Json::object(); rejects("unknown test key", r, "key 'env'"); }
    { auto r = base; r["tests"][1]["target_overrides"]["linux"]["cflags"] = Json::array(); rejects("test override other than link_args", r, "key 'cflags'"); }
    { auto r = base; r["status"]["beos"] = "pass"; rejects("status for an unknown OS", r, "key 'beos'"); }
    { auto r = base; r["notes"] = "text"; rejects("notes not an array", r, "must be an array"); }
    { auto r = base; r["notes"] = Json::array({std::string(9000, 'n')}); rejects("oversized note", r, "longer than"); }
    {   // nlohmann refuses to dump invalid UTF-8, so splice the byte into the text.
        auto r = base; r["notes"] = Json::array({"abcXYZdef"});
        auto text = r.dump(); text.replace(text.find("XYZ"), 3, "\xff");
        auto message = error_of(text);
        check(message.find("UTF-8") != std::string::npos, "invalid UTF-8 refused (" + message + ")");
    }

    // ---- JSON-level hazards ---------------------------------------------------------------------------------------
    {
        auto duplicated = bytes;
        auto at = duplicated.find("\"name\": \"zlib\"");
        check(at != std::string::npos, "fixture has the name key");
        duplicated.insert(at, "\"name\": \"zlib\",\n  ");
        auto message = error_of(duplicated);
        check(message.find("duplicate key 'name'") != std::string::npos, "duplicate object key refused (" + message + ")");
        check(error_of("{not json").find("not valid JSON") != std::string::npos, "malformed JSON refused");
        check(!error_of("[]").empty(), "non-object root refused");
        check(!error_of(std::string(2u << 20, ' ')).empty(), "oversized recipe refused");
    }

    std::printf("port-recipe-test: %zu checks passed\n", checks);
    return 0;
}
