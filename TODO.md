# TODO: CRT-CLI Work Queue

This file owns the concise, ordered implementation queue. Design and dependency
order belong to [the CLI design](docs/design/crt_cli.md); measured evidence
belongs to [acceptance](docs/acceptance/bootstrap_acceptance.md). Completed
work is checked off here and recorded in [HISTORY](HISTORY.md).

## Notice

- Follow [AGENTS.md](AGENTS.md). Implement one bounded, authorized increment at
  a time; this queue is not permission to execute the entire roadmap.
- In Progress identifies the current focus, not a claim that all its tasks are
  already being implemented. Update it before changing focus.
- Keep commands, measurements, failures, and per-host outcomes in their owning
  guide/acceptance/history document rather than duplicating them here.
- Check an item (`[x]`) when its work is done and validated, and record the
  outcome in HISTORY and the owning acceptance document. Checked items stay
  until every item of their section is complete; the section is then deleted
  only when the user asks. Never delete checked or unchecked items on your own.
- Do not update or create STATUS as a routine documentation step; do so only
  when explicitly requested. Keep the root README user-facing.
- Do not claim that a design, fixture, compile-only result, skipped test, or
  development SDK constitutes native runtime or release acceptance.

## In Progress

### CI Platform Expansion

- [x] CRT producer prerequisite: signed public macOS/arm64 and Windows/x64 host-tools bundles and their release-record pins are available;
  [crt#9 is closed](https://github.com/webos21/crt/issues/9#issuecomment-6998966548). [Producer handoff](docs/crt_host_tools_publication_request.md#macoswindows-follow-up-answered-2026-11-19).
- [x] Replay the six D3 suites locally on macOS/arm64 with the signed public `20261119.macos-aarch64.1` host-tools bundle and pinned public SDK/CLI inputs
  ([acceptance](docs/acceptance/sdk_install_acceptance.md#signed-public-host-tools-d3-replay-macosarm64-2026-11-19)); hosted integration and CI pin adoption remain separate.
- [x] Replay the public Windows/x64 host tools through D1 fail-closed controls and D3
  ([acceptance](docs/acceptance/host_environment_acceptance.md#windowsx64-published-host-tools-consumer-replay-2026-11-19)).
- [x] Replay the public Windows/arm64 host tools through D3; this is not a full D1 pass
  ([acceptance](docs/acceptance/host_environment_acceptance.md#windowsarm64-published-host-tools-consumer-replay-2026-11-19)).
- [ ] D2, cross-platform integration: add hosted macOS/arm64 and Windows/x64 jobs to `integration.yml` that run the native Port and
  oracle suites with reviewed integration pins and host-specific acceptance, including the hosted Windows symlink prerequisite.
  Producer publication is complete; native receipts above do not substitute for these jobs.
  - [x] macOS/arm64 implementation and local preflight on `codex/macos-host-tools-integration`
    ([candidate evidence](docs/acceptance/ci_acceptance.md#macosarm64-integration-expansion-2026-11-19)); not adopted on main yet.
  - [ ] macOS/arm64 hosted `pins`/`redirect`/`ports` verification and reviewed integration-pin adoption; local evidence is not hosted acceptance.
- [ ] Resolve the Windows/arm64 D1 `stdout-limit` timeout against the public CLI/SDK; retain the 31/32 and focused failure
  [receipts](docs/acceptance/host_environment_acceptance.md#windowsarm64-published-host-tools-consumer-replay-2026-11-19).
- [ ] Windows/arm64 under that policy: the leg is added, pinned, hosted-qualified and its `release.yml` rehearsal passed (checked sub-items below); open: a
  workflow-built, published and signed Preview.
  - [x] Wire `win-arm64` into `ci_bootstrap.py`, `ci_release.py`, `release.yml` and their tests; prepare and replay a local SDK and CLI candidate
    ([record](docs/acceptance/ci_acceptance.md#windowsarm64-host-wiring-and-local-candidate-preflight-2026-11-17)).
  - [x] CRT: Windows/arm64 rows in `sdk-integration.yml` and `sdk-release.yml`; Dev SDK `v0.0.1-preview.20261118.windows.arm64.1` published and signed.
  - [x] Publish and sign the first `win-arm64` Preview built from that SDK ([record](docs/acceptance/ci_acceptance.md#first-windowsarm64-preview-pins-and-the-ciyml-row-2026-11-18)).
  - [x] Pull request 19 pinned SDK and Preview and added the `ci.yml` matrix row; its hosted `windows-11-arm` leg passed on the pull request and on `main`
    ([record](docs/acceptance/ci_acceptance.md#hosted-windows-11-arm-result-of-the-win-arm64-pins-2026-11-18)).
  - [x] CRT: `windows-aarch64` pinned in `.github/ci/sdk-pins.json`, `cli_preview: none` and the `bootstrap` marker removed in one reviewed change (crt pull request 5); the consumer smoke by this
    Preview ran on `windows-11-arm` ([crt record](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#hosted-windowsarm64-consumer-smoke-by-the-pinned-cli-s7-steps-4-5-2026-11-18)).
  - [x] `release.yml` `dry_run: true` rehearsal for `win-arm64` on `windows-11-arm`: `package`, `verify` and an approved `publish` produced a verified draft whose archive equals the
    workflow artifact and whose executable is an ARM64 PE ([record](docs/acceptance/ci_acceptance.md#windowsarm64-releaseyml-rehearsal-2026-11-19)); the draft was deleted.
  - [ ] A Preview built, published and signed by the workflow for `win-arm64` (`dry_run: false`), with the owner's offline signature and its signature replay. Until then the pinned Preview is the hand-built
    first one. A later pin change to a workflow-built Preview is its own reviewed change in both repositories (here `pins.json`, in crt `.github/ci/sdk-pins.json`). The leg itself needs no manual
    `ci.yml` dispatch: it passed as a pull request and as a push to `main`.

## Planned

### Full SDK Support

Producer readiness and CLI support have different owners and neither blocks the first Preview.

- [ ] Keep Full SDK packaging/security/source/license review on its own path;
  Full installation/profile support is not a prerequisite for the first CLI preview. Resume when CRT completes its Full SDK review;
  CLI profile and installation support then joins the stage/profile item of D2 below and D5.

### CI Operations

- [ ] Sign-before-publish (the key stays offline, so the decision above is unchanged): the workflow stops at the verified draft, the owner attaches the `.sig` to the draft, and a
  `finalize` step verifies the signature with the pinned key and the draft's exact asset set before making the release public. This removes the unsigned public window and
  checks the signature inside the workflow; it needs the same change in `crt`'s `sdk-release.yml`. Open choices before building it: where the `release` approval sits (stage,
  finalize or both), whether a `dry_run` draft may be finalized (default no), and whether the signature also covers the `SHA256SUMS-*` file. GitHub artifact attestations are a
  separate, optional addition.
- [ ] Observe the daily cadence of the scheduled `integration.yml`: its first scheduled run happened and passed
  ([record](docs/acceptance/ci_acceptance.md#first-scheduled-integrationyml-run-2026-11-18)), but the 2026-11-18 03:23 UTC slot had no run at 09:44 UTC.
  Determine over several days whether runs are late, skipped or missing; a red run is a regression or a changed external input, not a known gap.
- [ ] Observe the ruleset rejecting a non-administrator's direct push or failing-check merge: administrator bypass is `always`, so a
  rejection was not seen. This needs a second account; tighten the bypass when a second maintainer exists. The ruleset setting and the
  passing and `BLOCKED` pull-request states are accepted; this is a recorded verification limit, not a verified enforcement.

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

D4 may proceed against the frozen D3 contracts; it does not enlarge D3 or become a first-preview release gate. General
updates/self-upgrade and Studio retain their later queues. Execution policy remains in
[the design](docs/design/crt_cli.md#execution-priorities-adopted-2026-11-05).

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
