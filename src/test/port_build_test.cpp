// Port executor: builds the reviewed zlib recipe through the pinned host-tools bundle and the SDK's crt-cc, in a job path
// that contains spaces and non-ASCII text. The inputs are machine-local, so the test names them in the environment and
// reports "skipped" when they are absent:
//   CRT_CLI_TEST_PORT_PROJECT        a project directory already configured for an SDK (holds .sdk.json)
//   CRT_CLI_TEST_PORT_HOST_TOOLS     a host-tools bundle directory
//   CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256  its manifest SHA-256
//   CRT_CLI_TEST_PORT_ARCHIVE        zlib-1.3.1.tar.gz (verified against the recipe's SHA-256 before use)
#include <crtcli/archive.h>
#include <crtcli/port_build.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
using namespace crtcli;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }
std::string getenv_or_empty(const char* name) { const char* v = std::getenv(name); return v ? v : ""; }
std::string error_of(const std::function<void()>& action) { try { action(); } catch (const Error& e) { return e.what(); } return {}; }
}  // namespace

int main() {
    const auto project = getenv_or_empty("CRT_CLI_TEST_PORT_PROJECT"), bundle = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS"),
               pin = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256"), archive = getenv_or_empty("CRT_CLI_TEST_PORT_ARCHIVE");
    if (project.empty() || bundle.empty() || pin.empty() || archive.empty()) {
        std::printf("port-build-test: skipped (CRT_CLI_TEST_PORT_* not set)\n");
        return 0;
    }
    std::string here = __FILE__;
    auto bytes = read_text(fs::path(here).parent_path() / "data" / "zlib.recipe.json");
    const std::string host_os = native_target().at("os").get<std::string>();
    const bool darwin = host_os == "macos";
    auto recipe = parse_port_recipe(bytes, host_os);
    check(file_sha256(archive) == recipe.source.sha256, "the supplied archive is the recipe's pinned source");

    const char* temporary = std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp";
    std::string pattern = std::string(temporary) + "/crt-port-build-XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    require(mkdtemp(buffer.data()) != nullptr, "cannot create a scratch directory");
    fs::path scratch = buffer.data();
    auto pristine = scratch / "pristine";
    fs::create_directory(pristine);
    extract_archive(archive, pristine);

    auto toolchain = Toolchain::load(project);
    auto tools = HostTools::load(bundle, pin);
    PortBuildRequest request;
    request.recipe = recipe;
    request.source_tree = pristine;
    request.test_root = toolchain.sdk;
    request.toolchain = &toolchain;
    request.tools = &tools;

    // ---- refusals before anything runs -------------------------------------------------------------------------------
    {
        auto bad = request;
        bad.job = scratch / "exists";
        fs::create_directory(bad.job);
        check(error_of([&] { run_port_build(bad); }).find("must be a new path") != std::string::npos, "an existing job path is refused");
        bad.job = toolchain.sdk / "job-inside-sdk";
        check(error_of([&] { run_port_build(bad); }).find("outside the host tools and the SDK") != std::string::npos, "a job inside the SDK is refused");
        check(!fs::exists(bad.job), "the refused job inside the SDK was not created");
        bad.job = tools.root / "job-inside-tools";
        check(error_of([&] { run_port_build(bad); }).find("outside the host tools and the SDK") != std::string::npos, "a job inside the host tools is refused");
        bad.job = scratch / "wrong test root";
        bad.test_root = scratch / "empty";
        fs::create_directory(bad.test_root);
        auto message = error_of([&] { run_port_build(bad); });
        check(message.find("test source is missing") != std::string::npos, "a missing test source fails the run (" + message + ")");
        check(!fs::exists(bad.job), "...before any job directory is created");
        fs::create_directories(bad.test_root / "porting/tests");
        write_text(bad.test_root / "porting/tests/zlib_roundtrip.c", "int main(void){return 0;}\n");
        message = error_of([&] { run_port_build(bad); });
        check(message.find("test source digest mismatch") != std::string::npos && !fs::exists(bad.job), "a test source other than the pinned one is refused before the build (" + message + ")");
    }

    // ---- the build -----------------------------------------------------------------------------------------------------
    request.job = scratch / "job space 한글";
    auto result = run_port_build(request);
    const auto& record = result.record;
    check(record.at("steps").size() == 7, "configure, make, make-install and two tests (build and run each), got " + std::to_string(record.at("steps").size()));
    check(record.at("patches").size() == 1 && record.at("patches")[0].at("replacements") == 1, "the generated Makefile prefix was patched once");
    for (const auto& step : record.at("steps")) check(step.at("exit_code") == 0 && !step.at("timed_out"), "step ok: " + step.at("name").get<std::string>());
    check(record.at("tests").size() == 2 && record.at("tests")[0].at("passed") == true && record.at("tests")[1].at("passed") == true, "both recipe tests passed");
    const std::vector<std::string> installed = darwin
        ? std::vector<std::string>{"include/zlib.h", "include/zconf.h", "lib/libz.a", "lib/libz.dylib", "lib/libz.1.dylib", "lib/libz.1.3.1.dylib", "lib/pkgconfig/zlib.pc"}
        : std::vector<std::string>{"include/zlib.h", "include/zconf.h", "lib/libz.a", "lib/libz.so", "lib/libz.so.1", "lib/libz.so.1.3.1", "lib/pkgconfig/zlib.pc"};
    for (const auto& relative : installed) check(fs::exists(result.prefix / relative), "installed: " + relative);
    check(fs::is_symlink(result.prefix / (darwin ? "lib/libz.dylib" : "lib/libz.so")) && fs::is_symlink(result.prefix / (darwin ? "lib/libz.1.dylib" : "lib/libz.so.1")), "shared aliases are symlinks");
    check(result.prefix.string().find(' ') != std::string::npos, "the install prefix contains a space");
    check(read_text(result.prefix / "lib/pkgconfig/zlib.pc").find("prefix=" + result.prefix.string()) != std::string::npos, "the pkg-config file records the real prefix");

    const auto& environment = record.at("environment");
    check(environment.at("CFLAGS") == (darwin ? "-O2" : "-O2 -O2 -fPIC"), "CFLAGS follow the oracle's append semantics");
    check(environment.at("CC").get<std::string>().rfind("/system/bin/sh /crt-bin/crt-cc", 0) == 0, "CC is the job wrapper started through the bundle shell");
    check(environment.at("PATH") == (tools.root / "system/bin").string(), "PATH holds only the host-tools directory");
    if (darwin) {
        // zlib's Darwin configure names the shared object by its install directory; the adapter's executor policy replaces that.
        check(environment.at("LDSHARED").get<std::string>().find("-install_name @rpath/libz.1.dylib") != std::string::npos, "LDSHARED carries the relocatable install name");
        check(environment.at("AR") == toolchain.archiver.string() && environment.at("RANLIB") == port_ranlib_for(toolchain.archiver).string(), "AR and RANLIB are the declared archiver and its ranlib");
    } else {
        check(environment.at("LDSHARED").get<std::string>().find("-Wl,-soname,libz.so.1") != std::string::npos, "LDSHARED comes from the recipe override");
    }
    check(!environment.contains("LD_LIBRARY_PATH") && !environment.contains("DYLD_LIBRARY_PATH"), "the build environment has no loader search path");

    if (darwin) {
        // The oracle runs the host's libtool here (zlib's Darwin configure forces it); the native executor must not.
        std::string make_log;
        for (const auto& step : record.at("steps")) if (step.at("name") == "make") make_log = read_text(result.logs / step.at("log").get<std::string>());
        check(!make_log.empty() && make_log.find("libtool") == std::string::npos, "the make step never ran libtool");
        check(make_log.find(toolchain.archiver.string() + " rc libz.a") != std::string::npos, "libz.a was archived by the declared archiver");
        std::string make_argv;
        for (const auto& step : record.at("steps")) if (step.at("name") == "make") for (const auto& arg : step.at("argv")) make_argv += arg.get<std::string>() + "\n";
        check(make_argv.find("AR=" + toolchain.archiver.string() + "\n") != std::string::npos && make_argv.find("ARFLAGS=rc\n") != std::string::npos, "the make command line carries the AR override");
    }

    // The produced shared object must be the one that was just linked, not the host's: it carries the recipe's SONAME / install name.
    auto so = read_text(result.prefix / (darwin ? "lib/libz.1.3.1.dylib" : "lib/libz.so.1.3.1"));
    check(so.find(darwin ? "@rpath/libz.1.dylib" : "libz.so.1") != std::string::npos, "the shared object names its SONAME / install name");
    if (darwin) check(so.find(result.prefix.string()) == std::string::npos, "the shared object records no job path");

    // An untouched pristine tree: the build ran on a copy.
    check(!fs::exists(pristine / recipe.source.source_dir / "configure.log") && !fs::exists(pristine / recipe.source.source_dir / "libz.a"), "the pristine source tree was not configured or built in place");

    std::error_code ignored;
    fs::remove_all(scratch, ignored);
    std::printf("port-build-test: %zu checks passed\n", checks);
    return 0;
}
