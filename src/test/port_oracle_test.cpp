// D3 step 6: oracle differential. Builds the reviewed zlib recipe with the native executor and with the SDK's own
// crt-port-build.py (packaged SDK mode, same SDK, same pristine source) and compares the installed trees and the recipe
// test output. The oracle is only run here; the CLI never depends on it. Env (machine-local; "skipped" when absent):
//   CRT_CLI_TEST_PORT_PROJECT / _HOST_TOOLS / _HOST_TOOLS_SHA256 / _ARCHIVE   as in port-build-test
//   CRT_CLI_TEST_PORT_PYTHON   optional python3 (default: python3 from PATH)
#include <crtcli/archive.h>
#include <crtcli/environment.h>
#include <crtcli/port_build.h>
#include <crtcli/port_store.h>
#include <algorithm>
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
// The members of a BSD or GNU `ar` archive by name, without the symbol-table members: the archive headers carry dates and
// owners and the two tools lay the files out differently, so only the member bytes are comparable.
std::map<std::string, std::string> archive_members(const fs::path& file) {
    auto data = read_text(file);
    require(data.compare(0, 8, "!<arch>\n") == 0, "not an ar archive: " + file.string());
    std::map<std::string, std::string> members;
    for (size_t at = 8; at + 60 <= data.size();) {
        auto name = data.substr(at, 16);
        while (!name.empty() && name.back() == ' ') name.pop_back();
        size_t size = std::stoul(data.substr(at + 48, 10));
        size_t begin = at + 60;
        require(begin + size <= data.size(), "ar member is truncated");
        std::string content = data.substr(begin, size);
        if (name.rfind("#1/", 0) == 0) {
            size_t length = std::stoul(name.substr(3));
            name = content.substr(0, length);
            name = name.substr(0, name.find('\0'));
            content = content.substr(length);
        }
        if (!name.empty() && name.back() == '/') name.pop_back();
        if (name.rfind("__.SYMDEF", 0) != 0 && name != "" && name != "/" && name != "//") members[name] = content;
        at = begin + size + (size % 2);
    }
    return members;
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
    const std::string host_os = native_target().at("os").get<std::string>();
    const bool darwin = host_os == "macos";
    const bool windows = host_os == "windows";
    auto recipe = parse_port_recipe(read_text(fs::path(__FILE__).parent_path() / "data" / "zlib.recipe.json"), host_os);
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
    // The oracle selects its own interpreters/tools (packaged SDK make on macOS); give it only a minimal host PATH.
    std::vector<std::string> oracle_command{"/usr/bin/env", "-i", "PATH=/usr/local/bin:/usr/bin:/bin", "HOME=" + oracle_root.string(),
                                          "TMPDIR=" + scratch.string(), "CRT_HOST_CC=" + toolchain.cc.string(), "CRT_HOST_CXX=" + toolchain.cxx.string()};
    if (darwin) {
        oracle_command.push_back("SDKROOT=" + port_os_sdk(toolchain).string());
        // /usr/bin/ranlib is an xcrun launcher even though it resolves to the same declared Xcode libtool. Do not let its
        // system cache writes escape the replay, or contaminate configure's no-warning capability probes.
        oracle_command.push_back("xcrun_nocache=1");
    }
    oracle_command.insert(oracle_command.end(), {python, "-I", script.string(), "--sdk-root", toolchain.sdk.string(), "--port", "zlib", "--test",
                        "--source-root", pristine.string(), "--work-root", (oracle_root / "work").string(),
                        "--install-prefix", oracle_prefix.string(), "--jobs", "4"});
    ProcessResult ran;
    if (windows) {
        // No env(1): the oracle gets an explicit environment. The CRT shell takes forward slashes, and the LLVM directory is on PATH
        // so the script can find the same host tools the executor was given.
        auto slashes = [](const fs::path& p) { return p.generic_string(); };
        auto system_root = getenv_or_empty("SystemRoot");
        ChildEnvironment environment{{"HOME=" + oracle_root.string(), "USERPROFILE=" + oracle_root.string(), "TMPDIR=" + scratch.string(),
                                      "TEMP=" + scratch.string(), "TMP=" + scratch.string(), "PYTHONUTF8=1", "SystemRoot=" + system_root,
                                      "CRT_SPAWN_NATIVE_WINDOWS=1",   // CRT's posix_spawn redirects a non-CRT child's stdio only when told so
                                      "PATH=" + toolchain.cc.parent_path().string() + ";" + system_root + "\\System32;" + system_root,
                                      "CRT_HOST_CC=" + slashes(toolchain.cc), "CRT_HOST_CXX=" + slashes(toolchain.cxx),
                                      "CRT_WINDOWS_SDK_LIBPATH=" + slashes(port_os_sdk(toolchain))}, Json::object(), Json::array()};
        std::vector<std::string> direct{python, "-I", script.string(), "--sdk-root", toolchain.sdk.string(), "--port", "zlib", "--test",
                                        "--source-root", pristine.string(), "--work-root", (oracle_root / "work").string(),
                                        "--install-prefix", oracle_prefix.string(), "--jobs", "4"};
        ran = execute_child(direct, environment, {}, 900, false);
    } else {
        ran = execute(oracle_command, 900, false, true);
    }
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
    if (windows) {
        // crt-port-build.py aliases every installed libfoo.a as foo.lib so that a *later* port's plain -lfoo resolves (lld-link looks
        // for foo.lib). The executor has no dependent ports in this slice and installs only what zlib's make install installs.
        auto alias = std::find(only_theirs.begin(), only_theirs.end(), "lib/z.lib");
        if (alias != only_theirs.end()) {
            check(theirs.at("lib/z.lib").kind == "file" && theirs.at("lib/z.lib").value == theirs.at("lib/libz.a").value, "the oracle-only lib/z.lib is a byte copy of its libz.a");
            only_theirs.erase(alias);
        }
    }
    auto join = [](const std::vector<std::string>& v) { std::string s; for (auto& x : v) s += (s.empty() ? "" : ", ") + x; return s; };
    check(only_ours.empty(), "no installed path is missing from the oracle: " + join(only_ours));
    check(only_theirs.empty(), "no installed path is missing from the native build: " + join(only_theirs));
    check(kind_differs.empty(), "file kinds and symlink targets agree: " + join(kind_differs));
    std::printf("port-oracle-test: %zu paths in both trees; contents differ in: [%s]\n", ours.size(), join(content_differs).c_str());

    // ---- 2. content ------------------------------------------------------------------------------------------------------
    for (const auto& path : content_differs) {
        bool known = path == "lib/pkgconfig/zlib.pc" || path == "lib/libz.a" || path.rfind(darwin ? "lib/libz.1." : "lib/libz.so.", 0) == 0;
        check(known, "an unexpected content difference: " + path);
    }
    // The pkg-config file differs only in the install prefix.
    auto pc_ours = replace_all(replace_all(read_text(native.prefix / "lib/pkgconfig/zlib.pc"), native.prefix.generic_string(), "@PREFIX@"), native.prefix.string(), "@PREFIX@");
    // The oracle resolves its --install-prefix (macOS: /var is a symlink to /private/var), so both spellings are normalized.
    auto pc_theirs = replace_all(replace_all(read_text(oracle_prefix / "lib/pkgconfig/zlib.pc"), fs::canonical(oracle_prefix).string(), "@PREFIX@"), oracle_prefix.string(), "@PREFIX@");
    if (windows) {
        // The oracle hands the CRT shell /c/Users/... where the executor hands C:/Users/...; both name the same directory.
        auto posix = oracle_prefix.generic_string();
        if (posix.size() > 2 && posix[1] == ':') posix = "/" + std::string(1, static_cast<char>(std::tolower(static_cast<unsigned char>(posix[0])))) + posix.substr(2);
        pc_theirs = replace_all(replace_all(pc_theirs, posix, "@PREFIX@"), oracle_prefix.generic_string(), "@PREFIX@");
    }
    check(pc_ours == pc_theirs, "zlib.pc is identical apart from the install prefix");
    for (const char* path : {"include/zlib.h", "include/zconf.h", "share/man/man3/zlib.3"})
        check(ours.at(path).value == theirs.at(path).value, std::string(path) + " is byte-identical");

    if (windows) {
        // ---- 3. PE identity of the shared object ---------------------------------------------------------------------------
        auto pe_ours = read_pe_image(native.prefix / "lib/libz.so.1.3.1"), pe_theirs = read_pe_image(oracle_prefix / "lib/libz.so.1.3.1");
        check(pe_ours && pe_theirs && pe_ours->kind == "shared" && pe_theirs->kind == "shared", "both shared objects are readable PE DLLs");
        check(pe_ours->dll_name == "libz.so.1.3.1" && pe_ours->dll_name == pe_theirs->dll_name, "the export DLL name agrees (the POSIX spelling is kept)");
        check(pe_ours->machine == pe_theirs->machine && pe_ours->needed == pe_theirs->needed, "machine and imports agree");
        // PE has no run path: unlike ELF and Mach-O, there is nothing job-specific for the two builds to disagree about.
        std::printf("port-oracle-test: libz.a identical=%s libz.so.1.3.1 identical=%s\n",
                    ours.at("lib/libz.a").value == theirs.at("lib/libz.a").value ? "yes" : "no",
                    ours.at("lib/libz.so.1.3.1").value == theirs.at("lib/libz.so.1.3.1").value ? "yes" : "no");
    }
    if (!darwin && !windows) {
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

    }
    if (darwin) {
        // ---- 3. Mach-O identity of the shared object -----------------------------------------------------------------------
        auto image_ours = read_macho_image(native.prefix / "lib/libz.1.3.1.dylib"), image_theirs = read_macho_image(oracle_prefix / "lib/libz.1.3.1.dylib");
        check(image_ours && image_theirs && image_ours->kind == "shared" && image_theirs->kind == "shared", "both shared objects are readable Mach-O dylibs");
        check(image_ours->cpu_type == image_theirs->cpu_type && image_ours->needed == image_theirs->needed, "cputype and the dependency list agree");
        // Deliberate difference (docs/design/source_port_adapter.md, macOS): zlib's Darwin configure names the shared object by
        // its install directory, which is the oracle's temporary install prefix. The executor's policy gives it a relocatable
        // install name, the Mach-O counterpart of SONAME libz.so.1, so a published item can move with the store.
        check(image_ours->install_name == "@rpath/libz.1.dylib", "the native install name is relocatable");
        check(image_theirs->install_name == (fs::canonical(oracle_prefix) / "lib/libz.1.dylib").string() || image_theirs->install_name == (oracle_prefix / "lib/libz.1.dylib").string(), "the oracle's install name is its install directory (" + image_theirs->install_name + ")");
        // crt-cc emits @loader_path for a dylib when CRT_PORT_RPATH_DIR is set. The oracle sets it for every link, whereas
        // the executor deliberately clears it during configure/build and sets it only for the unpublished test binaries.
        auto expected_rpaths = image_ours->rpaths;
        expected_rpaths.push_back("@loader_path");
        check(!image_ours->rpaths.empty() && image_theirs->rpaths == expected_rpaths,
              "oracle LC_RPATH = native LC_RPATH + @loader_path");
        std::printf("port-oracle-test: native LC_RPATH=[%s] oracle LC_RPATH=[%s]\n",
                    join(image_ours->rpaths).c_str(), join(image_theirs->rpaths).c_str());
        for (const auto& entry : image_ours->rpaths) check(entry.find(scratch.string()) == std::string::npos, "the native LC_RPATH names no job path");
        // The static archive: the oracle's `libtool -o` is the host's tool (zlib's configure forces it); the native build used
        // the declared `ar` + `ranlib`. The headers differ, the objects inside must not.
        auto members_ours = archive_members(native.prefix / "lib/libz.a"), members_theirs = archive_members(oracle_prefix / "lib/libz.a");
        check(members_ours.size() == 15 && members_ours.size() == members_theirs.size(), "both archives hold the 15 zlib objects, got " + std::to_string(members_ours.size()) + " and " + std::to_string(members_theirs.size()));
        size_t identical = 0;
        for (const auto& member : members_ours) {
            auto other = members_theirs.find(member.first);
            check(other != members_theirs.end(), "the oracle's archive has " + member.first);
            if (other->second == member.second) ++identical;
        }
        check(identical == members_ours.size(), "every archive member is byte-identical, " + std::to_string(identical) + " of " + std::to_string(members_ours.size()));
        std::printf("port-oracle-test: libz.a bytes identical=%s (members identical: %zu of %zu) libz.1.3.1.dylib identical=%s\n",
                    ours.at("lib/libz.a").value == theirs.at("lib/libz.a").value ? "yes" : "no", identical, members_ours.size(),
                    ours.at("lib/libz.1.3.1.dylib").value == theirs.at("lib/libz.1.3.1.dylib").value ? "yes" : "no");
    }

    // ---- 4. recipe test output ------------------------------------------------------------------------------------------
    std::vector<std::string> oracle_lines;
    size_t from = 0;
    while (from < ran.output.size()) {
        auto end = ran.output.find('\n', from);
        auto line = ran.output.substr(from, end == std::string::npos ? std::string::npos : end - from);
        while (!line.empty() && line.back() == '\r') line.pop_back();   // a Windows child writes CRLF
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
