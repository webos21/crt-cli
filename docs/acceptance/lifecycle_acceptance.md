# CLI Lifecycle Acceptance

This record owns complete CLI self-upgrade, project SDK/creation, scoped
diagnostics and distribution gates. Policy belongs to the
[CLI design](../design/crt_cli.md), order to [TODO](../../TODO.md), and initial
SDK-only/single-executable/HTTPS bootstrap to
[D0/D1 acceptance](bootstrap_acceptance.md). These complete lifecycle gates are **Not run**; the bounded Linux D0
implementation and its partial project evidence belong to bootstrap acceptance.
No release lifecycle is accepted here.

Local installation and explicit project SDK/contract selection now have
[five-pair D2 evidence](sdk_install_acceptance.md). Automatic first-build binding,
acquisition and the full lifecycle criteria remain separate. Under the
[adopted priorities](../design/crt_cli.md#execution-priorities-adopted-2026-11-05),
self-upgrade/rollback remains planned beyond the first preview and does not
block D3. This changes scheduling, not the acceptance requirements below.

Bounded self/environment/project diagnosis and explicit local resource-file
pin/signature checks now have [Linux D1 evidence](host_environment_acceptance.md).
They do not implement the full installation/port readiness, self-upgrade or
release lifecycle criteria below.

## Prerequisite Decisions

The owner choices in [product decisions](../design/crt_cli.md#10-decisions-for-the-first-implementation)
are recorded: crt-cli, shared immutable references, first-build acquisition,
and one executable with declared OS/data/temporary-helper allowances and no
permanent launcher. Exact trust/provisioning/activation mechanisms still need
implementation and evidence. Actual CLI/SDK formats must be identified; record
remaining prerequisite gaps instead of inferring installer support or a pass.

## D2 - CLI Self-Upgrade

| ID | Required evidence |
| --- | --- |
| UP-01 Release identity | Intended version/host, metadata trust/authentication policy, size/digest and supported schema verified before candidate execution; reject tampered or wrong-host inputs |
| UP-02 Staging failures | Interrupted/corrupt downloads and insufficient staging prerequisites fail without replacing the prior usable CLI or project locks |
| UP-03 Candidate health | Bounded version/self-doctor checks on the staged candidate with the agreed SDK-free runtime/resource boundary; retain rejected identities |
| UP-04 OS activation | Native Windows/x64, Linux/x86_64 and macOS/arm64 running-file/restart/activation behavior follows the single-file and helper policy; no assumed in-use overwrite |
| UP-05 Recovery | Failure before/after activation and interruption preserve or restore the prior usable selection; do not mask the failed attempt with retries |
| UP-06 Concurrency/lifetime | Concurrent upgrade and active-job cases respect locks/references; cleanup is bounded and does not delete an in-use version |
| UP-07 Project preservation | CLI upgrade leaves project sources, templates, SDK/port pins and locks unchanged; SDK/dependency updates are separate explicit operations |

The proposed `upgrade` command updates the CLI itself. Its exact syntax,
activation/helper and rollback implementation remain design work. Hash matching
alone is not publisher authentication, and candidate execution follows trust
validation. Test OS differences with native evidence rather than changing
global security settings to make a gate pass.

## D2/D5 - Project SDK and Creation

| ID | Required evidence |
| --- | --- |
| PR-01 Independent selection | Two projects use different SDK versions/content profiles concurrently without changing one another; host/target, capabilities and exact identities checked |
| PR-02 Physical storage | Chosen project-local/shared-immutable backend preserves the same pinned selection contract, inventories and input immutability; cache misses do not select a different SDK |
| PR-03 Locked/offline | Locked inputs may be fetched/materialized exactly; offline uses only available verified inputs and fails clearly when missing; no implicit lock migration |
| PR-04 Generated project | Versioned template creates manifest/basic sources/public build entry point without overwriting existing projects; generated data is separate from tracked source |
| PR-05 Acquisition boundary | The chosen new/install/first-build acquisition policy declares network/change scope and failure recovery; missing/corrupt SDKs do not fall back to producer trees or host libraries |
| PR-06 ABI and modes | One validated SDK combination per build target; ports bind SDK/target/tool identities; SDK content profiles remain separate from Debug/Release build modes |
| PR-07 Self-build/template parity | Once new/build/run/test exist, the CLI's own project and a fresh generated project use the same public SDK/build contract; internal module evolution has no private exemption |

Shared immutable payloads with exact project references are the selected
default. Handoff/export copies do not establish a second project-local
installer. Paths such as `.crt/sdk` remain illustrative, not implemented schemas.
Begin with validated cumulative SDK stage/Dev artifacts;
arbitrary component assembly is a separate producer contract.

## D6 - Distribution and Diagnostics

| ID | Required evidence |
| --- | --- |
| PK-01 Shipping executable | Actual CLI artifact meets the agreed one-executable/data/OS-dependency criterion on a host without Dev SDK/build/store fallbacks; native imports/mappings and HTTPS bootstrap linked |
| PK-02 Doctor scopes | Self, environment and project results distinguish missing/unsupported/unavailable inputs; self works without an SDK, project checks exact lock/SDK/port/tool readiness, and defaults do not repair/install/change security or locks |
| PK-03 Shipping upgrade | Repeat the applicable UP cases using real release candidates and prior versions, including bad metadata/artifacts, interruption and failed candidate health |
| PK-04 Provenance/adoption | CLI consumes the exact promoted SDK artifact after replay; source/notices obligations and per-host evidence preserved; versions/publication times need not match |

The CLI one-file goal does not require every generated application to be one
file. Applications package their actual runtime/worker/resource closure and
declared OS prerequisites under their SDK contracts.

## Evidence and Current Status

Use the [bootstrap evidence format](bootstrap_acceptance.md#evidence-record-format)
for exact source/artifact/host/tool identities, commands, inventories, failures
and outcomes. Link actual experiments/adoption in
[compatibility](../compatibility.md). Preserve original failed and known-good
identities; new code or a changed merge artifact requires new evidence.

All complete UP/PR/PK cases are **Not run**. The
[native Linux D0 experiment](bootstrap_acceptance.md#native-json-builder-on-linuxx86_64-2026-10-28)
exercises source-only C/C++ generation, existing-destination rejection and
self-build/template parity. Those results supply partial PR-04/PR-07 evidence;
they do not close template-versioning, installer, multi-SDK or three-host gates.
No production combination or public release is approved here.
Fixture/compile-only/skipped cases cannot close native lifecycle gates.

## P1-02 First-build diagnosis on Linux/x86_64 (2026-11-25)

The [first-build guide](../guides/development.md#diagnose-a-first-build)
reuses the existing `new`, pinned SDK acquisition and `build` workflow. Project
doctor checks `.sdk.json` and installed inputs ahead of the first build, and
uses the exact existing binding on an already configured project.

The isolated replay used a CLI built from a source export based on
`3733aeb108bcad5ba45f9feec234330d5c4ddbc4` plus the P1-02 changes. The final
consumer SHA-256 is `21593a77c6f0f321d9befa86e22cdea0052b2dc134c3ee3b616f014d6d9c80da`.
The native tools are Ubuntu LLVM 21.1.8 with explicitly declared absolute paths.
The public SDK is `v0.0.1-preview.20261125.linux.x64.1`, content
`d2edb36c64f8f7c6d67603763e07a83d0cb4c2858c99e7957e00dde2d54dc921`, policy
`54bb35e014c59af90c1e2b73c3f544ebc7815edc958e704e15bef097dbedd2d1`.
`sdk acquire --source` reports `signature_verified` with the independently
pinned public producer key and manifest from the Linux host CI input set.

The replay passed **28 CLI calls**. Bubblewrap used a private empty home,
read-only host filesystem outside the writable replay directory, a cleared
environment, empty `PATH` and hidden source checkouts and bootstrap workspace.
Only the explicit public SDK acquisition ran with networking. The two source-only
projects used a workspace path containing spaces and Hangul. Before SDK
installation, doctor explained missing LLVM, no installed CRT SDK and the
need to act explicitly when `.sdk.json` requested HTTPS. After installation,
doctor refused a missing `clang++` path and accepted a linker path after native
normalization preserved its driver name.
The unresolved lock remained byte-identical and diagnosis created neither
binding nor build outputs.

Both generated C and C++ projects passed build, test and run. The first build
recorded the exact SDK selection in each lock. Bound doctor and the workflow
ignored a changed `.sdk.json`. Doctor and build refused a missing locked tool,
preserved the lock and local binding bytes, and resumed zero-execution warm
builds when the exact binding was restored.

Test-only copies of the portable policy exercised the missing and invalid
OS SDK paths for macOS and Windows on Linux. The public policy remained intact.
The native suite also includes a host-conditional invalid OS SDK directory
case for the actual vendor SDK on future macOS and Windows runs. Those hosts
did not run this revision here. The P1-02 result covers the bounded first-user
workflow and leaves the full PR/PK lifecycle gates and editor-host runs open.

The initial fresh Linux two-stage bootstrap passed native **981**, editor
**389**, archive **221**, SDK store **114**, store locks **9** and recipe **147**.
After aligning tool validation with the exact paths the workflow passes to
LLVM, the CLI rebuilt and the final consumer passed all 389 editor checks and
the isolated workflow above. The build records are in `native/artifact/` and
`final-tool-validation/` under the same external work root.

The replay helper and initial harness assertion failure are retained under
ignored `out/p1-02-20261125/`. Successful raw envelopes, local config inputs,
public SDK source pins and `result.json` are under
`/home/syszone/crt-cli-p1-02-20261125/first-user-r3/`.

## D6-01 cancellation, recovery and diagnostics on macOS/arm64 (2026-11-26)

Host: macOS 27.2.1 (26B131), arm64, Xcode 27.2. Inputs: CRT SDK `v0.5.0-rc.2` (content
`9e35624a8bf568e86b076efba23d60b63057d1ac0fd6561972e4e4131ffb507b`, CRT `ce1808cd`) and the CLI built from
`8942a35` by RC1 (`4e4b0ac317de4e9d4bd64e0017697493049fafd7562fa092a33b3ea0d9306a24`), then the corrected build below.

### Defect: canceling the CLI left its children running

Every child runs in its own process group and the CLI handled no signal. With the RC1-built CLI:

- `run`: SIGINT or SIGHUP to the CLI's process group, and SIGTERM or SIGKILL to the CLI, end the CLI while the user's
  program keeps running.
- `build`: after Ctrl-C-style SIGINT to the group, the Clang driver and `-cc1` stay alive (3 of 3 first builds, 1 of 1
  bound rebuild). When the blocked compiler was released after the CLI had exited, it wrote the object and
  dependency file.

Lock and local binding bytes were preserved in every case and the next build recovered. A first measurement that
reported no first-build orphan was a probe error: it matched the CLI's own argument list before any compile started.

### Correction and regression

`src/main/process.cpp` follows the [cancellation contract](../reference/bootstrap_contract.md): forward SIGINT, SIGTERM,
SIGHUP or SIGQUIT to each owned group, SIGKILL after the grace period, then exit 128 + signal. The native `process`
phase gains four cancellations (a descendant tree with SIGINT, SIGTERM and SIGHUP, and a child that ignores them).
With only the test added, the contract replay fails (`cancellation stops owned descendants before delayed writes`).
A first correction that raised the signal again also failed: CRT's `raise()` and `kill()` of the calling process only
emulate the default action (measured with the RC2 SDK: SIGHUP and SIGQUIT return, SIGINT and SIGTERM exit 134). That
is a CRT libc deviation tracked outside this change.

`replay_cli_contract_macos.py` with the corrected source and RC2 passes: self-build 22 executed / 0 reused, warm 0 / 22,
bootstrap loop code and data identical, native **1040**, archive 221, SDK store 114, port recipe 147, store lock 9 and
editor 425 checks, only `libSystem` imported. Executable
`9d0eae29478b2075140c898497456a74b42fc5e8f90c7e68099813c0d52450f0` (4,542,824 bytes). With that executable the same
probes give: `run` canceled by SIGINT, SIGTERM and SIGHUP exits 130, 143 and 129 with the program stopped (SIGKILL
still leaves it, as documented); canceled first builds (3 of 3) and a canceled rebuild leave no process and write
nothing; binding bytes are preserved and the next build succeeds. A real zlib `port build` canceled during configure
and twice during make (up to 23 descendant processes) exits 130, 143 and 129, leaves no process, publishes nothing and
succeeds when rerun. Cancellations that arrive while the CLI owns no child keep the host's default action.

### Other recovery cases and diagnostics

- Compile and link failures keep the lock and binding, project Doctor passes afterwards and the restored source builds
  (RC1-built CLI).
- `sdk install` interrupted during extraction by SIGINT, SIGTERM and SIGKILL publishes nothing; a rerun removes the
  staging directory, installs and passes `sdk verify` (RC1-built CLI).
- `replay_sdk_selection_macos.py` passes first-build **86/86** and selection **66/66** with the RC1-built and with the
  corrected CLI.
- macOS OS SDK diagnostics pass for `build` and project Doctor (8 cases): a directory without
  `usr/lib/libSystem.tbd`, a missing directory, a relative `os_sdk`, and `xcrun` denied; each names the corrective
  input and writes nothing. Project Doctor with `sdk_source`, network denied, does not acquire and names the two
  explicit commands (RC1-built CLI).
- With the corrected CLI, the RC2 handoff replay passes **54/54** and `replay_port_macos.py` **28/28** (six port suites,
  447 checks). The RC1-built CLI also passes 28/28 with RC2.

### Limits

macOS/arm64 only. Linux has the same code path and runs the new native cases in CI but its interactive probes were not
repeated. Windows is **not** corrected or verified: the CRT PAL does not deliver process-directed signals or Ctrl-C
there, and the new native cases are skipped. No interactive terminal or editor task cancellation was driven; the
probes send the signals those paths use. The VS Code task terminate action was not exercised. The C provider replay was
not repeated with the corrected CLI. Evidence is retained outside the checkouts in
`crt-release-050-macos-arm64-20261126/rc2-followup/` (`contract-red/`, `contract-green/`, `cancel*/`, `diag/`,
`first-build*/`, `selection*/`, `handoff-fixed/`, `d3*/`).
