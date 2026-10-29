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
| Windows | `bin/windows/crt-cli.exe` | pending: build on a Windows host |

The macOS binary imports only `/usr/lib/libSystem.B.dylib` and has no RPATH. Evidence:
[macOS acceptance](../docs/acceptance/bootstrap_acceptance.md#incremental-build-cache-and-committed-binary-2026-10-29).
Rebuild outputs and intermediate objects live in the ignored `out/` directory.
