# CRT-CLI

CRT-CLI is a CRT-native development-environment manager for building C and C++
applications on Linux, Windows, and macOS. Its executable is `crt-cli`.

It aims to make [CRT](https://github.com/webos21/crt)'s Bionic-compatible runtime
and staged SDKs easier to use: select an SDK, create a project, add libraries,
and build/run/package an application without manually assembling startup
objects, sysroots, special compiler flags, or runtime search paths.

## What Works Today

The CRT-native C++ CLI reads `crt.json` and invokes LLVM directly. `new`
creates C/C++ sources under `src/main/` and `src/test/`, with headers in
`include/`. Explicit configure/build/test/run, scoped doctor, environment
reports and a bounded non-interactive CRT shell are implemented. Native app
builds and CLI self-build require no CMake, Python or shell.

Local SDK management is implemented: `sdk install` accepts pinned local
`tar.xz`/`zip` release assets, `sdk list|verify|recover` manage a shared immutable
store, and configure can select an installed SDK with its stored portable
compiler contract. Local installation and Stage 2 project selection have native
records on Linux/x86_64, Linux/aarch64, macOS/arm64 and Windows/x64/arm64 at
identified snapshots. They are local technical results, not a current-source
support matrix or a public release. See [STATUS](STATUS.md) for scope and
[SDK installation acceptance](docs/acceptance/sdk_install_acceptance.md) for evidence.

The current installer/project selection supports the native `dev` profile.
A fresh unresolved project can bind to an installed SDK on its first `build`
using declared LLVM paths, then use plain build/test/run. This increment has
[Linux/x86_64 evidence](docs/acceptance/sdk_install_acceptance.md#first-build-without-configure-linuxx86_64-2026-11-06);
its macOS/Windows replay remains separate. General cxx/upper-stage profile
selection, source-port/dependency management and self-upgrade remain planned.
[Explicit pinned HTTPS SDK acquisition](docs/reference/bootstrap_contract.md#explicit-pinned-https-sdk-source-experimental-format-0)
is implemented for the Linux workflow: `sdk acquire --source <file>` or first
`build --sdk-source <file>` verifies a caller-pinned source and reuses an exact
installed identity. `fetch` also supports explicit-key RSA-PSS verification.
Official default-download trust, channels and publisher promotion remain pending.

## Try the Local Workflow

Use a CLI built from the implementation being exercised; the dated
[local executable snapshots](bin/README.md) may predate first-build/acquisition
commands. For the local flow, select a native CRT Dev SDK release archive,
its manifest and named portable contract asset beside it, plus external LLVM tools. Substitute actual paths and the independently trusted
manifest SHA-256 below:

```text
crt-cli sdk install --archive <archive> --release-manifest <manifest> --release-manifest-sha256 <sha256>
crt-cli sdk list
crt-cli new hello --language cxx
crt-cli build --project hello --cc <absolute-clang> --cxx <absolute-clang++> --linker <absolute-ld.lld> --ar <absolute-llvm-ar>
crt-cli test --project hello
crt-cli run --project hello
```

The first build selects the only compatible installed SDK and records exact
pins after the build succeeds. LLVM declarations may also live in ignored
`.sdk.json`; subsequent builds need no tool flags. Use `configure` explicitly
to change an existing binding. If several SDKs fit, select one on the first build explicitly with `--sdk <directory-or-identity-prefix>`; there is no moving
default. On hosts awaiting first-build replay, retain the explicit `configure`
step before build/test/run from their recorded baseline. Windows also needs its OS SDK import-library location through
`CRT_WINDOWS_SDK_LIBPATH` or `--os-sdk`; macOS uses `xcrun` or an explicit
`--os-sdk`. Tool locations remain local state; the project lock pins SDK,
contract and tool identities. See the [implemented contract](docs/reference/bootstrap_contract.md#sdk-store-and-exact-inputs).

SDKs default to `$USER_HOME/.crt/sdks` on all hosts. Project-specific exact
references share immutable payloads; installation does not migrate another
project's lock. `new` creates sources only. The local example installs an archive
before first build; network acquisition requires an explicit pinned source.
The native builder supports executables, static/shared project libraries and
static/shared runtime linkage using SDK-owned startup/link/ABI policy.

For explicit HTTPS acquisition, supply a trusted format-0 source file with
CA/metadata/SDK/contract pins, then use `--sdk-source /absolute/source.json` on
the first build alongside LLVM declarations. Alternatively, put `sdk_source`
and LLVM paths in ignored `.sdk.json` and run plain `build`. `new` downloads
nothing; existing project pins change only through explicit configure. This
workflow has [Linux evidence](docs/acceptance/sdk_install_acceptance.md#pinned-https-sdk-acquisition-linuxx86_64-2026-11-06);
macOS/Windows need their affected-feature replay.


## Next User Workflow

[The active plan](TODO.md) retains the remaining native store/collision gates
and qualifies a Linux preview beyond the implemented first-build/acquisition
workflow. The zlib source-port adapter can proceed in
parallel against fixed contracts. Full-stage metadata, general updates,
self-upgrade, multi-port resolution and Studio are later increments. A preview
candidate is qualified for its declared hosts before wider support is claimed.

The implemented developer flow keeps `new` source-only and can acquire a
missing SDK on first configure/build from explicit pinned inputs. Default
channels and publication require approved trust and actual producer artifact
promotion before production CLI adoption.

## Platforms and Requirements

External Clang/LLD/LLVM ar and required vendor OS SDKs are prerequisites; neither
CRT nor this CLI bundles LLVM. The CLI ships toward one executable per host
with declared OS dependencies, resources/notices/certificate data and no
permanent launcher. Current committed binaries are local test snapshots with
per-host provenance in [bin/README](bin/README.md).

The independent CRT host-tools profile supplies mksh, toybox, awk and make for
explicit shell/port workflows. It is separate from native app builds and target
SDKs. Windows workflows using real symlinks require Developer Mode; settings
are not changed automatically. Studio remains a separate future GUI consumer.

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
