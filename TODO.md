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

### Dev SDK / CLI Preview preparation

D3's bounded zlib adapter contract is adopted for the exercised Linux/x86_64,
macOS/arm64 and Windows/x64 combinations. [The adoption ledger](docs/compatibility.md#d3-bounded-adapter-adoption-2026-11-09)
links the producer decision and metadata/key/old-item regression. Windows/arm64
D3 remains separate. D2's recorded installation/selection/first-build/pinned
HTTPS baselines are retained. Completed work belongs to HISTORY; STATUS is the
separately requested dated snapshot.

- [x] Freeze the [bounded clean Linux Dev SDK + CLI local candidate](docs/acceptance/preview_acceptance.md#linux-local-candidate-2026-11-10),
  LLVM inputs, complete notices, explicit-at-fetch trust and source-hidden workflow.
  Linux C shared SDK mode remains excluded under CRT 6.4.
- [x] Publish the selected producer source and approve scoped manual-pin trust;
  [promote and exercise actual public SDK bytes](docs/acceptance/preview_acceptance.md#public-sdk-promotion-and-consumer-adoption-2026-11-10).
- [x] Restricted HTTPS redirect policy implemented and Linux-replayed, including real
  GitHub `sdk acquire` ([record](docs/acceptance/preview_acceptance.md#restricted-https-redirects-and-github-acquisition-2026-11-10)).
- [x] Redirect policy replayed on macOS/arm64: 54/54 cases and real GitHub `sdk acquire` ([record](docs/acceptance/preview_acceptance.md#macos-cli-preview-release-2026-11-10)).
- [ ] Review explicit default/project pin adoption against the published Linux/x86_64,
  Linux/aarch64, macOS/arm64 and Windows/x64 SDK/CLI Previews;
  retain the [Windows publication baseline](docs/acceptance/preview_acceptance.md#windowsx64-cli-preview-release-2026-11-10) and the existing Linux/macOS records.
- [ ] Keep Full SDK packaging/security/source/license review on its own path;
  Full installation/profile support is not a prerequisite for the first CLI preview.

D4 may proceed against the frozen D3 contracts; it does not enlarge D3 or
become a first-preview release gate. General updates/self-upgrade and Studio
retain their later queues. Execution policy remains in
[the design](docs/design/crt_cli.md#execution-priorities-adopted-2026-11-05).

### GitHub CI

`ci.yml` (PR and main; a documentation-only change skips the matrix inside the workflow, and
`CI gate` is the one required check of the `main` ruleset) bootstraps from
the pinned published Preview and is described in the
[development guide](docs/guides/development.md#continuous-integration); completed steps live in HISTORY.
Linux/x86_64 (`ubuntu-24.04` and `ubuntu-26.04`), Linux/arm64 (`ubuntu-26.04-arm`), macOS/arm64 (`macos-15`) and Windows/x64
(`windows-2022`) are required legs.
[CI acceptance](docs/acceptance/ci_acceptance.md#current-hosted-coverage-2026-11-11)
owns the completed combinations, event scope and ruleset, including both repaired Linux
paths and the Linux/arm64 dispatch, push and pull-request runs.

- [ ] `integration.yml` (manual; the 54-case redirect replay passes on Linux x64 and arm64 in hosted run `42957820373`): the five
  port suites still need CRT to publish and pin a per-host independent host-tools bundle and a hosted-runner adaptation of
  `tools/replay_port_linux.py`; until then `pins` and `ports` fail on purpose. Add `schedule:` with the pin; macOS/Windows jobs follow.
- [ ] `release.yml` (manual; `package` → `verify` → approval-gated `publish`): the dry run stops at a verified draft and the entry
  gates reject an unchecked commit ([record](docs/acceptance/ci_acceptance.md#release-workflow-2026-11-11)). Not yet exercised: the
  real publish step (`dry_run: false`), whose first use should be the next genuine Preview; it needs the reviewer's approval on the
  `release` environment, and the approval's author is to be recorded with a comment. Linux x64 and arm64 only; macOS and Windows
  jobs follow the same host-key pattern.
- [ ] Add a Windows/arm64 leg under the [pin promotion policy](docs/guides/development.md#ci-pin-promotion) once its public SDK, its
  CLI Preview and a hosted runner exist. The policy and `tools/check_ci_pins.py` are in place (2026-11-11).

## Planned

### D2 - Management beyond the first workflow

- [ ] General stage/profile/version resolution and Full-stage installation;
  coordinate producer metadata and size/performance limits with CRT.
- [ ] Explicit SDK updates and broader installation/reference/GC policy without
  implicit project-lock migration.
- [ ] Verified CLI self-upgrade: candidate health, OS-specific activation,
  interruption/rollback and unchanged SDK/port pins
  ([lifecycle gates](docs/acceptance/lifecycle_acceptance.md)). This is outside
  the first preview's critical path and does not block D3.

### D4 - Dependencies, Variants, Locks, and Local Cache

- [ ] Add a bounded `amalgamation` executor/adapter for sqlite-amalgamation;
  the current D3 parser accepts only `configure`. Prove static/shared consumption.
- [ ] Add libpng -> zlib dependency selection and transitive build keys, provider/
  variant/lock/view/cache handling. Revisit the Windows oracle's `z.lib` layout
  difference before dependent ports; preserve existing immutable items.
- [ ] Replay ABI mismatch, rerun/recovery, and static/shared behavior on the
  initial three native host pairs.

### D5 - Application Workflow

- [ ] Extend the bounded native new/configure/build/test/run to dependency
  resolution and upper stages; app workflows must not require Python, shell or CMake.
- [ ] Validate external consumers, runtime closures, and Web worker layout
  without producer-private headers or build-tree inputs.
- [ ] Extend the initial workflow above to general locked/offline behavior,
  dependency/template parity and broader SDK profiles; keep layout evolution explicit.

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
