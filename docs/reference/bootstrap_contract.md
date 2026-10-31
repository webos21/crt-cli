# Implemented Native Bootstrap Contract

Experimental contract, not SDK Build Contract v1 or a promoted release. Native
policy export supports Linux/x86_64, Linux/aarch64, macOS/arm64, Windows/x64 and
Windows/arm64 (each replayed locally and unpromoted). Earlier format-0 evidence remains historical in
[acceptance](../acceptance/bootstrap_acceptance.md).

## Modules and Dependencies

`src/main/` contains command dispatch, project/target builds, toolchain selection,
PAL process execution, filesystem/hash operations and bootstrap diagnosis.
Project-owned headers live under `include/crtcli/`, not `src/main/`.
`src/test/` contains C++ regression tests. Generated projects have `include/`,
`src/main/` and `src/test/`; source paths are explicit, not globbed.

The CLI consumes public CRT/POSIX APIs, not host-specific preprocessor branches,
dyld declarations or `/proc` parsing. CRT's `crt/platform.h` supplies native
target information, executable screening and user-home lookup; `crt/dl.h`
enumerates real loaded images without fabricating ELF structures on Mach-O/PE.
These are documented CRT extensions, not functions attributed to Bionic.

JSON and SHA-2 are SDK dependencies: `nlohmann/json.hpp` and Mbed TLS SHA-256
replace the old CLI-owned nlohmann/PicoSHA2 copies. The Dev SDK also provides
embedded C cJSON and SHA-224/384/512. Source pins, notices and static/shared
evidence belong to CRT recipes and SDK metadata. The CLI does not fetch these
dependencies during its build, and redistribution still requires their notices.

LLVM and a selected SDK are app-build prerequisites. Command handlers invoke
native compiler/linker/archiver argv; they do not execute Python, shell, CMake,
Ninja or SDK wrappers. A producer-side seed/replay may use Python to expand the
same policy and build a **CRT-native** first CLI. The old host-C++ seed command
is obsolete because it cannot provide the public CRT PAL boundary. No seed or
permanent launcher is distributed. Optional configure/make port environments
remain distinct from native application builds.

## Windows Host Behaviour (2026-10-29)

Process creation is `posix_spawn` on every host. External native tools (compiler, linker, archiver) are marked as host tools
so CRT's Windows PAL redirects their stdio (`CRT_SPAWN_NATIVE_WINDOWS`); CRT-built children are not. `test`/`run` give a
child `PATH=<sdk>/bin;<sdk>/lib` on Windows only. A target may list `sdk_consumer_option_sources`: the SDK profile's
`consumer_compile_options` (Windows: `-U_WIN32 ...` for the curl headers) apply only to those sources and must stay away from
translation units that use `<filesystem>`. A running executable that the build must replace is renamed aside
(`<name>.old-<n>`, removed on the next build). Output, receipts and replies replace invalid UTF-8 from OS text.

Narrow strings are UTF-8 (2026-10-31): the SDK's link policy embeds CRT's UTF-8 active-code-page manifest, which the provider's
link argv carries, so the CLI and the programs it builds read `argv`, the environment and paths as UTF-8 (Windows 10 1903 or
later). `doctor --bootstrap` reports `narrow_encoding` and fails when it is not `utf-8`. Before any command the CLI rejects, with
exit 2, non-ASCII arguments in a process that is not UTF-8 and, on Windows, any argument that is not valid UTF-8; dependency
paths reported by the compiler get the same Windows check. A project directory's basename must still be a package identifier.

## Project and Target Declaration

New projects use `manifest_format: 1`:

```json
{
  "manifest_format": 1,
  "package": {"name": "hello", "version": "0.1.0"},
  "sdk": {"profile": "dev", "version": "*", "target": "native", "storage": "shared-immutable"},
  "include_dirs": ["include"],
  "targets": [
    {"name": "greeting", "kind": "static-library", "sources": ["src/main/greeting.cpp"]},
    {"name": "hello", "kind": "executable", "runtime_linkage": "static",
     "sources": ["src/main/main.cpp"], "dependencies": ["greeting"]}
  ],
  "tests": [{"name": "greeting-test", "sources": ["src/test/greeting_test.cpp"], "dependencies": ["greeting"]}]
}
```

