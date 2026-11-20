# Project Status

SDK and CLI evidence snapshot refreshed **2026-11-20** at the owner's request,
reviewing CLI `ec498ce` and CRT `9867851`. This refresh reviews the recorded
native results and the public release inventory. Each result retains its own
source revision, SDK pin and host scope.
[TODO](TODO.md) owns current work, [HISTORY](HISTORY.md) completed work and
[acceptance](docs/acceptance/sdk_install_acceptance.md) exact combinations.

## Implemented and exercised scope

| Surface | Evidence boundary |
| --- | --- |
| Native CLI/project builder | CRT-native C++17, crt.json -> direct LLVM argv; C/C++ source-only new, first build/configure/build/test/run; external LLVM/OS SDKs required |
| SDK install/receipts/selection | Immutable native Dev store, exact contracts, explicit repair/recovery and format-1 project pins; five-pair records retain their original baselines |
| First-build/HTTPS acquisition | Explicit pinned SDK source, CA/size/digest checks, installed-identity reuse and lock-preserving failure; Linux/macOS/Windows records are [per-host](docs/acceptance/sdk_install_acceptance.md) |
| Independent host tools | Pinned CRT shell/make/applets for port workflows; control/app paths need no host Python, shell or CMake; [host environment](docs/acceptance/host_environment_acceptance.md) owns loader/OS limits |
| D3 zlib source port | Native execution, separate store, ELF/Mach-O/PE audit, provider choice, port locks and the oracle run in hosted integration on all five host pairs. Exact policy and adoption records are in [compatibility](docs/compatibility.md#d3-bounded-adapter-adoption-2026-11-09) and [CI acceptance](docs/acceptance/ci_acceptance.md#windowsarm64-hosted-integration-expansion-2026-11-20). |
| Restricted HTTPS redirects | `fetch` and `sdk acquire` require `github-release` or an explicit authority policy to follow redirects. All five integration rows pass 54/54 against pinned public SDK inputs. The published Windows/arm64 package also acquires its SDK over GitHub HTTPS. [Hosted integration](docs/acceptance/ci_acceptance.md#windowsarm64-hosted-integration-expansion-2026-11-20) · [Public package](docs/acceptance/preview_acceptance.md#windowsarm64-workflow-preview-release-2026-11-20). |
| CLI Previews | Signed workflow-built prereleases are public for all five host pairs. The CI bootstrap inputs are listed below. [Release evidence](docs/acceptance/preview_acceptance.md) · [CI acceptance](docs/acceptance/ci_acceptance.md). |
| Publisher signatures | The owner signs offline and attaches `<asset>.sig`; `sdk acquire`, `sdk install`, `sdk repair` and `fetch` verify it with the pinned key and report `signature_verified`, otherwise `not_verified`. Native refusal and acceptance replays (37 test-key and 10 real-key cases) are recorded on macOS/arm64, Windows/x64, Windows/arm64, Linux/x86_64 and, on 2026-11-19, Linux/arm64 ([record](docs/acceptance/ci_acceptance.md#linuxarm64-sdk-publisher-signature-replay-2026-11-19)) |

**Pinned CI inputs** from `.github/ci/pins.json`. Every row pins the CLI and SDK signatures and enables `sdk_source_publisher`.

| Host | Bootstrap CLI Preview | Pinned Dev SDK Preview |
| --- | --- | --- |
| Linux/x86_64 | `v0.1.0-preview.20261116.linux-x86_64.1` | `v0.0.1-preview.20261118.linux.x64.1` |
| Linux/arm64 | `v0.1.0-preview.20261116.linux-arm64.1` | `v0.0.1-preview.20261110.linux.arm64.2` |
| macOS/arm64 | `v0.1.0-preview.20261117.macos-arm64.1` | `v0.0.1-preview.20261110.macos.1` |
| Windows/x64 | `v0.1.0-preview.20261117.windows-x86_64.1` | `v0.0.1-preview.20261110.windows.1` |
| Windows/arm64 | `v0.1.0-preview.20261118.windows-arm64.1` | `v0.0.1-preview.20261118.windows.arm64.1` |

The required `CI gate` covers six native jobs across five host pairs, with Ubuntu 24.04 and 26.04 on Linux/x86_64. Signed public
host-tools bundles feed pins, redirects and five port suites in the 15-job daily/manual integration workflow. The full workflow passed
on the Windows/arm64 expansion branch and the adoption passed main CI. Scheduled cadence is tracked in [CI Operations](TODO.md#ci-operations).
[Integration and CI evidence](docs/acceptance/ci_acceptance.md#windowsarm64-hosted-integration-expansion-2026-11-20).

The signed Windows/arm64 workflow Preview `v0.1.0-preview.20261120.windows-arm64.1` passes the native C/C++ public package replay
and D1 32/32 with the original deadlines. The CI bootstrap row uses the signed hand-built 20261118 release.
[Publication and public replay](docs/acceptance/preview_acceptance.md#windowsarm64-workflow-preview-release-2026-11-20).

The Windows/x64 private-DLL protocol passed a local 27-case replay with actual shared AWK, app-local DLLs, cwd/PATH collisions,
payload refusal and same-pin recovery. The public CI bundles use static host tools.
[Windows/x64 local qualification](docs/acceptance/host_environment_acceptance.md#windowsx64-private-dll-host-tools-replay-2026-11-20).

Windows/x64's earlier concurrent-install crash was fixed in the producer libunwind recipe and replayed with the fixed SDK.
The Windows port-store publication test uses independent CLI processes. The CRT in-process concurrent read failure is retained in
[the producer reproducer record](docs/acceptance/ci_acceptance.md#windowsarm64-hosted-integration-expansion-2026-11-20).

## Limits and ordered follow-up

- CLI SDK installation/selection currently supports native `dev` at `02-cxx`;
  general profiles and Full `06-web` installation are separate work.
- The retained Linux SDK's C shared SDK-provider control resolves host libz;
  this is [unendorsed diagnostic evidence](docs/acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09),
  not SDK shared qualification. Producer 6.4 retains policy/runtime-search review.
- The earlier Linux/x86_64 SDK `.3` C++ shared-runtime loader failure is closed
  for the repaired SDK `.4` on the two recorded Ubuntu hosts
  ([hosted qualification](docs/acceptance/ci_acceptance.md#completed-linux-hosted-qualification)).
  This does not qualify the separate pure-C shared SDK mode or other loaders.
- D4 requires a new SQLite amalgamation adapter plus libpng -> zlib dependency,
  variant/key/lock/cache handling; existing configure-only D3 is not that resolver.
- The owner's detached signature is attached after publication. Sign-before-publish is tracked in [CI Operations](TODO.md#ci-operations).
  Official default trust and URLs, key rotation and revocation, Authenticode, and macOS Developer ID signing and notarization remain open.
  A user project's `crt.lock` pins its own SDK explicitly. The repository root `crt.lock` is `unresolved` so one source tree builds on every host.
- General updates/self-upgrade, GC, wider profiles/cross targets and Studio retain
  their later queues. No new runtime/SDK build or default pin is adopted here.
- Pins move under the [promotion policy](docs/guides/development.md#ci-pin-promotion). Linux/arm64 CI and publisher-signature replay
  cover Ubuntu 26.04 / glibc 2.43. [CI acceptance](docs/acceptance/ci_acceptance.md).

Native evidence and limits belong to [compatibility](docs/compatibility.md) and
acceptance. Future STATUS refreshes remain explicit owner requests.
