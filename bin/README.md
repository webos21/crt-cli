# Prebuilt `crt-cli` (temporary)

For hands-on testing and to steer the contract, the final executable of each host is
committed here until a release process exists. These are **local, unpromoted**
builds, not releases: they are built by the CLI itself (`crt-cli build`) from
identified `src/` and `include/` snapshots (including explicitly recorded local
working-tree candidates) against a locally produced Development SDK whose
provenance is recorded per host below (some require `--allow-dirty-sdk`). Do not treat them as signed or reproducible
release artifacts.

| Host | Path | Status |
| --- | --- | --- |
| macOS/arm64 | `bin/macos/crt-cli` | present; SHA-256 `acb4f7e1abd0c2b1f7f0516326b46c1a351525cb2b55fdc0cc2a872647e15b62` (3,927,832 bytes; 2026-11-05 Stage 2 replay of `da3f3a9`, clean local CRT `784389b`) |
| Linux/x86_64 | `bin/linux-x64/crt-cli` | present; SHA-256 `9b82bd300379b45be4cb36becfbef56bf41b1ff5cc3d2cb510b9e60447b10284` (13,889,400 bytes; 2026-11-02 D1 working-tree candidate based on `adc03b0`, clean local CRT `49879bf`) |
| Linux/aarch64 | `bin/linux-arm64/crt-cli` | present; SHA-256 `69118ac17f998ea0cc42ff7fcf422155ee3fefd016a617374a5701dd439bbfef` (16,352,720 bytes; 2026-11-05 Stage 2 replay of `9d711d2`, clean local CRT `78ed80f`) |
| Windows/x64 | `bin/win-x64/crt-cli.exe` | present; SHA-256 `9948bcd919d6f0a93f2564902124fbbcf9edaf9d3f785caba4ffcf5f1771fe2d` (29,552,640 bytes, built `-O1 -g`; 2026-11-04 D2 installer replay of `2df5610` plus the Windows edits, clean local CRT `1ee0379`) |
| Windows/arm64 | `bin/win-arm64/crt-cli.exe` | present; SHA-256 `3f587b71177958b1c3bbfc03e8f695c48fe8a3144c33abcdf7737ef188032c52` (29,425,664 bytes, built `-O1 -g`; 2026-11-04 D2 installer replay of `13bba78`, clean local CRT `b20a460`) |

macOS ships only arm64, so it keeps the single `bin/macos/` directory. Linux and
Windows are split by architecture: `linux-x64`, `linux-arm64`, `win-x64`, `win-arm64`.

