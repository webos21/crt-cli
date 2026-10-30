# Contract Compatibility and Adoption

This document owns the cross-repository contract adoption record, not SDK ABI
policy or a duplicate test log. Producer evidence belongs to CRT acceptance;
CLI behavior to [bootstrap acceptance](acceptance/bootstrap_acceptance.md);
Studio GUI/editor/integration evidence to Studio acceptance. Architecture and
work order remain in [the CLI design](design/crt_cli.md) and [TODO](../TODO.md).

## Review Baseline, Not an Artifact Lock

The 2026-10-21 documentation review inspected these local pre-change revisions:

| Repository | Inspected revision |
| --- | --- |
| CRT | `a23a1732f982e7bd60ecc01a6704a9d40cd3adc9` |
| CRT-CLI | `a76d8ad9e8bcd69923f24e2d303197461e99ddf3` |
| CRT Studio | `d3793e3d22f423fe589551b0822a1fc3b6e9fb78` |

Both consumer repositories contain documentation only at those revisions.
Their original adopted design baseline remains CRT
`d21430ddeb59aea76cf6abd58c15a69f55cfda52`. Neither that design revision nor
this review snapshot identifies a downloaded/validated SDK or a runnable
CLI/Studio combination. Record later contract adoption explicitly; never
silently follow `main` as an artifact identity.

## Local Candidate Workflow Review (2026-10-28)

Reviewed CRT `3c7f43747872de099eb627c51a0376befb40506b` (plus its existing
uncommitted numbered TODO plan) and CLI
`5fcdc995d231fe2f9dbab4292000a81e0b4fce93` before this working-tree documentation
update. The original design/review revisions above remain unchanged. This
review defines a development workflow; these revisions are not SDK artifact
locks, a schema implementation or an exercised runtime combination.

