# Implemented Native Bootstrap Contract

This is an experimental Linux/x86_64 contract, not SDK Build Contract v1 or a
promoted release. The owner replaced the initial CMake prototype on 2026-10-28.
Its dated evidence remains in [acceptance](../acceptance/bootstrap_acceptance.md).

## Modules and Dependencies

`src/main/main.cpp` dispatches commands. `project.cpp` reads declarations and
creates/builds projects; `toolchain.cpp` verifies inputs and expands SDK policy;
`process.cpp` executes native argv with a private environment and timeout;
`core.cpp` supplies JSON, hashing and filesystem operations; `bootstrap.cpp`
checks CRT C++/zlib/curl/Mbed TLS. Tests are C++ in `src/test/`.

LLVM Clang/LLD and an identified Dev SDK are explicit app-build prerequisites.
CMake, Ninja, Python, shell scripts and SDK compiler wrappers are not invoked
by CLI self-build or native project configure/build/test/run. The temporary
hosted C++ `tools/bootstrap_host.cpp` seed shares the backend and builds the
first CRT-native CLI. It is not shipped, a launcher, or a runtime dependency.
Upstream configure/make ports may need explicitly selected CRT shell/tools;
that future port environment is separate from this native app backend.

Pinned unmodified nlohmann/json and PicoSHA2 headers, provenance/hashes and
licenses are under `third_party/`. No dependencies are fetched during a CLI
build. Distribution must retain their licenses with the SDK/runtime notices.

## Project Declaration and Exact Inputs

`crt.json` uses `manifest_format: 0`:

```json
{
  "manifest_format": 0,
  "package": {"name": "hello", "version": "0.1.0"},
  "sdk": {"profile": "dev", "version": "*", "target": "native", "storage": "shared-immutable"},
  "include_dirs": ["src/main"],
  "application": {"name": "hello", "sources": ["src/main/main.cpp", "src/main/greeting.cpp"]},
  "tests": [{"name": "greeting-test", "sources": ["src/test/greeting_test.cpp", "src/main/greeting.cpp"]}]
}
```

Use `.c` sources for C99 or `.cpp`/`.cc`/`.cxx` for C++17. Explicit source and
include paths must stay inside the project, including resolved symlinks. Optional
`sdk_libraries` names select static archives from the selected SDK. Source files
are compiled individually; test targets have their own main. Optional test `timeout_seconds` is bounded
from 1 to 3600 (default 180; the CLI integration suite selects 600). This first backend
rebuilds targets and has no incremental/header-dependency cache or general
library/dependency graph. Build optimization is currently fixed at O1 with debug
information; content profiles are not build modes.

`new` generates sources and an unresolved format-0 JSON `crt.lock`; it creates
no SDK binding/download. Explicit local `configure` validates the SDK and writes
`state: local-experiment` plus exact selection into the lock. Absolute SDK/tool
paths live in ignored `.crt/local.json`. The lock pins version/profile/target,
producer source, full SDK inventory, provider-contract hash and LLVM/linker/resource
hashes. Every build verifies these bytes before invoking selected compiler tools.
The repository's checked-in lock remains unresolved: no production pin is adopted.
Automatic first-build SDK acquisition, installer/store/GC, `--locked`/`--offline`,
registry/dependency resolution and lock migration remain D2/D4/D5 work.

## Producer-owned Compiler Policy

CRT's `tools/export_cli_toolchain.py` is an SDK-producer migration tool. It
captures actual installed wrapper argv for C/C++ compilation and static hosted
linking with exceptions/RTTI. Python/shell are required only by this existing
producer-side export, not by the CLI or application build path. CRT runtime/SDK
production remains CMake-based and does not depend on CLI source.

The separately hashed `crt.experimental.native-toolchain`, format 0, binds a
complete read-only SDK inventory and clean matching format-1 manifest/Dev
packaging provenance. Commands contain typed argv slots for SDK/compiler-resource
roots, sources, objects, output, application flags and SDK libraries. ABI,
startup objects, runtime archive order and freestanding boundaries remain
producer-owned; the CLI supplies execution rather than copying those policies
into command handlers. This narrow variant supports native Linux/x86_64 only.
Existing SDK wrappers/CMake entry points remain available to other consumers.

The provider stays beside the immutable SDK, not inside an older artifact.
Changing either yields a new input identity; exporter hash and wrapper bytes are
recorded. SDK content identity is SHA-256 of compact UTF-8 JSON (`ensure_ascii`
false, no separator whitespace) of sorted `[relative_path, kind, value]` arrays:
`file` values are file SHA-256, `symlink` values are literal targets. Directories
are excluded; symlinks must resolve inside the SDK. The earlier CMake fingerprint
uses a different algorithm and is retained under its original identity.
Contract/content hashes are local pinning, not publisher authentication.

## Commands, Results and Limits

Current commands: `--help`, `--version`, `doctor --bootstrap`,
`new <directory> [--language c|cxx]`, `configure`, `build`, `test`, `run`.
Use `--project <directory>` for an existing project. First local configure also
requires `--sdk-root`, `--sdk-contract`, `--contract-sha256`, `--cc`, `--cxx`,
`--linker`; subsequent workflows use the project binding.

`--json` occurs once and emits one final `crt-cli.bootstrap-result`, format 1,
with command/ok/data. Child stdout is captured/relayed to stderr, keeping JSON
stdout parseable. Exit 0 means success, 1 an input/build/runtime/test failure,
2 an unsupported top-level command. Child exit/signal/timeout outcomes remain
in test/run results. Linux execution uses native ELF paths, direct execve argv,
a private PATH and no inherited loader/compiler injection variables. Child
stdout capture/relay is bounded at 8 MiB and reports truncation in test/run
outcomes. Each child
has a timeout and owned process group; broader interactive cancellation and
three-host process-tree behavior remain D6 acceptance.

`out/native/last-build.json` records selection, compiler/linker argv and outcomes.
Bootstrap doctor verifies C++ exceptions, zlib and linked HTTP/TLS initialization;
actual HTTPS is `not_run` and authenticated executable integrity `not_verified`.
Self-upgrade/package/install and general environment/project doctor are not
implemented; unsupported commands fail. No public release is authorized.
