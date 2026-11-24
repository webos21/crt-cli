# Project Status

Reviewed on **2026-11-20** against CLI `e362a51`, CRT `278c5d9` and the
editor scaffold `b547406`. The snapshot includes the recorded public releases,
CI pins, completed work and current work queue. Each native result retains its
original source revision, SDK pin and host scope.
[TODO](TODO.md) · [HISTORY](HISTORY.md) · [Acceptance](docs/acceptance/sdk_install_acceptance.md).

## Implemented and exercised scope

| Surface | Evidence boundary |
| --- | --- |
| Native CLI/project builder | CRT-native C++17, crt.json -> direct LLVM argv; C/C++ source-only new, first build/configure/build/test/run; external LLVM/OS SDKs required |
| Command JSON | Global `--json` emits one final `crt-cli.bootstrap-result` envelope, format 1. It covers `build`, `sdk list` and `port list`. [Implemented contract](docs/reference/bootstrap_contract.md#commands-and-results). |
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
on the Windows/arm64 expansion branch and the adoption passed main CI. The scheduled trigger is confirmed (seven runs); its late starts and macOS runner shortage are GitHub-side limits, recorded in the
[scheduled-run record](docs/acceptance/ci_acceptance.md#scheduled-integration-history-and-the-macos-runner-failure-2026-11-24).
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

- **P0 editor API.** Compilation database export, `project info --json` and
  structured capability discovery are proposed. The existing final JSON
  envelope is the starting point. Streaming job events need a separate opt-in
  D6 protocol. `crt-vscode` is the first editor consumer. Record exercised
  SDK/CLI/editor combinations through D7.
  [Editor API queue](TODO.md#p0---minimal-public-editor-api-d7-slice-coordinated-with-d5d6) ·
  [Adoption evidence](docs/compatibility.md#adoption-and-exercised-combinations).
- **P1 producer handoff and first-user setup.** The retained Linux pure C shared
  SDK-provider control resolves host libz and remains unqualified. The Linux
  C++ shared loader repair has its own Ubuntu 24.04 and 26.04 results. Replay
  CRT's C shared and Windows `<filesystem>` corrections with the exact new SDK
  candidates. First-user work covers clearer SDK, LLVM and OS SDK guidance.
  [Linux C control](docs/acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09) ·
  [C++ loader evidence](docs/acceptance/ci_acceptance.md#completed-linux-hosted-qualification) ·
  [Consumer handoff](TODO.md#p1---producer-correctness-handoff-and-first-user-workflow-d5).
- **P1 release signing.** The current Preview flow attaches the owner's offline
  signature after publication. Sign-before-publish, official default trust and
  URLs, key lifecycle and platform code signing have separate gates.
  [Sign Before Publishing](TODO.md#p1---sign-before-publishing).
- **CI Operations.** CI pins move under the reviewed promotion policy.
  Linux/arm64 CI and native signature replay cover Ubuntu 26.04 with glibc 2.43.
  Raspberry Pi, additional loader versions and non-admin
  ruleset enforcement have open verification work.
  [CI follow-up](TODO.md#ci---monitoring-and-pin-promotion).
- **Planned expansion.** SDK installation and selection currently cover native
  `dev` at `02-cxx`. Broader profiles, Full SDK installation, SQLite/libpng
  dependency resolution, SDK updates, garbage collection and self-upgrade
  remain planned. Each project keeps its own exact `crt.lock` pins. The
  repository lock is `unresolved` so its source builds on multiple hosts.
  [Planned work](TODO.md#planned).

[Compatibility](docs/compatibility.md) and the owning acceptance records
preserve exact inputs and native evidence.
