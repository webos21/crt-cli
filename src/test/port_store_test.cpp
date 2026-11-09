// Port audit, build key and store (D3 step 4). The ELF and audit cases mutate a real SDK shared object; the end-to-end part
// builds the reviewed zlib recipe with the step 3 executor and publishes it. Inputs are machine-local, named in the
// environment (the same ones as port-build-test), and the test reports "skipped" without them:
//   CRT_CLI_TEST_PORT_PROJECT, CRT_CLI_TEST_PORT_HOST_TOOLS, CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256, CRT_CLI_TEST_PORT_ARCHIVE
#include <crtcli/archive.h>
#include <crtcli/port_store.h>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <unistd.h>
using namespace crtcli;
namespace {
size_t checks = 0;
void check(bool condition, const std::string& message) { require(condition, "test failed: " + message); ++checks; }
std::string getenv_or_empty(const char* name) { const char* v = std::getenv(name); return v ? v : ""; }
std::string error_of(const std::function<void()>& action) { try { action(); } catch (const Error& e) { return e.what(); } return {}; }

bool has_rule(const PortAudit& audit, const std::string& rule) {
    for (const auto& finding : audit.findings) if (finding.rule == rule) return true;
    return false;
}

const std::string host_os = native_target().at("os").get<std::string>();
const bool darwin = host_os == "macos";

PortAuditRequest audit_of(const fs::path& root, const fs::path& sdk, std::vector<std::string> forbidden = {}) {
    PortAdapter adapter;
    if (darwin) {
        adapter.layouts["macos"] = {{"lib/libz.1.dylib"}, {"@rpath/libz.1.dylib"}, "lib/libz.a", "lib/libz.dylib"};
    } else {
        adapter.expected_files = {"lib/libz.so.1"};
        adapter.expected_sonames = {"libz.so.1"};
    }
    return {root, root, sdk, adapter, std::move(forbidden), {}, native_target().at("arch").get<std::string>(), host_os};
}

// A one-library item: the SDK's own libz with the alias layout the zlib install has.
void make_item(const fs::path& item, const fs::path& library) {
    fs::create_directories(item / "lib");
    if (darwin) {
        fs::copy_file(library, item / "lib/libz.1.3.1.dylib");
        fs::create_symlink("libz.1.3.1.dylib", item / "lib/libz.1.dylib");
        fs::create_symlink("libz.1.dylib", item / "lib/libz.dylib");
        return;
    }
    fs::copy_file(library, item / "lib/libz.so.1.3.1");
    fs::create_symlink("libz.so.1.3.1", item / "lib/libz.so.1");
    fs::create_symlink("libz.so.1", item / "lib/libz.so");
}

void patch_bytes(const fs::path& file, size_t offset, const std::string& replacement) {
    auto data = read_text(file);
    data.replace(offset, replacement.size(), replacement);
    write_text(file, data);
}
}  // namespace

