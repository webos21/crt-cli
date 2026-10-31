# Prebuilt `crt-cli` (temporary)

For hands-on testing and to steer the contract, the final executable of each host is
committed here until a release process exists. These are **local, unpromoted**
builds, not releases: they are built by the CLI itself (`crt-cli build`) from the
committed `src/` and `include/` against a locally produced Development SDK whose
provenance is recorded per host below (some require `--allow-dirty-sdk`). Do not treat them as signed or reproducible
release artifacts.

| Host | Path | Status |
| --- | --- | --- |
| macOS/arm64 | `bin/macos/crt-cli` | present; SHA-256 `e1d65adafc806c21b227c61d966947bdbc45003f15d80f072b1c6b9af8d6de44` (3,023,032 bytes; 2026-10-30 replay of `754ca29`) |
| Linux/x86_64 | `bin/linux-x64/crt-cli` | present; SHA-256 `2a84c61debf31ce87c441f21ca4b0111701e6773567199aa5a2ec71a34e79967` (12,066,824 bytes; 2026-10-31 D1 experiment, base `36c6f65` plus signature/test delta, clean local CRT `f826397`) |
| Linux/aarch64 | `bin/linux-arm64/crt-cli` | present; SHA-256 `94ba0f0c37159e6aa07fe78ea637fd202b658cab45daca70680c380f12450eea` (10,994,232 bytes; 2026-10-30 replay, dirty local SDK) |
| Windows/x64 | `bin/win-x64/crt-cli.exe` | present; SHA-256 `183e1e6c7f60cfc823ca0e4e260ff913b252798b5b0d0daebe82c9f3a7529806` (16,883,200 bytes, built `-O1 -g`) |
| Windows/arm64 | `bin/win-arm64/crt-cli.exe` | present; SHA-256 `2cfd57291e983cf2a171c29271a3319aac05c558f0586a5b30c69a50005e03a5` (18,779,136 bytes, built `-O1 -g`; 2026-10-31 replay with `fetch` and UTF-8 narrow strings, SDK extracted from a local release-asset ZIP of CRT `85546d9`) |

macOS ships only arm64, so it keeps the single `bin/macos/` directory. Linux and
Windows are split by architecture: `linux-x64`, `linux-arm64`, `win-x64`, `win-arm64`.

The macOS binary imports only `/usr/lib/libSystem.B.dylib` and has no RPATH. Evidence:
[macOS acceptance](../docs/acceptance/bootstrap_acceptance.md#macos-revalidation-after-windows-and-linux-changes-2026-10-30).

The Windows/x64 binary imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; it was built by the CLI itself
against a clean local Dev SDK (CRT `d054a66`). Evidence:
[Windows acceptance](../docs/acceptance/bootstrap_acceptance.md#native-json-builder-on-windowsx64-2026-10-29).
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

The Linux/x86_64 binary is the 2026-10-31 D1 audit replay's bare executable:
no DT_NEEDED/RPATH/RUNPATH, interpreter `/lib64/ld-linux-x86-64.so.2` only.
With SDK/source/provider/compiler access hidden, help/version/doctor pass;
PAL images are the CLI, loader and vdso. It uses a fresh clean CRT `f826397` Dev
SDK with `crt-narrow-encoding-1`. The native suite passes 692 checks, including
owned-descendant timeout control, and 32 HTTPS/signature cases pass with SDKs,
sources and providers hidden. Explicit-key signatures are experimental; no
official publisher key or complete D1 acceptance is claimed. See
[D1 audit](../docs/acceptance/bootstrap_acceptance.md#d1-publisher-and-host-environment-audit-2026-10-31).
The prior `a611f6c5...` executable/config are retained under the D1 evidence root;
the earlier `a4419776...` result remains in the preceding revalidation record.

The Linux/aarch64 binary is the 2026-10-30 replay's bare executable: no DT_NEEDED/RPATH/RUNPATH,
interpreter `/lib/ld-linux-aarch64.so.1` only. It was built against a local Dev SDK from CRT `641b034`
plus an uncommitted fix to the aarch64 `O_NOFOLLOW`/`O_DIRECTORY` values and a Linux/aarch64 toolchain-export
allowance (explicit dirty local candidate). The first aarch64 build has no earlier aarch64 binary, so the
bootstrap loop proves a stage1/stage2 fixed point, not a bootstrap from a previous release. See
[Linux/aarch64 replay](../docs/acceptance/bootstrap_acceptance.md#linuxaarch64-native-replay-2026-10-30).
