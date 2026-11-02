# Prebuilt `crt-cli` (temporary)

For hands-on testing and to steer the contract, the final executable of each host is
committed here until a release process exists. These are **local, unpromoted**
builds, not releases: they are built by the CLI itself (`crt-cli build`) from the
committed `src/` and `include/` against a locally produced Development SDK whose
provenance is recorded per host below (some require `--allow-dirty-sdk`). Do not treat them as signed or reproducible
release artifacts.

| Host | Path | Status |
| --- | --- | --- |
| macOS/arm64 | `bin/macos/crt-cli` | present; SHA-256 `72577b9b7dd1e602635ccdabef0fe728e66441c7bd9b07607ae8994ff2de949e` (3,538,040 bytes; 2026-11-01 replay of `a9d43c6`, clean local CRT `8db69f4`) |
| Linux/x86_64 | `bin/linux-x64/crt-cli` | present; SHA-256 `2a84c61debf31ce87c441f21ca4b0111701e6773567199aa5a2ec71a34e79967` (12,066,824 bytes; 2026-10-31 D1 experiment, base `36c6f65` plus signature/test delta, clean local CRT `f826397`) |
| Linux/aarch64 | `bin/linux-arm64/crt-cli` | present; SHA-256 `9e250db8589077f376091e4b506259601b1ec12b96713db9aef9059e41bbc9cf` (12,444,248 bytes; 2026-11-02 replay of `9e0110f`, clean local CRT `c4e19f3`) |
| Windows/x64 | `bin/win-x64/crt-cli.exe` | present; SHA-256 `a04ff851fb1156a161f01f308c368648ba1b091a001977e449fb86009820237a` (20,404,224 bytes, built `-O1 -g`; 2026-11-02 replay of `1499a7b`, clean local CRT `b4c5b5a`) |
| Windows/arm64 | `bin/win-arm64/crt-cli.exe` | present; SHA-256 `6fce97e028c9956716ad1729d8655532282944f8dfd7f9ec33f44d92cf0e787c` (20,286,976 bytes, built `-O1 -g`; 2026-11-02 replay of the latest sources against a clean local SDK from CRT `d12e210`) |

macOS ships only arm64, so it keeps the single `bin/macos/` directory. Linux and
Windows are split by architecture: `linux-x64`, `linux-arm64`, `win-x64`, `win-arm64`.

The macOS binary imports only `/usr/lib/libSystem.B.dylib` and has no RPATH.
The current-source native suite passes 732 checks; cold/warm self-build, the
previous-binary bootstrap loop, SDK-hidden help/version/doctor and all 32
HTTPS/signature cases pass against a fresh clean CRT `8db69f4` Dev SDK.
The direct make space-SHELL limitation remains separate from the CLI workflow.
Evidence: [macOS acceptance](../docs/acceptance/bootstrap_acceptance.md#macos-latest-source-revalidation-2026-11-01).
Previous binary/config bytes are retained under ignored `out/macos-revalidation-20261101/previous/`.

The Windows/x64 binary is the 2026-11-02 replay's bare executable: it imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`, was built by the CLI itself
against a fresh clean Dev SDK from CRT `b4c5b5a` (`v0.4.0-cli.20261102`, includes `d12e210`), and passes 844 native checks, the bare audit and 32 SDK-hidden
HTTPS/signature cases; see the [Windows/x64 latest-source record](../docs/acceptance/bootstrap_acceptance.md#windowsx64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02).
The previous `c9707454...` binary is retained under ignored `out/windows-x64-20261102/previous/`. That earlier binary was built by the CLI itself
against a local `development` Dev SDK (CRT `fa5dc30` plus the recorded build-time uncommitted owned-spawn PAL/test fix).
The same producer inputs are now committed as [`8db69f4`](https://github.com/webos21/crt/commit/8db69f404dcfa70b5aa0569cae51a962fd0615d2);
this does not relabel the SDK as a clean build.
Evidence: [Windows fix acceptance](../docs/acceptance/bootstrap_acceptance.md#windowsx64-owned-descendant-timeout-fix-2026-11-01).
The previous binary and failed candidate records are retained under ignored `out/`.
Rebuild outputs and intermediate objects live in the ignored `out/` directory.

The Windows/arm64 binary imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll` (PE machine `0xAA64`). It is the
2026-10-31 replay's bare executable; it includes the `fetch` command and embeds CRT's UTF-8 active-code-page manifest (the
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

The Linux/x86_64 binary is the 2026-11-02 environment replay's bare executable:
no DT_NEEDED/RPATH/RUNPATH, interpreter `/lib64/ld-linux-x86-64.so.2` only.
With SDK/source/provider/compiler access hidden, help/version/doctor pass;
PAL images are the CLI, loader and vdso. It uses a fresh clean CRT `f826397` Dev
SDK with `crt-narrow-encoding-1`. The native suite passes 754 checks, including
owned-descendant timeout control and shared child-environment diagnostics;
SDK-hidden control/value-redaction checks and 32 HTTPS/signature cases pass with SDKs,
sources and providers hidden. Explicit-key signatures are experimental; no
official publisher key or complete D1 acceptance is claimed. See
[environment acceptance](../docs/acceptance/bootstrap_acceptance.md#linux-child-environment-diagnostics-2026-11-02).
The prior `2a84c61d...` executable/config are retained under `out/d1-env-20261102/previous/`;
the `a611f6c5...` executable/config remain under the earlier D1 evidence root;
the earlier `a4419776...` result remains in the preceding revalidation record.

The Linux/aarch64 binary is the 2026-11-02 replay's bare executable, SHA-256
`9e250db8589077f376091e4b506259601b1ec12b96713db9aef9059e41bbc9cf`
(12,444,248 bytes). It uses a fresh clean CRT `c4e19f3` Development SDK with
`crt-narrow-encoding-1`; no DT_NEEDED/RPATH/RUNPATH, interpreter
`/lib/ld-linux-aarch64.so.1` only. All 766 native checks, SDK-hidden control/
redaction and 32 HTTPS/signature cases pass. The previous aarch64 executable
bootstraps current sources to a byte-identical stage1/stage2 fixed point.
Previous bytes remain under `out/linux-arm64-revalidation-20261102/previous/`.
Evidence: [Linux/aarch64 acceptance](../docs/acceptance/bootstrap_acceptance.md#linuxaarch64-latest-source-revalidation-2026-11-02).
