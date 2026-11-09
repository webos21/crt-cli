// D3 step 6: oracle differential. Builds the reviewed zlib recipe with the native executor and with the SDK's own
// crt-port-build.py (packaged SDK mode, same SDK, same pristine source) and compares the installed trees and the recipe
// test output. The oracle is only run here; the CLI never depends on it. Env (machine-local; "skipped" when absent):
//   CRT_CLI_TEST_PORT_PROJECT / _HOST_TOOLS / _HOST_TOOLS_SHA256 / _ARCHIVE   as in port-build-test
//   CRT_CLI_TEST_PORT_PYTHON   optional python3 (default: python3 from PATH)
#include <crtcli/archive.h>
#include <crtcli/port_build.h>
#include <crtcli/port_store.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <unistd.h>
using namespace crtcli;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }
std::string getenv_or_empty(const char* name) { const char* v = std::getenv(name); return v ? v : ""; }
struct Entry { std::string kind, value; };   // value: symlink target or content SHA-256
std::map<std::string, Entry> tree_of(const fs::path& root) {
    std::map<std::string, Entry> result;
    for (const auto& row : inventory(root)) result[row[0].get<std::string>()] = {row[1].get<std::string>(), row[2].get<std::string>()};
    return result;
}
std::string replace_all(std::string text, const std::string& from, const std::string& to) {
    for (size_t at = 0; (at = text.find(from, at)) != std::string::npos; at += to.size()) text.replace(at, from.size(), to);
    return text;
}
}  // namespace

