# Prebuilt `crt-cli` (temporary)

For hands-on testing and to steer the contract, the final executable of each host is
committed here until a release process exists. These are **local, unpromoted**
builds, not releases: they are built by the CLI itself (`crt-cli build`) from the
committed `src/` and `include/` against a locally produced Development SDK whose
provenance is dirty (`--allow-dirty-sdk`). Do not treat them as signed or reproducible
release artifacts.

| Host | Path | Status |
| --- | --- | --- |
| macOS/arm64 | `bin/macos/crt-cli` | present; SHA-256 `e1d65adafc806c21b227c61d966947bdbc45003f15d80f072b1c6b9af8d6de44` (3,023,032 bytes; 2026-10-30 replay of `754ca29`) |
| Linux/x86_64 | `bin/linux-x64/crt-cli` | present; SHA-256 `a44197764f1361b6691dc145f03fd794a880eb9fd1d4d5601a669f4cb0f3044d` (10,928,472 bytes) |
| Linux/aarch64 | `bin/linux-arm64/crt-cli` | pending: build on a Linux/aarch64 host |
| Windows/x64 | `bin/win-x64/crt-cli.exe` | present; SHA-256 `183e1e6c7f60cfc823ca0e4e260ff913b252798b5b0d0daebe82c9f3a7529806` (16,883,200 bytes, built `-O1 -g`) |
| Windows/arm64 | `bin/win-arm64/crt-cli.exe` | pending: build on a Windows/arm64 host |

macOS ships only arm64, so it keeps the single `bin/macos/` directory. Linux and
Windows are split by architecture: `linux-x64`, `linux-arm64`, `win-x64`, `win-arm64`.

The macOS binary imports only `/usr/lib/libSystem.B.dylib` and has no RPATH. Evidence:
[macOS acceptance](../docs/acceptance/bootstrap_acceptance.md#macos-revalidation-after-windows-and-linux-changes-2026-10-30).

The Windows/x64 binary imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; it was built by the CLI itself
against a clean local Dev SDK (CRT `d054a66`). Evidence:
[Windows acceptance](../docs/acceptance/bootstrap_acceptance.md#native-json-builder-on-windowsx64-2026-10-29).
Rebuild outputs and intermediate objects live in the ignored `out/` directory.

The Linux/x86_64 binary is the 2026-10-30 merged-source replay's bare executable:
no DT_NEEDED/RPATH/RUNPATH, interpreter `/lib64/ld-linux-x86-64.so.2` only.
With SDK/source/provider/compiler access hidden, help/version/doctor pass;
PAL images are the CLI, loader and vdso. It uses CRT `a46e430` plus the recorded
non-Windows SETPGROUP fix (explicit dirty local candidate). Native suite,
execve audit and the previous-binary -> new-SDK -> self-build loop pass. See
[Linux revalidation](../docs/acceptance/bootstrap_acceptance.md#linux-revalidation-after-windows-changes-2026-10-30).
The prior `cfc41532...` Linux result remains historical, not overwritten evidence.
