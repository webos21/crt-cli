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
| macOS/arm64 | Pass, local candidate | Bounded SDK-free execution only; complete D1 remains open | Native JSON builder replay below |

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

## Native JSON Builder on macOS/arm64 (2026-10-29)

**Pass, local/unpromoted D0 replay.** This closes macOS's bounded counterpart to
the Linux native-builder record, not Windows or full D1/D5/D6. Installation,
general dependencies, actual CLI HTTPS, authenticated integrity and self-upgrade
remain unimplemented/unverified.

### Exact Inputs

Before editing producer tools, produced a native Dev SDK from clean CRT
`17de3510c37cdb07e3c5c98583b22ebe4e1463ec`:

```sh
cmake --fresh --preset macos-host-ninja-debug -DCRT_RELEASE_TAG=v0.4.0-cli.20261029
cmake --build --preset macos-host-ninja-debug --target crt-development-sdk-test --parallel 4
ctest --preset macos-host-ninja-debug
```

This incrementally updates the existing core/imported-C++ build after fresh
configuration, then builds fresh static/shared zlib/Mbed TLS/curl. The ordinary
Dev consumer passes eight HTTPS/inflate/trust cases; CTest passes **143/143**.
Producer/base/compile commits all equal `17de3510...`, with clean flags. No
libc/PAL/stdlib source or public ABI fix was needed. The version is local,
not a Git tag or public release.

The complete SDK is copied outside both repositories to
`/private/tmp/crt-cli-macos-20261029.GyTATl/SDK space 한글/dev`; final CLI source
is `source-r2` under that work root. No producer-private headers or loose build
objects are consumer inputs.

| Input | Identity |
| --- | --- |
| Dev archive | `crt-v0.4.0-cli.20261029-macos-aarch64-dev.tar.xz`, 6,358,772 bytes |
| Archive SHA-256 | `f29e2e0c3a825fdd99745b124bc9665b6c39c9e9d92e3009c0a9a1a650c0c7b4` |
| Provider SDK content | `669cfcc9d25e8949bb6420c140168214f8439744295bc194a17d33e7ca53766e`, 2,570 file/symlink entries |
| Provider SHA-256 | `b950a94613e0b657a04e6a521ca942bb4742b6c05aaa72004661a0dfbd94e112` |
| Exporter SHA-256 | `70f4dbea7d31faba74eceebdc15a4cbcd618e56bdb80867133dc38d3a34dfff9` |
| CLI base | `0f14ed3bee6b6512a6969b6118dcbd82c1c564b7` plus local macOS source delta |
| Exported patch SHA-256 | `750266772bfddddc2f33d46e1734b04238caca7ad8b2265f78145742dc51d98d` |
| Source-export inventory SHA-256 | `b7b20691b0cd11fb8a333374e7097eb1588ba1183da91caac5d616a91853ed0f` |

The provider captures installed wrapper argv with explicit `-isysroot` and
`external_os_sdk`. Native tools are real Xcode Clang 21 and Apple ld 1166.2;
LLD is not installed/accepted here. The vendor SDK supplies linker/libSystem
discovery, not host target headers: `-nostdinc`, `-nostdinc++`, public CRT include
paths and static archive order remain intact. Complete external OS SDK/tool
runtime attestation remains open. Provider/project/lock format 0, bootstrap
result format 1 and SDK manifest/Dev profile format 1 retain their distinct roles.

### Isolated Native Workflows

A disposable hosted C++ seed builds the CRT-native CLI, which then self-builds
and drives its native integration suite. Parent Python orchestration stays
outside the product policy; the seed and native visibility helper are not
shipped product dependencies. Seatbelt denies reads/writes of both original
repositories, including CRT build outputs, and all process execution except
selected native tools, CLI and generated test/application paths. Network is
denied and `PATH=/nonexistent`. The helper confirms **two roots/two known files
inaccessible and 11 forbidden tools unexecutable**, including shell, Python,
CMake/Ninja/make/env and SDK wrappers. This is OS enforcement, not PATH filtering;
no macOS execve trace/count or Linux mount-namespace semantics are claimed.

Final suite: **474 assertions pass**, covering native C/C++ generation,
multi-source configure/build/test/run, JSON errors, duplicate keys, bad pins/
formats/targets/provenance, script compilers, incomplete SDK, profile/version
mismatch, traversal, existing destinations and lock preservation. Real C/C++
exit-7 test failures remain structured failures. A sleeping child times out with
124; a 10 MiB stdout producer completes with an 8 MiB/truncation result. Spaces,
Korean, `@` and shell metacharacters stay literal. The count differs from Linux's
earlier 429 because of new ownership/version checks and platform argument counts;
Linux was not rerun here.

The unresolved repository SDK request is now `*`, not a Linux-candidate version.
Explicit configure still locks the exact selected version/content/tool bytes.
The original repository lock remains unresolved; no existing project pin changes.

### One Executable and Runtime Audit

The bare policy additionally hides the exported source, SDK and provider and
denies compiler/linker execution. Its helper confirms **four roots/four files
inaccessible and 13 forbidden tools unexecutable**. Help, version and bootstrap
doctor pass from one copied executable:

| Observation | Result |
| --- | --- |
| Bare CLI | Mach-O arm64, 2,923,800 bytes |
| SHA-256 | `a1c23c4d86048306153094044fd77f8e2207d2480923201d0cd2914bc7422db7` |
| Direct dylib imports | `/usr/lib/libSystem.B.dylib` only |
| LC_RPATH | None |
| CRT-owned undefined host bindings | None; existing binding audit passes |
| Allocator | CLI defines malloc; actual dladdr(new/delete) resolves both to this executable |
| Doctor | C++ exceptions/allocation ownership, zlib and HTTP/Mbed TLS initialization pass |
| CLI HTTPS / authenticated integrity | `not_run` / `not_verified` |

