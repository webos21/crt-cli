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

### Dev SDK / CLI Preview preparation

- [ ] Publisher signatures: `sdk acquire` and the CI host-tools pins accept/require them (Linux-verified, see
  [ci_acceptance](docs/acceptance/ci_acceptance.md#publisher-signatures-for-host-tools-pins-and-sdk-acquire-2026-11-16)); `sdk install` and the bootstrap/SDK pin
  signatures are done too ([record](docs/acceptance/ci_acceptance.md#publisher-signatures-for-the-bootstrap-cli-pin-and-sdk-install-2026-11-16));
  open: the two unchecked sub-items below.
  - [x] Replay native macOS/arm64 `sdk acquire`, `sdk install` and `sdk repair`
    publisher authentication ([record](docs/acceptance/ci_acceptance.md#macosarm64-sdk-publisher-signature-replay-2026-11-17)).
  - [x] Pin CLI Previews with these options as the bootstrap on every host, so CI's SDK download is natively verified with the pinned key
    (`sdk_source_publisher`; macOS/arm64 and Windows/x64 [moved to the 20261117 Previews](docs/acceptance/ci_acceptance.md#macos-and-windows-bootstrap-pins-moved-to-the-20261117-preview-2026-11-17),
    Windows/arm64 [pinned with its first Preview](docs/acceptance/ci_acceptance.md#first-windowsarm64-preview-pins-and-the-ciyml-row-2026-11-18)).
  - [ ] Replay native Windows (x64 and arm64) `sdk acquire`, `sdk install` and `sdk repair` publisher authentication, including the refusals (wrong signature, wrong key,
    tampered archive), as was done for macOS/arm64. CI exercises only the accepting path on Windows.
  - [x] Decide where release signing happens: the owner signs offline after publication and `release.yml` never holds a key
    ([decision](docs/design/crt_cli.md#release-signing-decision-2026-11-18)).

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
- [x] Adopt the default/project pin policy against the published Linux/x86_64,
  Linux/aarch64, macOS/arm64 and Windows/x64 SDK/CLI Previews, and close it by confirming the existing records
  (retain the [Windows publication baseline](docs/acceptance/preview_acceptance.md#windowsx64-cli-preview-release-2026-11-10)
  and the Linux/macOS records). The policy is two separate contracts, not a new lock format: the repository root
  `crt.lock` stays `unresolved` (one source tree builds on every host, so it cannot pin one SDK); a user project's
  `crt.lock` pins its own SDK and ports explicitly and an SDK change never migrates it silently; the CI bootstrap pins
  live per host in `.github/ci/pins.json` under the checked [promotion policy](docs/guides/development.md#ci-pin-promotion).
  Done when acceptance records show project-lock creation, reuse and tampered-lock refusal with the real public SDK on all
  four hosts; add only the missing cases.
  - [x] Record the [root/project/CI policy and four-host evidence table](docs/acceptance/preview_acceptance.md#project-pin-policy-evidence-audit-2026-11-16).
  - [x] Linux/aarch64: public CLI/SDK lock creation, byte-identical reuse, tampered-lock refusal and preservation under changed SDK selection
    ([54-case native replay](docs/acceptance/preview_acceptance.md#linuxaarch64-public-byte-replay)).
  - [x] macOS/arm64: confirm public CLI/SDK identities, project-lock creation and byte-identical reuse;
    close project-lock tamper and changed-selection gaps ([54-case native Seatbelt replay](docs/acceptance/preview_acceptance.md#macosarm64-public-byte-replay)).
  - [x] Windows/x64: public CLI/SDK identities, C/C++ lock creation/reuse, tampered-lock refusal and changed-selection preservation
    ([58-case native replay](docs/acceptance/preview_acceptance.md#windowsx64-public-byte-replay)).
  - [x] Linux/x86_64: public CLI/SDK identities, C/C++ lock creation/reuse, tampered-lock refusal and changed-selection preservation
    ([54-case native replay](docs/acceptance/preview_acceptance.md#linuxx86_64-public-byte-replay)); the four-host policy audit is complete.
- The Full SDK review item moved to Planned: [Full SDK Support](#full-sdk-support).

D4 may proceed against the frozen D3 contracts; it does not enlarge D3 or
become a first-preview release gate. General updates/self-upgrade and Studio
retain their later queues. Execution policy remains in
[the design](docs/design/crt_cli.md#execution-priorities-adopted-2026-11-05).

## Planned

### Full SDK Support

Producer readiness and CLI support have different owners and neither blocks the first Preview.

- [ ] Keep Full SDK packaging/security/source/license review on its own path;
  Full installation/profile support is not a prerequisite for the first CLI preview. Resume when CRT completes its Full SDK review;
  CLI profile and installation support then joins the stage/profile item of D2 below and D5.

### CI Platform Expansion

- [ ] D2, cross-platform integration: add hosted macOS/arm64 and Windows/x64 jobs to `integration.yml` that run the native Port and
  oracle suites with their own pinned host-tools bundles. Resume after D1 and the per-host bundles; each host needs its own acceptance record.
- [ ] Windows/arm64 under that policy: the leg is added, pinned and hosted-qualified (checked sub-items below); open: its `release.yml` rehearsal and a
  workflow-built Preview.
  - [x] Wire `win-arm64` into `ci_bootstrap.py`, `ci_release.py`, `release.yml` and their tests; prepare and replay a local SDK and CLI candidate
    ([record](docs/acceptance/ci_acceptance.md#windowsarm64-host-wiring-and-local-candidate-preflight-2026-11-17)).
  - [x] CRT: Windows/arm64 rows in `sdk-integration.yml` and `sdk-release.yml`; Dev SDK `v0.0.1-preview.20261118.windows.arm64.1` published and signed.
  - [x] Publish and sign the first `win-arm64` Preview built from that SDK ([record](docs/acceptance/ci_acceptance.md#first-windowsarm64-preview-pins-and-the-ciyml-row-2026-11-18)).
  - [x] Pull request 19 pinned SDK and Preview and added the `ci.yml` matrix row; its hosted `windows-11-arm` leg passed on the pull request and on `main`
    ([record](docs/acceptance/ci_acceptance.md#hosted-windows-11-arm-result-of-the-win-arm64-pins-2026-11-18)).
  - [x] CRT: `windows-aarch64` pinned in `.github/ci/sdk-pins.json`, `cli_preview: none` and the `bootstrap` marker removed in one reviewed change (crt pull request 5); the consumer smoke by this
    Preview ran on `windows-11-arm` ([crt record](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#hosted-windowsarm64-consumer-smoke-by-the-pinned-cli-s7-steps-4-5-2026-11-18)).
  - [ ] `release.yml` for `win-arm64` (this repository owns it; crt only consumes the result): a `dry_run: true` rehearsal on `windows-11-arm`, then a Preview built, published and signed by the
    workflow with its signature replay. Until then the pinned Preview is the hand-built first one. A later pin change to a workflow-built Preview is its own reviewed change in both repositories
    (here `pins.json`, in crt `.github/ci/sdk-pins.json`). The leg itself needs no manual `ci.yml` dispatch: it passed as a pull request and as a push to `main`.

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