`kind` is `executable` (default), `static-library` or `shared-library`.
`runtime_linkage` selects `static` or `shared`, independently of the CLI product's
single-executable distribution policy. Executables default to static CRT;
shared libraries require shared CRT. Static libraries are PIC archives, with
their final runtime selected by the consumer. Executable consumers of direct
or transitive shared-library dependencies must select shared CRT to avoid a
second runtime instance. Artifact names/startup/PIC/link flags come from the SDK
provider, not OS branches in the CLI. Native LLVM/platform `ar` is explicitly
selected and hashed; compiler drivers own linking.

`dependencies` names project library targets. The driver builds them in
dependency order, rejects cycles/unknown/executable dependencies and propagates
C++ link requirements. SDK library names use `sdk_libraries`; optional
`sdk_library_linkage` selects static/shared artifacts declared by the provider,
defaulting to target runtime linkage. Static archives do not embed dependency
libraries: executable/shared consumers resolve their transitive dependencies.

Use `.c` for C99, `.cpp`/`.cc`/`.cxx` for C++17. Source/include paths must remain
inside the project after resolving symlinks; output symlinks are rejected.
Target names are unique across build/test targets. Legacy format-0 `application`
declarations are read for migration; `application` and `targets` cannot coexist.
The driver caches requested target steps using the command, SDK/tool identity
and input/header content; changed inputs invalidate the affected steps.
Optimization remains O1 with debug info, separate from SDK profiles.
Tests have an executable main and a bounded `timeout_seconds` (1..3600).

`build --target <name>` builds one target and its dependencies; plain build
builds all declared targets. `test --target <test-name>` selects a test.
`run --target <name>` requires an executable; libraries cannot be run.
Without a run target, the first executable declaration is selected.

## SDK Store and Exact Inputs

Every host uses `$USER_HOME/.crt/sdks/<sdk-id>/` as the default SDK installation
root. CRT PAL resolves the home (`HOME` on Unix, `USERPROFILE` on Windows).
There is no XDG/AppData/Application Support default for SDKs. The driver never
falls back to its working directory if home is unavailable.

`configure --sdk-id <relative-id>` selects an existing SDK under that root and
rejects traversal/escaping symlinks. `--sdk-root` selects an explicit local
handoff instead; the two options are mutually exclusive. Download, verified
extraction, atomic installation and GC remain D2 work: selecting a test copy
does not implement an installer.

`new` writes sources and an unresolved format-0 `crt.lock` without acquiring an
SDK. Explicit configure writes `state: local-experiment` with exact version,
profile, target, provenance, full SDK inventory, provider hash and tool/resource
hashes. Absolute paths stay in ignored `.crt/local.json`. A dirty SDK is rejected
unless `--allow-dirty-sdk true` is explicitly selected; that choice and the true
dirty provenance are locked. It is an unpromoted local experiment, never a clean
release claim. The repository lock remains unresolved.

### Local `.sdk.json`

`.sdk.json` (ignored by git, also in the `.gitignore` written by `new`) records
this machine's SDK handoff and host tools so `configure` needs no flags. It is
local state, never a lock and never committed. Format 0 is a flat JSON object:

| Key | Meaning |
| --- | --- |
| `format` | must be `0` |
| `sdk_root` / `sdk_id` | absolute handoff path, or an id under the SDK store (mutually exclusive) |
| `sdk_contract`, `contract_sha256` | provider JSON path and its exact SHA-256 |
| `cc`, `cxx`, `linker`, `ar` | absolute external tool paths (the `ld.lld` name is preserved) |
| `allow_dirty_sdk` | boolean; `true` only for an unpromoted local experiment |
| `seed`, `origin` | informational strings (the previous CLI executable, where the values came from); `configure` ignores them |

`configure` reads `<project>/.sdk.json`, or the file named by `--sdk-config
<path>`, as defaults. Explicit flags override per key, and `--sdk-root` /
`--sdk-id` replace the file's SDK selection instead of conflicting with it.
Unknown keys, relative tool paths and a wrong provider digest are rejected. The
file only supplies inputs: everything is still verified and the resulting
identities are written to the project lock.

## Producer-owned Native Policy

CRT exports `crt.experimental.native-toolchain`, **format 1**, from installed
SDK wrappers. The exporter is producer tooling, not a CLI runtime dependency.
Format-0 providers are rejected rather than implicitly migrated. The Dev SDK
must declare `crt-platform-queries-1` and `json-sha2-1` capabilities.

The provider binds the full SDK inventory and packaging provenance; `modes`
contains C/C++ compile/link argv for `executable-static`, `executable-shared`,
`static-library` and `shared-library`, with a native archive argv for archives.
Typed slots carry SDK/resource roots, source, objects, output/name, project
include/compile options and selected libraries. The producer owns ABI, startup,
runtime ordering, PIC and artifact naming. Every workflow checks exact selected
bytes before executing external tools. Local hashes are pinning, not publisher
authentication or complete attestation of the vendor SDK/tool runtime.

