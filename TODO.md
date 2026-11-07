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

### First user workflow: local SDK selection -> first build -> pinned acquisition

The local D2 installer and Stage 2 stored-contract/project-selection flow have
native evidence on Linux/x86_64, Linux/aarch64, macOS/arm64 and Windows/x64/arm64.
Retain those exact per-host baselines in [SDK installation acceptance](docs/acceptance/sdk_install_acceptance.md);
Windows/x64 Stage 2 is complete. The later receipt/pinned-repair increment has
its own [Linux isolated evidence](docs/acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-linuxx86_64-2026-11-05)
and [macOS replay](docs/acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-macosarm64-2026-11-05).
The [Windows/x64 receipt boundaries pass](docs/acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-windowsx64-2026-11-05); its
concurrent-install native crash was root-caused in the CRT libunwind recipe and is
[closed](docs/acceptance/sdk_install_acceptance.md#windowsx64-concurrent-install-sigsegv-fix-2026-11-06).
The earlier shape-only verify guard keeps its original developer/unit scope.
[STATUS](STATUS.md) is the separately refreshed, dated baseline snapshot.

Keep D0-D7 identifiers. Dependencies are working capabilities, not completion
of every checkbox in an earlier stage. The execution/validation policy belongs
to [the CLI design](docs/design/crt_cli.md#execution-priorities-adopted-2026-11-05).

Work can proceed in three coordinated streams: CLI workflow implementation,
affected-host replay/producer fixes, and preview trust/packaging. Pin shared
contracts before parallel work; changes to the same manifest/store/lock format
need one coordinated producer/consumer review. No new implementation or replay
is claimed by this queue update.

### D2 - Close the bounded local installation/selection baseline first

Installation-time receipts, required/absent contract pin checks and explicit
release-based repair are verified on Linux/macOS and bounded Windows/x64 paths; details
live in [Linux acceptance](docs/acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-linuxx86_64-2026-11-05),
[macOS replay](docs/acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-macosarm64-2026-11-05)
and [HISTORY](HISTORY.md).

- [x] Replay the bounded receipt/repair delta on Windows/x64 with the existing
  identified SDK, including pin replacement, dead/live metadata writers and an
  actual symlink receipt; [evidence](docs/acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-windowsx64-2026-11-05).
- [x] Windows/x64 concurrent-install native SIGSEGV: root-caused to the pinned
  libunwind not restoring callee-saved XMM6-XMM15 on catch, fixed in the CRT
  recipe, and replayed with a CLI self-built against the fixed SDK
  ([record](docs/acceptance/sdk_install_acceptance.md#windowsx64-concurrent-install-sigsegv-fix-2026-11-06)). Windows/arm64 repeated the scenario 15 of 15 times without a crash.
- [x] Replay the receipt/repair delta on Windows/arm64 using its existing
  identified SDK ([installer/receipt driver 101/101](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsarm64-2026-11-06)).
  Earlier Stage 2 evidence is not a replay of the new code.
- [x] Replay the full-identity matcher fix and installed-SDK collision cases
  on Windows/x64 ([68/68](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsx64-2026-11-06), including decoy executables and a documented working-directory DLL limitation) and Windows/arm64 ([68/68](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsarm64-2026-11-06)). The
  [Linux selection slice passes](docs/acceptance/sdk_install_acceptance.md#installed-sdk-selection-and-search-path-collisions-linuxx86_64-2026-11-05);
  [macOS replay passes](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-macosarm64-2026-11-06).
  Preserve per-host SDK/tool pins and keep Windows concurrency closure separate.

### D2 / basic D5 - First build and acquisition, in this order

Installed-SDK first build now has a bounded
[Linux implementation and acceptance](docs/acceptance/sdk_install_acceptance.md#first-build-without-configure-linuxx86_64-2026-11-06);
completed details live in HISTORY.

Explicit pinned HTTPS acquisition and installed-identity reuse also have
[Linux evidence](docs/acceptance/sdk_install_acceptance.md#pinned-https-sdk-acquisition-linuxx86_64-2026-11-06);
first build and acquisition currently support the native `dev` profile. General
profiles, channels and registry resolution remain separate work.
The [macOS selection/first-build/acquisition replay](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-macosarm64-2026-11-06)
also passes, with a shared consumer correction for actual producer release envelopes.

- [ ] Qualify one Linux preview candidate: CLI/resources/notices, an identified
  SDK and portable contract, external-tool setup and the end-to-end user flow.
  A developer pin/test key is not official publisher trust. General default
  downloads require approved official trust inputs and actual producer promotion
  before consumer adoption/publication.
- [x] Replay the new workflow on Windows/x64 ([first build 86/86, pinned HTTPS 74/74](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsx64-2026-11-06))
  and Windows/arm64 ([first build 86/86, pinned HTTPS 74/74](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsarm64-2026-11-06))
  before claiming that feature there. Existing aarch64/Windows-arm64 evidence stays valid for
  its recorded scope; extending new-feature support has its own replay queue.
- [x] Replay the release-envelope correction with original producer assets on
  Windows/x64 (rehearsal.8, same record) and Windows/arm64 (original rehearsal.7 manifest, [record](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsarm64-2026-11-06)); Linux/aarch64 if wanted remains. Linux/x86_64 passes
  ([record](docs/acceptance/sdk_install_acceptance.md#release-envelope-correction-with-original-producer-assets-linuxx86_64-2026-11-06));
  the earlier Linux fixture evidence is not relabeled as a run of the corrected candidate.

### D3 - First source-built port, parallel with first-build/acquisition work

- [x] Define the bounded recipe/compiler adapter: [design record](docs/design/source_port_adapter.md)
  (2026-11-06; design only, producer review of its adapter table pending).
- [ ] Implement it in order (design §3): ~~hardened `tar.gz` reading~~ (done, [record](docs/acceptance/sdk_install_acceptance.md#hardened-targz-reading-d3-step-1-linuxx86_64-2026-11-07)), recipe subset reader, executor through the host-tools
  bundle, audit/store publish, explicit provider choice with the consumer proof, then the `crt-port-build.py` differential.
  Executes the reviewed zlib JSON recipe natively into a separate immutable port store.
- [ ] Prove fresh source build, static/shared link/run and actual consumption
  from that store. The Dev SDK already contains zlib: define SDK-provided versus
  explicit isolated source-build handling; do not overwrite it or count linking
  its existing library as the first source port. Classify make as a host tool.
- [ ] Preserve SDK/source inputs and host-tool/compiler identities. Linux is
  the first implementation slice; full D3 closure retains the native Windows
  CRT-shell/make static/shared gate and applicable native OS replay.

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

- [ ] Add SQLite and libpng-to-zlib resolution, build keys, project views,
  collision handling, and validated local-cache reuse.
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
