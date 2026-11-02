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

### D1 - Windows Host Environment and CLI Bootstrap

D0 is complete for the recorded local technical scope; completed work and exact
candidate selection remain in [HISTORY](HISTORY.md) and
[compatibility](docs/compatibility.md#common-source-candidate-selection-2026-11-02).
D1 remains open. Next increment: define/adopt the independent producer
host-tools contract before implementing the CLI `port-build` adapter.
Use the [local Dev SDK candidate loop](docs/guides/development.md#local-dev-sdk-candidate-loop);
CLI consumption stays SDK-only, without requiring a public SDK release.

- [ ] Define and consume the separate CRT host-tools/rootfs contract, then
  implement the CLI `port-build` adapter with exact host-tool and SDK identities.
- [ ] Implement environment/project diagnosis and shell support. Existing
  control/app-build/app-run `env` diagnostics share actual child construction;
  retain their [implemented scope](docs/reference/bootstrap_contract.md#child-environment-diagnostics-experimental-format-0).
- [ ] Validate full CRT mksh/toybox/awk/make workflows, spaces/Unicode paths,
  aliases, private DLL resolution and owned-process cleanup without host-tool
  fallbacks or global PATH changes. Retain the measured make `SHELL` space-path
  failure and explicitly declared temporary native-tool bridge.
- [ ] Qualify the single-executable bootstrap trust/data/notices/helper package
  and remaining resource/negative cases without development SDK/build inputs.
  Preserve the [D1 audit's measured results and limits](docs/acceptance/bootstrap_acceptance.md#d1-publisher-and-host-environment-audit-2026-10-31).
- [ ] Complete [D1-07 HTTPS bootstrap](docs/acceptance/bootstrap_acceptance.md#sdk-free-https-bootstrap-gate)
  with the selected package/runtime/trust inputs and tampered metadata/artifact
  cases. Disposable test CA/signing identities are owner-selected; official
  publisher trust and release-metadata policy remain separate qualification.
- [ ] Replay new D1 increments on native Windows/x64, Linux/x86_64 and
  macOS/arm64; retain other architectures and earlier passes at their exact
  recorded combinations in [acceptance](docs/acceptance/bootstrap_acceptance.md).
  Do not treat existing bootstrap passes as complete host-tools/port acceptance.

## Planned

Follow [D0-D7 dependency order](docs/design/crt_cli.md#9-joint-development-sequence).
These are future increments, not implemented features.

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
