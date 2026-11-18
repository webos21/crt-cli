# Project Status

Evidence-based SDK/CLI snapshot refreshed **2026-11-19** at the owner's explicit
request, reviewing repository checkpoint `129646d`. It is a documentation review
of the recorded evidence plus one native run made that day (the Linux/arm64
publisher-signature replay, below); nothing else was rebuilt or replayed. Each
Preview and CI result retains its own source revision, public SDK pin and host
scope. The last product-source change is `0b26dbf` (2026-11-16, publisher signatures for `sdk install`/`repair`).
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
| Restricted HTTPS redirects | Opt-in `github-release` / explicit-authority policy for `fetch` and `sdk acquire` (default stays refusal); Linux/x86_64 ([record](docs/acceptance/preview_acceptance.md#restricted-https-redirects-and-github-acquisition-2026-11-10)) and macOS/arm64 ([record](docs/acceptance/preview_acceptance.md#macos-cli-preview-release-2026-11-10)) each pass 54/54 replay cases and a real GitHub acquisition of their Dev SDK Preview; Windows/x64 also passes 54/54 and real acquisition ([record](docs/acceptance/preview_acceptance.md#windowsx64-cli-preview-release-2026-11-10)); Linux/aarch64 passes 54/54 and real acquisition from its public package ([record](docs/acceptance/preview_acceptance.md#linuxaarch64-redirect-replay-and-sandboxed-package-run-2026-11-11)) |
| CLI Previews | Published prereleases on all five hosts (tags `v0.1.0-preview.<date>.<os>-<arch>.1`); each host's pinned Preview and its pinned Dev SDK are listed in the table below, with the owner's offline `.sig` pinned for both. Evidence per Preview: [preview acceptance](docs/acceptance/preview_acceptance.md) and [CI acceptance](docs/acceptance/ci_acceptance.md) |
| Publisher signatures | The owner signs offline and attaches `<asset>.sig`; `sdk acquire`, `sdk install`, `sdk repair` and `fetch` verify it with the pinned key and report `signature_verified`, otherwise `not_verified`. Native refusal and acceptance replays (37 test-key and 10 real-key cases) are recorded on macOS/arm64, Windows/x64, Windows/arm64, Linux/x86_64 and, on 2026-11-19, Linux/arm64 ([record](docs/acceptance/ci_acceptance.md#linuxarm64-sdk-publisher-signature-replay-2026-11-19)) |

**Pinned inputs per host** (`.github/ci/pins.json`, read on 2026-11-19; every row has a CLI signature pin, an SDK signature pin and `sdk_source_publisher`):

| Host | Bootstrap CLI Preview | Pinned Dev SDK Preview |
| --- | --- | --- |
| Linux/x86_64 | `v0.1.0-preview.20261116.linux-x86_64.1` | `v0.0.1-preview.20261118.linux.x64.1` |
| Linux/arm64 | `v0.1.0-preview.20261116.linux-arm64.1` | `v0.0.1-preview.20261110.linux.arm64.2` |
| macOS/arm64 | `v0.1.0-preview.20261117.macos-arm64.1` | `v0.0.1-preview.20261110.macos.1` |
| Windows/x64 | `v0.1.0-preview.20261117.windows-x86_64.1` | `v0.0.1-preview.20261110.windows.1` |
| Windows/arm64 | `v0.1.0-preview.20261118.windows-arm64.1` | `v0.0.1-preview.20261118.windows.arm64.1` |

The six required GitHub CI legs cover Ubuntu 24.04 and 26.04 x86_64, Ubuntu 26.04 arm64, macOS/arm64, Windows/x64 and Windows/arm64
(`windows-11-arm`). Each bootstraps from its host's pinned, signed Preview and pinned Dev SDK and builds the checked-out sources; `main` is
protected by a ruleset that requires the single `CI gate` check, and the `push` runs on `main` at `3aa7030` and `129646d` succeeded
(2026-11-18). The `integration.yml` workflow runs daily and by hand on Linux x86_64 and arm64 (redirect replay, pin gate and the port suites
through CRT's published host-tools Previews); its first scheduled run passed, and whether the daily cadence holds is still being observed.
`release.yml` published four Previews with `dry_run: false` behind the `release` approval: Linux x86_64 and arm64 (`20261116`), macOS/arm64 and Windows/x64 (`20261117`).
The Windows/arm64 Preview (`20261118`) was built by hand, because a host's first Preview has no bootstrap pin for the workflow to use. [CI acceptance](docs/acceptance/ci_acceptance.md) owns the current
combinations and limits.

Windows/x64's earlier concurrent-install crash was root-caused/fixed in the
producer libunwind recipe and replayed with the fixed SDK; its retained failure
is not a current unresolved gate. Windows/arm64 D2 is distinct from unqualified
D3. The repository no longer commits executables (`bin/` was removed); their dated identities remain in Git history.
Apart from the published Previews above, none of these records is a CLI release or a universal latest-source support matrix.

## Limits and ordered follow-up

- CLI SDK installation/selection currently supports native `dev` at `02-cxx`;
  general profiles and Full `06-web` installation are separate work.
- D3 adapter review is approved (CRT `90989ac`) and adopted for new publications; Windows/x64 evidence is reused only for
  unchanged policy/product inputs; Windows/arm64 D3 is separate.
- The retained Linux SDK's C shared SDK-provider control resolves host libz;
  this is [unendorsed diagnostic evidence](docs/acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09),
  not SDK shared qualification. Producer 6.4 retains policy/runtime-search review.
- The earlier Linux/x86_64 SDK `.3` C++ shared-runtime loader failure is closed
  for the repaired SDK `.4` on the two recorded Ubuntu hosts
  ([hosted qualification](docs/acceptance/ci_acceptance.md#completed-linux-hosted-qualification)).
  This does not qualify the separate pure-C shared SDK mode or other loaders.
- The previews target Dev SDK + CLI. CLI Previews and CRT's Dev SDK Previews are published for all five hosts (table above). Existing Full
  assembly/approval records remain valid for their own inputs; Full does not block the CLI preview.
- D4 requires a new SQLite amalgamation adapter plus libpng -> zlib dependency,
  variant/key/lock/cache handling; existing configure-only D3 is not that resolver.
- Publisher authentication is `signature_verified` only where the caller supplies the owner's public key and the `.sig`; every CI pin does, and an asset used without a
  key and signature remains `not_verified`. All nine published CLI Previews and all nine CRT releases (Dev SDK and host-tools) carry a `.sig` asset (existence listed on
  2026-11-19); this snapshot verified the pinned ones, and the Linux/arm64 signed Preview and SDK natively that day, not every asset. The signature is an
  offline owner step after publication, so an unsigned window exists between publication and signing, by design. Official default trust and URLs, key rotation
  and revocation, Authenticode, and macOS Developer ID signing and notarization remain open, and a quarantined (browser) download was not tested. Developer test CA,
  rehearsal assets and the repository root `crt.lock` (`unresolved` by policy: one source tree builds on every host) are not promotion. A user project's `crt.lock` pins its own SDK explicitly.
- General updates/self-upgrade, GC, wider profiles/cross targets and Studio retain
  their later queues. No new runtime/SDK build or default pin is adopted here.
- The port suites run in the daily `integration.yml` on Linux x86_64 and arm64 only; macOS, Windows and the Windows/arm64 D3 port have no integration job. The approvals of the two Linux
  publications carry an empty comment, so the reviewer's intent is not written into those records (the macOS and Windows/x64 approvals carry `OK`). A workflow-built Windows/arm64 Preview and
  the `release.yml` `dry_run` rehearsal for `win-arm64` remain open ([TODO](TODO.md)). Pins move under a written
  [promotion policy](docs/guides/development.md#ci-pin-promotion). Linux/arm64 CI and the signature replay cover Ubuntu 26.04 / glibc 2.43 only; passing process workers do not resolve the
  retained Windows threaded spawn/file-I/O observations. See [CI acceptance](docs/acceptance/ci_acceptance.md).

Native evidence and limits belong to [compatibility](docs/compatibility.md) and
acceptance. Future STATUS refreshes remain explicit owner requests.
