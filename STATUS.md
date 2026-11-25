# Project Status

Reviewed on **2026-11-24** against CLI `af849da`, CRT `88b172c` and the
editor scaffold `e418379`. The snapshot includes the recorded public releases,
CI pins, completed work and current work queue. Each native result retains its
original source revision, SDK pin and host scope. The release section was
refreshed on **2026-11-25** against CLI `20ee7e6` and CRT `c89ae1ac`;
the other evidence baselines are unchanged.
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

The [November 25 Linux/x86_64 CLI](docs/acceptance/preview_acceptance.md#linuxx86_64-sign-before-publishing-preview-2026-11-25)
was published only after both payload signatures verified. The
[hosted signing gate](docs/acceptance/ci_acceptance.md#hosted-sign-before-publishing-gate-2026-11-25)
also rejected a rehearsal and the unsigned real draft. A fresh public
download matched the receipt and verified both signatures. The CLI's tested
SDK remains the November 18 pin in the table; the new SDK and CLI were not
exercised together, and no CI pin was promoted.

The Windows/x64 private-DLL protocol passed a local 27-case replay with actual shared AWK, app-local DLLs, cwd/PATH collisions,
payload refusal and same-pin recovery. The public CI bundles use static host tools.
[Windows/x64 local qualification](docs/acceptance/host_environment_acceptance.md#windowsx64-private-dll-host-tools-replay-2026-11-20).

Windows/x64's earlier concurrent-install crash was fixed in the producer libunwind recipe and replayed with the fixed SDK.
The Windows port-store publication test uses independent CLI processes. A
separate in-process descriptor-concurrency failure was fixed in CRT and
replayed on later unpromoted candidates; that does not update the public pins.
[Historical reproducer](docs/acceptance/ci_acceptance.md#windowsarm64-hosted-integration-expansion-2026-11-20) ·
[Producer fix and replay](https://github.com/webos21/crt/blob/88b172cf153a8b786fc98a8e17872ebce8b0efa2/docs/acceptance/release_qualification.md#windows-descriptor-ownership-and-stdio-block-transfers-2026-11-23).

## Limits and ordered follow-up

- **P0 editor API.** Compilation database export, `project info --json` and
  structured capability discovery are proposed. The existing final JSON
  envelope is the starting point. Streaming job events need a separate opt-in
  D6 protocol. `crt-vscode` is the first editor consumer. Record exercised
  SDK/CLI/editor combinations through D7.
  [Editor API queue](TODO.md#p0---minimal-public-editor-api-d7-slice-coordinated-with-d5d6) ·
  [Adoption evidence](docs/compatibility.md#adoption-and-exercised-combinations).
- **P1 producer handoff and first-user setup.** The pure C shared provider
  and Windows C++ compile-policy replays are closed on the recorded corrected
  candidates. Later native PAL replays cover direct path-taking streams and
  supplementary-plane Windows filenames. Earlier failing controls retain
  their SDK identities; candidate verification is not promotion or public
  pin adoption. First-user work covers clearer SDK, LLVM and OS SDK guidance.
  [Completed replays](HISTORY.md#replay-the-external-c17-persona-consumer-on-windowsarm64-and-close-p1-01) ·
  [Windows PAL combination](docs/compatibility.md#native-windowsx64-pal-consumer-combination-2026-11-23) ·
  [Consumer handoff](TODO.md#p1---producer-correctness-handoff-and-first-user-workflow-d5).
- **Trust scope.** Publisher signatures cover individual files. Official
  default trust and URLs, key lifecycle and platform code signing are outside
  the completed [publication gate](docs/acceptance/ci_acceptance.md#hosted-sign-before-publishing-gate-2026-11-25).
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