int main() {
    const auto project = getenv_or_empty("CRT_CLI_TEST_PORT_PROJECT"), bundle = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS"),
               pin = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256"), archive = getenv_or_empty("CRT_CLI_TEST_PORT_ARCHIVE");
    if (project.empty() || bundle.empty() || pin.empty() || archive.empty()) {
        std::printf("port-oracle-test: skipped (CRT_CLI_TEST_PORT_* not set)\n");
        return 0;
    }
    auto python = getenv_or_empty("CRT_CLI_TEST_PORT_PYTHON");
    if (python.empty()) python = "python3";
    auto recipe = parse_port_recipe(read_text(fs::path(__FILE__).parent_path() / "data" / "zlib.recipe.json"), "linux");
    check(file_sha256(archive) == recipe.source.sha256, "the supplied archive is the recipe's pinned source");

    const char* temporary = std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp";
    std::string pattern = std::string(temporary) + "/crt-port-oracle-XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    require(mkdtemp(buffer.data()) != nullptr, "cannot create a scratch directory");
    fs::path scratch = buffer.data();
    auto pristine = scratch / "pristine";
    fs::create_directory(pristine);
    extract_archive(archive, pristine);

    auto toolchain = Toolchain::load(project);
    auto tools = HostTools::load(bundle, pin);
    auto sdk_before = inventory(toolchain.sdk);

    // ---- native executor -------------------------------------------------------------------------------------------------
    PortBuildRequest request;
    request.recipe = recipe;
    request.source_tree = pristine;
    request.test_root = toolchain.sdk;
    request.toolchain = &toolchain;
    request.tools = &tools;
    request.job = scratch / "native job";
    auto native = run_port_build(request);

    // ---- oracle: the SDK's own copy of the script, same SDK, same pristine tree ----------------------------------------
    auto oracle_root = scratch / "oracle";
    fs::create_directory(oracle_root);
    auto script = toolchain.sdk / "tools/crt-port-build.py";
    check(fs::is_regular_file(script), "the SDK ships the oracle script");
    auto oracle_prefix = oracle_root / "install";
    // The oracle uses the host's own shell and make (that is what it is); give it the minimal host PATH only.
    auto ran = execute({"/usr/bin/env", "-i", "PATH=/usr/local/bin:/usr/bin:/bin", "HOME=" + oracle_root.string(), python, "-I", script.string(), "--sdk-root", toolchain.sdk.string(), "--port", "zlib", "--test",
                        "--source-root", pristine.string(), "--work-root", (oracle_root / "work").string(),
                        "--install-prefix", oracle_prefix.string(), "--jobs", "4"},
                       900, false, true);
    write_text(oracle_root / "oracle.log", ran.output);
    check(ran.exit_code == 0 && !ran.timed_out, "the oracle build and its tests succeeded (log: " + (oracle_root / "oracle.log").string() + ")");

    // ---- 1. file inventory ---------------------------------------------------------------------------------------------
    auto ours = tree_of(native.prefix), theirs = tree_of(oracle_prefix);
    std::vector<std::string> only_ours, only_theirs, kind_differs, content_differs;
    for (const auto& [path, entry] : ours) {
        auto other = theirs.find(path);
        if (other == theirs.end()) only_ours.push_back(path);
        else if (other->second.kind != entry.kind || (entry.kind == "symlink" && other->second.value != entry.value)) kind_differs.push_back(path);
        else if (entry.kind == "file" && other->second.value != entry.value) content_differs.push_back(path);
    }
    for (const auto& item : theirs) if (!ours.count(item.first)) only_theirs.push_back(item.first);
    auto join = [](const std::vector<std::string>& v) { std::string s; for (auto& x : v) s += (s.empty() ? "" : ", ") + x; return s; };
    check(only_ours.empty(), "no installed path is missing from the oracle: " + join(only_ours));
    check(only_theirs.empty(), "no installed path is missing from the native build: " + join(only_theirs));
    check(kind_differs.empty(), "file kinds and symlink targets agree: " + join(kind_differs));
    std::printf("port-oracle-test: %zu paths in both trees; contents differ in: [%s]\n", ours.size(), join(content_differs).c_str());

    // ---- 2. content ------------------------------------------------------------------------------------------------------
    for (const auto& path : content_differs) {
        bool known = path == "lib/pkgconfig/zlib.pc" || path == "lib/libz.a" || path.rfind("lib/libz.so.", 0) == 0;
        check(known, "an unexpected content difference: " + path);
    }
    // The pkg-config file differs only in the install prefix.
    auto pc_ours = replace_all(read_text(native.prefix / "lib/pkgconfig/zlib.pc"), native.prefix.string(), "@PREFIX@");
    auto pc_theirs = replace_all(read_text(oracle_prefix / "lib/pkgconfig/zlib.pc"), oracle_prefix.string(), "@PREFIX@");
    check(pc_ours == pc_theirs, "zlib.pc is identical apart from the install prefix");
    for (const char* path : {"include/zlib.h", "include/zconf.h", "share/man/man3/zlib.3"})
        check(ours.at(path).value == theirs.at(path).value, std::string(path) + " is byte-identical");

    // ---- 3. ELF identity of the shared object -----------------------------------------------------------------------------
    auto elf_ours = read_elf_dynamic(native.prefix / "lib/libz.so.1.3.1"), elf_theirs = read_elf_dynamic(oracle_prefix / "lib/libz.so.1.3.1");
    check(elf_ours && elf_theirs, "both shared objects are readable ELF");
    check(elf_ours->soname == "libz.so.1" && elf_ours->soname == elf_theirs->soname, "SONAME agrees");
    check(elf_ours->machine == elf_theirs->machine && elf_ours->needed == elf_theirs->needed, "e_machine and DT_NEEDED agree");
    // Deliberate difference: crt-cc appends -Wl,-rpath,$CRT_PORT_RPATH_DIR to every link; the oracle leaves it set to its
    // install lib directory, the executor clears it for build steps (a published item must not name a job path).
    // Both carry crt-cc's standard "$ORIGIN:<sdk>/lib"; the oracle's has the extra install lib directory on the end.
    // (read_elf_dynamic reports DT_RUNPATH entries split on ':', so compare the joined text.)
    auto expected = elf_ours->runpath;
    expected.push_back((oracle_prefix / "lib").string());
    check(!elf_ours->runpath.empty() && elf_ours->rpath.empty() && elf_theirs->rpath.empty(), "the native shared object has the standard runpath only");
    std::string joined_ours;
    for (auto& r : elf_ours->runpath) joined_ours += r + ":";
    check(joined_ours.find(scratch.string()) == std::string::npos, "the native runpath names no job path");
    {
        auto split = [](const std::vector<std::string>& v) { std::string s; for (auto& x : v) s += (s.empty() ? "" : ":") + x; return s; };
        check(split(elf_theirs->runpath) == split(expected), "oracle runpath = native runpath + the oracle's install lib (" + split(elf_theirs->runpath) + ")");
    }
    std::printf("port-oracle-test: libz.a identical=%s libz.so.1.3.1 identical=%s\n",
                ours.at("lib/libz.a").value == theirs.at("lib/libz.a").value ? "yes" : "no",
                ours.at("lib/libz.so.1.3.1").value == theirs.at("lib/libz.so.1.3.1").value ? "yes" : "no");

    // ---- 4. recipe test output ------------------------------------------------------------------------------------------
    std::vector<std::string> oracle_lines;
    size_t from = 0;
    while (from < ran.output.size()) {
        auto end = ran.output.find('\n', from);
        auto line = ran.output.substr(from, end == std::string::npos ? std::string::npos : end - from);
        if (line.rfind("zlib_roundtrip_test:", 0) == 0) oracle_lines.push_back(line);
        if (end == std::string::npos) break;
        from = end + 1;
    }
    check(oracle_lines.size() == 2, "the oracle printed the roundtrip line twice, got " + std::to_string(oracle_lines.size()));
    check(native.record.at("tests").size() == 2, "the native build ran both recipe tests");
    for (size_t i = 0; i < 2; ++i) {
        auto actual = native.record.at("tests")[i].at("actual").get<std::string>();
        check(actual == oracle_lines[i], "recipe test output agrees (" + actual + " vs " + oracle_lines[i] + ")");
    }

    // ---- 5. the SDK is unchanged -----------------------------------------------------------------------------------------
    check(inventory(toolchain.sdk) == sdk_before, "neither build changed the SDK");

    std::error_code ignored;
    fs::remove_all(scratch, ignored);
    std::printf("port-oracle-test: %zu checks passed\n", checks);
    return 0;
}
