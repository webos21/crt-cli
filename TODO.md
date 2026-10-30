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

### D0 - SDK Consumer Contracts and Bootstrap Definition

Current focus: PAL-only native contract, typed build targets and SDK-owned
JSON/SHA-2, followed by candidate coherence.
Temporary: each host's final executable is committed under `bin/` (`macos`, `linux-x64`,
`linux-arm64`, `win-x64`, `win-arm64`; see `bin/README.md`). macOS, Linux/x86_64 and Windows/x64 are present;
`linux-arm64` and `win-arm64` remain to be built on their hosts. Local SDK/tool inputs live in the ignored `.sdk.json`.
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
- [ ] **D0.5** Implement bounded help/version/bootstrap diagnosis and check
  single-executable link feasibility for the chosen CRT/C++/HTTP/TLS stack.
  Linux, macOS/arm64 and Windows/x64 pass (local, unpromoted); the merged Windows changes now pass Linux and macOS replay (2026-10-30).
- [ ] **D0.6** Validate relocated build/run/imports with producer trees unavailable;
  Linux, macOS/arm64 and Windows/x64 pass ([Windows evidence](docs/acceptance/bootstrap_acceptance.md#native-json-builder-on-windowsx64-2026-10-29));
  [macOS evidence](docs/acceptance/bootstrap_acceptance.md#native-json-builder-on-macosarm64-2026-10-29).
  [Current Linux merged-source replay](docs/acceptance/bootstrap_acceptance.md#linux-revalidation-after-windows-changes-2026-10-30) passes; [macOS merged-source replay](docs/acceptance/bootstrap_acceptance.md#macos-revalidation-after-windows-and-linux-changes-2026-10-30) passes.
- [ ] **D0.7** Link exact candidate/case evidence in [compatibility](docs/compatibility.md),
  resolve asset coherence with CRT and adopt final pins only after producer promotion.

## Planned

Follow [D0-D7 dependency order](docs/design/crt_cli.md#9-joint-development-sequence).
These are future increments, not implemented features.

### D1 - Windows Host Environment and CLI Bootstrap

- [ ] Validate the single CLI executable and self-doctor without a Dev SDK;
  settle trust/data/notices/helper policy and inspect actual OS/runtime imports.
- [ ] Consume/validate the separate CRT host-tools/rootfs contract; define
  environment/project diagnosis, env/shell and per-child environments.
- [ ] Verify actual CRT mksh/toybox/awk/make, SDK-free CLI execution, paths,
  private DLL resolution, and bounded owned-process cleanup.
- [ ] Pass [D1-07 HTTPS bootstrap](docs/acceptance/bootstrap_acceptance.md#sdk-free-https-bootstrap-gate)
  with only the shipped CLI runtime/trust inputs; exercise invalid trust and
  tampered metadata/artifacts without SDK/global DLL fallback.

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