int main() {
    const auto project = getenv_or_empty("CRT_CLI_TEST_PORT_PROJECT"), bundle = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS"),
               pin = getenv_or_empty("CRT_CLI_TEST_PORT_HOST_TOOLS_SHA256"), archive = getenv_or_empty("CRT_CLI_TEST_PORT_ARCHIVE");
    if (project.empty() || bundle.empty() || pin.empty() || archive.empty()) {
        std::printf("port-store-test: skipped (CRT_CLI_TEST_PORT_* not set)\n");
        return 0;
    }
    std::string here = __FILE__;
    auto recipe = parse_port_recipe(read_text(fs::path(here).parent_path() / "data" / "zlib.recipe.json"), host_os);
    const char* temporary = std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp";
    std::string pattern = std::string(temporary) + "/crt-port-store-XXXXXX";
    std::vector<char> buffer(pattern.begin(), pattern.end());
    buffer.push_back('\0');
    require(mkdtemp(buffer.data()) != nullptr, "cannot create a scratch directory");
    fs::path scratch = buffer.data();
    auto toolchain = Toolchain::load(project);
    auto tools = HostTools::load(bundle, pin);
    auto library = toolchain.sdk / (darwin ? "lib/libz.1.3.1.dylib" : "lib/libz.so.1.3.1");
    check(fs::is_regular_file(library), "the SDK ships its libz shared object (the audit fixture)");
    const std::string main_library = darwin ? "lib/libz.1.3.1.dylib" : "lib/libz.so.1.3.1";
    const std::string alias_name = darwin ? "lib/libz.1.dylib" : "lib/libz.so.1", link_name = darwin ? "lib/libz.dylib" : "lib/libz.so";

    if (!darwin) {
    // ---- ELF reader --------------------------------------------------------------------------------------------------
    {
        write_text(scratch / "text.txt", "not an ELF\n");
        check(!read_elf_dynamic(scratch / "text.txt").has_value(), "a text file is not ELF");
        auto info = read_elf_dynamic(library);
        check(info && info->kind == "shared" && info->soname == "libz.so.1", "the SDK libz is a shared object with SONAME libz.so.1");
        check(info && std::find(info->needed.begin(), info->needed.end(), "libc.so") != info->needed.end(), "its DT_NEEDED names the CRT libc (libc.so, not libc.so.6)");
        check(info && info->runpath == std::vector<std::string>{"$ORIGIN"}, "its runpath is $ORIGIN");
        check(info && info->machine == (native_target().at("arch") == "x86_64" ? 62u : 183u), "its machine is the target's");

        auto data = read_text(library);
        auto broken = [&](const std::string& name, const std::function<void(std::string&)>& mutate) {
            auto copy = data;
            mutate(copy);
            auto file = scratch / name;
            write_text(file, copy);
            return error_of([&] { read_elf_dynamic(file); });
        };
        check(!broken("short.so", [](std::string& d) { d.resize(100); }).empty(), "a truncated header is refused");
        check(!broken("half.so", [](std::string& d) { d.resize(d.size() / 4); }).empty(), "a file cut inside its dynamic section is refused");
        check(broken("class.so", [](std::string& d) { d[4] = 1; }).find("64-bit") != std::string::npos, "32-bit ELF is refused");
        check(broken("endian.so", [](std::string& d) { d[5] = 2; }).find("little-endian") != std::string::npos, "big-endian ELF is refused");
        check(broken("type.so", [](std::string& d) { d[16] = 4; }).find("unsupported ELF type") != std::string::npos, "a core file is refused");
        // Point the program header table past the end of the file.
        check(!broken("phoff.so", [](std::string& d) { for (int i = 0; i < 8; ++i) d[32 + i] = static_cast<char>(i == 7 ? 0x7f : 0xff); }).empty(), "a program header table outside the file is refused");
    }

    // ---- audit rules -------------------------------------------------------------------------------------------------
    {
        auto item = scratch / "item";
        make_item(item, library);
        auto clean = audit_port_item(audit_of(item, toolchain.sdk));
        check(clean.findings.empty(), "a library with the zlib alias layout and the SDK's DT_NEEDED passes" + (clean.findings.empty() ? std::string() : " (" + clean.findings[0].rule + ": " + clean.findings[0].detail + ")"));
        check(clean.libraries.size() == 1 && clean.libraries[0].at("soname") == "libz.so.1", "the audit lists the library");

        auto fresh = [&](const std::string& name) { auto d = scratch / name; make_item(d, library); return d; };
        {
            auto d = fresh("absolute");
            fs::create_symlink("/etc/passwd", d / "lib/abs");
            check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "symlink-absolute"), "an absolute symlink is a finding");
        }
        {
            auto d = fresh("escape");
            fs::create_symlink("../../../etc/passwd", d / "lib/out");
            check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "symlink-escapes"), "a symlink out of the item is a finding");
        }
        {
            auto d = fresh("dangling");
            fs::create_symlink("nothing.so", d / "lib/dangling");
            check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "symlink-dangling"), "a dangling symlink is a finding");
        }
        {
            auto d = fresh("noalias");
            fs::remove(d / "lib/libz.so.1");
            fs::remove(d / "lib/libz.so");
            auto audit = audit_port_item(audit_of(d, toolchain.sdk));
            check(has_rule(audit, "soname-alias") && has_rule(audit, "expected-file"), "a shared object without its SONAME file is a finding");
        }
        {
            auto d = fresh("missing");
            fs::remove_all(d / "lib");
            fs::create_directory(d / "lib");
            auto audit = audit_port_item(audit_of(d, toolchain.sdk));
            check(has_rule(audit, "expected-soname") && has_rule(audit, "expected-file"), "an install without the expected library is a finding");
        }
        {
            auto d = fresh("leak");
            write_text(d / "lib/note.txt", "built in /home/someone/job/prefix\n");
            auto audit = audit_port_item(audit_of(d, toolchain.sdk, {"/home/someone/job"}));
            check(has_rule(audit, "host-path-reference"), "a file naming the build job is a finding");
            check(!has_rule(audit_port_item(audit_of(d, toolchain.sdk, {"/elsewhere"})), "host-path-reference"), "...and only then");
        }
        {
            auto d = fresh("setuid");
            fs::permissions(d / "lib/libz.so.1.3.1", fs::perms::set_uid, fs::perm_options::add);
            check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "special-mode"), "a setuid file is a finding");
        }
        {
            auto d = fresh("runpath");
            auto data = read_text(d / "lib/libz.so.1.3.1");
            auto at = data.find("$ORIGIN");
            check(at != std::string::npos, "the fixture carries a $ORIGIN runpath string");
            patch_bytes(d / "lib/libz.so.1.3.1", at, "/tmp/xx");
            check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "runpath"), "an absolute runpath outside the item and the SDK is a finding");
            patch_bytes(d / "lib/libz.so.1.3.1", at, "$ORIGIN");
            check(!has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "runpath"), "$ORIGIN inside the item is accepted");
        }
        {
            auto d = fresh("needed");
            auto data = read_text(d / "lib/libz.so.1.3.1");
            auto at = data.find("libdl.so");
            check(at != std::string::npos, "the fixture names libdl.so");
            patch_bytes(d / "lib/libz.so.1.3.1", at, "libXY.so");
            check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "needed-not-in-sdk"), "a DT_NEEDED that neither the SDK nor the item provides is a finding");
        }
        {
            auto d = fresh("machine");
            patch_bytes(d / "lib/libz.so.1.3.1", 18, std::string("\x28\x00", 2));   // EM_ARM
            check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "machine"), "a binary for another machine is a finding");
        }
        {
            auto d = fresh("broken");
            patch_bytes(d / "lib/libz.so.1.3.1", 4, std::string("\x01", 1));
            check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "elf-unreadable"), "an ELF the reader cannot account for fails the audit instead of being skipped");
        }
        if (fs::exists("/bin/ls") && read_elf_dynamic("/bin/ls") && !read_elf_dynamic("/bin/ls")->needed.empty()) {
            auto d = fresh("host");
            fs::copy_file("/bin/ls", d / "lib/hostbin");
            check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "needed-not-in-sdk"), "a host binary linked against the host libc is a finding");
        }
        {
            auto d = fresh("skip");
            fs::create_directories(d / "evidence/logs");
            write_text(d / "evidence/logs/1.log", "/home/someone/job\n");
            write_text(d / "manifest.json", "{}\n");
            PortAuditRequest request = audit_of(d, toolchain.sdk, {"/home/someone/job"});
            request.skip_top_level = {"evidence", "manifest.json"};
            check(audit_port_item(request).findings.empty(), "skipped top-level names (evidence, manifest) are outside the audit");
        }
    }

    }

    if (darwin) {
        // ---- Mach-O reader -----------------------------------------------------------------------------------------------
        {
            write_text(scratch / "text.txt", "not a Mach-O\n");
            check(!read_macho_image(scratch / "text.txt").has_value(), "a text file is not Mach-O");
            auto info = read_macho_image(library);
            check(info && info->kind == "shared" && info->install_name == "@rpath/libz.1.dylib", "the SDK libz is a dylib with install name @rpath/libz.1.dylib");
            check(info && std::find(info->needed.begin(), info->needed.end(), "@rpath/libc.dylib") != info->needed.end() &&
                  std::find(info->needed.begin(), info->needed.end(), "/usr/lib/libSystem.B.dylib") != info->needed.end(), "it depends on the CRT libc and on libSystem");
            check(info && info->rpaths == std::vector<std::string>{"@loader_path"}, "its run-path is @loader_path");
            check(info && info->cpu_type == (native_target().at("arch") == "x86_64" ? 0x01000007u : 0x0100000cu), "its cputype is the target's");

            auto data = read_text(library);
            auto broken = [&](const std::string& name, const std::function<void(std::string&)>& mutate) {
                auto copy = data;
                mutate(copy);
                auto file = scratch / name;
                write_text(file, copy);
                return error_of([&] { read_macho_image(file); });
            };
            auto put32 = [](std::string& d, size_t at, uint32_t v) { for (int i = 0; i < 4; ++i) d[at + i] = static_cast<char>((v >> (8 * i)) & 0xff); };
            check(!broken("short.dylib", [](std::string& d) { d.resize(20); }).empty(), "a truncated header is refused");
            check(!broken("half.dylib", [](std::string& d) { d.resize(200); }).empty(), "a file cut inside its load commands is refused");
            check(broken("thirtytwo.dylib", [&](std::string& d) { put32(d, 0, 0xfeedface); }).find("64-bit") != std::string::npos, "32-bit Mach-O is refused");
            check(broken("bigendian.dylib", [&](std::string& d) { put32(d, 0, 0xcffaedfe); }).find("little-endian") != std::string::npos, "big-endian Mach-O is refused");
            check(broken("fat.dylib", [&](std::string& d) { put32(d, 0, 0xbebafeca); }).find("universal") != std::string::npos, "a universal (fat) file is refused");
            check(broken("core.dylib", [&](std::string& d) { put32(d, 12, 4); }).find("unsupported Mach-O file type") != std::string::npos, "a core file is refused");
            check(broken("ncmds.dylib", [&](std::string& d) { put32(d, 16, 0x7fffffff); }).find("more load commands") != std::string::npos, "more load commands than fit is refused");
            check(!broken("short-string-command.dylib", [&](std::string& d) {
                put32(d, 32, 0xd); put32(d, 36, 8);
            }).empty(), "a dylib string command shorter than its header is refused");
            check(!broken("metadata-string.dylib", [&](std::string& d) {
                put32(d, 32, 0xd); put32(d, 40, 12);
            }).empty(), "a dylib name cannot point into its version metadata");
            check(broken("cmdsize.dylib", [&](std::string& d) { put32(d, 36, 0); }).find("impossible size") != std::string::npos ||
                  !broken("cmdsize2.dylib", [&](std::string& d) { put32(d, 36, 0); }).empty(), "a load command with size zero is refused");
            // The first LC_ID_DYLIB/LC_LOAD_DYLIB-style command's name offset is moved past its own size.
            check(!broken("nameoff.dylib", [&](std::string& d) {
                      for (uint32_t offset = 32, i = 0; i < 64 && offset + 8 <= d.size(); ++i) {
                          uint32_t command = 0, size = 0;
                          for (int b = 3; b >= 0; --b) { command = (command << 8) | static_cast<unsigned char>(d[offset + b]); size = (size << 8) | static_cast<unsigned char>(d[offset + 4 + b]); }
                          if (command == 0xd || command == 0xc) { put32(d, offset + 8, size + 8); return; }
                          offset += size;
                      }
                  }).empty(), "a load command naming a string outside itself is refused");
        }

        // ---- audit rules -------------------------------------------------------------------------------------------------
        {
            auto item = scratch / "item";
            make_item(item, library);
            auto clean = audit_port_item(audit_of(item, toolchain.sdk));
            check(clean.findings.empty(), "a dylib with the zlib alias layout and the SDK's dependencies passes" + (clean.findings.empty() ? std::string() : " (" + clean.findings[0].rule + ": " + clean.findings[0].detail + ")"));
            check(clean.libraries.size() == 1 && clean.libraries[0].at("install_name") == "@rpath/libz.1.dylib", "the audit lists the library");

            auto fresh = [&](const std::string& name) { auto d = scratch / name; make_item(d, library); return d; };
            auto replace_in = [&](const fs::path& d, const std::string& from, const std::string& to) {
                auto text = read_text(d / "lib/libz.1.3.1.dylib");
                auto at = text.find(from);
                check(at != std::string::npos && from.size() == to.size(), "the fixture carries " + from);
                patch_bytes(d / "lib/libz.1.3.1.dylib", at, to);
            };
            { auto d = fresh("absolute"); fs::create_symlink("/etc/passwd", d / "lib/abs"); check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "symlink-absolute"), "an absolute symlink is a finding"); }
            { auto d = fresh("escape"); fs::create_symlink("../../../etc/passwd", d / "lib/out"); check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "symlink-escapes"), "a symlink out of the item is a finding"); }
            { auto d = fresh("dangling"); fs::create_symlink("nothing.dylib", d / "lib/dangling"); check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "symlink-dangling"), "a dangling symlink is a finding"); }
            {
                auto d = fresh("noalias");
                fs::remove(d / "lib/libz.1.dylib");
                fs::remove(d / "lib/libz.dylib");
                auto audit = audit_port_item(audit_of(d, toolchain.sdk));
                check(has_rule(audit, "install-name-alias") && has_rule(audit, "expected-file"), "a dylib without the file its install name points to is a finding");
            }
            { auto d = fresh("missing"); fs::remove_all(d / "lib"); fs::create_directory(d / "lib");
              auto audit = audit_port_item(audit_of(d, toolchain.sdk));
              check(has_rule(audit, "expected-soname") && has_rule(audit, "expected-file"), "an install without the expected library is a finding"); }
            {
                auto d = fresh("leak");
                write_text(d / "lib/note.txt", "built in /home/someone/job/prefix\n");
                check(has_rule(audit_port_item(audit_of(d, toolchain.sdk, {"/home/someone/job"})), "host-path-reference"), "a file naming the build job is a finding");
                check(!has_rule(audit_port_item(audit_of(d, toolchain.sdk, {"/elsewhere"})), "host-path-reference"), "...and only then");
            }
            {
                // Seatbelt may strip/refuse a new setuid bit. A replay can supply a pre-created data-only item; the
                // test still requires the real mode bit and the audit finding, never a skipped assertion.
                const char* fixture = std::getenv("CRT_CLI_TEST_PORT_SETUID_ITEM");
                auto d = fixture ? fs::path(fixture) : fresh("setuid");
                if (!fixture) fs::permissions(d / "lib/libz.1.3.1.dylib", fs::perms::set_uid, fs::perm_options::add);
                check((fs::status(d / "lib/libz.1.3.1.dylib").permissions() & fs::perms::set_uid) != fs::perms::none,
                      "the negative fixture really has a setuid mode bit");
                check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "special-mode"), "a setuid file is a finding");
            }
            {
                // The install name of zlib's own Darwin configure is the absolute install directory: not relocatable.
                auto d = fresh("installname");
                replace_in(d, "@rpath/libz.1.dylib", "/lib/xxxxxxxxxx.dyl");
                auto audit = audit_port_item(audit_of(d, toolchain.sdk));
                check(has_rule(audit, "install-name") && has_rule(audit, "expected-soname"), "an absolute install name is a finding");
            }
            {
                auto d = fresh("rpath");
                replace_in(d, "@loader_path", "/tmp/xxxxxxx");
                check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "runpath"), "an absolute LC_RPATH outside the item and the SDK is a finding");
                replace_in(d, "/tmp/xxxxxxx", "@loader_path");
                check(!has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "runpath"), "@loader_path inside the item is accepted");
                replace_in(d, "@loader_path", "relative_xxx");
                check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "runpath"), "a relative LC_RPATH entry is a finding");
            }
            {
                auto d = fresh("needed");
                replace_in(d, "@rpath/libm.dylib", "@rpath/libQ.dylib");
                check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "needed-not-in-sdk"), "an @rpath dependency that neither the SDK nor the item provides is a finding");
            }
            {
                auto d = fresh("hostlib");
                replace_in(d, "/usr/lib/libSystem.B.dylib", "/usr/lib/libxxxxxx.B.dylib");
                check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "needed-not-in-sdk"), "a dependency on another system library is a finding");
            }
            {
                auto d = fresh("machine");
                const bool arm = native_target().at("arch") == "aarch64";
                patch_bytes(d / "lib/libz.1.3.1.dylib", 4, arm ? std::string("\x07\x00\x00\x01", 4) : std::string("\x0c\x00\x00\x01", 4));
                check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "machine"), "a binary for another CPU is a finding");
            }
            { auto d = fresh("broken"); patch_bytes(d / "lib/libz.1.3.1.dylib", 0, std::string("\xce\xfa\xed\xfe", 4));
              check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "macho-unreadable"), "a Mach-O the reader cannot account for fails the audit instead of being skipped"); }
            {
                auto d = fresh("elf");
                write_text(d / "lib/foreign.so", std::string("\x7f" "ELF") + std::string(100, '\0'));
                check(has_rule(audit_port_item(audit_of(d, toolchain.sdk)), "foreign-format"), "an ELF image in a macOS item is a finding");
            }
            {
                auto d = fresh("skip");
                fs::create_directories(d / "evidence/logs");
                write_text(d / "evidence/logs/1.log", "/home/someone/job\n");
                write_text(d / "manifest.json", "{}\n");
                PortAuditRequest request = audit_of(d, toolchain.sdk, {"/home/someone/job"});
                request.skip_top_level = {"evidence", "manifest.json"};
                check(audit_port_item(request).findings.empty(), "skipped top-level names (evidence, manifest) are outside the audit");
            }
        }
    }

    // ---- build key ---------------------------------------------------------------------------------------------------
    auto pristine = scratch / "pristine";
    fs::create_directory(pristine);
    check(file_sha256(archive) == recipe.source.sha256, "the supplied archive is the recipe's pinned source");
    extract_archive(archive, pristine);
    PortBuildRequest request;
    request.recipe = recipe;
    request.source_tree = pristine;
    request.test_root = toolchain.sdk;
    request.toolchain = &toolchain;
    request.tools = &tools;
    request.job = scratch / "job one";
    auto inputs = port_key_inputs(request);
    auto key = port_build_key(inputs);
    check(key.size() == 64 && key == port_build_key(port_key_inputs(request)), "the key is a SHA-256 and repeatable");
    if (darwin) {
        check(inputs.at("os_sdk").at("settings_sha256") == file_sha256(port_os_sdk(toolchain) / "SDKSettings.json"),
              "the configured OS SDK metadata is a key input");
        auto moved_toolchain = toolchain;
        moved_toolchain.os_sdk = (scratch / "OS SDK copy").string();
        fs::create_directory(moved_toolchain.os_sdk);
        fs::copy_file(port_os_sdk(toolchain) / "SDKSettings.json", fs::path(moved_toolchain.os_sdk) / "SDKSettings.json");
        auto moved_request = request; moved_request.toolchain = &moved_toolchain;
        check(port_build_key(port_key_inputs(moved_request)) == key, "the OS SDK's location is not a key input");
        write_text(fs::path(moved_toolchain.os_sdk) / "SDKSettings.json", "{\"Version\":\"changed-test-fixture\"}\n");
        check(port_build_key(port_key_inputs(moved_request)) != key, "changed OS SDK metadata changes the build key");
    } else {
        check(!inputs.contains("os_sdk") && !inputs.at("environment").contains("executor"), "Linux key inputs keep their previous shape");
    }
    {
        auto other = request;
        other.job = scratch / "another job elsewhere";
        other.jobs = 3;
        other.step_timeout_seconds = 5;
        check(port_build_key(port_key_inputs(other)) == key, "the job directory, -j and the step timeout are not key inputs");
        check(inputs.dump().find(scratch.string()) == std::string::npos, "no scratch path appears in the key inputs");

        auto changed = request;
        changed.recipe.env.push_back({"EXTRA", "1", false});
        check(port_build_key(port_key_inputs(changed)) != key, "a different declared environment changes the key");
        changed = request;
        changed.recipe.env[0].value += " ";
        check(port_build_key(port_key_inputs(changed)) != key, "a different environment value changes the key");
        changed = request;
        changed.recipe.source.sha256 = std::string(64, '0');
        check(port_build_key(port_key_inputs(changed)) != key, "a different source digest changes the key");

        auto compiler_copy = toolchain;
        const char* true_binary = fs::exists("/bin/true") ? "/bin/true" : "/usr/bin/true";
        compiler_copy.cc = true_binary;
        changed = request;
        changed.toolchain = &compiler_copy;
        check(port_build_key(port_key_inputs(changed)) != key, "a different compiler binary changes the key");
        compiler_copy = toolchain;
        compiler_copy.linker = true_binary;
        check(port_build_key(port_key_inputs(changed)) != key, "a different linker binary changes the key");

        auto tools_copy = tools;
        tools_copy.manifest["content_sha256"] = std::string(64, '1');
        changed = request;
        changed.tools = &tools_copy;
        check(port_build_key(port_key_inputs(changed)) != key, "a different host-tools bundle changes the key");

        // The SDK's identity is read from its content: a directory with other bytes is another SDK.
        auto fake_sdk = scratch / "fake-sdk";
        fs::create_directories(fake_sdk);
        write_text(fake_sdk / "VERSION", "9.9.9\n");
        write_text(fake_sdk / "file", "x\n");
        auto toolchain_copy = toolchain;
        toolchain_copy.sdk = fake_sdk;
        changed = request;
        changed.toolchain = &toolchain_copy;
        auto first = port_build_key(port_key_inputs(changed));
        check(first != key, "a different SDK content identity changes the key");
        write_text(fake_sdk / "file", "y\n");
        check(port_build_key(port_key_inputs(changed)) != first, "...and so does one changed byte in it");
    }

    // ---- build, refuse bad results, publish ---------------------------------------------------------------------------
    auto result = run_port_build(request);
    PortStore store(scratch / "crt home 한글" / "ports");
    {
        // A finished build whose prefix names its job directory is refused, and nothing is published or left staged.
        write_text(result.prefix / "share/leak.txt", "built in " + result.prefix.string() + "\n");
        auto message = error_of([&] { store.publish(request, result, inputs); });
        check(message.find("host-path-reference") != std::string::npos, "a leaked job path stops the publication (" + message + ")");
        check(store.list().empty() && fs::is_empty(store.root() / ".staging"), "the refused publication left no item and no staging");
        fs::remove(result.prefix / "share/leak.txt");

        auto failed = result;
        failed.record["tests"][0]["passed"] = false;
        check(error_of([&] { store.publish(request, failed, inputs); }).find("did not pass") != std::string::npos, "a failing recipe test stops the publication");
        failed = result;
        failed.record["tests"].erase(failed.record["tests"].begin());
        check(error_of([&] { store.publish(request, failed, inputs); }).find("does not hold every recipe test") != std::string::npos, "a result with missing tests is refused");
        auto stale = inputs;
        stale["sdk"]["content_sha256"] = std::string(64, '2');
        check(error_of([&] { store.publish(request, result, stale); }).find("SDK changed") != std::string::npos, "inputs naming another SDK are refused");
        auto other_recipe = inputs;
        other_recipe["recipe"]["sha256"] = std::string(64, '3');
        check(!error_of([&] { store.publish(request, result, other_recipe); }).empty(), "inputs for another recipe are refused");
        check(store.list().empty(), "none of the refused publications created an item");
    }

    auto sdk_before = read_identity(toolchain.sdk).content_sha256;
    auto published = store.publish(request, result, inputs);
    check(!published.already_published && published.build_key == key && published.path == store.item_path(key), "the build was published under its key");
    check(store.list() == std::vector<std::string>{key}, "the store lists the item");
    const auto& item = published.path;
    for (const std::string relative : {std::string("include/zlib.h"), std::string("include/zconf.h"), std::string("lib/libz.a"), main_library, std::string("manifest.json"), std::string("evidence/logs/1-configure.log")})
        check(fs::is_regular_file(item / relative), "the item holds " + relative);
    check(fs::is_symlink(item / alias_name) && fs::read_symlink(item / alias_name) == fs::path(main_library).filename(), "the SONAME / install-name alias is a relative symlink");
    auto pc = read_text(item / "lib/pkgconfig/zlib.pc");
    check(pc.find("prefix=" + item.string()) != std::string::npos && pc.find(request.job.string()) == std::string::npos,
          "zlib.pc names the item, not the job");
    check(published.manifest.at("relocations").size() == 1 && published.manifest.at("relocations")[0].at("file") == "lib/pkgconfig/zlib.pc",
          "the one relocation is recorded");
    auto manifest = read_json(item / "manifest.json");
    check(manifest == published.manifest, "the returned manifest is the stored one");
    check(manifest.at("build_key") == key && manifest.at("key_inputs") == inputs && manifest.at("tests").size() == 2, "the manifest records the key, its inputs and the tests");
    check(manifest.at("libraries").size() == 1 && manifest.at("libraries")[0].at(darwin ? "install_name" : "soname") == (darwin ? "@rpath/libz.1.dylib" : "libz.so.1"), "the manifest lists the shared object");
    check(manifest.dump().find(result.prefix.parent_path().string()) == std::string::npos, "the manifest names no build-job path");
    check(!manifest.at("unsupported_scope").empty() && manifest.at("crt_review") == "pending", "unsupported scope and the pending CRT review are stated");
    check(store.verify(key, toolchain.sdk) == manifest.at("files").size() + 1, "verify accepts the item and counts the manifest too");
    check(read_identity(toolchain.sdk).content_sha256 == sdk_before, "the SDK is byte-identical after the publication");

    // The same finished build again is idempotent.
    auto again = store.publish(request, result, inputs);
    check(again.already_published && again.path == item, "publishing the same key again changes nothing");

    // ---- concurrent publication of one key ----------------------------------------------------------------------------
    {
        PortStore other(scratch / "concurrent" / "ports");
        other.lock_wait_seconds = 120;
        PortPublishResult first, second;
        std::string failure_one, failure_two;
        std::thread one([&] { try { first = other.publish(request, result, inputs); } catch (const std::exception& e) { failure_one = e.what(); } });
        std::thread two([&] { try { second = other.publish(request, result, inputs); } catch (const std::exception& e) { failure_two = e.what(); } });
        one.join(); two.join();
        check(failure_one.empty() && failure_two.empty(), "both concurrent publications finish (" + failure_one + failure_two + ")");
        check(first.already_published != second.already_published, "exactly one of two concurrent publications created the item");
        check(other.list() == std::vector<std::string>{key} && other.verify(key, toolchain.sdk) > 0, "the concurrently published item verifies");
        check(fs::is_empty(other.root() / ".staging") && fs::is_empty(other.root() / ".locks"), "no staging or lock is left behind");
    }

    // ---- verification catches changes ---------------------------------------------------------------------------------
    {
        auto header = item / "include/zlib.h";
        auto original = read_text(header);
        write_text(header, original + "/* tampered */\n");
        check(error_of([&] { store.verify(key, toolchain.sdk); }).find("differ from its manifest inventory") != std::string::npos, "a changed file fails verify");
        check(error_of([&] { store.publish(request, result, inputs); }).find("differ") != std::string::npos, "a damaged item is not silently replaced by a publication");
        write_text(header, original);
        check(store.verify(key, toolchain.sdk) > 0, "restoring the bytes verifies again");

        write_text(item / "include/extra.h", "x\n");
        check(error_of([&] { store.verify(key, toolchain.sdk); }).find("differ") != std::string::npos, "an added file fails verify");
        fs::remove(item / "include/extra.h");
        auto alias_target = fs::read_symlink(item / link_name);
        fs::remove(item / link_name);
        check(error_of([&] { store.verify(key, toolchain.sdk); }).find("differ") != std::string::npos, "a removed alias fails verify");
        fs::create_symlink(alias_target, item / link_name);

        auto stored = manifest;
        stored["build_key"] = std::string(64, '4');
        write_text(item / "manifest.json", stored.dump(2) + "\n");
        check(error_of([&] { store.verify(key, toolchain.sdk); }).find("names another build key") != std::string::npos, "a manifest naming another key fails verify");
        stored = manifest;
        stored["key_inputs"]["variants"] = Json::array({"static"});
        write_text(item / "manifest.json", stored.dump(2) + "\n");
        check(error_of([&] { store.verify(key, toolchain.sdk); }).find("does not match its recorded inputs") != std::string::npos, "inputs edited after the fact no longer produce the key");
        write_text(item / "manifest.json", manifest.dump(2) + "\n");
        check(store.verify(key, toolchain.sdk) > 0, "the restored manifest verifies");
        check(error_of([&] { store.verify(std::string(64, '5'), toolchain.sdk); }).find("no such port store item") != std::string::npos, "an unknown key is reported");
        check(!error_of([&] { store.verify("../" + key, toolchain.sdk); }).empty(), "a key that is not 64 hex digits is refused");
    }

    // ---- recovery of an interrupted publication -----------------------------------------------------------------------
    {
        auto dead = std::to_string(4194000);
        fs::create_directories(store.root() / ".staging" / "4194000-1" / "item");
        write_text(store.root() / ".staging" / "4194000-1" / "txn.json", Json{{"pid", 4194000}, {"state", "staging"}}.dump());
        fs::create_directories(store.root() / ".locks" / "stale.lock");
        write_text(store.root() / ".locks" / "stale.lock" / "owner", dead + "\n");
        fs::create_directories(store.root() / ".staging" / (std::to_string(getpid()) + "-live"));
        write_text(store.root() / ".staging" / (std::to_string(getpid()) + "-live") / "txn.json", Json{{"pid", static_cast<long>(getpid())}}.dump());
        auto removed = store.recover();
        check(removed.size() == 2 && !fs::exists(store.root() / ".staging" / "4194000-1") && !fs::exists(store.root() / ".locks" / "stale.lock"),
              "recover removes staging and locks whose owner is gone");
        check(fs::exists(store.root() / ".staging" / (std::to_string(getpid()) + "-live")), "...and keeps those of a live process");
        check(store.list() == std::vector<std::string>{key}, "published items are untouched by recovery");
    }

    std::error_code ignored;
    fs::remove_all(scratch, ignored);
    std::printf("port-store-test: %zu checks passed\n", checks);
    return 0;
}