Public dyld introspection records Apple's libc++/libc++abi and other system
images after exception handling; they are not hidden or labeled CRT payloads.
They are not direct product imports: the CLI supplies its C++/`__cxa_throw`
definitions, has no CRT/C++ dylib dependency, passes the binding audit and owns
its allocation functions. Darwin libSystem unwinding remains the SDK's existing
OS boundary. This is bounded runtime evidence, not complete host-image attestation
or a new binding-audit exception.

Seed-built/self-built Mach-O bytes differ. All section hashes match; only N_OSO
object timestamps, derived LC_UUID and code-signature bytes differ. The comparison
normalizes those fields in memory only; accepted binaries/SDKs are not rewritten.
No byte-reproducibility claim is inferred from Linux's earlier equal-byte result.

### Failures, Evidence and Limits

The initial direct Xcode seed lacked explicit vendor SDK lookup; a launcher
variant attempted an unlisted xcodebuild path. Explicit OS SDK arguments and
real Clang/ld resolve this without permitting interpreted build tools. A bare
visibility probe also correctly failed when JSON `\u` escapes did not match the
Korean SDK path in SBPL. Literal UTF-8 fixes the policy, not the probe. CRT's
shared external-SDK driver has the same correction and passes eight cases with
the original Korean-path SDK inaccessible.

Producer evidence root: `out/cli-macos-20261029/`. It retains configure/build/
core-test logs, `handoff.json`, archive, `provider-identity-apple.json`,
`source-identity-r2.json`, `cli-source-r2.patch`, `commands.json`, `replay-final.log`,
`native-*.stdout/stderr`, `bare-*.stdout/stderr`, `binary-audit.json`,
`self-build-comparison.json`, `result.json` and `sdk-external-utf8/`. External work
retains the policies/probes and final `source-r2`; earlier source/failed attempts
remain separate. Original/handoff SDK inventories and 19 non-Markdown source/
config/import inputs are unchanged. Both STATUS files are untouched. The producer
tooling suite passes **147/154**, seven conditional skips. Hosted GitHub CI and
other-host native replay are Not run here.

`post-run-review.json` rechecks all 19 current non-Markdown inputs, exporter,
SDK inventory and selected binary against these receipts. Final `result.json`
SHA-256: `a219001a5e16babf032fa301a91089e1dc1e19339983893a7148465a2ff8c983`.

Windows D0, arbitrary/cross/other-profile providers, full D1 HTTPS/trust, complete
tool attestation, installer/store, upgrades, packaging and formal promotion remain
open. No public SDK/CLI release or production lock is adopted.

## PAL-only Native Contract on macOS/arm64 (2026-10-29, revised)

**Pass, local/unpromoted replay of the revised initial contract.** The first macOS
pass above showed that the CLI used host macros and direct dyld/procfs calls. The
revision moves those queries behind CRT (`<crt/platform.h>`, `<crt/dl.h>`), puts
CLI headers under `include/`, drops the vendored JSON/SHA-2 sources in favor of
Development-SDK ports (nlohmann/json, cJSON, Mbed TLS SHA-2), adds typed
build targets (executable, static-library, shared-library; static/shared
runtime linkage) and selects `$USER_HOME/.crt/sdks` on every host. The CLI
source has no `__linux__`/`__APPLE__`/`_WIN32`/`CRT_TARGET_OS_*` use; the
replay driver rejects such source and `src/main/*.h`.

Producer: local modified CRT tree on `17de351` (Development SDK, dirty
provenance explicitly allowed by `--allow-dirty-sdk`); driver
`tools/replay_cli_contract_macos.py` (Seatbelt: network and producer/CLI trees
denied, only the declared compiler, linker, `ar`/`ranlib` executable). Evidence:
`out/cli-contract-20261029/replay-r5/` in the CRT checkout. SDK inventory
SHA-256 `8466f99f09c3ba72d9813886954e0fc452bd43e5be872cfe6a7441dd38b05deb`,
provider `8f31c90042f60a24c20b455a3c5f49c1e7095c76d966de54055fc0fa12d28f25`,
CLI source `6bcc01a6954e696bfa35cfaa57eb56052d8f8152a1e505e42b7b80e1bc9acbc3`,
binary `b21aa0b55f480108a33c3ddb14ff68837b7810168645dba85732b62acaedc8b7`
(2,964,952 bytes, imports only `/usr/lib/libSystem.B.dylib`, no RPATH).
Results: seed build, SDK-only self-build, **576** native checks (C and C++
projects, the six typed variants, hostile paths, installed-SDK copy under
`<home>/.crt/sdks`), and help/version/doctor with SDK, source and compilers
denied. The SDK inventory is unchanged.

Gaps found and fixed by this replay (earlier attempts retained as `replay`,
`replay-r2` ... `replay-r4`):

- `sendfile` returned `ENOSYS` on macOS/Windows while libc++'s
  `filesystem::copy_file` selects it whenever `<sys/sendfile.h>` exists and does
  not fall back, so the SDK installer copy threw. CRT now emulates `sendfile`
  with `pread`/`read` plus `write` (`libc/tests/sendfile_test.c`).
- The sandbox policy omitted `ranlib`, which `ar` runs itself; the native test
  budget (`crt.json`, 600 s) was too small for the typed-target matrix on this
  host and is now 1800 s.

Open: Linux/Windows replay of this revised contract, actual CLI HTTPS, full D1,
upgrades and promotion.

