# Project Status

Evidence-based snapshot refreshed **2026-11-05**, explicitly requested by the
owner. Reviewed CLI `3978cedbefa2abda7c4cd0c11ca1649912d7943a` and CRT
`433a7d65b34ee25d71eb9d7c96976c817f5936cd` before this documentation update.
This refresh runs no build/replay and adopts no new runtime combination.
Update this file only on explicit request; [TODO](TODO.md) owns current work,
[HISTORY](HISTORY.md) completed changes and acceptance the original evidence.

## Implemented Scope

| Surface | Current implementation and evidence boundary |
| --- | --- |
| Native project builder | C/C++ crt.json -> direct LLVM argv; source-only new, explicit configure/build/test/run, executables and static/shared libraries; [reference](docs/reference/bootstrap_contract.md) |
| Standalone CLI bootstrap | Native Linux x86_64/aarch64, macOS/arm64 and Windows x64/arm64 records with per-host source/SDK/import identities; [bootstrap acceptance](docs/acceptance/bootstrap_acceptance.md) |
| Host environment/diagnosis | Scoped doctor, environment reports, independently pinned tools and non-interactive shell; D1 Linux/macOS/Windows-x64 scope and remaining private-import limits are [recorded separately](docs/acceptance/host_environment_acceptance.md) |
| Local SDK installation | tar.xz/zip, pinned release metadata, safe extraction, immutable shared store, transactions, verify/recover and interruption/concurrency cases on five native pairs; [installation evidence](docs/acceptance/sdk_install_acceptance.md) |
| Installed SDK/project selection | Stored portable contract, exact identity selection and lock format 1, local LLVM/OS SDK paths; Stage 2 has records on all five native pairs, including [Windows/x64](docs/acceptance/sdk_install_acceptance.md#toolchain-contract-and-project-sdk-selection-replay-stage-2-windowsx64-2026-11-05) |
| Explicit HTTPS fetch | Caller-selected CA, size/SHA-256 and optional RSA-PSS public-key/signature checks; independent bootstrap tests, not automatic SDK acquisition |

These are exercised local technical baselines, not a five-pair replay of the
latest HEAD or universal release support. Exact artifacts belong to acceptance
and [compatibility](docs/compatibility.md); the [committed test binaries](bin/README.md)
retain their own source snapshots.

## Current Limits

- Installation/automatic selection currently accepts native `dev` at stage
  `02-cxx`; generic `cxx`, upper profiles and Full `06-web` installation are not
  implemented. CRT Dev already contains zlib and other CLI build dependencies.
- New projects still need explicit configure and external LLVM selection.
  First-build automatic binding and missing-SDK download are unimplemented.
  External LLVM/required OS SDKs remain user prerequisites.
- The latest `sdk verify` change checks a present stored contract's shape,
  portability and SDK identity on Linux/x86_64 at developer/unit level only;
  no new isolated replay or binary refresh was done for it. The store does
  not retain the installation-time contract digest/required-presence record;
  an existing project lock detects changed bytes, but a fresh configure can
  adopt changed valid bytes. [Exact scope](docs/acceptance/sdk_install_acceptance.md#sdk-verify-covers-the-stored-contract-linuxx86_64-2026-11-05).
- Installed-SDK host path/tool/DLL collision qualification remains open.
  Stage 2 project selection itself is implemented; earlier TODO dependencies
  saying it had not started were stale.
- Source-port execution/store consumption, dependency resolution, general
  updates, CLI self-upgrade/rollback, GC and Studio integration remain planned.
  D3 must distinguish actual isolated source-built zlib from the SDK-provided one.
- Official default-download publisher trust, release metadata and public
  SDK/CLI delivery are pending. Rehearsal archives and explicit test-key
  signatures are not promoted releases. Root project locks are not changed
  by this documentation refresh.

## Current Direction

The owner adopted [capability-based execution and change-impact gates](docs/design/crt_cli.md#execution-priorities-adopted-2026-11-05):
close the bounded local-store gaps, then installed-SDK first build, then pinned
remote acquisition and one Linux preview candidate. D3 zlib may proceed in
parallel using fixed producer/consumer contracts. Full metadata, general
updates and self-upgrade are outside the first flow's critical path.

Existing five-pair evidence is retained. New workflow support extends to
macOS/arm64 and Windows/x64 through native replay before being advertised;
other architectures retain their original scope. Archive/transaction/trust/
PAL/ABI changes still require relevant failure/recovery/native checks before
merge. No nightly automation or new feature execution is claimed here.
