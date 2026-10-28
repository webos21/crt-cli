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

The criteria below define acceptance; actual per-host outcomes are in the
ledger and dated records. A contract, fixture or compile-only result does not
satisfy runtime acceptance. SDK management and full workflows continue in D2-D6;
this document is not a claim that those later features are implemented.

## D0 - SDK Consumer and Minimal CLI

| ID | Required evidence |
| --- | --- |
| D0-01 Inputs | Exact SDK/archive/content and tool identities, including local unreleased candidates; producer/CLI snapshots and execution host/target recorded independently; no mutable branch/latest identity |
| D0-02 Build boundary | Fresh minimal CRT-native CLI configure/compile/link using copied/extracted public SDK compiler policy outside producer trees; relocate and validate with CRT source/build trees unavailable |
| D0-03 Basic execution | Actual help/version and implemented basic command/error behavior, with exit results and structured output separated from logs where supported |
| D0-04 Runtime ABI | ELF/Mach-O/PE dependency checks and runtime mapping evidence as applicable; CRT standard runtimes rather than substituted host C/C++ libraries; declared OS boundaries identified |
| D0-05 Contract tests | Initial identity/lock/result fixtures distinguish host/target, unsupported schemas/capabilities and invalid inputs; draft schemas are not reported as an existing SDK feature |
| D0-06 Initial structure | Reviewed initial source/module/build/template contract before implementation; evolving internal layout uses the same public project/SDK build interface |
| D0-07 One-executable feasibility | Minimal CLI and chosen CRT/C++/HTTP/TLS link/import evidence on each initial native pair; sidecar CRT runtime does not satisfy the goal; this is not D1 HTTPS acceptance |
| D0-08 Product decisions | Record command prefix, exact single-file/resource/helper policy, SDK storage default and acquisition timing before implementation relies on them |

### Local Candidate Boundary

