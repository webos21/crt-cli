# Prebuilt `crt-cli` (temporary)

For hands-on testing and to steer the contract, the final executable of each host is
committed here until a release process exists. These are **local, unpromoted**
builds, not releases: they are built by the CLI itself (`crt-cli build`) from the
committed `src/` and `include/` against a locally produced Development SDK whose
provenance is dirty (`--allow-dirty-sdk`). Do not treat them as signed or reproducible
release artifacts.

| Host | Path | Status |
| --- | --- | --- |
| macOS/arm64 | `bin/macos/crt-cli` | present; SHA-256 `152b027f053fa7150dc7a3170ee361fd9615e2f305a915d45bef984576a5a55b` (2,983,112 bytes) |
| Linux/x86_64 | `bin/linux-x64/crt-cli` | present; SHA-256 `cfc41532e6a582982dcd19831d0e6dcf7919be7d189558821845e257432fe1d5` (10,804,200 bytes) |
| Linux/aarch64 | `bin/linux-arm64/crt-cli` | pending: build on a Linux/aarch64 host |
| Windows/x64 | `bin/win-x64/crt-cli.exe` | present; SHA-256 `8ab2912d1b55537d89ecf99747dc8a51067aa4e764eb09261752c43049e6c6e9` (16,654,848 bytes, built `-O1 -g`) |
| Windows/arm64 | `bin/win-arm64/crt-cli.exe` | pending: build on a Windows/arm64 host |

macOS ships only arm64, so it keeps the single `bin/macos/` directory. Linux and
Windows are split by architecture: `linux-x64`, `linux-arm64`, `win-x64`, `win-arm64`.

The macOS binary imports only `/usr/lib/libSystem.B.dylib` and has no RPATH. Evidence:
[macOS acceptance](../docs/acceptance/bootstrap_acceptance.md#incremental-build-cache-and-committed-binary-2026-10-29).

The Windows/x64 binary imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; it was built by the CLI itself
against a clean local Dev SDK (CRT `d054a66`). Evidence:
[Windows acceptance](../docs/acceptance/bootstrap_acceptance.md#native-json-builder-on-windowsx64-2026-10-29).
Rebuild outputs and intermediate objects live in the ignored `out/` directory.

The Linux/x86_64 binary has no `DT_NEEDED`, RUNPATH or RPATH and only the interpreter
`/lib64/ld-linux-x86-64.so.2`. With the SDK, sources, replay tree and `PATH` hidden in a
Bubblewrap mount namespace (cleared environment), `--version`, `--help` and
`doctor --bootstrap` pass and the mapped images are the loader, the binary and the vdso.
It was built by stage1 (itself built by an earlier seed) from a clean `out/native` against
the CRT `d054a66` Development SDK; stage1 and stage2 are byte-identical. This is a local
check, not the CRT Bubblewrap/execve replay; see [HISTORY](../HISTORY.md).
