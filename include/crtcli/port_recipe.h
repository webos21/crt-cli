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

// CRT-owned meaning of a reviewed recipe, keyed by the SHA-256 of the recipe file. Roles are never guessed from names.
struct PortAdapter {
    std::string recipe_sha256, name, version;
    std::vector<std::string> host_tools, target_ports, variants;
    std::string sdk_stage;                                      // minimum SDK stage that satisfies the build
    std::map<std::string, std::string> test_sources;            // recipe-relative test source -> SHA-256
    std::string crt_review;                                     // "pending" until CRT accepts the entry
};

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
