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

Active increment (opened 2026-11-03, with CRT Tranche 3): **D2 local SDK installer**. In scope: `sdk install` of a
local `tar.xz`/`zip` archive (digest check, safe extraction, inventory validation, per-identity lock, atomic
same-filesystem publication, interrupted-install recovery, immutability) and `sdk list`/`sdk verify`, replayed on
Linux first. Out of scope for this increment: network acquisition, updates, project-lock migration, CLI self-upgrade,
GC. The xz decoder comes from a CRT Development-SDK port; CRT 3.1 defines the install metadata. Linux/x86_64: tar.xz and zip
install/list/verify/recover, manifest install metadata and the isolated replay are done ([acceptance](docs/acceptance/sdk_install_acceptance.md));
macOS/arm64 isolated replay (48/48, self-built SDK-only CLI) and Windows/x64 isolated replay (62/62, self-built SDK-only CLI;
[evidence](docs/acceptance/sdk_install_acceptance.md#windowsx64-isolated-replay-2026-11-04)) are done, as is Linux/aarch64 (48/48;
[evidence](docs/acceptance/sdk_install_acceptance.md#linuxaarch64-isolated-replay-2026-11-04)) and Windows/arm64 (62/62;
[evidence](docs/acceptance/sdk_install_acceptance.md#windowsarm64-isolated-replay-2026-11-04)). D1 local replay is recorded in [HISTORY](HISTORY.md) and
[acceptance](docs/acceptance/host_environment_acceptance.md#windowsx64-independent-host-environment-replay-2026-11-03).
Installation is recorded as passing on Linux/x86_64, Linux/aarch64, macOS/arm64, Windows/x64 and Windows/arm64.

Follow [D0-D7 dependency order](docs/design/crt_cli.md#9-joint-development-sequence).
These are future increments, not implemented features.

### D2 - Safe SDK Management

- [ ] Implement stage/profile resolution, immutable installation/selection,
  verification, transactions, interrupted-install recovery, and explicit updates.
  - [x] Immutable installation from a local `tar.xz`/`zip`, verification, transactions and interrupted-install
    recovery ([acceptance](docs/acceptance/sdk_install_acceptance.md)).
  - [ ] Stage/profile resolution and project selection of an installed SDK.
  - [ ] Explicit updates (network acquisition is a later increment).
- [ ] Replay installation failures and path/tool collisions on the three
  initial native host pairs; never migrate project locks implicitly.
  - [x] Installation failures replayed on all five native pairs (Linux x86_64/aarch64, macOS/arm64, Windows x64/arm64).
  - [ ] Path/tool collisions with an installed SDK; no implicit lock migration (needs project selection).
- [ ] Implement/validate shared immutable SDK storage and project references;
  version/profile/target/content pins remain project-specific.
  - [x] Shared immutable store (`sdk list|verify|recover`).
  - [ ] Project references and pins.
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

## Planned

### Deferred Beyond the Initial Workflow

Trusted binary caches, optional recipe-registry separation, SDK delta-component
archives, additional architectures/cross pairs, and IDE/plugin services require
separate scope and evidence. Do not make them prerequisites for D0-D3.

Plan lightweight cross-repository contract/reference checks after concrete
metadata/fixtures exist; use the [manual review procedure](docs/compatibility.md#shared-change-review)
now rather than adding another full-build CI path.
