# D0/D1 Bootstrap Acceptance

This document owns bootstrap acceptance criteria and execution evidence for
the initial CRT-native CLI and host environment. The dependency order and
architecture belong to [the CLI design](../design/crt_cli.md); current work
belongs to [TODO](../../TODO.md).

## Scope and Meaning of Completion

D0 proves SDK-only CLI consumption and explicit identities/contracts.
D1 proves a standalone minimal CLI runtime and the Windows CRT host tools.
An independently distributed Host Tools Bundle is a producer-side target, not
an artifact assumed to exist already. Record unavailable contracts as gaps;
do not substitute producer build trees or Git Bash and call the gate complete.

The criteria below are future acceptance requirements. Writing the contract,
adding a fixture, or passing a compile-only check does not satisfy runtime
acceptance. SDK management and full port/project workflows continue in D2-D6;
this document is not a claim that those later features are implemented.

## D0 - SDK Consumer and Minimal CLI

| ID | Required evidence |
| --- | --- |
| D0-01 Inputs | Exact SDK/archive/content and tool identities; execution host and target recorded independently; no mutable branch/latest identity |
| D0-02 Build boundary | Fresh minimal CRT-native CLI configure/compile/link using installed public SDK wrappers/toolchain, without CRT private headers or source/build-tree inputs |
| D0-03 Basic execution | Actual help/version and implemented basic command/error behavior, with exit results and structured output separated from logs where supported |
| D0-04 Runtime ABI | ELF/Mach-O/PE dependency checks and runtime mapping evidence as applicable; CRT standard runtimes rather than substituted host C/C++ libraries; declared OS boundaries identified |
| D0-05 Contract tests | Initial identity/lock/result fixtures distinguish host/target, unsupported schemas/capabilities and invalid inputs; draft schemas are not reported as an existing SDK feature |

## D1 - Bootstrap and Host Environment

| ID | Required evidence |
| --- | --- |
| D1-01 Standalone CLI | CLI bundle runs without a development SDK/source/build tree; minimal runtime closure and initial Windows DLL resolution verified |
| D1-02 CRT tools | Actual CRT mksh/sh aliases, toybox-supported applets, separate awk and make execute with their declared runtime/bridge/rootfs layout |
| D1-03 Scoped environment | Correct control/port-build/app-build/app-run separation; no global PATH/registry/security changes or silent Git Bash/MSYS fallback |
| D1-04 Paths and collisions | Real spaced/Unicode path execution, preserved symlinks/aliases, colliding tool/DLL names, and immutable input inventories |
| D1-05 Child lifecycle | Bounded configure-like child creation, exit reporting and owned-process cleanup; cancellation claims match actual PAL support |
| D1-06 Failure behavior | Missing tools/Developer Mode, wrong host/target, incomplete/corrupt runtime bundle, and interrupted selection fail clearly and preserve the previous usable installation |
| D1-07 HTTPS bootstrap | Actual metadata verification and artifact download on an SDK-free clean host using only the shipped CRT-native CLI runtime and explicit trust inputs; positive and rejection cases below |

### SDK-free HTTPS Bootstrap Gate

This is a future gate, **Not run**, not an SDK installer already implemented
in D1. A minimal fetch/verify path may be exercised before D2 transactions;
D2 still owns SDK extraction, installation, selection and rollback acceptance.
Agree the HTTP/TLS stack, trust store and metadata-authentication policy in D0.
Do not assume signing exists or treat an archive checksum as publisher identity.

For each initial native host pair, record:

- A clean host (or explicitly controlled equivalent) without a development SDK,
  producer source/build tree, globally installed CRT runtime or injected SDK
  paths. Extract the CLI bundle using OS facilities; record its file/runtime
  inventory and initial loader resolution, especially Windows DLLs.
- A real HTTPS metadata fetch, verification under the declared trust/publisher
  policy, and artifact download with expected size/digest. Identify the URL or
  controlled HTTPS endpoint, metadata bytes, trust inputs and resulting digest;
  redact credentials. The CLI must use its shipped CRT-built download stack,
  not host curl, Git Bash, or a development SDK's DLLs.
- Bounded negative cases: missing/wrong trust inputs, invalid/expired server
  certificate, tampered metadata against the declared verification policy,
  artifact size/digest mismatch, and interrupted download. Fail closed without
  disabling certificate verification or publishing a failed candidate; retain
  any previous usable CLI/selection and do not modify project locks.
- External native tools or interpreters, if actually required, declared by
  identity/role. Downloading metadata must not implicitly require CRT host
  build tools or target SDK libraries. Host-tool/rootfs acceptance is D1-02,
  not proof of HTTPS bootstrap.

Fixtures alone do not close D1-07. Record per-host runtime results and link
the exact combination in the [adoption record](../compatibility.md); absence
of an agreed trust/authentication contract is a prerequisite gap, not a pass.

Test absent prerequisites through controlled fixtures or an appropriate host;
do not toggle Developer Mode or global security settings merely to manufacture
a negative case. Agree on repetition counts/timeouts before long lifecycle
runs, retain the failing case, and do not retry until a flaky gate looks green.

## Evidence Record Format

For every executed case record:

1. Case ID, date, CLI revision or clearly identified dirty source snapshot.
2. Build-host and target tuple, native/cross/runner classification, OS details.
3. SDK, host-tool, external-toolchain, recipe/fixture, and artifact identities.
4. Exact command/argv, relevant sanitized environment, paths, timeouts/repeats.
5. Exit/status, assertion results, log/artifact references, and ABI/runtime
   observations. Record expected and actual behavior for failures.
6. Before/after input inventories and child-process cleanup/restoration where
   relevant; preserve immutable SDK/tool/store content.
7. Limitations, unsupported variants, skipped cases and the linked producer
   issue/commit when a CRT fix is needed.

Use distinct outcomes: **Pass**, **Fail**, **Not run**, **Skipped** (with reason),
or **Compile/package only**. A skipped or cross-compiled case is not a native
pass. Keep failed attempts separate from later accepted artifact identities.
Do not log tokens, private keys, or sensitive environment values. Store large
raw logs/binaries outside tracked documentation and retain usable references.

## Initial Evidence Ledger

No product execution has been performed for this repository at contract
initialization. Only documentation has been created and checked.

| Native host/target | D0 | D1 | Evidence |
| --- | --- | --- | --- |
| Windows/x64 | Not run | Not run | No CLI/build entry point yet |
| Linux/x86_64 | Not run | Not run | No CLI/build entry point yet |
| macOS/arm64 | Not run | Not run | No CLI/build entry point yet |

Update this ledger from actual executions, with detailed case records and
limitations. WSL development checks are labeled as such; they do not silently
replace native-device/GPU/media acceptance. Additional architectures or cross
pairs require their own evidence. Documentation checks belong in HISTORY and
do not turn the table above green.

Closing these bootstrap gates does not authorize a public release. Retain
source/license obligations and SDK/CLI release decisions as separate conditions.
