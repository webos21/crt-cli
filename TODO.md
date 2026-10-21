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

- [ ] Select and record actual immutable SDK/host-tool candidates, external
  toolchain identities, and public entry points for the first native build.
- [ ] Inventory SDK wrapper/import/rootfs behavior; separate host tools and
  generators from target artifacts and record producer-side contract gaps.
- [ ] Define initial SDK/host/target identity, lock and JSON-result contracts
  with focused fixtures, without assuming new SDK schemas already exist.
- [ ] Populate the [compatibility adoption record](docs/compatibility.md) with
  actual candidate identities and case evidence; the documentation review is
  not a validated SDK/CLI/Studio combination.
- [ ] Define the minimal CRT-native CLI and bootstrap runtime closure,
  including the HTTP/TLS/trust choice and Windows loader behavior.
- [ ] After contract review, establish the minimal SDK-only CMake/Ninja/CTest
  entry points and actual CLI build/run evidence; document real commands.

## Planned

Follow [D0-D7 dependency order](docs/design/crt_cli.md#9-joint-development-sequence).
These are future increments, not implemented features.

### D1 - Windows Host Environment and CLI Bootstrap

- [ ] Consume/validate the CRT host-tools/rootfs contract and a minimal CLI
  bundle; define doctor/env/shell and per-child environments.
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

### D6 - Packaging and Automation

- [ ] Complete test/package/doctor, versioned JSON/job events, cancellation,
  locked/offline behavior, and explicit recovery/cleanup.
- [ ] Validate CLI/app packages without development SDK/build/store roots;
  preserve notices/source obligations and declared OS prerequisites.

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
