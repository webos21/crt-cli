#include "crtcli/port_recipe.h"
#include "crtcli/archive.h"
#include <algorithm>
#include <set>

namespace crtcli {
namespace {

constexpr size_t kMaxRecipeBytes = 1u << 20, kMaxString = 8192, kMaxItems = 256;

[[noreturn]] void refuse(const std::string& where, const std::string& why) { throw Error("recipe " + where + ": " + why); }

bool is_hex(const std::string& v, size_t length) {
    return v.size() == length && std::all_of(v.begin(), v.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

// Control characters are refused everywhere except the text of a patch, where a newline or tab is legitimate.
void check_text(const std::string& where, const std::string& v, bool multiline = false) {
    if (v.size() > kMaxString) refuse(where, "longer than " + std::to_string(kMaxString) + " bytes");
    if (!valid_utf8(v)) refuse(where, "not valid UTF-8");
    for (unsigned char c : v) {
        if (c == 0x7f || (c < 0x20 && !(multiline && (c == '\n' || c == '\t')))) refuse(where, "contains a control character");
    }
}

const Json& get(const Json& object, const char* key, const std::string& where) {
    if (!object.contains(key)) refuse(where, std::string("missing '") + key + "'");
    return object.at(key);
}
std::string text_of(const Json& value, const std::string& where, bool multiline = false) {
    if (!value.is_string()) refuse(where, "must be a string");
    auto v = value.get<std::string>();
    check_text(where, v, multiline);
    return v;
}
std::vector<std::string> strings_of(const Json& value, const std::string& where) {
    if (!value.is_array()) refuse(where, "must be an array");
    if (value.size() > kMaxItems) refuse(where, "has more than " + std::to_string(kMaxItems) + " items");
    std::vector<std::string> out;
    for (size_t i = 0; i < value.size(); ++i) out.push_back(text_of(value[i], where + "[" + std::to_string(i) + "]"));
    return out;
}
void only_keys(const Json& object, const std::set<std::string>& allowed, const std::string& where) {
    if (!object.is_object()) refuse(where, "must be an object");
    for (auto it = object.begin(); it != object.end(); ++it)
        if (!allowed.count(it.key())) refuse(where, "key '" + it.key() + "' is outside the supported subset");
}
const std::set<std::string>& os_names() { static const std::set<std::string> v{"linux", "macos", "windows"}; return v; }
const std::set<std::string>& flag_vars() { static const std::set<std::string> v{"CFLAGS", "CPPFLAGS", "CXXFLAGS", "LDFLAGS", "LIBS"}; return v; }

bool simple_name(const std::string& v) {
    if (v.empty() || v.size() > 128) return false;
    for (unsigned char c : v)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '+' || c == '-')) return false;
    return v[0] != '.' && v[0] != '-';
}

// Every @TOKEN@ must be one this slice substitutes.
void check_placeholders(const std::string& where, const std::string& v) {
    static const std::set<std::string> known{"@PORT_PREFIX@", "@PORT_PREFIX_POSIX@", "@PORT_PREFIX_MAKE@", "@CC@"};
    for (size_t at = v.find('@'); at != std::string::npos; at = v.find('@', at)) {
        size_t end = at + 1;
        while (end < v.size() && ((v[end] >= 'A' && v[end] <= 'Z') || (v[end] >= '0' && v[end] <= '9') || v[end] == '_')) ++end;
        if (end < v.size() && v[end] == '@' && end > at + 1) {
            auto token = v.substr(at, end - at + 1);
            if (!known.count(token)) refuse(where, "placeholder " + token + " is outside the supported subset");
            at = end + 1;
        } else at = at + 1;
    }
}

// Duplicate object keys are a parser differential waiting to happen: refuse them instead of keeping the last one.
Json parse_strict(const std::string& bytes) {
    std::vector<std::set<std::string>> keys;
    Json::parser_callback_t callback = [&](int, Json::parse_event_t event, Json& parsed) {
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        else if (event == Json::parse_event_t::object_end) keys.pop_back();
        else if (event == Json::parse_event_t::key && !keys.back().insert(parsed.get<std::string>()).second)
            throw Error("recipe: duplicate key '" + parsed.get<std::string>() + "'");
        return true;
    };
    try { return Json::parse(bytes, callback); }
    catch (const Error&) { throw; }
    catch (const std::exception& e) { throw Error(std::string("recipe is not valid JSON: ") + e.what()); }
}

std::vector<PortEnvOp> env_ops(const Json& env, bool append, const std::string& where) {
    if (!env.is_object()) refuse(where, "must be an object");
    if (env.size() > kMaxItems) refuse(where, "has too many variables");
    std::vector<PortEnvOp> out;
    for (auto it = env.begin(); it != env.end(); ++it) {
        const auto& name = it.key();
        bool valid = !name.empty() && name.size() <= 64 && !(name[0] >= '0' && name[0] <= '9');
        for (char c : name) valid = valid && ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');
        if (!valid) refuse(where, "variable name '" + name + "' is not [A-Z0-9_]+");
        if (name == "PATH" || name == "SHELL" || name == "IFS" || name.rfind("LD_", 0) == 0 || name.rfind("DYLD_", 0) == 0 || name.rfind("BASH_", 0) == 0)
            refuse(where, "variable '" + name + "' may not be set by a recipe");
        auto value = text_of(it.value(), where + "." + name);
        check_placeholders(where + "." + name, value);
        out.push_back({name, value, append});
    }
    return out;
}

}  // namespace

const std::vector<PortAdapter>& port_adapters() {
    // Seed of the later registry index, not a new recipe format. Entries are reviewed by CRT (the recipe owner);
    // `crt_review` stays "pending" until that review is recorded.
    static const std::vector<PortAdapter> table = {
        {"dde944984d799197c0af1553ac7cf38139edc7b2e4e673ef9b4c17dd8c1a349b", "zlib", "1.3.1",
         /*host_tools*/ {"make"}, /*target_ports*/ {}, /*variants*/ {"static", "shared"}, /*sdk_stage*/ "01-c",
         {{"porting/tests/zlib_roundtrip.c", "3d7460fb51e215424ef4fa36593cd7662bdcf83bbd1e986eb0ed746b07aebe62"}}, "pending"},
    };
    return table;
}

const PortAdapter& port_adapter_for(const std::string& recipe_sha256) {
    for (const auto& entry : port_adapters())
        if (entry.recipe_sha256 == recipe_sha256) return entry;
    throw Error("recipe digest " + recipe_sha256 + " is not in the reviewed adapter table");
}

PortRecipe parse_port_recipe(const std::string& bytes, const std::string& target_os) {
    if (bytes.size() > kMaxRecipeBytes) throw Error("recipe is larger than " + std::to_string(kMaxRecipeBytes) + " bytes");
    // Digest first: nothing below runs for an unreviewed file.
    return parse_port_recipe_with(bytes, target_os, port_adapter_for(sha256(bytes)));
}

PortRecipe parse_port_recipe_with(const std::string& bytes, const std::string& target_os, const PortAdapter& adapter) {
    if (!os_names().count(target_os)) throw Error("unsupported target OS for a recipe: " + target_os);
    if (bytes.size() > kMaxRecipeBytes) throw Error("recipe is larger than " + std::to_string(kMaxRecipeBytes) + " bytes");
    PortRecipe recipe;
    recipe.recipe_sha256 = sha256(bytes);
    if (adapter.recipe_sha256 != recipe.recipe_sha256) throw Error("recipe digest does not match the adapter entry it is parsed against");
    recipe.target_os = target_os;

    auto root = parse_strict(bytes);
    only_keys(root, {"schema_version", "name", "version", "source", "dependencies", "build", "tests", "status", "notes"}, "(top level)");
    if (!get(root, "schema_version", "(top level)").is_number_integer() || root.at("schema_version") != 1) refuse("schema_version", "must be 1");
    recipe.name = text_of(get(root, "name", "(top level)"), "name");
    recipe.version = text_of(get(root, "version", "(top level)"), "version");
    if (!simple_name(recipe.name)) refuse("name", "not a simple package name");
    if (!simple_name(recipe.version)) refuse("version", "not a simple version string");
    if (recipe.name != adapter.name || recipe.version != adapter.version) refuse("name/version", "differ from the reviewed adapter entry");

    const auto& source = get(root, "source", "(top level)");
    only_keys(source, {"url", "archive", "source_dir", "sha256", "sha1", "md5"}, "source");
    recipe.source.url = text_of(get(source, "url", "source"), "source.url");
    if (recipe.source.url.rfind("https://", 0) != 0 || recipe.source.url.find('@') != std::string::npos || recipe.source.url.find_first_of(" \\") != std::string::npos)
        refuse("source.url", "must be a plain https:// URL without credentials");
    recipe.source.archive = text_of(get(source, "archive", "source"), "source.archive");
    recipe.source.source_dir = text_of(get(source, "source_dir", "source"), "source.source_dir");
    if (!simple_name(recipe.source.archive)) refuse("source.archive", "must be one plain file name");
    if (!simple_name(recipe.source.source_dir)) refuse("source.source_dir", "must be one plain directory name");
    recipe.source.sha256 = text_of(get(source, "sha256", "source"), "source.sha256");
    if (!is_hex(recipe.source.sha256, 64)) refuse("source.sha256", "must be 64 lowercase hex digits");
    if (source.contains("sha1")) { recipe.source.sha1 = text_of(source.at("sha1"), "source.sha1"); if (!is_hex(recipe.source.sha1, 40)) refuse("source.sha1", "must be 40 lowercase hex digits"); }
    if (source.contains("md5")) { recipe.source.md5 = text_of(source.at("md5"), "source.md5"); if (!is_hex(recipe.source.md5, 32)) refuse("source.md5", "must be 32 lowercase hex digits"); }

    recipe.dependencies = strings_of(get(root, "dependencies", "(top level)"), "dependencies");
    {   // Roles come from the reviewed table; the recipe may only name what the table classifies.
        std::set<std::string> named(recipe.dependencies.begin(), recipe.dependencies.end()), known(adapter.host_tools.begin(), adapter.host_tools.end());
        known.insert(adapter.target_ports.begin(), adapter.target_ports.end());
        if (named.size() != recipe.dependencies.size()) refuse("dependencies", "lists a name twice");
        if (named != known) refuse("dependencies", "differ from the roles the reviewed adapter entry classifies");
    }

    const auto& build = get(root, "build", "(top level)");
    only_keys(build, {"system", "configure_args", "env", "post_configure_patch", "make_args", "target_overrides"}, "build");
    if (text_of(get(build, "system", "build"), "build.system") != "configure") refuse("build.system", "only 'configure' is supported");
    recipe.configure_args = strings_of(get(build, "configure_args", "build"), "build.configure_args");
    recipe.make_args = strings_of(get(build, "make_args", "build"), "build.make_args");
    for (const auto& list : {recipe.configure_args, recipe.make_args})
        for (const auto& arg : list) check_placeholders("build arguments", arg);
    if (build.contains("env")) for (auto& op : env_ops(build.at("env"), false, "build.env")) recipe.env.push_back(op);
    if (build.contains("post_configure_patch")) {
        const auto& patches = build.at("post_configure_patch");
        if (!patches.is_array() || patches.size() > kMaxItems) refuse("build.post_configure_patch", "must be an array of at most " + std::to_string(kMaxItems));
        for (size_t i = 0; i < patches.size(); ++i) {
            auto where = "build.post_configure_patch[" + std::to_string(i) + "]";
            only_keys(patches[i], {"file", "find", "replace", "reason"}, where);
            PortPatch patch;
            patch.file = text_of(get(patches[i], "file", where), where + ".file");
            patch.find = text_of(get(patches[i], "find", where), where + ".find", true);
            patch.replace = text_of(get(patches[i], "replace", where), where + ".replace", true);
            patch.reason = text_of(get(patches[i], "reason", where), where + ".reason", true);
            if (patch.find.empty()) refuse(where + ".find", "must not be empty");
            if (auto problem = archive_path_problem(patch.file); !problem.empty()) refuse(where + ".file", problem);
            check_placeholders(where + ".find", patch.find);
            check_placeholders(where + ".replace", patch.replace);
            recipe.post_configure_patch.push_back(std::move(patch));
        }
    }
    if (build.contains("target_overrides")) {
        const auto& overrides = build.at("target_overrides");
        if (!overrides.is_object()) refuse("build.target_overrides", "must be an object");
        for (auto it = overrides.begin(); it != overrides.end(); ++it) {
            if (!os_names().count(it.key())) refuse("build.target_overrides", "unknown OS '" + it.key() + "'");
            auto where = "build.target_overrides." + it.key();
            only_keys(it.value(), {"env"}, where);
            auto ops = it.value().contains("env") ? env_ops(it.value().at("env"), true, where + ".env") : std::vector<PortEnvOp>{};
            if (it.key() == target_os) recipe.env.insert(recipe.env.end(), ops.begin(), ops.end());
        }
    }

    const auto& tests = get(root, "tests", "(top level)");
    if (!tests.is_array() || tests.size() > kMaxItems) refuse("tests", "must be an array");
    std::set<std::string> test_names;
    for (size_t i = 0; i < tests.size(); ++i) {
        auto where = "tests[" + std::to_string(i) + "]";
        only_keys(tests[i], {"name", "type", "source", "cflags", "link_args", "expect_stdout", "target_overrides"}, where);
        PortTest test;
        test.name = text_of(get(tests[i], "name", where), where + ".name");
        if (!simple_name(test.name) || !test_names.insert(test.name).second) refuse(where + ".name", "must be a unique simple name");
        if (text_of(get(tests[i], "type", where), where + ".type") != "compile-run") refuse(where + ".type", "only 'compile-run' is supported");
        test.source = text_of(get(tests[i], "source", where), where + ".source");
        if (auto problem = archive_path_problem(test.source); !problem.empty()) refuse(where + ".source", problem);
        auto listed = adapter.test_sources.find(test.source);
        if (listed == adapter.test_sources.end()) refuse(where + ".source", "'" + test.source + "' is not a test source the reviewed adapter entry pins");
        test.cflags = tests[i].contains("cflags") ? strings_of(tests[i].at("cflags"), where + ".cflags") : std::vector<std::string>{};
        test.link_args = tests[i].contains("link_args") ? strings_of(tests[i].at("link_args"), where + ".link_args") : std::vector<std::string>{};
        test.expect_stdout = text_of(get(tests[i], "expect_stdout", where), where + ".expect_stdout");
        if (tests[i].contains("target_overrides")) {
            const auto& overrides = tests[i].at("target_overrides");
            if (!overrides.is_object()) refuse(where + ".target_overrides", "must be an object");
            for (auto it = overrides.begin(); it != overrides.end(); ++it) {
                if (!os_names().count(it.key())) refuse(where + ".target_overrides", "unknown OS '" + it.key() + "'");
                only_keys(it.value(), {"link_args"}, where + ".target_overrides." + it.key());
                if (it.key() == target_os && it.value().contains("link_args"))
                    test.link_args = strings_of(it.value().at("link_args"), where + ".target_overrides." + it.key() + ".link_args");
            }
        }
        for (const auto& list : {test.cflags, test.link_args}) for (const auto& arg : list) check_placeholders(where, arg);
        recipe.tests.push_back(std::move(test));
    }
    if (recipe.tests.empty()) refuse("tests", "a recipe without a test is not accepted");

    if (root.contains("status")) {
        only_keys(root.at("status"), os_names(), "status");
        for (auto it = root.at("status").begin(); it != root.at("status").end(); ++it) text_of(it.value(), "status." + it.key());
    }
    if (root.contains("notes")) strings_of(root.at("notes"), "notes");
    return recipe;
}

std::string substitute_port_placeholders(const std::string& value, const PortPlaceholders& p) {
    std::string escaped = p.port_prefix;
    for (size_t at = 0; (at = escaped.find(' ', at)) != std::string::npos; at += 2) escaped.insert(at, "\\");
    std::string out;
    for (size_t at = 0; at < value.size();) {
        if (value[at] == '@') {
            size_t end = at + 1;
            while (end < value.size() && ((value[end] >= 'A' && value[end] <= 'Z') || (value[end] >= '0' && value[end] <= '9') || value[end] == '_')) ++end;
            if (end < value.size() && value[end] == '@' && end > at + 1) {
                auto token = value.substr(at, end - at + 1);
                if (token == "@PORT_PREFIX@" || token == "@PORT_PREFIX_POSIX@") out += p.port_prefix;
                else if (token == "@PORT_PREFIX_MAKE@") out += escaped;
                else if (token == "@CC@") out += p.cc;
                else throw Error("placeholder " + token + " is outside the supported subset");
                at = end + 1;
                continue;
            }
        }
        out += value[at++];
    }
    return out;
}

void apply_port_env(std::map<std::string, std::string>& env, const std::vector<PortEnvOp>& ops, const PortPlaceholders& placeholders) {
    for (const auto& op : ops) {
        auto value = substitute_port_placeholders(op.value, placeholders);
        auto existing = env.find(op.name);
        if (op.append_flags && flag_vars().count(op.name) && existing != env.end() && !existing->second.empty()) existing->second += " " + value;
        else env[op.name] = value;
    }
}

}  // namespace crtcli
