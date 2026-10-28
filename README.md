# CRT-CLI

CRT-CLI is a CRT-native development-environment manager for building C and C++
applications on Linux, Windows, and macOS. Examples currently use `crt`;
choosing `crt` versus `crt-cli` remains an early product decision.

It aims to make [CRT](https://github.com/webos21/crt)'s Bionic-compatible runtime
and staged SDKs easier to use: select an SDK, create a project, add libraries,
and build/run/package an application without manually assembling startup
objects, sysroots, special compiler flags, or runtime search paths.

## Project Maturity

This repository is in the design/bootstrap phase. It contains project guidance,
designs, and a work queue, but no CLI executable, build system, installer, or
published CLI package yet. Commands below illustrate the intended experience;
they are not an installation guide or working commands for this repository.

## Intended Experience

```sh
crt sdk install --profile cxx
crt doctor
crt new hello --template cxx
cd hello
crt add zlib
crt build
crt run
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
host, able to start before a development SDK is installed. Native feasibility
and the precise resource/helper policy still require validation. Planned
`upgrade` updates the CLI itself; `doctor` distinguishes self, environment and
project health. Projects pin their own SDK version/profile independently of
CLI updates. SDK storage and acquisition defaults remain under review.
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