CRT owns local Dev SDK production, public interfaces and technical verification;
CLI owns the independent consumer and this candidate/adoption record. The
[producer assembly evidence](https://github.com/webos21/crt/blob/3c7f43747872de099eb627c51a0376befb40506b/docs/acceptance/release_qualification.md)
and [SDK split evidence](https://github.com/webos21/crt/blob/3c7f43747872de099eb627c51a0376befb40506b/docs/acceptance/sdk_split_acceptance.md)
do not grant CLI D0/D1 acceptance. The producer's implemented Dev packaging
profile is distinct from the proposed general SDK development contract.

### Local Candidates and Promotion

A candidate can be built/verified locally without a public SDK release. One
authorized session may work on both repositories; follow the
[handoff procedure](guides/development.md#local-dev-sdk-candidate-loop).
The CLI consumes a copied/extracted SDK outside producer trees. Access to CRT
source/build results for diagnosis does not make them consumer build inputs.

Record an actual local experiment with the combination fields below and links
to [D0 case evidence](acceptance/bootstrap_acceptance.md#local-candidate-boundary),
explicitly identifying its local/unpromoted state. Development results may be
Pass/Fail/Not run for a bounded case; they do not silently enter the approved
combination or promoted-lock set. Capture changed artifacts under new identities
and preserve failed records. No candidate or CLI execution is selected by this
documentation update; the runtime ledger remains Not run.

SDK/CLI versions and publication times are independent. Before final consumer
lock adoption/public CLI delivery, validate/promote the producer, replay against
the actual promoted SDK and adopt that exact artifact identity. Local candidate
success does not validate a different merge/release artifact, nor authorize
publication. No new lock/schema version or local-install command is introduced.

### Product Planning Clarification (2026-10-28)

This checkpoint precedes the owner choices and native experiment below;
the [later record](#owner-decisions-and-linux-bootstrap-2026-10-28) supersedes
its pending-choice/runtime status without changing the original review baseline.

The same working-tree review adds early initial-layout decisions, a
single-executable distribution goal, CLI self-upgrade, scoped doctor and
project-specific SDK version/profile selection. The prior dynamic-bundle
default is replaced as a product goal, not invalidated as historical producer
evidence. Source/module directories may evolve while the public project/build
contract remains the same as generated projects.

| Planned surface | Owner and review boundary |
| --- | --- |
| CLI executable/resources | CLI; CRT supplies link/runtime contracts; native one-file and SDK-free HTTPS proof required |
| CLI upgrade/doctor | CLI; explicit self/environment/project scopes and verified candidate activation/recovery; no project-pin mutation |
| Project SDK binding/new | CLI; exact SDK/profile/target/capability/content identities, with producer artifact/ABI rules retained |
| Physical storage/acquisition | CLI decision pending: project-local versus shared immutable payload; new/install/first-build timing not selected |

The executable prefix and detailed data/launcher/helper policy also remain
[open decisions](design/crt_cli.md#10-decisions-for-the-first-implementation).
No new schema version, backend support or implemented command is assigned by
this plan. [Bootstrap](acceptance/bootstrap_acceptance.md) and
[lifecycle](acceptance/lifecycle_acceptance.md) criteria are future gates; all
runtime/adoption results remain Not run. Original pinned baselines above are
unchanged and this update selects no actual SDK/CLI artifact combination.

## Owner Decisions and Linux Bootstrap (2026-10-28)

The owner selected `crt-cli`, shared immutable SDK payloads with exact project
references, first-configure/build acquisition (source-only new), and one shipped
executable allowing OS dependencies/data/certificates/cache/previous versions/
temporary helpers but no permanent launcher. Storage/acquisition/updater are
still planned mechanisms, not capabilities inferred from this decision.

This is the retained CMake checkpoint, superseded for active development by
the native JSON builder record below.

Local experiment `linux-d0-20261028` is **Pass for native Linux D0 only** and
**not promoted**. Original evidence and artifact identities are owned by
[Linux bootstrap acceptance](acceptance/bootstrap_acceptance.md#linuxx86_64-d0-bootstrap-2026-10-28).
It consumes clean producer SDK `3741ea515f129483efa631c6d2663d27de9ebc62`, Dev
profile at the rehearsal version, with exact archive/content hashes in that
record. CLI execution uses the exported dirty source based on
`cfd27bb5c21f95c056cf4567216c03cd32f7d44f`, snapshot
`b07b2ed65ac4918a6a3ea56747a8ebbe7ef44d74975cc71cb92bace8883a9e7c`.
No clean-source compile or matching public-release version is inferred.

Implemented subset: help/version/doctor --bootstrap, CLI-owned bootstrap SDK
input/result format 1, and host-side experimental manifest/lock format 0.
These are not general SDK development-contract v1, native project resolution
or adopted stable lock formats. `crt.lock` remains unresolved; CMake consumes
an explicitly fingerprinted local SDK handoff. Inspector support is currently
native, known-clean Dev input only, not arbitrary Full/dirty/legacy profiles.

Two CTest suites, actual argument/contract/configure rejection cases,
source/SDK-hidden replay and bare-image import/mapping evidence pass. The
compiled input and program bytes are identified in the original receipts.
Executable authentication and actual CLI HTTPS are not verified; D1-07,
macOS/Windows replay, installer/new/updater and formal promotion/adoption remain
open. Older source-asset filename variants remain a producer promotion gate;
this experiment does not substitute a different asset under a recipe.

## Native Manifest/Driver Experiment (2026-10-28)

**Pass, native Linux/x86_64 only; unpromoted.** The owner superseded the initial
CMake/Python prototype with `crt.json`, internal compiler-driver modules and
C/C++ `src/main`/`src/test`. The native app/control/self-build path invokes LLVM
argv directly and has no Python/shell/CMake/Ninja prerequisite. SDK production
keeps its own existing tools; optional upstream configure/make ports are separate.

The earlier clean SDK exposed the getdents64 type defect. Producer fix
`64b447ba40ef339a65ca009adeda80530a83689c` is used by a fresh clean local Dev SDK
`v0.4.0-cli.20261028`, with separately hashed experimental native provider format 0.
CLI base is `cfd27bb` plus source export
`5c5416312a73e0d7b2f8eec7bd94ab00d4db5e971cea1919a4ac83f6cd9ffe66`.
Exact SDK/archive/provider/binary hashes, owner fix, commands, failed attempts,
429 native assertions, SDK/source-hidden replay and import/maps audit are in
[native bootstrap acceptance](acceptance/bootstrap_acceptance.md#native-json-builder-on-linuxx86_64-2026-10-28).
The hosted seed and native self-build yield identical bytes in this experiment.

Bounded new/configure/build/test/run now exist with explicit local inputs.
Format-0 JSON locks distinguish unresolved from local-experiment; no stable or
promoted root pin is adopted. Automatic acquisition/store installation, self-upgrade,
CLI HTTPS, complete host-tool runtime attestation and macOS/Windows replay remain
open. These are not SDK Build Contract v1 or general D5/D6 completion claims.

## Committed Source Checkpoint (2026-10-29)

The source commits for the native Linux experiment are now explicit:

| Role | Commit / meaning |
| --- | --- |
| CRT runtime and SDK build baseline | `64b447ba40ef339a65ca009adeda80530a83689c`; unchanged clean local Dev SDK |
| CRT exporter/contract documentation | `17de3510c37cdb07e3c5c98583b22ebe4e1463ec`; includes the runtime fix and experimental exporter |
| CLI implementation | `1270695ee8e1ab53b661c4540af859cb2463d840`; native modules, manifest/lock, tests and pinned dependency headers |

The [producer contract](https://github.com/webos21/crt/blob/17de3510c37cdb07e3c5c98583b22ebe4e1463ec/docs/design/sdk_development_contract.md#experimental-native-compiler-provider-2026-10-28)
and [exporter](https://github.com/webos21/crt/blob/17de3510c37cdb07e3c5c98583b22ebe4e1463ec/tools/export_cli_toolchain.py)
are reviewed at that immutable source revision. This supplements the original
`d21430d` design baseline; it does not replace historical provenance.
All 19 non-Markdown inputs in the original receipt match the CLI implementation
commit. The exporter bytes match its recorded SHA-256. Execution still belongs
to the 2026-10-28 exported source/candidate; no later clean-build replay, new SDK
identity or acceptance of a changed artifact is inferred from committing files.

Documentation cleanup follows the implementation commit. It preserves original
receipts, failed attempts and the unresolved repository lock. No new code was
needed in this consolidation; version/bootstrap doctor was checked again using
the byte-identical review executable. Complete macOS/Windows/lifecycle gates
remain open, and repository pushes do not promote or publish SDK/CLI releases.

## macOS Native Candidate Review (2026-10-29)

**Local D0 Pass, unpromoted.** Reviewed clean CRT SDK build baseline
`17de3510c37cdb07e3c5c98583b22ebe4e1463ec`, CLI
`0f14ed3bee6b6512a6969b6118dcbd82c1c564b7` plus its exact exported macOS patch,
and unchanged Studio `27548e1c6d29b5a63d7eb6adce8aaf47b2f5d807`.
The [native macOS record](acceptance/bootstrap_acceptance.md#native-json-builder-on-macosarm64-2026-10-29)
owns SDK/archive/provider/exporter/source and bare-binary hashes, 474 assertions,
actual OS isolation, imports/maps, failed attempts and limits. The older Linux
experiment and accepted input identities remain unchanged.

CRT owns the macOS provider's explicit vendor SDK and Bionic runtime/startup
flags; CLI owns native host/format inspection, POSIX process execution and the
local project binding. Provider format 0 adds a macOS target and host-bound
`external_os_sdk` field; existing SDK manifest/profile format 1 is unchanged.
The SDK's copied bytes are not modified. Any later provider or SDK requires a
new exact identity/pin; no promoted root lock is adopted here. The unresolved
repository request is now `*`, while resolved projects still pin the selected
version/content/tool identities and reject version mismatch.

The bootstrap result remains experimental format 1, with additive allocation-
ownership evidence; this is not general SDK Build Contract v1 or stable D6
automation. SDK-free help/version/doctor is exercised, not complete D1 HTTPS,
installer, package/updater or Studio integration. Windows replay and clean
producer promotion/consumer adoption remain open. No public release or
cross-repository merge/lock update is authorized by this local candidate.

## Contract Inventory

This is the original pinned review inventory. The implemented bootstrap-only
formats and current local result are identified above; they do not assign
versions/support to the full proposed command/job/project contracts below.

Versions below are independent. A proposed first version is not an implemented
schema, compatibility guarantee, or public release.

| Surface | Owner / consumer | Observed format or proposal | Required validation |
| --- | --- | --- | --- |
| SDK `manifest.json` | CRT / CLI, Studio | Existing `format: 1`; not a development-contract version | Exact SDK identity, stage/target/options, inventory and consumer fixture |
| Isolated-stage recipe | CRT / stage builder | Existing `schema_version: 2`; not a port recipe | Predecessor/stage/OS and source/build recipe checks |
| Port recipe and executor | CRT technical policy; CLI adapter / CLI | Existing JSON `schema_version: 1`; adapter/registry version not assigned | Pinned installed driver/recipe/tests/shims and actual zlib static/shared build/run |
| SDK development contract | CRT / CLI, Studio | Proposed v1 sidecar/extension; schema/path not implemented | Schema/negative fixtures plus SDK-only consumer compile/link/run and runtime audit |
| Independent Host Tools Bundle | CRT / CLI | Proposed; independent contract version not assigned | Native tool/alias/rootfs/DLL execution; reject incompatible target tools on the build host |
| CLI command/final JSON/job events | CLI / Studio, automation | Draft; implemented wire versions/capabilities not assigned | Versioned fixtures, actual commands, errors/cancellation and separate output streams |
| Public UI capabilities for Studio | CRT / Studio | Existing public APIs; no separate versioned "Studio UI Profile v1" | Exact SDK capabilities and native GUI/input/IME evidence in Studio |

Source observations are pinned to the reviewed CRT revision:
[manifest producer](https://github.com/webos21/crt/blob/a23a1732f982e7bd60ecc01a6704a9d40cd3adc9/tools/create_dist.py),
[stage schema](https://github.com/webos21/crt/blob/a23a1732f982e7bd60ecc01a6704a9d40cd3adc9/distribution/stage_recipe.schema.json),
[zlib recipe](https://github.com/webos21/crt/blob/a23a1732f982e7bd60ecc01a6704a9d40cd3adc9/porting/recipes/zlib.json).
Existing CRT acceptance does not automatically satisfy these new consumer gates.
When a candidate is selected, record its actual formats rather than assuming
that later artifacts retain these versions.

## Windows Native Candidate Review (2026-10-29)

**Local D0 Pass, unpromoted.** Clean CRT SDK build baseline `d054a665612fc6b0008e5de5bdc40eb85688a4c8` (detached worktree,
`v0.4.0-cli.20261029`, `crt-c-test` 166/166), CLI `265105a51e4a17767419c237e33a73f3936d6a53` plus the exact local Windows
delta (source fingerprint in the
[Windows record](acceptance/bootstrap_acceptance.md#native-json-builder-on-windowsx64-2026-10-29)), Studio unchanged. Provider
format 0 gains a `windows/x86_64` target with `.exe`/`.dll` artifacts and `external_os_sdk` set to the Windows SDK import-library
directory; SDK manifest/profile format 1 is unchanged and the SDK inventory order is now defined component-wise and
case-sensitively. CRT owns the Windows wrappers, startup and the provider; the CLI owns `posix_spawn` execution, the
native-child hint for external tools, per-child runtime `PATH` and the `sdk_consumer_option_sources` project key
(experimental format 0 addition). The changed CLI sources are **not** replayed on Linux/macOS; those records remain valid only
for their original sources. Open producer items (Unicode paths, per-library consumer options, `fread` cost, process groups)
are listed in the Windows record. No promoted combination is adopted.

## Linux Merged-source Revalidation (2026-10-30)

**Pass, local/unpromoted Linux/x86_64**, CLI `ddaf288322e3292fbba53bc2bc4946c701ab5241`.
The Windows-shared process/hash/output/HTTP/cache changes now have fresh Linux
evidence. CRT baseline `a46e4307e2350c3c9de0826bd467bd32cae609c4` needed a
non-Windows `POSIX_SPAWN_SETPGROUP` implementation; the failed clean candidate
and corrected dirty source snapshot are separately identified in
[Linux revalidation](acceptance/bootstrap_acceptance.md#linux-revalidation-after-windows-changes-2026-10-30).
No CLI code changed; the original 15 code/config inputs match the executed export.

The corrected Dev SDK is `development`, explicitly dirty/unpromoted, with
producer patch/content/provider identities and native runtime audits in that
record. The root lock stays unresolved. The existing committed Linux executable
also bootstraps current sources against the new SDK through `.sdk.json`, then
rebuilds itself through the native backend; the full native suite belongs to
the separate passing replay. This does not adopt a production SDK
lock or validate a future clean/release artifact. The changed sources/common
PAL path still need native macOS replay; Windows' prior record remains at its
own source/SDK identities.

The producer fix is committed as
[`74ea992a4ef702c853435cb6319e329647db7cee`](https://github.com/webos21/crt/commit/74ea992a4ef702c853435cb6319e329647db7cee).
The exercised SDK remains the recorded `a46e430` plus dirty patch candidate;
committing/pushing the fix does not relabel it as a clean SDK build or adopt a
production lock. CLI source inputs remain byte-identical to `ddaf288`; the
consumer commit records documentation and the verified Linux test executable.

## Adoption and Exercised Combinations

There is **no promoted production combination yet**. The identified Linux D0
local experiment above passes; the macOS and Windows/x64 local D0 replays pass (Linux replay renewed 2026-10-30; current-source macOS replay remains), complete D1 and Studio S0/S1
remain Not run. Use the owning acceptance ledgers for scope, not a general
claim that the SDK/CLI/Studio product combination is compatible.

Add a combination record only with these fields and links to original evidence:

- Review date, scope and producer/CLI/Studio revisions, including dirty-source
  identity if applicable; separate original design provenance from adoption.
- SDK archive size/digest, source/build/content identity, stage, target and
  options/capabilities; build host, host-tools and external-toolchain identities.
- Adopted development/host-tool/recipe/CLI schema versions, or explicit
  draft/unavailable status; implemented command/capability subset consumed.
- Actual CLI distribution/resource/helper policy and project SDK storage mode;
  keep SDK content profile, build mode and physical backend distinct.
- Original case IDs/evidence for fixtures, consumer build/link, native runtime,
  SDK-free HTTPS bootstrap, and Studio live integration as applicable.
- Outcome per scope/host: **Pass**, **Fail**, **Not run**, **Skipped** with
  reason, or **Compile/package only**. Fixture results never stand in for live
  command/GUI evidence; omitted capabilities are not implicitly approved.
- Owner review, limits, rejected/previous combinations, migration/rejection
  policy and exact lock updates. D7 formal approval requires the applicable
  CLI and Studio integration evidence, not merely a version match.

Early SDK-only UI PoCs and protocol fixtures do not wait for D7. Live project
build/run requires implemented workflow commands; JSON drafts begin in D0 and
applicable job/cancellation/package behavior is stabilized in D6. Studio owns
its detailed feature prerequisites in `docs/design/crt_studio.md`; tranche
numbers or fixture success alone are not capability detection.

## Shared-change Review

The producer owns the common review procedure in
`crt: docs/design/sdk_development_contract.md`, section 8 (Cross-repository
Contract Review), in the [CRT repository](https://github.com/webos21/crt).
Use it for owner/baseline changes, version/capability/error/lifetime changes,
proposed vs implemented commands, TODO-to-acceptance links, and producer-first
artifact promotion followed by consumer lock adoption. Preserve the original
design baseline and previous working combinations; do not rewrite them.

This ledger records adoption through linked repository changes, not atomic
three-repository merges. Review source references at immutable revisions before
claiming a new baseline. An unimplemented interface remains a proposal even if
all three documents describe it consistently.

The checklist is manual now. Lightweight automated reference/owner/version/
capability and TODO-to-acceptance checks are planned, not an existing CI job.
They must not require identical Markdown across owners or rebuild WebKit for
every documentation edit. Remote link availability was not a runtime test.

## macOS Merged-source Revalidation (2026-10-30)

**Pass, local/unpromoted macOS/arm64**, CLI `754ca2992e7b6b2ffa339d965f6085f05f505297`.
The Windows-shared process, hash, output, HTTP, cache and `.sdk.json` changes now
have macOS evidence against a Development SDK built from CRT
[`74ea992`](https://github.com/webos21/crt/commit/74ea992a4ef702c853435cb6319e329647db7cee)
(dirty only by the macOS replay driver; used with explicit dirty-SDK acceptance).
The common `POSIX_SPAWN_SETPGROUP` fix works on macOS. The previously committed
macOS executable bootstraps the current sources, and the result rebuilds them
through `.sdk.json`; the two builds match in code and data segments but not in
`LC_UUID`/`__LINKEDIT`, which ld64 varies per build. Details and hashes:
[macOS revalidation](acceptance/bootstrap_acceptance.md#macos-revalidation-after-windows-and-linux-changes-2026-10-30).
No production lock is adopted and the root lock stays unresolved.

