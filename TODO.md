# TODO: CRT-CLI Work Queue

This file owns the concise, ordered implementation queue. Design and dependency
order belong to [the CLI design](docs/design/crt_cli.md); measured evidence
belongs to [acceptance](docs/acceptance/bootstrap_acceptance.md). Completed
work moves to [HISTORY](HISTORY.md), not into an expanding Done list here.

## Notice

- Follow [AGENTS.md](AGENTS.md). Implement one bounded, authorized increment at
  a time; this queue is not permission to execute the entire roadmap.
- In Progress identifies the current focus, not a claim that all its tasks are
  already being implemented. Update it before changing focus.
- Keep commands, measurements, failures, and per-host outcomes in their owning
  guide/acceptance/history document rather than duplicating them here.
- Move completed work to HISTORY after validation. Checked prerequisite items
  may remain briefly only when needed to explain an active sequence.
- Do not update or create STATUS as a routine documentation step; do so only
  when explicitly requested. Keep the root README user-facing.
- Do not claim that a design, fixture, compile-only result, skipped test, or
  development SDK constitutes native runtime or release acceptance.

## In Progress

### D0/D1 - SDK Consumer Contracts, Bootstrap and Trust

D0 is complete for the recorded local technical scope: D0.5/D0.6 retain their
[five native combinations](docs/acceptance/bootstrap_acceptance.md#technical-bootstrap-gate-review-2026-11-02),
and D0.7 now selects the [canonical common-source candidate](docs/compatibility.md#common-source-candidate-selection-2026-11-02).
Current work is D1; production adoption remains separate.

Next increment: define/adopt the independent producer host-tools contract before
implementing the CLI `port-build` adapter. Experimental control/app-build/app-run
`env` and shared child construction pass fresh Linux/x86_64 verification
([evidence](docs/acceptance/bootstrap_acceptance.md#linux-child-environment-diagnostics-2026-11-02));
Linux/aarch64 also passes
([evidence](docs/acceptance/bootstrap_acceptance.md#linuxaarch64-latest-source-revalidation-2026-11-02));
replay this increment on the other native hosts. Earlier host passes retain
their own revisions. `port-build`, shell and environment/project doctor remain open.

Remaining focus: D1 host-environment/resource gates. Windows/x64 now also passes the latest sources with the CRT spawn-environment fix
([evidence](docs/acceptance/bootstrap_acceptance.md#windowsx64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02)). Windows/arm64 passes the latest sources
([evidence](docs/acceptance/bootstrap_acceptance.md#windowsarm64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02));
Latest macOS/arm64 native, bootstrap and SDK-hidden
HTTPS/signature replay also passes
([macOS evidence](docs/acceptance/bootstrap_acceptance.md#macos-latest-source-revalidation-2026-11-01),
[Windows evidence](docs/acceptance/bootstrap_acceptance.md#windowsx64-owned-descendant-timeout-fix-2026-11-01)).
The Linux D1 environment/resource gates remain open. The owner selected
disposable test CA/signing identities on 2026-11-01; their authentication replay
passes without waiting for official release keys/URLs. Official trust remains
separate release qualification; independent host-tools, port-build and shell remain open.
The earlier Linux-only trust increment retains its separate host scope. Linux/x86_64 passes against a fresh clean-source Dev SDK with
`crt-narrow-encoding-1`; the exact scope is in
[the latest Linux record](docs/acceptance/bootstrap_acceptance.md#linux-latest-source-revalidation-2026-10-31).
Temporary: each host's final executable is committed under `bin/` (`macos`, `linux-x64`,
`linux-arm64`, `win-x64`, `win-arm64`; see `bin/README.md`). All five are present;
`linux-arm64` and `win-arm64` were added on 2026-10-30. Local SDK/tool inputs live in the ignored `.sdk.json`.
Bootstrap loop goal: a changed SDK plus an already built `crt-cli` produce a new `crt-cli`, which then rebuilds and tests itself.
The native JSON builder replaces the initial CMake prototype. Bootstrap and
bounded new/configure/build/test/run pass on Linux and macOS without Python/shell;
SDK installation, general dependency workflows and self-upgrade remain planned. See
[D0/D1 acceptance](docs/acceptance/bootstrap_acceptance.md).
Develop together with CRT Tranche 1 in one authorized local session, using the
[local Dev SDK candidate loop](docs/guides/development.md#local-dev-sdk-candidate-loop).
Public SDK release is not a prerequisite; CLI consumption stays SDK-only.

- [x] **D0.1** Record owner choices: crt-cli, shared immutable SDK references,
  first-build acquisition and one-executable/data/temporary-helper policy.
- [x] **D0.2** Define the initial evolving source/module/template contract and
  SDK-only native manifest/driver entry points, src/main modules and C/C++ src/test.
- [x] **D0.3** Copy/extract the Linux Dev SDK outside producer trees; record
  producer/CLI snapshots, hashes, host/target/tools and public wrapper/import/rootfs inputs.
- [x] **D0.4** Implement bootstrap SDK/result records and experimental format-0
  manifest/lock inputs; the general SDK/project contracts remain distinct.
- [x] **D0.5** Validate bounded help/version/bootstrap diagnosis and one-executable
  feasibility on the five recorded native pairs
  ([technical review](docs/acceptance/bootstrap_acceptance.md#technical-bootstrap-gate-review-2026-11-02)).
- [x] **D0.6** Review relocated SDK-only build/run/imports and producer-tree
  isolation at those recorded combinations; not production adoption
  ([technical review](docs/acceptance/bootstrap_acceptance.md#technical-bootstrap-gate-review-2026-11-02)).
- [x] **D0.7** Select coherent common-source assets and the exact local SDK/provider/
  CLI/toolchain combination, with original consumer evidence and explicit limits
  ([selection](docs/compatibility.md#common-source-candidate-selection-2026-11-02)).
  Root production lock remains unresolved; promote producer artifacts before any adoption.

## Planned

Follow [D0-D7 dependency order](docs/design/crt_cli.md#9-joint-development-sequence).
These are future increments, not implemented features.

### D1 - Windows Host Environment and CLI Bootstrap

- [ ] Validate the single CLI executable and self-doctor without a Dev SDK;
  settle trust/data/notices/helper policy and inspect actual OS/runtime imports.
- [ ] Consume/validate the separate CRT host-tools/rootfs contract; implement
  environment/project diagnosis, port-build and shell. Control/app-build/app-run
  diagnostics share actual child construction; their first native replay passes
  on Linux ([evidence](docs/acceptance/bootstrap_acceptance.md#linux-child-environment-diagnostics-2026-11-02)).
- [ ] Verify actual CRT mksh/toybox/awk/make, SDK-free CLI execution, paths,
  private DLL resolution, and bounded owned-process cleanup.
- [ ] Windows UTF-8 narrow strings pass scoped workflows on Windows/arm64
  ([record](docs/acceptance/bootstrap_acceptance.md#windows-utf-8-narrow-strings-implemented-2026-10-31)) and Windows/x64
  ([record](docs/acceptance/bootstrap_acceptance.md#windowsx64-latest-source-revalidation-2026-11-01)); x64 host-tool Unicode startup probes pass.
  Full port-tool workflows remain open. Linux/aarch64 now passes with a fresh clean
  SDK ([evidence](docs/acceptance/bootstrap_acceptance.md#linuxaarch64-latest-source-revalidation-2026-11-02)); Linux/x86_64 and macOS/arm64 retain their recorded passes.
- [ ] Pass [D1-07 HTTPS bootstrap](docs/acceptance/bootstrap_acceptance.md#sdk-free-https-bootstrap-gate)
  with only the shipped CLI runtime/trust inputs; exercise invalid trust and
  tampered metadata/artifacts without SDK/global DLL fallback. Partial: `fetch` and 17 loopback cases pass on
  Windows/arm64 ([record](docs/acceptance/bootstrap_acceptance.md#windowsarm64-https-fetch-unicode-paths-and-release-asset-sdk-2026-10-31));
  a signed-metadata policy and a real publisher remain. Linux/x86_64 also passes
  all 17 controlled-endpoint cases with SDK/source/provider hidden (2026-10-31);
  Windows/arm64 now passes 32 SDK-hidden cases ([evidence](docs/acceptance/bootstrap_acceptance.md#windowsarm64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02)); Linux/aarch64 passes 32 cases
  ([evidence](docs/acceptance/bootstrap_acceptance.md#linuxaarch64-latest-source-revalidation-2026-11-02));
  macOS/arm64 passes 32 SDK-hidden cases ([evidence](docs/acceptance/bootstrap_acceptance.md#macos-latest-source-revalidation-2026-11-01)).
  The [D1 audit](docs/acceptance/bootstrap_acceptance.md#d1-publisher-and-host-environment-audit-2026-10-31)
  adds explicit-key signature cases and a real repository HTTPS probe. The
  [owner-selected test-identity replay](docs/acceptance/bootstrap_acceptance.md#linux-test-identity-replay-2026-11-01)
  passes 32 cases; official key distribution, release metadata and anti-replay
  policy remain later qualification rather than prerequisites for this test run.
  Windows/x64 also passes all 32 with disposable identities and inaccessible
  SDK/source/provider roots (2026-11-01); owned-descendant cleanup now passes on x64.
- [ ] Define the independent host-tools contract and CLI port-build adapter;
  retain the measured make `SHELL` space-path failure and declare its temporary
  native-tool bridge. Qualify the notices/trust package and remaining D1 negatives.

### D2 - Safe SDK Management

- [ ] Implement stage/profile resolution, immutable installation/selection,
  verification, transactions, interrupted-install recovery, and explicit updates.
- [ ] Replay installation failures and path/tool collisions on the three
  initial native host pairs; never migrate project locks implicitly.
- [ ] Implement/validate shared immutable SDK storage and project references;
  version/profile/target/content pins remain project-specific.
- [ ] Implement verified CLI self-upgrade with candidate health checks,
  OS-specific activation, interruption/rollback and unchanged project SDK/port pins
  ([lifecycle gates](docs/acceptance/lifecycle_acceptance.md)).

### D3 - First Source-built Port

- [ ] Implement native execution of the reviewed JSON zlib recipe contract;
  fetch/verify/build/test and publish into a separate immutable port store.
- [ ] Require Windows CRT-shell/make static/shared link/run evidence and
  unchanged SDK/source inputs before accepting the first port.

### D4 - Dependencies, Variants, Locks, and Local Cache

- [ ] Add SQLite and libpng-to-zlib resolution, build keys, project views,
  collision handling, and validated local-cache reuse.
- [ ] Replay ABI mismatch, rerun/recovery, and static/shared behavior on the
  initial three native host pairs.

### D5 - Application Workflow

- [ ] Extend the bounded native new/configure/build/test/run to dependency
  resolution and upper stages; app workflows must not require Python, shell or CMake.
- [ ] Validate external consumers, runtime closures, and Web worker layout
  without producer-private headers or build-tree inputs.
- [ ] Validate first-configure/build SDK acquisition, source-only new, generated layout,
  locked/offline failures and CLI self-build/template parity; keep layout evolution explicit.

### D6 - Packaging and Automation

- [ ] Complete test/package/doctor, versioned JSON/job events, cancellation,
  locked/offline behavior, and explicit recovery/cleanup.
- [ ] Validate CLI/app packages without development SDK/build/store roots;
  preserve notices/source obligations and declared OS prerequisites.
- [ ] Replay the CLI one-executable, scoped doctor and self-upgrade/recovery gates
  on the actual release artifact; application packages retain their own runtime closures.
- [ ] Coordinate public SDK/CLI delivery: verify the promoted producer artifact,
  adopt its exact pin, then publish the CLI; retain independent product versions.

### D7 - Studio Integration

- [ ] Stabilize the public CLI/JSON/debug-tool contract for the separate
  Studio frontend; coordinate only consumer-driven generic CRT API changes.
  D7 formally approves exercised combinations; it does not block SDK-only S0
  PoCs or earlier live experiments using actually implemented capabilities.

### Deferred Beyond the Initial Workflow

Trusted binary caches, optional recipe-registry separation, SDK delta-component
archives, additional architectures/cross pairs, and IDE/plugin services require
separate scope and evidence. Do not make them prerequisites for D0-D3.

Plan lightweight cross-repository contract/reference checks after concrete
metadata/fixtures exist; use the [manual review procedure](docs/compatibility.md#shared-change-review)
now rather than adding another full-build CI path.
