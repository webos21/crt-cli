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
| Linux | `bin/linux/crt-cli` | pending: build on a Linux host |
| Windows/x64 | `bin/windows/crt-cli.exe` | present; SHA-256 `8ab2912d1b55537d89ecf99747dc8a51067aa4e764eb09261752c43049e6c6e9` (16,654,848 bytes, built `-O1 -g`) |

The macOS binary imports only `/usr/lib/libSystem.B.dylib` and has no RPATH. The Windows binary imports only `kernel32.dll` and
`api-ms-win-core-synch-l1-2-0.dll`; it was built from the uncommitted Windows source delta against a clean local Dev SDK
(`d054a66`), see the [Windows acceptance](../docs/acceptance/bootstrap_acceptance.md#native-json-builder-on-windowsx64-2026-10-29). Evidence:
[macOS acceptance](../docs/acceptance/bootstrap_acceptance.md#incremental-build-cache-and-committed-binary-2026-10-29).
Rebuild outputs and intermediate objects live in the ignored `out/` directory.
