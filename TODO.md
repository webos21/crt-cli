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
