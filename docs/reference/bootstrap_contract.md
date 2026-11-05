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

`configure --sdk <ref>` selects an SDK published by `sdk install` (directory `<version>-<content12>`) by that name or
by at least 8 hex digits of its content identity. Without `--sdk`, `--sdk-id` or `--sdk-root`, the installed SDK is used
only when exactly one fits the project (profile `dev`, this host's target, the manifest's `sdk.version`); none or several
is an error that lists the candidates, so a project never follows a moving default. The directory name must equal the
version and content identity the SDK proves, so a renamed or substituted directory is refused. `--sdk-id <relative-id>`
(any directory under the root, traversal and escaping symlinks rejected) and `--sdk-root` remain local handoff
selections; the three options are mutually exclusive. A SDK installed from a release manifest that names a toolchain contract keeps that contract at
`$USER_HOME/.crt/sdks/.contracts/<directory>.json`; `configure` then needs no `--sdk-contract` (the lock records
`contract_source: store` and the digest, `.crt/local.json` no path), and an explicit `--sdk-contract` still overrides.
A contract that refers to the host's OS SDK (`@OS_SDK@`, `os_sdk.kind`) is completed on this host (`xcrun`, or
`CRT_WINDOWS_SDK_LIBPATH`, or `--os-sdk <directory>`); the location is local state and never locked. Download and GC
remain later work.

### Installation receipt (CLI-owned format 1)

New installations record `.receipts/<directory>.json` outside the immutable SDK
payload. Schema `crt-cli.sdk-install-receipt`, format 1, contains `sdk` (the
exact version, full content SHA-256 and directory) and `contract`: either null
(explicitly none at installation) or `{"sha256":"<installation-time-pin>"}`
(that file is required). The digest comes from the independently pinned release
manifest/verified install request, not from reading the current stored file.
This does not change SDK install metadata, native-provider or lock formats.

`sdk verify` validates SDK content plus receipt identity, required presence,
contract bytes/format/portability and SDK binding. Fresh store-based configure
uses that receipt pin, so a valid-looking modified contract cannot be adopted
as a new project pin. An explicit provider cannot replace a known installation
pin; an installation without a contract can still use an explicitly pinned
external provider. Local handoff roots remain explicit developer inputs.

Ordinary reinstall is idempotent and preserves required contract state. A
contract may be added to a receipt-marked installation that previously declared
none, but an already known contract pin is immutable. The SDK is published by
one directory rename; contract/receipt files use temporary writes and atomic
renames, with the receipt as final commit record. A crash between publications
fails closed rather than accepting partial metadata. `sdk recover` sweeps only
dead-writer contract/receipt temporaries; it never invents missing trust pins.

Existing SDK directories without receipts are legacy/untrusted for store-based
selection. Explicit recovery is:

```text
crt-cli sdk repair <sdk-directory> --archive <original-archive> --release-manifest <original-manifest> --release-manifest-sha256 <trusted-pin> [--store <dir>]
```

Repair verifies the original manifest's archive/contract sizes/digests, extracts
and validates the archive, and requires its full identity to equal the named
existing SDK and actual installed bytes. It restores metadata only; SDK payload
and project locks are unchanged. Raw archive SHA alone is insufficient for
repair. A known receipt cannot be downgraded or assigned a different contract;
it can gain the first contract after an interrupted explicit addition. Missing
legacy receipts are established only from these pinned release inputs. Unknown,
malformed or symlinked receipts are refused for manual diagnosis, not silently
rewritten. Losing/changing a required contract can be repaired with its original
pin. None of these local digests authenticates an official publisher.