The macOS binary imports only `/usr/lib/libSystem.B.dylib` and has no RPATH.
The current-source native suite passes 869 checks plus the archive and SDK-store suites; cold/warm self-build,
the previous-binary bootstrap loop and SDK-hidden help/version/doctor pass against a clean CRT `784389b` Dev SDK,
and the D2 install, contract and project-selection replay passes 69/69 with it
([D2 acceptance](../docs/acceptance/sdk_install_acceptance.md#toolchain-contract-and-project-sdk-selection-replay-stage-2-macosarm64-2026-11-05)).
The direct make space-SHELL limitation remains separate from the CLI workflow.
Earlier D1 evidence: [macOS acceptance](../docs/acceptance/bootstrap_acceptance.md#macos-latest-source-revalidation-2026-11-01).

The Windows/x64 binary is the 2026-11-04 D2 replay's bare executable (it adds `sdk install|list|verify|recover`; 857 native, 151 archive and 66 SDK-store
checks, 62/62 installer checks, previous `a04ff851...` bytes under ignored `out/windows-d2-20261104/previous/`; see the
[D2 Windows record](../docs/acceptance/sdk_install_acceptance.md#windowsx64-isolated-replay-2026-11-04)). The following paragraph describes its predecessor, the 2026-11-02 refresh, whose method is unchanged: the bare executable: it imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`, was built by the CLI itself
against a fresh clean Dev SDK from CRT `b4c5b5a` (`v0.4.0-cli.20261102`, includes `d12e210`), and passes 844 native checks, the bare audit and 32 SDK-hidden
HTTPS/signature cases; see the [Windows/x64 latest-source record](../docs/acceptance/bootstrap_acceptance.md#windowsx64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02).
The previous `c9707454...` binary is retained under ignored `out/windows-x64-20261102/previous/`. That earlier binary was built by the CLI itself
against a local `development` Dev SDK (CRT `fa5dc30` plus the recorded build-time uncommitted owned-spawn PAL/test fix).
The same producer inputs are now committed as [`8db69f4`](https://github.com/webos21/crt/commit/8db69f404dcfa70b5aa0569cae51a962fd0615d2);
this does not relabel the SDK as a clean build.
Evidence: [Windows fix acceptance](../docs/acceptance/bootstrap_acceptance.md#windowsx64-owned-descendant-timeout-fix-2026-11-01).
The previous binary and failed candidate records are retained under ignored `out/`.
Rebuild outputs and intermediate objects live in the ignored `out/` directory.

The Windows/arm64 binary is the 2026-11-04 D2 replay's bare executable (it adds `sdk install|list|verify|recover`; 870 native, 151 archive and 66 SDK-store
checks, 62/62 installer checks, imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`, PE machine `0xAA64`; previous `6fce97e0...` bytes under ignored
`out/windows-arm64-d2-20261104/previous/`; see the
[D2 Windows/arm64 record](../docs/acceptance/sdk_install_acceptance.md#windowsarm64-isolated-replay-2026-11-04)). The following paragraph describes its
predecessor, the 2026-10-31 replay's bare executable, whose method is unchanged: it imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll` (PE machine `0xAA64`); it includes the `fetch` command and embeds CRT's UTF-8 active-code-page manifest (the
SDK's link policy supplies it), so it needs Windows 10 1903 or later for non-ASCII paths and its `doctor` reports
`narrow_encoding`. The SDK was extracted from the locally collected `crt-dev-release` ZIP of a clean CRT `85546d9`
(version `v0.4.0-rehearsal.20261031`, not published or promoted). The earlier
2026-10-30 binary (`e25b449b...`, no `fetch`) was a stage1/stage2 fixed point of the sources before that command; the new
one's loop was not repeated. See
[Windows/arm64 replay](../docs/acceptance/bootstrap_acceptance.md#windowsarm64-native-replay-2026-10-30) and the
[fetch, Unicode and release-asset record](../docs/acceptance/bootstrap_acceptance.md#windowsarm64-https-fetch-unicode-paths-and-release-asset-sdk-2026-10-31) and
[UTF-8 narrow strings](../docs/acceptance/bootstrap_acceptance.md#windows-utf-8-narrow-strings-implemented-2026-10-31).
The 2026-11-02 refresh adds the shared child-environment, publisher-signature and owned-descendant timeout sources; see
[Windows/arm64 latest-source record](../docs/acceptance/bootstrap_acceptance.md#windowsarm64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02).

The Linux/x86_64 binary is the selected D1 source snapshot
`2aa32f07ea920ee4405970f194ccdf1183608f3ba8675ca28e60e80920930155`.
It uses the same clean CRT `49879bf` Development SDK
(`v0.4.0-sourcefix.20261102.5`) with dirty opt-in disabled. All 767 native
checks, cold 13/0 and warm 0/13, 31 isolated host-tools/resource cases and 32
SDK-free HTTPS/signature cases pass. No DT_NEEDED/RPATH/RUNPATH; only the OS
loader and vdso are mapped. New scoped doctor, non-interactive shell and
resource-pin checks have [D1 evidence](../docs/acceptance/host_environment_acceptance.md).
The previous `6457b1a6...` binary and config remain under
`out/tranche2-20261102/previous/`. Plain doctor keeps HTTPS/integrity
unavailable results explicit. This is not a production release; native replay
of these new features on the other hosts remains open.

The Linux/aarch64 binary is the 2026-11-05 Stage 2 replay's bare executable (project SDK selection and the stored toolchain contract; 841 native, 158 archive and 83 SDK-store
checks, 69/69 installer checks, no DT_NEEDED/RPATH/RUNPATH, interpreter `/lib/ld-linux-aarch64.so.1` only, previous `fc7842f3...` bytes under ignored
`out/linux-arm64-s2-20261105/previous-crt-cli`; see the
[Stage 2 Linux/aarch64 record](../docs/acceptance/sdk_install_acceptance.md#toolchain-contract-and-project-sdk-selection-replay-stage-2-linuxaarch64-2026-11-05)).
The following paragraph describes its predecessor, the 2026-11-04 D2 replay's bare executable (it adds `sdk install|list|verify|recover`; 779 native, 158 archive and 66 SDK-store
checks, 48/48 installer checks, no DT_NEEDED/RPATH/RUNPATH, interpreter `/lib/ld-linux-aarch64.so.1` only, previous `9e250db8...` bytes under ignored
`out/linux-arm64-d2-20261104/previous-crt-cli`; see the
[D2 Linux/aarch64 record](../docs/acceptance/sdk_install_acceptance.md#linuxaarch64-isolated-replay-2026-11-04)). The following paragraph describes its
predecessor, the 2026-11-02 replay's bare executable, SHA-256
`9e250db8589077f376091e4b506259601b1ec12b96713db9aef9059e41bbc9cf`
(12,444,248 bytes). It uses a fresh clean CRT `c4e19f3` Development SDK with
`crt-narrow-encoding-1`; no DT_NEEDED/RPATH/RUNPATH, interpreter
`/lib/ld-linux-aarch64.so.1` only. All 766 native checks, SDK-hidden control/
redaction and 32 HTTPS/signature cases pass. The previous aarch64 executable
bootstraps current sources to a byte-identical stage1/stage2 fixed point.
Previous bytes remain under `out/linux-arm64-revalidation-20261102/previous/`.
Evidence: [Linux/aarch64 acceptance](../docs/acceptance/bootstrap_acceptance.md#linuxaarch64-latest-source-revalidation-2026-11-02).
