# TODO: CRT-CLI Work Queue

This file owns the concise, ordered implementation queue. Design and dependency
order belong to [the CLI design](docs/design/crt_cli.md); measured evidence
belongs to [acceptance](docs/acceptance/bootstrap_acceptance.md). Completed
work is checked off here and recorded in [HISTORY](HISTORY.md).

## Notice

- When the same task ID appears in In Progress and Planned, record the
  verification in HISTORY and link it from both entries in the same update.
  Check both entries together once the evidence satisfies both scopes. If only
  the release scope passes, check the In Progress entry and keep the follow-up
  open with a link to the release evidence.
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

## Done

See [`HISTORY.md`](HISTORY.md) for the full, dated, reverse-chronological
record of completed work. This section stays empty in `TODO.md` itself. Finished
top-level work moves to `HISTORY.md`; terse checked prerequisite gates may remain
under an active tranche only to make its execution order legible.

<!-- Keep the anchors used by existing documentation links. -->
<a name="p0---minimal-public-editor-api-d7-slice-coordinated-with-d5d6"></a>
<a name="p1---producer-correctness-handoff-and-first-user-workflow-d5"></a>

[P0 editor API](HISTORY.md#close-the-minimal-public-editor-api-p0) ·
[P1 first-user workflow](HISTORY.md#close-the-first-user-workflow-p1).

## In Progress

### Release 0.5.0 - Standalone CLI

Use the existing five native SDK/CLI target pairs. Publish only host pairs
qualified on the final artifacts. Reuse the completed
[Editor API 1](HISTORY.md#close-the-minimal-public-editor-api-p0),
[first-user workflow](HISTORY.md#close-the-first-user-workflow-p1) and
[sign-before-publishing](docs/acceptance/ci_acceptance.md#hosted-sign-before-publishing-gate-2026-11-25)
evidence as baselines.

The checkboxes below cover 0.5.0. Items with the same ID in Planned
cover the follow-up scope. R05 numbers link the release work across repositories.

#### D5 - First application workflow

- [ ] **D5-01** (R05-03) With the candidate CLI, explicitly acquire the signed
  Dev SDK, build, test and run fresh C and C++ projects, and verify warm reuse.
  Replay the existing D3 port path with the selected Host Tools. See
  [project lifecycle acceptance](docs/acceptance/lifecycle_acceptance.md#d2d5---project-sdk-and-creation).
  Linux/aarch64: with a CLI built from the local RC1 candidate the D3 port path passes (25/25), and the editor policy replay (8 compile commands, C/C++ build, run,
  warm reuse) passes with the signed Preview CLI ([record](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#linuxaarch64-dev-sdk-050-rc1-2026-11-26));
  acquiring the signed candidate with the candidate CLI remains, because it is not signed or published yet.
  macOS/arm64: the local candidate handoff passes C/C++ first build, test, run and warm reuse (54/54), C provider 76/76 and D3 28/28
  ([record](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#macosarm64-dev-sdk-050-rc1-2026-11-26)); signed acquisition remains.

#### D6 - Standalone CLI packaging and promotion

Apply the [distribution gates](docs/acceptance/lifecycle_acceptance.md#d6---distribution-and-diagnostics)
to the standalone release and record the applicable
[CI evidence](docs/acceptance/ci_acceptance.md).

- [ ] **D6-01** (R05-03) Verify project Doctor and the handling of failed or
  canceled operations in the supported workflow. Verify recovery without
  changing existing locks.
- [ ] **D6-02** (R05-03) Verify that the standalone release archive contains
  the intended executable and notices, and that the executable runs on clean
  hosts without development SDK, build or store roots. Linux/aarch64: a local package of the candidate CLI extracts to one
  executable with notices and runs SDK-free ([record](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#linuxaarch64-dev-sdk-050-rc1-2026-11-26)).
  macOS/arm64: the local package also runs with the checkouts, SDKs, store, Xcode and Command Line Tools unreadable, and passes
  version, self and pinned-resource Doctor and missing-tool diagnostics for D6-03 ([record](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#macosarm64-dev-sdk-050-rc1-2026-11-26)).
- [ ] **D6-03** (R05-03) Verify the candidate's `--version` and self Doctor on
  each qualified host before SDK setup. Check actionable diagnostics when a
  required input is missing.
- [ ] **D6-04** (R05-04) Define the 0.5.0 version, tag and archive rules.
  Review the candidates, then adopt the exact pins from the verified signed
  Dev SDK in the
  [producer release queue](https://github.com/webos21/crt/blob/main/TODO.md#d6---dev-sdk-and-host-tools-distribution).
  Verify the archive built against those inputs and the affected scheduled
  integration. Publish through the existing signing and approval workflow
  and verify the downloaded release bytes. Retain the previous bootstrap
  pins for recovery. Keep SDK and CLI version identities independent.

#### D7 - Release compatibility

- [ ] **D7-02** (R05-06) Record the exercised SDK, Host Tools, CLI and VSIX
  combinations in [compatibility](docs/compatibility.md). Include artifact
  digests, OS/architecture, external tool identities and the applicable
  native and editor evidence. Review each artifact's rows before publication
  and include the verified host/mode limits in the release notes.

The [extension release queue](https://github.com/webos21/crt-vscode/blob/main/TODO.md#release-050---vs-code-extension)
consumes the approved SDK/CLI tuple for each editor host.

## Planned

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
- [ ] **D2-04** No in-process exclusion in `Lock`: CRT `flock` is built on `fcntl` record locks, so two `Lock` objects in one process do not
  exclude each other. Today's stores never publish the same key from two threads of one process. Before adding
  threaded publication, add an in-process mutex per lock path (or a CRT per-open-file-description `flock`) and a
  thread-level regression.
- [ ] **D2-05** Lock files are never removed, so `.locks` gains one empty file per SDK content or port build key (`recover()` skips them).
  Decide whether a bounded cleanup is worth the risk of racing a live holder; removing a lock file under a holder
  breaks exclusion, so any cleanup must hold the lock first.

### D4 - Dependencies, Variants, Locks, and Local Cache

P2 expansion beyond the 0.5.0 standalone CLI release scope.

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
  resolution and upper stages after the [0.5.0 replay](#d5---first-application-workflow);
  app workflows must not require Python, shell or CMake.
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
. Coordinate broader SDK contracts with
[crt D6-01 through D6-03](https://github.com/webos21/crt/blob/main/TODO.md#tranche-7-deployment-and-automation-contracts-d6).
See [D6-04 in the 0.5.0 distribution queue](#d6---standalone-cli-packaging-and-promotion).

- [ ] **D6-01** Extend test/package/doctor for general application workflows and
  implement versioned job events, broader cancellation, locked/offline behavior,
  and recovery/cleanup.
- [ ] **D6-02** Validate general application packages without development
  SDK/build/store roots, including runtime resources, notices and OS prerequisites.
- [ ] **D6-03** Replay self-upgrade and rollback with actual CLI release
  artifacts when self-upgrade is implemented. Validate application runtime
  closures independently.

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
- [ ] **D7-02** Record further exercised SDK/CLI/editor versions and immutable
  pins in [compatibility](docs/compatibility.md) beyond the [0.5.0 release matrix](#d7---release-compatibility).
  Debug combinations need separate native evidence. On Windows this
  includes the CRT ABI (DWARF exceptions, emutls and the CRT runtime).
  SDK-only Studio PoCs can proceed independently.

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
