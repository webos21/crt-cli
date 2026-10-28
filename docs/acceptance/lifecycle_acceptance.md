# CLI Lifecycle Acceptance

This record owns future CLI self-upgrade, project SDK/creation, scoped
diagnostics and distribution gates. Policy belongs to the
[CLI design](../design/crt_cli.md), order to [TODO](../../TODO.md), and initial
SDK-only/single-executable/HTTPS bootstrap to
[D0/D1 acceptance](bootstrap_acceptance.md). These complete lifecycle gates are **Not run**; the bounded Linux D0
implementation and its partial project evidence belong to bootstrap acceptance.
No release lifecycle is accepted here.

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
