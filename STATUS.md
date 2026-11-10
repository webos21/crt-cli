# Project Status

Evidence-based SDK/CLI snapshot refreshed **2026-11-10** at the owner's explicit
request. Reviewed CLI product source `a55c0d0` and CRT `8e89ee1`; the Windows
Preview below is built against the unchanged public SDK from CRT `b31276a`.
Linux/macOS Preview and other-host records retain their own exact source commits.
[TODO](TODO.md) owns current work, [HISTORY](HISTORY.md) completed work and
[acceptance](docs/acceptance/sdk_install_acceptance.md) exact combinations.

## Implemented and exercised scope

| Surface | Evidence boundary |
| --- | --- |
| Native CLI/project builder | CRT-native C++17, crt.json -> direct LLVM argv; C/C++ source-only new, first build/configure/build/test/run; external LLVM/OS SDKs required |
| SDK install/receipts/selection | Immutable native Dev store, exact contracts, explicit repair/recovery and format-1 project pins; five-pair records retain their original baselines |
| First-build/HTTPS acquisition | Explicit pinned SDK source, CA/size/digest checks, installed-identity reuse and lock-preserving failure; Linux/macOS/Windows records are [per-host](docs/acceptance/sdk_install_acceptance.md) |
| Independent host tools | Pinned CRT shell/make/applets for port workflows; control/app paths need no host Python, shell or CMake; [host environment](docs/acceptance/host_environment_acceptance.md) owns loader/OS limits |
| D3 zlib source port | Native execution, separate store, ELF/Mach-O/PE audit, provider choice, port locks and packaged oracle have Linux/x86_64, macOS/arm64 and Windows/x64 records; the bounded contract is [approved and adopted](docs/compatibility.md#d3-bounded-adapter-adoption-2026-11-09); Windows/arm64 is separate |
| Restricted HTTPS redirects | Opt-in `github-release` / explicit-authority policy for `fetch` and `sdk acquire` (default stays refusal); Linux/x86_64 ([record](docs/acceptance/preview_acceptance.md#restricted-https-redirects-and-github-acquisition-2026-11-10)) and macOS/arm64 ([record](docs/acceptance/preview_acceptance.md#macos-cli-preview-release-2026-11-10)) each pass 54/54 replay cases and a real GitHub acquisition of their Dev SDK Preview; Windows/x64 also passes 54/54 and real acquisition ([record](docs/acceptance/preview_acceptance.md#windowsx64-cli-preview-release-2026-11-10)) |
| Linux/x86_64 CLI Preview | [Published prerelease](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261110.linux-x86_64.1) (unsigned, digests only); the packaged executable acquires the public Dev SDK and builds/tests/runs C and C++ with an empty `PATH`; [record](docs/acceptance/preview_acceptance.md#linuxx86_64-cli-preview-release-2026-11-10) |
| macOS/arm64 CLI Preview | [Published prerelease](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261110.macos-arm64.1) (unsigned, ad-hoc linker signature only, digests only, not notarized); built from its sources by the public macOS Dev SDK alone (native 942 checks); the packaged executable acquires that SDK from GitHub and builds/tests/runs C and C++ under Seatbelt with an empty `PATH`; [record](docs/acceptance/preview_acceptance.md#macos-cli-preview-release-2026-11-10) |
| Windows/x64 CLI Preview | [Published prerelease](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261110.windows-x86_64.1) (unsigned, digests only); public Dev SDK-only self-build, 54/54 redirect cases, 12/12 SDK-free package and 39/39 packaged GitHub/C/C++ workflow; [record](docs/acceptance/preview_acceptance.md#windowsx64-cli-preview-release-2026-11-10) |

Windows/x64's earlier concurrent-install crash was root-caused/fixed in the
producer libunwind recipe and replayed with the fixed SDK; its retained failure
is not a current unresolved gate. Windows/arm64 D2 is distinct from unqualified
D3. Current committed binaries retain their [dated identities](bin/README.md).
Apart from the Linux/x86_64, macOS/arm64 and Windows/x64 Previews above, none of these records is a CLI release or a universal latest-source support matrix.

## Limits and ordered follow-up

- CLI SDK installation/selection currently supports native `dev` at `02-cxx`;
  general profiles and Full `06-web` installation are separate work.
- D3 adapter review is approved (CRT `90989ac`) and adopted for new publications; Windows/x64 evidence is reused only for
  unchanged policy/product inputs; Windows/arm64 D3 is separate.
- The retained Linux SDK's C shared SDK-provider control resolves host libz;
  this is [unendorsed diagnostic evidence](docs/acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09),
  not SDK shared qualification. Producer 6.4 retains policy/runtime-search review.
- The preview targets Dev SDK + CLI. Linux/x86_64, macOS/arm64 and Windows/x64 CLI Previews are published;
  Linux/aarch64 and Windows/arm64 have none. The Dev SDK Previews of Linux, macOS and Windows/x64 are published by CRT. Existing Full
  assembly/approval records remain valid for their own inputs; Full does not block the CLI preview.
- D4 requires a new SQLite amalgamation adapter plus libpng -> zlib dependency,
  variant/key/lock/cache handling; existing configure-only D3 is not that resolver.
- Publisher authentication stays `not_verified`: releases rely on caller-pinned digests and an explicit CA. Official default
  trust/URLs, signing, and default/project SDK pin adoption remain open. The macOS executable is not Developer ID signed or notarized and a quarantined (browser) download was not tested. Developer test CA,
  rehearsal assets and root unresolved locks are not promotion.
- General updates/self-upgrade, GC, wider profiles/cross targets and Studio retain
  their later queues. No new runtime/SDK build or default pin is adopted here.

Native evidence and limits belong to [compatibility](docs/compatibility.md) and
acceptance. Future STATUS refreshes remain explicit owner requests.
