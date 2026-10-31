# CRT-CLI

CRT-CLI is a CRT-native development-environment manager for building C and C++
applications on Linux, Windows, and macOS. Its executable is `crt-cli`.

It aims to make [CRT](https://github.com/webos21/crt)'s Bionic-compatible runtime
and staged SDKs easier to use: select an SDK, create a project, add libraries,
and build/run/package an application without manually assembling startup
objects, sysroots, special compiler flags, or runtime search paths.

## Project Maturity

The Linux/x86_64 and macOS/arm64 bootstraps consume a produced Dev SDK. The native C++ builder
reads `crt.json` and invokes LLVM directly. `new` creates separate C/C++ files
under `src/main/` and `src/test/`, with headers in `include/`; bounded configure/build/test/run and bootstrap
doctor are implemented. CMake, Python and shell are excluded from this app path.
The SDK installer, dependency resolver, self-upgrade, Windows native replay
and public release remain pending.

See the [build guide](docs/guides/development.md#build-and-test-the-bootstrap)
and [implemented contract](docs/reference/bootstrap_contract.md) for actual
entry points and limitations. Bootstrap doctor reports integrity as not verified
and HTTPS as not run; it is not complete installation/project health acceptance.

## Intended Experience

```sh
crt-cli sdk install --profile cxx
crt-cli doctor
crt-cli new hello --language cxx
cd hello
crt-cli add zlib
crt-cli build
crt-cli run
```

- **Choose an SDK:** use the existing CRT stage appropriate for C, C++,
  graphics, media, UI, or Web applications rather than always downloading WebKit.
- **Use additional libraries:** fetch reviewed upstream sources and build them
  locally through CRT port recipes; keep exact SDK and dependency locks.
- **Develop applications:** use one workflow for project creation, build, run,
  test, and packaging, with structured diagnostics for CI and automation.
- **Keep environments separate:** manage CRT host tools and runtime dependencies
  without silently replacing tools or changing the system PATH.

The CLI itself is intended to use CRT and ship as one executable per supported
host, able to start before a development SDK is installed. Full three-host validation remains pending. The selected policy allows OS
dependencies, data/cache/certificates, previous versions and temporary update
helpers; no permanent auxiliary launcher is used. Planned
`upgrade` updates the CLI itself; `doctor` distinguishes self, environment and
project health. Projects pin their own SDK version/profile independently of
CLI updates. SDK payloads use shared immutable storage with exact project references;
first configure/build acquires a missing SDK, while new only creates sources.
Automatic acquisition and shared-store installation remain planned; the current
experiment selects a local SDK and separately hashed compiler policy explicitly.
The default SDK store is `$USER_HOME/.crt/sdks` on all hosts. Native project
targets distinguish executables, static libraries and shared libraries, with
static/shared runtime linkage; the CLI's own one-executable goal does not
restrict the applications it builds. JSON and SHA-2 are supplied by the Dev SDK.
CRT Studio is a later, separate native IDE built on this CLI workflow.

## Platforms and Requirements

Initial native validation targets are Windows/x64, Linux/x86_64, and
macOS/arm64. These are design targets, not a completed support matrix.
External LLVM and native build tools remain prerequisites; the CRT SDK owns
the target compilation/linking contract. Windows build/source-stage hosts
require Developer Mode and real filesystem symlinks. The CLI must diagnose
missing prerequisites rather than change security settings automatically.

## Learn More and Contribute

- [Documentation](docs/README.md): design, development guidance, and acceptance.
- [Development guide](docs/guides/development.md): the current bootstrap process
  and what is not implemented yet.
- [TODO](TODO.md): active work and ordered plans.
- [HISTORY](HISTORY.md): dated completed work.
- [AGENTS.md](AGENTS.md): repository development rules.

Design and bootstrap contributions should preserve the SDK-only consumer
boundary and avoid making the CRT runtime depend on CLI sources.

## License

Unless otherwise noted, project-owned source code and documentation in this
repository are licensed under the [Apache License, Version 2.0](LICENSE)
(SPDX identifier: `Apache-2.0`).

The CRT runtime and other third-party components retain their respective
licenses. See [dependency ownership and notices](third_party/README.md);
the project license does not replace their license or notice requirements.