Public SDK release is not a prerequisite for D0. Apply the
[local candidate procedure](../guides/development.md#local-dev-sdk-candidate-loop)
to a produced Dev SDK, with these additional evidence requirements:

- Bind the handoff to actual producer source/build provenance, clean/dirty
  snapshot and SDK archive/content identities; record the CLI snapshot and
  host/target/tool identities independently. Repository HEAD is not proof of
  the compiled SDK's identity.
- Verify the ordinary/profile package and preserve its file/symlink inventory
  when copying/extracting. The consumer must not reach private headers, loose
  objects/libraries or generators from the producer source/build tree.
- Record how the producer trees were made unavailable during relocated
  configure/build/run and runtime inspection. Path declarations or output
  audits alone do not prove this isolation or an OS sandbox.
- Keep each failed/rebuilt candidate distinct and check handoff inputs remain
  unchanged. Link actual case outcomes in the
  [candidate/adoption record](../compatibility.md#local-candidates-and-promotion).

A local **Pass** applies only to the identified case/host/candidate. It does
not promote the artifact, establish a production lock or authorize public
release. The dated D0/D1 ledger below owns actual execution outcomes;
changing a fixture or contract alone supplies no native runtime evidence.

## D1 - Bootstrap and Host Environment

| ID | Required evidence |
| --- | --- |
| D1-01 Standalone CLI | One CLI executable runs without Dev SDK/source/build trees or sidecar non-OS CRT runtime; declared OS imports and agreed resource/helper policy verified, including Windows initial loading |
| D1-02 CRT tools | Actual CRT mksh/sh aliases, toybox-supported applets, separate awk and make execute with their declared runtime/bridge/rootfs layout |
| D1-03 Scoped environment | Correct control/port-build/app-build/app-run separation; no global PATH/registry/security changes or silent Git Bash/MSYS fallback |
| D1-04 Paths and collisions | Real spaced/Unicode path execution, preserved symlinks/aliases, colliding tool/DLL names, and immutable input inventories |
| D1-05 Child lifecycle | Bounded configure-like child creation, exit reporting and owned-process cleanup; cancellation claims match actual PAL support |
| D1-06 Failure behavior | Scope-required missing tools/Developer Mode, wrong host/target, corrupt CLI/runtime resources and interrupted selection fail clearly and preserve the previous usable installation |
| D1-07 HTTPS bootstrap | Actual metadata verification and artifact download on an SDK-free clean host using only the shipped CRT-native CLI runtime and explicit trust inputs; positive and rejection cases below |
| D1-08 Self doctor | SDK-free self/version/architecture/bootstrap checks; trusted metadata/integrity and unavailable evidence distinguished; missing unrelated project tools do not fail self scope |
| D1-09 Bootstrap resources | Trust/notices/data and helper/launcher handling comply with the agreed one-file criterion; no undeclared SDK, CRT DLL or interpreter fallback |

The single-executable target requires per-host evidence; Linux bootstrap proof
below does not establish macOS/Windows or actual HTTPS readiness.
Keep an unsupported target or changed distribution proposal explicit rather
than closing this gate with a dynamic bundle. Host tools in D1-02 are separate
development assets. Initial source layout and command/schema choices remain
provisional until reviewed; the case IDs do not assign implementation versions.
Follow-on self-upgrade/project/package gates belong to
[lifecycle acceptance](lifecycle_acceptance.md).

### SDK-free HTTPS Bootstrap Gate

This is a future gate, **Not run**, not an SDK installer already implemented
in D1. A minimal fetch/verify path may be exercised before D2 transactions;
D2 still owns SDK extraction, installation, selection and rollback acceptance.
Agree the HTTP/TLS stack, trust store and metadata-authentication policy in D0.
Do not assume signing exists or treat an archive checksum as publisher identity.

For each initial native host pair, record:

- A clean host (or explicitly controlled equivalent) without a development SDK,
  producer source/build tree, globally installed CRT runtime or injected SDK
  paths. Place the single CLI executable using OS facilities and record any
  data/helper behavior permitted by the agreed policy, file/runtime inventory
  and initial loader resolution, especially Windows DLLs.
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

At contract initialization no product execution had been performed. The
dated Linux record below supersedes that initial state;
it does not grant other-host replay or complete D1.

| Native host/target | D0 | D1 | Evidence |
| --- | --- | --- | --- |
| Windows/x64 | Not run | Not run | Native bootstrap/loader replay pending |
| Linux/x86_64 | Pass, local candidate | Not run as a complete stage | D0-01..08 and bounded SDK-free bootstrap below; actual HTTPS/host-tool gates remain |
| macOS/arm64 | Not run | Not run | Native bootstrap/loader replay pending |

Update this ledger from actual executions, with detailed case records and
limitations. WSL development checks are labeled as such; they do not silently
replace native-device/GPU/media acceptance. Additional architectures or cross
pairs require their own evidence. Documentation checks belong in HISTORY and
do not turn the table above green.

Closing these bootstrap gates does not authorize a public release. Retain
source/license obligations and SDK/CLI release decisions as separate conditions.

## Linux/x86_64 D0 Bootstrap (2026-10-28)

**Historical CMake checkpoint.** The owner subsequently selected native JSON
builds without Python/shell/CMake. This source export and its receipts remain
unchanged; the later native-builder record owns the active architecture.

**Accepted locally:** minimal native `crt-cli` through a relocated Dev SDK,
not public release or final lock adoption. The original CLI base is
`cfd27bb5c21f95c056cf4567216c03cd32f7d44f` plus the exact exported dirty source
snapshot `b07b2ed65ac4918a6a3ea56747a8ebbe7ef44d74975cc71cb92bace8883a9e7c`.
`source-identity.json` retains every exported file hash; later documentation
does not relabel this as a clean compile. Source/contract and entry points are
described in the [implemented contract](../reference/bootstrap_contract.md)
and [build guide](../guides/development.md#build-and-test-the-bootstrap).

SDK producer: clean CRT `3741ea515f129483efa631c6d2663d27de9ebc62`, version
`v0.4.0-rehearsal.20261027`, Linux/x86_64 Dev packaging profile. Its archive is
8,939,540 bytes, SHA-256
`beb0783073547cca309b8880542e2332ac46e43a5afa08c95b3dbe3cbf891b24`.
The producer verifier passes before handoff; all **2,579** file/symlink entries
match the original SDK. CLI inspection content SHA-256:
`81b98ae503c5a6d23d1638adcc04821403c490e8fa8318cc557564a500900ef6`.
Existing manifest/profile format 1 is not a general SDK contract version.

Replay root: `/home/syszone/GitLocal/crt-cli-replay/tranche1-linux-20261028-final 한글`.
Use the guide's actual replay command with a new, disjoint output directory.
The driver exports only CLI source, copies the verified SDK into a path with
spaces/Korean text, and configures/builds/tests under Linux bubblewrap with
CRT main, frozen producer worktree, original CLI and initial SDK roots hidden
by empty namespace mounts. Hidden-root probes verify this, not merely PATH
filtering. The single binary then runs with source/build/relocated SDK hidden too.

- Both CTest suites pass: executable help/version/doctor and **14** argument
  rejections; **nine** host-side SDK/project contract unit tests, with rejection
  subcases. C++ exceptions, zlib round-trip and curl/Mbed TLS linkage/lifecycle pass.
- **Seven real configure negatives** reject their specific cause: tampered
  toolchain, unknown profile format, stale provenance, wrong target, missing
  static curl, no SDK toolchain and missing input pin. The tampered toolchain's
  sentinel never executes: content inspection precedes toolchain loading.
- ELF is native x86_64 with **zero DT_NEEDED and zero RPATH/RUNPATH**. The only
  interpreter is the SDK-declared OS boundary `/lib64/ld-linux-x86-64.so.2`.
  Actual `/proc/self/maps` contains only the bare CLI and that OS loader.
- Bare executable: **2,959,296 bytes**, SHA-256
  `ef44ac89305a4975d5a9dfcf57f9d25095af78c1834810063b299931794dc550`.
  SDK inventories remain unchanged after testing. Shared SDK installation and
  automatic acquisition are not claimed by the manual experimental handoff.

Receipts: `result.json`, `source-identity.json`, `commands.json`,
`hidden-roots.log`, configure/build/CTest logs, ELF logs, bare-binary/doctor logs
and seven negative logs. The handoff directory separately retains archive,
inventory and producer-verifier receipts. Raw logs/binaries remain outside Git.
`post-run-review.json` records host/tool identities and verifies that all ten
non-Markdown source/config/test files still match the executed snapshot.
Original `result.json` SHA-256:
`0b67371895e2b9111155a92e37355935db4dd71a438ac50aa442b290ed36a97c`.

Initial attempts are preserved: the public SDK defaults disable C++ exceptions;
the consumer explicitly enabled SDK-owned exception/RTTI/hosted controls.
An argument-test harness first misclassified duplicate JSON flags as human
output; a replay first lacked the versioned LLVM archiver selection, and CMake
line wrapping required whitespace normalization of the expected diagnostic.
These are consumer/harness corrections, not relaxed native/SDK gates. The
Unicode replay also preserves valid UTF-8 runtime-image paths in JSON.

**Limits:** executable integrity is `not_verified`; actual CLI HTTPS is
`not_run`. D1-07, full host-tool lifecycle, installer/new/updater, trusted
metadata, macOS/Windows native replay and promotion remain open. The root
format-0 lock is unresolved; no promoted SDK/CLI combination is adopted.

## Native JSON Builder on Linux/x86_64 (2026-10-28)

**Pass, local/unpromoted:** the owner's replacement for the CMake prototype.
The CLI and generated projects use `crt.json`, native C++ modules under
`src/main/`, and C/C++ `src/test/` tests. The
[implemented contract](../reference/bootstrap_contract.md) owns schemas and scope.
Automatic SDK installation, general dependencies, self-upgrade, complete D1
HTTPS and native macOS/Windows replay remain open.

### Inputs and Producer Fix

The older Dev SDK exposed a Linux PAL defect during native filesystem inventory:
`readdir` read getdents64 record-tail padding as `d_type`. Native enumeration
returned 2,562 entries against the actual 2,579 and misclassified a directory.
`out/native-inventory.json` and the failed native SDK-selection receipt are
retained in the CLI checkout; this was not bypassed in the scanner.
CRT fix `64b447ba40ef339a65ca009adeda80530a83689c` reads byte 18, following
Bionic's getdents64 behavior. The strengthened `file_path_test` fails with the
older immutable SDK (`file_path_test: readdir type`) and passes after the fix.
No public ABI/header change or upstream patch was required.

A clean detached producer worktree at that exact commit rebuilt core, imported
libc++/libc++abi/libunwind, zlib, Mbed TLS and curl; only source caches were
copied. The temporary clean producer worktree was archived after validation;
its source commit/snapshot can be restored through the chat attachment. All
needed outputs remain outside the worktree. Producer evidence root:
`/home/syszone/GitLocal/crt-cli-producer-20261028`.
`configure.log`, `build-r3.log`, `core-test.log`, `source-caches.json`,
`build/development-consumer/result.json` and CMake cache preserve prerequisites
and commands. The C suite passes **144/144**; the Dev consumer passes static/shared
HTTPS+inflate and trust-negative cases. These producer HTTPS cases do not grant
CLI D1 HTTPS acceptance.

The external build-directory fallback initially selected the wrong make source
path. Failed `build.log`, `build-r2.log` and `failed-alias-out-build` remain;
an ignored `out/build` symlink in the clean worktree points to the external
build directory for the existing preset adapter. No source/runtime input was
patched or relabeled to repair this host path selection.

New immutable handoff:
`/home/syszone/GitLocal/crt-sdk-handoff/dev-linux-x86_64-cli-64b447b/dev`.
Version `v0.4.0-cli.20261028` is local only: no Git tag or public release.
Producer/base/compile commits all equal `64b447b`, with clean flags.

| Input | Identity |
| --- | --- |
| Local Dev archive | `crt-v0.4.0-cli.20261028-linux-x86_64-dev.tar.xz`, 8,945,884 bytes |
| Archive SHA-256 | `1ddbed4ac37ae3f43ea7bbaa59eb057a3b62e0a2ef42a959529c3a72d0e98532` |
| Native inventory SHA-256 | `6d34a450bc95ea238108b90acbb863031a1112e914ad90065864206975bf8115`, 2,579 entries |
| Separate provider SHA-256 | `5e1d3678dca0b4110ff40d145efc45a9c2abd3402696c626149d82bf844e2870` |
| CLI source base | `cfd27bb5c21f95c056cf4567216c03cd32f7d44f` plus exported implementation |
| CLI source-export fingerprint | `5c5416312a73e0d7b2f8eec7bd94ab00d4db5e971cea1919a4ac83f6cd9ffe66` |

The provider is experimental `crt.experimental.native-toolchain`, format 0,
exported separately from installed wrapper argv. It records the exporter hash
and full SDK inventory; it is not installed into or substituted under an older
SDK identity. Host Clang/LLD 21.1.8 and compiler-resource hashes are in the lock
selection/receipts. Driver-file hashes are not an authenticated tool publisher
or complete OS/shared-host-tool runtime attestation.

### Isolated Native Replay

Original receipts:
`/home/syszone/GitLocal/crt-cli-replay/tranche1-native-linux-20261028-r2`.
The source export, SDK copy (`SDK space 한글/dev`), standalone review binary,
`commands.json`, all stdout/stderr and execve logs remain there. The external
`orchestrate.py` is acceptance orchestration outside the product namespace;
Python is unavailable inside every tested consumer build/test/runtime invocation.
No Python test runner, CMake configuration, Ninja or SDK wrapper is a product
build/test dependency.

Bubblewrap makes **seven** producer/original source/SDK directories empty and
masks **seven** installed shell/Python/CMake/CTest/Ninja/make executable paths.
Native `hidden-roots-probe` verifies those actual boundaries. The bare probe
also hides the relocated source/SDK and provider: nine empty directories and
eight unavailable files. Network is isolated. `PATH=/nonexistent`; compiler
processes receive a private environment, without inherited loader/compiler
injection variables. Exact namespace/tool argv is recorded, not inferred from
output imports alone.

The hosted C++ seed compiles directly through native LLVM, configures the
exported project and builds a CRT-native CLI. That CLI then rebuilds itself and
runs its native C++ integration suite through the same manifest backend:

- **429 native assertions pass**, including real source-only C/C++ generation,
  separate main/support/test files, configure/build/test/run, and deliberate
  C/C++ test exit 7 with structured failure retained.
- Rejections cover unknown manifest/provider formats, foreign target, stale
  provenance, bad provider pin, incomplete SDK, script compiler, unsupported
  profile, missing selection, traversal, existing destination, bad lock and
  actual command/argument errors. Duplicate JSON keys fail. A rejected build
  leaves its project lock unchanged.
- An actual child timeout reports exit 124. A 10 MiB stdout producer completes
  without pipe deadlock; capture is capped at 8 MiB and reports truncation.
  The integration target explicitly selects a bounded 600-second timeout;
  generated tests default to 180 seconds.
- Space/Korean/`@`/shell-metacharacter paths remain literal. All **136 execve**
  calls use native executables; shell/Python/CMake/CTest/Ninja/make and
  `crt-cc`/`crt-c++` wrapper calls are **zero**.

The first native export attempt is retained at
`tranche1-native-linux-20261028`: its instrumented integration suite exceeded
the then-fixed 180-second child limit and returned 124, not a pass. The final
source adds explicit bounded per-test timeouts. Final strace uses its kernel
syscall filter to avoid tracing unrelated syscalls. An initial visibility probe
expected readable empty files; masking also makes a file unreadable, so the
corrected probe verifies that such a file is non-executable. The earlier probe
failure/review stays in that original root. These are separate attempt identities.

### Binary and Runtime Audit

| Observation | Result |
| --- | --- |
| Native CLI bytes | 10,224,592 |
| SHA-256 | `02109290e911dd860e7ea6e492cce5f6b004f1e03e3791a386c57a6476c55954` |
| Seed-built vs native self-built | Byte-identical in this experiment |
| ELF DT_NEEDED | 0 |
| RPATH/RUNPATH | 0 |
| Interpreter | `/lib64/ld-linux-x86-64.so.2`, declared SDK OS boundary |
| Actual mapped file images | Bare CLI and `/usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2` only |
| SDK/source-free doctor | C++ exceptions, zlib, HTTP/Mbed TLS initialization pass |
| CLI HTTPS / authenticated integrity | `not_run` / `not_verified` |

`result.json` SHA-256:
`55d456f54889438cc6ad6eb58201645a4e8f9f1e913063d109baa1110b6f5a52`.
`post-run-review.json` rechecks **19** current non-Markdown source/config/import
files against the executed export. Configure intentionally resolves only the
exported project's lock; the repository lock stays unresolved. Original handoff,
producer SDK, relocated SDK and provider bytes/symlinks remain unchanged.

This validates the bounded native Linux combination only. Complete tool runtime
attestation, arbitrary/cross/other-profile providers, general recipe execution,
installer/GC, locked/offline, interactive/three-host process-tree cancellation,
packaging/self-upgrade and full SDK contract v1 remain unimplemented or unverified.
No production lock, SDK/CLI release or other-host support is promoted.

### Committed Source Reconciliation (2026-10-29)

CLI implementation commit `1270695ee8e1ab53b661c4540af859cb2463d840` contains the same
19 non-Markdown source/config/import inputs identified by the accepted export.
CRT exporter/contract commit `17de3510c37cdb07e3c5c98583b22ebe4e1463ec` preserves exporter
SHA-256 `bd4a9752365c6609e7102378198028db11e207bfd96e2f8dc2a4e4e518670502`.
The SDK build baseline remains `64b447b`, not the later documentation commit.
[Compatibility](../compatibility.md#committed-source-checkpoint-2026-10-29)
owns this cross-repository source mapping.

The original `result.json` and its SHA-256 are unchanged. Consolidation rechecked
source/dependency/exporter/binary hashes, local documentation links, version
and bootstrap doctor. The 429-assertion/144-core-test results remain dated
2026-10-28; no repeated full build or new native-host pass is claimed.
