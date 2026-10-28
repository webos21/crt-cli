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

Current focus: contracts and bootstrap planning. No CLI source, executable,
build configuration, or runtime acceptance is implemented yet. See
[D0/D1 acceptance](docs/acceptance/bootstrap_acceptance.md).
Develop together with CRT Tranche 1 in one authorized local session, using the
[local Dev SDK candidate loop](docs/guides/development.md#local-dev-sdk-candidate-loop).
Public SDK release is not a prerequisite; CLI consumption stays SDK-only.

- [ ] **D0.1** Review command naming, one-executable/resources/helpers, project SDK
  storage and acquisition timing; [open decisions](docs/design/crt_cli.md#10-decisions-for-the-first-implementation)
  must not become implicit implementation defaults.
- [ ] **D0.2** Define the initial evolving source/module/template contract and
  SDK-only CMake/Ninja/CTest entry points before implementing the minimal CLI.
- [ ] **D0.3** Copy/extract an identified Dev SDK outside producer trees; record
  producer/CLI snapshots, hashes, host/target/tools and public wrapper/import/rootfs inputs.
- [ ] **D0.4** Define SDK identity/capability, lock and JSON fixtures; distinguish
  content profiles from build modes and existing formats from proposed schemas.
- [ ] **D0.5** Implement bounded help/version/bootstrap diagnosis and check
  single-executable link feasibility for the chosen CRT/C++/HTTP/TLS stack.
- [ ] **D0.6** Validate relocated build/run/imports with producer trees unavailable;
  feed gaps to CRT and retest each candidate on the initial native pairs.
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
- [ ] Decide/validate project-local versus shared immutable SDK storage and
  cache behavior; version/profile/target/content pins remain project-specific.
- [ ] Implement verified CLI self-upgrade with candidate health checks,
  OS-specific activation, interruption/rollback and unchanged project SDK/port pins
  ([lifecycle gates](docs/acceptance/lifecycle_acceptance.md)).

### D3 - First Source-built Port

- [ ] Reuse the existing JSON zlib recipe/driver through a versioned adapter;
  fetch/verify/build/test and publish into a separate immutable port store.
- [ ] Require Windows CRT-shell/make static/shared link/run evidence and
  unchanged SDK/source inputs before accepting the first port.

### D4 - Dependencies, Variants, Locks, and Local Cache

- [ ] Add SQLite and libpng-to-zlib resolution, build keys, project views,
  collision handling, and validated local-cache reuse.
- [ ] Replay ABI mismatch, rerun/recovery, and static/shared behavior on the
  initial three native host pairs.

### D5 - Application Workflow

- [ ] Connect new/add/configure/build/run to SDK-only CMake imports and
  selected dependencies; expand templates from C/C++ to supported upper stages.
- [ ] Validate external consumers, runtime closures, and Web worker layout
  without producer-private headers or build-tree inputs.
- [ ] Validate the chosen new-project SDK acquisition timing, generated layout,
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
