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

Task IDs are scoped to their topic heading. Refer to the repository and topic
when a prefix such as `P1` or `V1` appears under multiple headings.

## In Progress

Queue reviewed on 2026-11-20 against local CLI `e362a51`, CRT `278c5d9` and
extension `b547406`. This is source/document review, not fresh native acceptance
or a pin change. Existing five-host Preview/CI and D3 results retain their exact
identities in [CI acceptance](docs/acceptance/ci_acceptance.md) and
[Preview acceptance](docs/acceptance/preview_acceptance.md).

### P0 - Minimal Public Editor API (D7 slice, coordinated with D5/D6)

`crt-vscode` is the first editor consumer. Other editors and automation can use
the same API, and clangd reads the generated database. This D7 slice can use
the existing Dev SDK and bounded project workflow.

Contract baseline:
[implemented native policy](docs/reference/bootstrap_contract.md#producer-owned-native-policy)
and [commands and results](docs/reference/bootstrap_contract.md#commands-and-results) .
The proposed API gates below are pending. Coordinate SDK fixtures with
[crt SDK support P1-01](https://github.com/webos21/crt/blob/main/TODO.md#p1---sdk-support-for-the-first-editor-workflow)
. P0-01 through P0-03 supply
[crt-vscode V0-01 through V0-03](https://github.com/webos21/crt-vscode/blob/main/TODO.md#v0---crt-cli-contract-owned-by-crt-cli-requirements-only-here)
. Include final-envelope fixtures for V0-04 under P0-04.

- [ ] **P0-01** Specify and implement proposed `export compile-commands --output <file>`
  from the same SDK-expanded per-source argv used by the native builder.
  Include working directory, absolute source, target/sysroot, standard-library
  headers, macros and language options; exclude link/startup inputs. Handle
  spaces/Unicode and stale SDK/lock inputs without silently changing project
  pins or downloading/building ports for an export.
- [ ] **P0-02** Specify and implement proposed `project info --json` for project,
  SDK/profile/target, lock identity, build output/executable and unresolved or
  incompatible state. Queries must be read-only and report missing prerequisites.
- [ ] **P0-03** Define versioned capability discovery and command/error compatibility
  for editor consumers; current help text is not a structured capability API.
  Document existing `sdk list` and `port list` JSON before adding another list
  command or schema.
- [ ] **P0-04** Add focused contract fixtures and native consumer evidence to
  [the implemented reference](docs/reference/bootstrap_contract.md) and
  [compatibility](docs/compatibility.md). Proposed command spellings/fields above
  remain unimplemented until adopted and exercised.
- Acceptance: exported C/C++ compile entries match actual cold-build compiler
  invocations; queries preserve manifests/locks and report exact selected inputs;
  unsupported commands/versions fail clearly in the extension. Start with the
  Linux/x86_64 editor workflow, then qualify macOS/arm64 and Windows/x64.

Global `--json`, including `build --json`, already returns one
`crt-cli.bootstrap-result`, format 1 envelope; child output is relayed to stderr
([dispatch](src/main/main.cpp), [reply](src/main/core.cpp), [process](src/main/process.cpp)).
Stabilize those existing results/errors; streaming progress/job events need a
separate opt-in stream/file protocol under D6, not JSON mixed into stdout.

### P1 - Producer Correctness Handoff and First-user Workflow (D5)

Acceptance baselines:
[Linux shared-provider control](docs/acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09)
,
[Windows filesystem reproduction](docs/acceptance/bootstrap_acceptance.md#findings-fixed-in-this-pass)
and
[project lifecycle PR-03 through PR-05](docs/acceptance/lifecycle_acceptance.md#d2d5---project-sdk-and-creation)
. P1-01 consumes
[crt P0-01 / Tranche 6.4](https://github.com/webos21/crt/blob/main/TODO.md#p0---pure-c-shared-provider-policy-tranche-64--d5)
and
[crt Windows C++ P1-01](https://github.com/webos21/crt/blob/main/TODO.md#p1---windows-c-public-compile-options-d5)
. P1-02 supplies setup and diagnosis for
[crt-vscode V1-03 and V1-06](https://github.com/webos21/crt-vscode/blob/main/TODO.md#v1---extension-mvp)
.

- [ ] **P1-01** Coordinate CRT Tranche **6.4** C shared provider/runtime-search review and
  Windows Dev-profile `<filesystem>` compile-option replay. After producer
  qualification, consume an immutable SDK candidate, replay external C/C++
  build/run and provider/load checks, then adopt exact pins. Preserve failed
  identities; do not compensate with CLI/extension compiler-policy overrides.
- [x] Linux/x86_64 clean C-provider candidate replay and early mixed-runtime refusal
  ([evidence](docs/acceptance/sdk_install_acceptance.md#clean-linux-c-shared-candidate-replay-2026-11-21)).
  SDK promotion and pin adoption, affected native hosts and Windows C++ remain open.
- [x] Windows/arm64: the CI-built SDK of CRT `5325cea` with CLI `44fbf24`: native 1,041 checks, and the pure C static, shared and shared + project DLL modes 36/36 with the loaded `libc.dll` equal to the installed SDK's
  (the published `20261118` SDK fails six of the same 36 cases)
  ([record](docs/acceptance/sdk_install_acceptance.md#windowsarm64-pure-c-shared-sdk-replay-and-integration-at-44fbf24-2026-11-22)). A CI artifact is not a release: promotion, pin adoption, Windows/x64 and macOS remain open.
- [x] Store `Lock` replaced by an OS file lock with a multi-process regression; verified on Windows/x64 only
  ([HISTORY](HISTORY.md), 2026-11-22). Rerun the scheduled Integration job and Linux/macOS/arm64 before closing:
  both conditions are met at `44fbf24` (the sub-items below; the `CI` run passed all six native legs, which run `store-lock-test`).
  - [x] Windows/arm64: the manual Integration run at `44fbf24` passed all 15 jobs including `ports (win-arm64)`, and the native replay passes the 9-check `store-lock-test`; the `CI` run at `44fbf24` passed
    all six native legs, which run `store-lock-test` as a PR test
    ([record](docs/acceptance/sdk_install_acceptance.md#windowsarm64-pure-c-shared-sdk-replay-and-integration-at-44fbf24-2026-11-22)).
  - [x] The first scheduled Integration run after the lock fixes passed all 15 jobs, including `ports (win-arm64)` (the one before, at `8374470`, failed there)
    ([record](docs/acceptance/sdk_install_acceptance.md#windowsarm64-pure-c-shared-sdk-replay-and-integration-at-44fbf24-2026-11-22)). Scheduled runs start between 11:23 and 11:54 GMT, not at the 03:23 GMT slot.
- [ ] No in-process exclusion in `Lock`: CRT `flock` is built on `fcntl` record locks, so two `Lock` objects in one process do not
  exclude each other. Today's stores never publish the same key from two threads of one process. Before adding
  threaded publication, add an in-process mutex per lock path (or a CRT per-open-file-description `flock`) and a
  thread-level regression.
- [ ] Lock files are never removed, so `.locks` gains one empty file per SDK content or port build key (`recover()` skips them).
  Decide whether a bounded cleanup is worth the risk of racing a live holder; removing a lock file under a holder
  breaks exclusion, so any cleanup must hold the lock first.
- [ ] **P1-02** Improve the documented new/install/build/run path with scoped SDK/LLVM/OS
  SDK diagnosis and concrete missing-input guidance. Reuse explicit pinned SDK
  acquisition and declared tool paths; official defaults/trust/channels require
  their own reviewed contract, not implicit downloads on workspace open.
- Acceptance: a clean user environment reaches C/C++ build/test/run using only
  public SDK inputs and declared host tools, with explicit acquisition consent,
  accurate failures and unchanged existing project locks on failure.

<a name="ci-operations"></a>

### P1 - Sign Before Publishing

Coordinate release finalization with CRT. The current workflows publish before
the owner attaches the offline signature.

Existing baseline:
[hosted publication and attached signatures](docs/acceptance/ci_acceptance.md#first-real-publication-and-its-signatures-2026-11-17). Rehearse the new finalization path with
[crt SDK signing P1-01 and P1-02](https://github.com/webos21/crt/blob/main/TODO.md#p1---sign-before-publishing-sdks-d6).

- [ ] **P1-01** Sign-before-publish: the workflow stops at the verified draft, the owner attaches the offline `.sig` to the draft, and a
  `finalize` step verifies the signature with the pinned key and the draft's exact asset set before making the release public. This removes the unsigned public window and
  checks the signature inside the workflow; it needs the same change in `crt`'s `sdk-release.yml`. Open choices before building it: where the `release` approval sits (stage,
  finalize or both), whether a `dry_run` draft may be finalized (default no), and whether the signature also covers the `SHA256SUMS-*` file. GitHub artifact attestations are a
  separate, optional addition.
- Acceptance: missing/wrong signatures, changed/extra assets and invalid
  finalization attempts stay non-public; approved signed bytes finalize once and
  independently verify after download. Preserve interrupted/retry safety and
  keep key distribution/rotation/revocation and OS code signing separate.

## Planned

<a name="ci-monitoring-and-pin-promotion"></a>

### CI - Monitoring and Pin Promotion

Acceptance records:
[pin promotion](docs/acceptance/ci_acceptance.md#ci-pin-promotion-2026-11-11) and
[20261120 Windows/arm64 release](docs/acceptance/preview_acceptance.md#windowsarm64-workflow-preview-release-2026-11-20)
for CI-01,
[ARM64 hosted limits](docs/acceptance/ci_acceptance.md#linuxarm64-leg-2026-11-11) and
[crt native compatibility P1-01](https://github.com/webos21/crt/blob/main/TODO.md#p1---native-compatibility-beyond-hosted-runners)
for CI-02, and
[ruleset observations](docs/acceptance/ci_acceptance.md#current-hosted-coverage-2026-11-11-1)
for CI-04. CI-03 links the scheduled-run evidence below.

- [ ] **CI-01** Apply the existing [CI pin-promotion policy](docs/guides/development.md#ci-pin-promotion)
  to qualified signed SDK/CLI candidates. Review the published Windows/arm64
  20261120 workflow Preview separately from the retained 20261118 bootstrap;
  promote only with release-record, signature and affected-suite evidence, not
  because its tag is newer. Keep previous working pins and project locks.
- [ ] **CI-02** Track native distribution/device compatibility separately from CI:
  coordinate CRT's Raspberry Pi/older-glibc qualification before advertising it;
  the ARM64 Ubuntu 26.04/glibc 2.43 record is the current bounded evidence.
- [ ] **CI-03** Observe the daily cadence of scheduled `integration.yml`: recorded runs pass, including the first Windows run,
  but observed start times differ from the configured cron ([observations](docs/acceptance/ci_acceptance.md#first-scheduled-windowsx64-integration-run-2026-11-19)).
  Continue tracking late, skipped or missing runs; timing causes remain unverified and a red run is a regression or a changed external input.
- [ ] **CI-04** Observe the ruleset rejecting a non-administrator's direct push or failing-check merge: administrator bypass is `always`, so a
  rejection was not seen. This needs a second account; tighten the bypass when a second maintainer exists. The ruleset setting and the
  passing and `BLOCKED` pull-request states are accepted; this is a recorded verification limit, not a verified enforcement.

### D2 - Management beyond the first workflow

Acceptance:
[project lifecycle PR-01 through PR-06](docs/acceptance/lifecycle_acceptance.md#d2d5---project-sdk-and-creation)
for D2-01 and D2-02, and
[upgrade lifecycle UP-01 through UP-07](docs/acceptance/lifecycle_acceptance.md#d2---cli-self-upgrade)
for D2-03. Coordinate metadata and lifecycle boundaries with
[crt D5-03](https://github.com/webos21/crt/blob/main/TODO.md#tranche-6-sdk-only-application-workflow-d5)
and
[crt SDK-01 and SDK-02](https://github.com/webos21/crt/blob/main/TODO.md#sdk-workflow-beyond-the-first-preview)
.

- [ ] **D2-01** General stage/profile/version resolution and Full-stage installation;
  coordinate producer metadata and size/performance limits with CRT.
- [ ] **D2-02** Explicit SDK updates and broader installation/reference/GC policy without
  implicit project-lock migration.
- [ ] **D2-03** Verified CLI self-upgrade: candidate health, OS-specific activation,
  interruption/rollback and unchanged SDK/port pins
  ([lifecycle gates](docs/acceptance/lifecycle_acceptance.md)). This is outside
  the first preview's critical path and does not block D3.

### D4 - Dependencies, Variants, Locks, and Local Cache

P2 expansion after the selected Preview correctness and minimal editor API work.

D4 may proceed against the frozen D3 contracts; it does not enlarge D3 or become a first-preview release gate. General
updates/self-upgrade and broader developer-tool integration retain their later queues. Execution policy remains in
[the design](docs/design/crt_cli.md#execution-priorities-adopted-2026-11-05).

Acceptance baseline:
[D3 zlib adoption](docs/compatibility.md#d3-bounded-adapter-adoption-2026-11-09) for
zlib and
[Windows/x64 adapter replay](docs/acceptance/sdk_install_acceptance.md#source-port-d3-windowsx64-implementation-and-replay-2026-11-09)
for platform limits. New SQLite and libpng cases are pending. Coordinate CRT recipes and
replay with
[crt D4-01 and D4-02](https://github.com/webos21/crt/blob/main/TODO.md#tranche-5-dependencies-variants-locks-and-cache-d4)
.

- [ ] **D4-01** Add a bounded `amalgamation` executor/adapter for sqlite-amalgamation;
  the current D3 parser accepts only `configure`. Prove static/shared consumption.
- [ ] **D4-02** Add libpng -> zlib dependency selection and transitive build keys, provider/
  variant/lock/view/cache handling. Revisit the Windows oracle's `z.lib` layout
  difference before dependent ports; preserve existing immutable items.
- [ ] **D4-03** Replay ABI mismatch, rerun/recovery, and static/shared behavior on the
  initial three native host pairs.

### D5 - Application Workflow

Acceptance:
[project lifecycle PR-01 through PR-07](docs/acceptance/lifecycle_acceptance.md#d2d5---project-sdk-and-creation)
and
[Web stage acceptance](https://github.com/webos21/crt/blob/main/docs/acceptance/crtweb_acceptance.md)
as a producer baseline for workers/resources. Pair with
[crt D5-01 through D5-03](https://github.com/webos21/crt/blob/main/TODO.md#tranche-6-sdk-only-application-workflow-d5)
.

- [ ] **D5-01** Extend the bounded native new/configure/build/test/run to dependency
  resolution and upper stages; app workflows must not require Python, shell or CMake.
- [ ] **D5-02** Validate external consumers, runtime closures, and Web worker layout
  without producer-private headers or build-tree inputs.
- [ ] **D5-03** Extend the initial workflow above to general locked/offline behavior,
  dependency/template parity and broader SDK profiles; keep layout evolution explicit.

### D6 - Packaging and Automation

Acceptance:
[distribution gates PK-01 through PK-04](docs/acceptance/lifecycle_acceptance.md#d6---distribution-and-diagnostics)
,
[locked/offline gate PR-03](docs/acceptance/lifecycle_acceptance.md#d2d5---project-sdk-and-creation)
and [bounded host-environment evidence](docs/acceptance/host_environment_acceptance.md)
. Coordinate SDK contracts and pin adoption with
[crt D6-01 through D6-03](https://github.com/webos21/crt/blob/main/TODO.md#tranche-7-deployment-and-automation-contracts-d6)
.

- [ ] **D6-01** Complete test/package/doctor, versioned JSON/job events, cancellation,
  locked/offline behavior, and explicit recovery/cleanup.
- [ ] **D6-02** Validate CLI/app packages without development SDK/build/store roots;
  preserve notices/source obligations and declared OS prerequisites.
- [ ] **D6-03** Replay the CLI one-executable, scoped doctor and self-upgrade/recovery gates
  on the actual release artifact; application packages retain their own runtime closures.
- [ ] **D6-04** Coordinate public SDK/CLI delivery: verify the promoted producer artifact,
  adopt its exact pin, then publish the CLI; retain independent product versions.

### D7 - Developer Tool Integration

Acceptance and adoption:
[compatibility review](docs/compatibility.md#shared-change-review) ,
[crt D7-01 and D7-02](https://github.com/webos21/crt/blob/main/TODO.md#tranche-8-developer-tool-integration-and-exercised-combinations-d7)
,
[crt-vscode V1-10](https://github.com/webos21/crt-vscode/blob/main/TODO.md#v1---extension-mvp)
,
[crt-vscode V1 follow-up V1-01](https://github.com/webos21/crt-vscode/blob/main/TODO.md#v1-follow-up---native-replay)
and
[crt-vscode V3-01 through V3-03](https://github.com/webos21/crt-vscode/blob/main/TODO.md#v3---debugging)
. Record the exact installed combinations after the respective editor or native debug
run.

- [ ] **D7-01** Extend the minimal public API with debug-tool/executable
  discovery as consumers need it. Adopt versioned job events once D6-01
  provides the separate opt-in protocol.
  Keep CLI APIs editor-neutral; `crt-vscode` is the first editor consumer, and
  standalone Studio remains a later frontend with its own S0-S5 acceptance.
- [ ] **D7-02** Record exercised SDK/CLI/editor versions and immutable pins in
  [compatibility](docs/compatibility.md). D7 approval applies to tested
  combinations; it does not gate SDK-only Studio PoCs or earlier experiments
  using implemented capabilities. Windows debug support needs separate CRT ABI
  validation (DWARF exceptions, emutls and the CRT runtime).

### Full SDK Support (P2, producer-gated)

Producer readiness and CLI support have different owners and neither blocks the first Preview.

Producer gate:
[crt Full SDK SDK-02](https://github.com/webos21/crt/blob/main/TODO.md#sdk-workflow-beyond-the-first-preview)
and its linked assembly review. Consumer installation requires
[D2-01](#d2---management-beyond-the-first-workflow) and
[project lifecycle acceptance](docs/acceptance/lifecycle_acceptance.md#d2d5---project-sdk-and-creation)
. Rehearsal and native Dev support are prior evidence.

- [ ] **P2-01** Keep Full SDK packaging/security/source/license review on its own path;
  Full installation/profile support is not a prerequisite for the first CLI preview. Resume when CRT completes its Full SDK review;
  CLI profile and installation support then joins the stage/profile item of D2 and D5.

### Deferred Beyond the Initial Workflow

Trusted binary caches, optional recipe-registry separation, SDK delta-component
archives, additional architectures/cross pairs, and IDE/plugin services require
separate scope and evidence. Do not make them prerequisites for D0-D3.

Plan lightweight cross-repository contract/reference checks after concrete
metadata/fixtures exist; use the [manual review procedure](docs/compatibility.md#shared-change-review)
now rather than adding another full-build CI path.
