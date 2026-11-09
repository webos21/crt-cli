#pragma once
// D3 step 4: the binary audit of a finished port build, the build key, and the port store (`ports/store/<build-key>/`).
// See docs/design/source_port_adapter.md D3-7/D3-8 and docs/design/crt_cli.md §5. Linux ELF and macOS Mach-O only for now;
// a prefix that holds a binary this module cannot read is refused, never skipped.
//
// A store item is published only after the recipe tests passed and the audit found nothing; it is created in private
// staging on the store's file system and appears with one directory rename. Failed builds never become items.
#include "crtcli/port_build.h"
#include "crtcli/sdk_store.h"
#include <optional>

namespace crtcli {

// ---- ELF reader -------------------------------------------------------------------------------------------------------
struct ElfDynamic {
    std::string kind;                         // "shared", "executable" or "relocatable"
    unsigned machine = 0;                     // e_machine
    std::string soname;
    std::vector<std::string> needed, rpath, runpath;
};
// nullopt when the file does not start with the ELF magic. Throws Error for an ELF image this reader cannot account for
// (not 64-bit little-endian, truncated tables, strings outside the string table, ...): the audit fails closed.
std::optional<ElfDynamic> read_elf_dynamic(const fs::path& file);

// ---- Mach-O reader ----------------------------------------------------------------------------------------------------
// The install-relevant load commands of a 64-bit little-endian Mach-O image. A fat/universal file, a 32-bit or big-endian
// image, or any table this reader cannot account for is an error, never a skip.
struct MachoImage {
    std::string kind;                         // "shared" (MH_DYLIB), "executable" or "relocatable"
    unsigned cpu_type = 0;                    // CPU_TYPE_ARM64 0x0100000c, CPU_TYPE_X86_64 0x01000007
    std::string install_name;                 // LC_ID_DYLIB (empty for anything but a dylib)
    std::vector<std::string> needed;          // LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, LC_REEXPORT_DYLIB, LC_LOAD_UPWARD_DYLIB
    std::vector<std::string> rpaths;          // LC_RPATH
};
// nullopt when the file does not start with a Mach-O magic. Throws Error for an image this reader cannot account for.
std::optional<MachoImage> read_macho_image(const fs::path& file);

// ---- audit ------------------------------------------------------------------------------------------------------------
struct PortAuditFinding {
    std::string path, rule, detail;
    Json to_json() const { return {{"path", path}, {"rule", rule}, {"detail", detail}}; }
};
struct PortAuditRequest {
    fs::path root;                            // the directory to audit (a staged prefix or a published item)
    fs::path installed_at;                    // where it will live: absolute runpaths and $ORIGIN resolve against this
    fs::path sdk;                             // the SDK whose lib/ satisfies DT_NEEDED
    PortAdapter adapter;                      // expected files and SONAMEs
    std::vector<std::string> forbidden;       // strings that must not occur in any file (build-job paths)
    std::vector<std::string> skip_top_level;  // top-level names that are not part of the install ("evidence", "manifest.json")
    std::string target_arch;                  // "x86_64" or "aarch64"
    std::string target_os = "linux";          // "linux" (ELF) or "macos" (Mach-O): picks the adapter layout and the binary format
};
struct PortAudit {
    std::vector<PortAuditFinding> findings;
    Json libraries = Json::array();           // one entry per ELF file: path, kind, soname, needed, runpath
    Json to_json() const;
};
PortAudit audit_port_item(const PortAuditRequest& request);

// ---- build key and store ----------------------------------------------------------------------------------------------
// The normalized inputs of the key: source/recipe/adapter identities, SDK content identity, host/target, the identity of
// every external tool (by SHA-256 of the binary), host-tools identity, variants, declared environment and the (empty, for
// now) target-port dependency keys. Reads the SDK inventory and hashes the toolchain binaries.
Json port_key_inputs(const PortBuildRequest& request);
// SHA-256 of the compact serialization of the inputs (object keys sorted), 64 lowercase hex digits.
std::string port_build_key(const Json& inputs);

struct PortPublishResult {
    std::string build_key;
    fs::path path;
    bool already_published = false;           // an identical, verified item was there; nothing was changed
    Json manifest;
};

class PortStore {
public:
    explicit PortStore(fs::path root);        // the directory that holds store/, .staging/ and .locks/ (CRT_HOME/ports)
    const fs::path& root() const { return root_; }
    fs::path item_path(const std::string& build_key) const { return root_ / "store" / build_key; }

    // Publishes the finished build. `inputs` must be what port_key_inputs returned for the same request *before* the build:
    // the SDK is read again and must still have the recorded identity. Throws Error (and publishes nothing) when a recipe
    // test did not pass, the audit has a finding, or the key does not match the inputs.
    PortPublishResult publish(const PortBuildRequest& request, const PortBuildResult& result, const Json& inputs);

    // Re-reads an item: the manifest, the key against its recorded inputs, the exact file inventory, and the audit.
    // Returns the number of inventory entries checked; throws Error when anything differs, is missing or was added.
    uint64_t verify(const std::string& build_key, const fs::path& sdk) const;

    // Item keys, sorted. Staging and lock entries are not items.
    std::vector<std::string> list() const;

    // Removes leftovers of interrupted publications (staging whose owner is gone, stale locks).
    std::vector<std::string> recover();

    unsigned lock_wait_seconds = 0;

private:
    fs::path root_;
};

// `crt-cli port build|list|verify|recover` (experimental; see src/main/port_command.cpp).
Reply port_command(const std::vector<std::string>& args);

// `crt-cli port lock [--project <dir>]`: records the key, manifest digest and SDK identity of every provider-"port"
// dependency in the project's `crt.lock` (defined in project.cpp, which owns the manifest and the lock).
Reply port_lock(const std::vector<std::string>& args);

}  // namespace crtcli