Older CLI versions do not enforce receipts; they are not qualified readers of
this store policy. The earlier [shape-only guard](../acceptance/sdk_install_acceptance.md#sdk-verify-covers-the-stored-contract-linuxx86_64-2026-11-05)
keeps its original evidence; the receipt increment has its own native record.

`new` writes sources and an unresolved format-1 `crt.lock` without acquiring an
SDK. Lock format 0 is no longer read: `build`/`test`/`run` refuse it and say to run `configure` again; a lock is never
migrated implicitly. Explicit configure writes `state: local-experiment` with exact version,
profile, target, provenance, full SDK inventory, provider hash and tool/resource
hashes, and `source` (`store` with the SDK `sdk_directory`, or `handoff`). A store selection keeps no machine path: it is
found again through the store by its locked identity, and is an error if that SDK is no longer installed. Absolute
paths (handoff SDK root and external tools) stay in ignored `.crt/local.json`. A dirty SDK is rejected
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
| `sdk` / `sdk_root` / `sdk_id` | a published store SDK reference, an absolute handoff path, or an id under the SDK store (mutually exclusive) |
| `sdk_contract`, `contract_sha256` | provider JSON path and its exact SHA-256 |
| `cc`, `cxx`, `linker`, `ar` | absolute external tool paths (the `ld.lld` name is preserved) |
| `allow_dirty_sdk` | boolean; `true` only for an unpromoted local experiment |
| `seed`, `origin` | informational strings (the previous CLI executable, where the values came from); `configure` ignores them |

`configure` reads `<project>/.sdk.json`, or the file named by `--sdk-config
<path>`, as defaults. Explicit flags override per key, and `--sdk-root` /
`--sdk-id` / `--sdk` replace the file's SDK selection instead of conflicting with it.
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
`--help`, `--version`, `doctor`, `env`, `shell`, `fetch`, `configure`, `build`, `test`, `run`.
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
in-memory executable attestation are not implemented. The local resource pin
check below compares executable file bytes. No production trust policy or
new SDK schema is assigned by this experimental primitive.

### Child environment diagnostics (experimental format 0)

`env --profile control|app-build|app-run [--project <directory>]` reports the
same environment construction used by `execute`, without launching a shell,
building targets, writing project bindings or changing the parent environment.
It uses the normal result envelope; `data.environment_format` is `0`.
Human output is the diagnostic JSON, not a shell script to evaluate.

| Profile | Prerequisites | Child environment |
| --- | --- | --- |
| `control` | None; `--project` is rejected | Fixed PATH `/nonexistent`, locale `C`, bounded inheritance |
| `app-build` | Configured local project, verified SDK/provider/tools | Control values plus `CRT_SPAWN_NATIVE_WINDOWS=1` for Windows vendor tools; compiler roles use absolute executable paths |
| `app-run` | Same verified project binding | Control values; Windows PATH contains only the selected SDK's `bin` and `lib` runtime directories |

`fixed` contains computed variable values; `inherited_names` lists only present
names from `HOME`, `USERPROFILE`, `TMPDIR`, `SystemRoot`. Inherited values are
never printed. Other ambient variables, including PATH, compiler/loader inputs
and credentials, are excluded from children. `control` describes helper-child
construction, not the environment of the already-running CLI. This filtering
is not an OS sandbox; inherited home paths can still affect tool behavior. The list is what the CLI passes: the operating system may add its own
variables to a new process (Windows/arm64 adds `PROCESSOR_ARCHITECTURE`; Windows/x64 did not in the measured run), and CRT's Windows runtime removes its own spawn-bootstrap variables from a child's
environment (CRT `d12e210`). The native suite tolerates only the OS-added name.
Windows runtime directories must be absolute UTF-8 paths without semicolons;
search-list delimiters cannot represent a semicolon inside a directory name.

Project profiles include the exact existing `selection`, separate `build_host`
and canonical project path. Only `app-build` lists compiler executable roles.
Validation reuses `Toolchain::load`, including SDK inventory, provider and tool
digests and the native compiler resource-directory query; it requires installed
development inputs even for the `app-run` diagnostic. It does not inspect a
packaged application's runtime closure. Runtime-only package acceptance remains
separate. No SDK acquisition, tool discovery or host-tools identity is invented.

The independent `port-build` profile and non-interactive `shell` are described
below. Interactive terminal handling and the general source-port recipe engine
remain unimplemented. No producer SDK schema or official trust policy changes.

`--json` emits one `crt-cli.bootstrap-result`, format 1, with command/ok/data.
Child stdout is bounded at 8 MiB and relayed to stderr; result JSON remains
parseable. Private child environments preserve only explicit user-home/temp/OS
root variables plus fixed PATH/locale, not loader/compiler injection variables.
Timeouts own a process group and retain child exit/signal/truncation outcomes.
Normal completion also kills remaining members of that owned process group.
Broader Windows and interactive cancellation acceptance remains open.

`out/native/last-build.json` records target kinds/linkage, artifact hashes,
compiler/archive/link argv and outcomes. Bootstrap doctor checks C++ exceptions,
local new/delete ownership through public `dladdr`, compression and HTTP/TLS
initialization, and reports the PAL SDK store/image list. The doctor itself does not use
the network, so its HTTPS check stays `not_run` (`fetch` is exercised separately), and
authenticated executable integrity stays `not_verified`. SDK installation,
self-upgrade, general package resolution/deployment and public release are not
implemented by this contract increment.

### Independent host environment (experimental format 1)

CRT owns [`crt.experimental.host-tools`, format 1](https://github.com/webos21/crt/blob/main/docs/design/host_tools_contract.md).
It is independently versioned and contains CRT-native mksh, toybox, separate awk,
make, explicit applet aliases, runtime dependencies and notices. The CLI validates
the explicit manifest pin, execution host, complete payload, alias dispatchers and
executable permissions before constructing an environment. Unknown schema/host,
changed bytes, missing tools and escaping links fail without a host fallback.
This local-only artifact does not alter the target SDK/provider formats.

```sh
crt-cli env --profile port-build --host-tools /path/to/tools \
  --host-tools-sha256 <manifest-sha256> --work-root /path/to/new-job
crt-cli shell --host-tools /path/to/tools --host-tools-sha256 <manifest-sha256> \
  --work-root /path/to/new-job --script /path/to/script.sh \
  --directory /path/to/existing-working-directory --timeout 180
```

`env` validates and reports; it creates no job. `shell` requires a script, a new
work directory with an existing parent, and a timeout in 1..3600 seconds (default
180). The fresh job rootfs links `system`, `bin`, `usr` to the immutable bundle
and creates writable `tmp`, `data/local`, `data/local/tmp`. Parent-directory links
are resolved before rejecting jobs inside immutable inputs. Windows requires
real directory links/Developer Mode; the CLI never changes that setting.
The job remains caller-owned scratch after execution; no automatic store GC.

PATH contains only the bundle's physical `system/bin`. `CRT_ROOTFS` selects the
job overlay; `SHELL`/`CONFIG_SHELL`, `MAKE`, `AWK` use the bundle's virtual
`/system/bin` paths. GNU make also receives `MAKEFLAGS=SHELL=/system/bin/sh`
(`sh.exe` on Windows), avoiding physical shell-path splitting. Temp variables
point inside the job; ambient PATH, MAKEFLAGS, CRT_ROOTFS and compiler/credential
variables are excluded. As for other profiles, home/OS-root names are bounded
inheritance and their values are not printed. Filtering/rootfs mapping is not
an OS sandbox for scripts.

Optional `--project <configured-dir>` verifies the existing native SDK/provider
binding, records its exact selection separately from host-tools identity and
sets `CRT_SYSROOT`. Without it the environment is SDK-free. No compiler-wrapper
bridge, recipe engine, target execution or cross-target build support is implied;
those belong to D3. Child cwd uses `posix_spawn_file_actions_addchdir_np`, leaving
the parent unchanged. JSON data retains child stdout, exit_code, timed_out and
output_truncated; exit 124 denotes timeout, other child exits propagate. This is
non-interactive; terminal/session UX and adversarial process escapes are outside
the bounded owned-process-group contract.

### Scoped doctor and bootstrap resource pins

`doctor` defaults to `--scope self`, requiring no SDK/compiler. `doctor --bootstrap`
retains its existing result for old callers. Plain self diagnosis reports
integrity `not_verified` and HTTPS `not_run`. Additional scopes are read-only:

- `doctor --scope environment --host-tools <dir> --host-tools-sha256 <hex>` checks
  the selected tools and aliases, with execution `not_run`; it needs no target SDK.
- `doctor --scope project [--project <dir>]` validates the current manifest,
  SDK/provider/tool binding through app-build diagnostics. It does not compile,
  repair locks, acquire an SDK or diagnose a packaged application's closure.
- `doctor --scope self --resources <dir> --resources-sha256 <hex>` validates a
  `crt-cli.bootstrap-resources`, format 1 manifest and its complete regular-file
  inventory, target, executable filename and explicit trust-data declaration.
  The running executable's file hash must match the selected package. Success
  reports `executable_integrity: pinned-file-match`; this is not memory attestation.
  Paired `--publisher-key`/`--signature` authenticate the exact manifest with the
  same explicit RSA-PSS policy as fetch. Without them publisher authentication
  remains `not_verified`; a local pin alone is not publisher identity.

The producer-only `tools/create_bootstrap_resources.py` assembles one executable,
dependency notices/provenance and optional `share/trust/ca.pem`, refusing named
output replacement, symlinks or overlap with immutable inputs. It is not executed
by the product. External pins/public keys/signatures remain separate from the
inventory. `fetch` still requires an explicit `--ca-file`; it never implicitly
adopts a package CA. No helper/launcher is needed by this package. Installation,
self-upgrade, official release metadata/expiry/key policy and native release
qualification remain separate gates. Evidence and exact limits are in
[D1 host-environment acceptance](../acceptance/host_environment_acceptance.md).