On macOS the provider records an explicit external vendor SDK and `-isysroot`
for libSystem/linker lookup while retaining `-nostdinc`/`-nostdinc++` and CRT
headers. It is not permission to add host standard headers or a host C++ runtime.

## Commands and Results

`new <dir> [--language c|cxx] [--kind executable|static-library|shared-library]
[--runtime-linkage static|shared]` creates sources only. Existing commands are
`--help`, `--version`, `doctor --bootstrap`, `fetch`, `configure`, `build`, `test`, `run`.
Configure requires SDK selection, provider path/hash, `--cc`, `--cxx`, `--linker`
and `--ar`. Subsequent commands use the locked local binding.

`fetch --url <https-url> --ca-file <pem> --output <file> --sha256 <hex> --size <bytes>
[--timeout <seconds>]` (2026-10-31) is the minimal HTTPS download primitive for the D1-07
gate, not an SDK installer. Every option except `--timeout` (default 60, at most 3600 s)
is required. The URL must be `https://`, name a host and carry no credentials; the CA
file is the **only** trust input (no system store, environment variable or default
bundle is consulted); the SHA-256 is 64 lowercase hex digits and the size, in
1..2 GiB, is both the cap and the expectation. libcurl with Mbed TLS runs with peer and
host verification always on (there is no option to disable them), TLS 1.2 or newer, no
proxy, no redirects and HTTP 200 only. The body is written to a staging file beside the
output, checked for size and digest, then renamed. Failure removes the staging file and
publishes nothing; an existing output is never replaced (exit 2, content untouched). Exit
2 is an invalid request (no network was used), exit 1 a transfer or verification failure
with `data.code` one of `tls`, `network`, `timeout`, `interrupted`, `http_status`,
`too_large`, `size_mismatch`, `digest_mismatch`. Success reports bytes, digest, HTTP
status and the trust settings, plus `integrity: pinned-digest` and
`publisher_authentication: not_verified`: a digest the caller pinned says the bytes match
it, not who published them.

Experimental explicit-key authentication adds paired options
`--publisher-key <public.pem> --signature <signature.bin>`. The caller supplies
a trusted PEM SubjectPublicKeyInfo RSA public key independently of the download;
the CLI has no official embedded key or key-discovery service. RSA keys must be
2048..4096 bits. The detached binary signature covers the exact downloaded bytes
using RSA-PSS with SHA-256, SHA-256 MGF1 and exactly 32 salt bytes. Other padding,
salt lengths, weak/private/malformed keys and mismatched signatures are rejected.
Key/signature reads are bounded to 16 KiB/512 bytes; invalid inputs fail before
network use. A signature mismatch returns exit 1 with
`publisher_signature_invalid`, removes staging and publishes nothing.

Successful signed fetch reports `publisher_authentication: signature_verified`
and `publisher.trust: explicit-public-key`, the algorithm and the SHA-256 of the
canonical DER SubjectPublicKeyInfo. This authenticates bytes to the explicitly
selected key; it does not label that key an official CRT publisher. Unsigned
fetch remains available and reports `not_verified`. Metadata may be fetched
and authenticated this way, but release-metadata schema, artifact binding,
expiry/replay protection, official key distribution/rotation/revocation and
executable self-integrity are not implemented. No production trust policy or
new SDK schema is assigned by this experimental primitive.

`--json` emits one `crt-cli.bootstrap-result`, format 1, with command/ok/data.
Child stdout is bounded at 8 MiB and relayed to stderr; result JSON remains
parseable. Private child environments preserve only explicit user-home/temp/OS
root variables plus fixed PATH/locale, not loader/compiler injection variables.
Timeouts own a process group and retain child exit/signal/truncation outcomes.
Broader Windows and interactive cancellation acceptance remains open.

`out/native/last-build.json` records target kinds/linkage, artifact hashes,
compiler/archive/link argv and outcomes. Bootstrap doctor checks C++ exceptions,
local new/delete ownership through public `dladdr`, compression and HTTP/TLS
initialization, and reports the PAL SDK store/image list. The doctor itself does not use
the network, so its HTTPS check stays `not_run` (`fetch` is exercised separately), and
authenticated executable integrity stays `not_verified`. SDK installation,
self-upgrade, general package resolution/deployment and public release are not
implemented by this contract increment.
