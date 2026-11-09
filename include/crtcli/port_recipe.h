#pragma once
// Reader for the bounded subset of the CRT source-port recipe format (schema_version 1) that the D3 first slice executes.
// See docs/design/source_port_adapter.md. The recipe is untrusted data: it is accepted only when its file digest is in the
// reviewed adapter table, and every key, placeholder and value outside the subset is refused rather than ignored.
#include "crtcli/core.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace crtcli {

struct PortSource {
    std::string url, archive, source_dir, sha256, sha1, md5;   // sha1/md5 are informational and empty when absent
};
struct PortPatch { std::string file, find, replace, reason; };
struct PortTest {
    std::string name, source, expect_stdout;
    std::vector<std::string> cflags, link_args;                // link_args already resolved for the requested OS
};
// How an environment assignment combines with what the job environment already holds, exactly as the oracle
// (tools/crt-port-build.py apply_recipe_env): the base `env` replaces; a `target_overrides.<os>.env` value for a flag
// variable (CFLAGS CPPFLAGS CXXFLAGS LDFLAGS LIBS) is appended with a space when the variable is non-empty, else replaces.
struct PortEnvOp { std::string name, value; bool append_flags = false; };

struct PortRecipe {
    std::string recipe_sha256;
    std::string name, version, target_os;
    PortSource source;
    std::vector<std::string> dependencies, configure_args, make_args;
    std::vector<PortEnvOp> env;                                 // base ops first, then the requested OS's ops
    std::vector<PortPatch> post_configure_patch;
    std::vector<PortTest> tests;
};

// What a correct install of one OS looks like. `expected_names` are the SONAMEs (ELF) or install names (Mach-O) the shared
// objects must carry; for Mach-O they are written as the audit compares them, e.g. "@rpath/libz.1.dylib".
struct PortOsLayout {
    std::vector<std::string> expected_files, expected_names;
    std::string static_library, shared_library;
};

// CRT-owned meaning of a reviewed recipe, keyed by the SHA-256 of the recipe file. Roles are never guessed from names.
struct PortAdapter {
    std::string recipe_sha256, name, version;
    std::vector<std::string> host_tools, target_ports, variants;
    std::string sdk_stage;                                      // minimum SDK stage that satisfies the build
    std::map<std::string, std::string> test_sources;            // recipe-relative test source -> SHA-256
    std::string crt_review;                                     // publication-time review metadata; excluded from build keys
    // What a correct install prefix must contain (D3 step 4 audit): prefix-relative files (a symlink to a file counts) and
    // the SONAMEs the shared objects must carry. The recipe does not say this; CRT's review of the entry does.
    std::vector<std::string> expected_files, expected_sonames;
    // How a project consumes the item (D3 step 5): the SDK contract library name the port replaces (a closure may not
    // reach both), and the item-relative include directory and link inputs.
    std::string sdk_library, include_dir, static_library, shared_library;
    // The Linux layout above is the first slice; other hosts add their own (D3 macOS). `layouts` is keyed by OS name and
    // never holds "linux" (see port_layout()).
    std::map<std::string, PortOsLayout> layouts;
    // Environment the executor adds after the recipe's own ops for one OS, for toolchain policy the recipe does not carry
    // (macOS: the install name of the shared library). Part of the build key; empty for Linux.
    std::map<std::string, std::vector<PortEnvOp>> executor_env;
    // `make` arguments the executor appends after the recipe's own for one OS. The literal token @AR@ becomes the configured
    // archiver. macOS: zlib's Darwin configure forces AR to the host's /usr/bin/libtool whatever the environment says, which
    // would bring an undeclared host tool into the build; the command line overrides it with the declared archiver.
    std::map<std::string, std::vector<std::string>> executor_make_args;
};

// The layout this adapter entry declares for `os` ("linux", "macos"); throws when the entry has none for that OS.
PortOsLayout port_layout(const PortAdapter& adapter, const std::string& os);
// The executor-added environment for `os` (empty when none).
const std::vector<PortEnvOp>& port_executor_env(const PortAdapter& adapter, const std::string& os);
// The executor-added make arguments for `os` (empty when none).
const std::vector<std::string>& port_executor_make_args(const PortAdapter& adapter, const std::string& os);

// The reviewed table (compiled in). Throws for a digest that is not listed.
const PortAdapter& port_adapter_for(const std::string& recipe_sha256);
const std::vector<PortAdapter>& port_adapters();

// Parses and validates; `target_os` is "linux", "macos" or "windows" (the OS whose overrides are applied). Throws Error
// naming the offending key path. Nothing is read from the file system.
PortRecipe parse_port_recipe(const std::string& bytes, const std::string& target_os);

// The same parse against an explicitly supplied entry (its digest must equal the file's). Only tests use this, to exercise
// the subset rules on mutated recipes; the executor goes through parse_port_recipe.
PortRecipe parse_port_recipe_with(const std::string& bytes, const std::string& target_os, const PortAdapter& adapter);

struct PortPlaceholders {
    std::string port_prefix;        // @PORT_PREFIX@ and @PORT_PREFIX_POSIX@ (the same spelling on POSIX hosts)
    std::string cc;                 // @CC@
};
// @PORT_PREFIX_MAKE@ is the prefix with every space backslash-escaped. Any other @TOKEN@ is an error.
std::string substitute_port_placeholders(const std::string& value, const PortPlaceholders& placeholders);
// Applies the ops in order to `env`, substituting placeholders in each value (with @CC@ taken from the placeholders).
void apply_port_env(std::map<std::string, std::string>& env, const std::vector<PortEnvOp>& ops, const PortPlaceholders& placeholders);

}  // namespace crtcli
