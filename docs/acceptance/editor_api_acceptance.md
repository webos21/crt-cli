# Minimal Public Editor API Acceptance

This record owns the CLI's bounded P0 implementation and native consumer
evidence. The [versioned public contract](../reference/bootstrap_contract.md#public-editor-api-version-1)
owns commands, fields and error compatibility. The extension owns V0/V1
adoption, clangd configuration and Extension Development Host evidence;
producer P1-01 owns the SDK policy prerequisites.

## Scope and immutable inputs (2026-11-25)

Reviewed synchronized CLI `6cabc72`, CRT `a4f73033` and `crt-vscode` `5f468d1`.
CLI P0 uses the existing installed native policy/lock formats. CRT's
[P1-01 handoff](https://github.com/webos21/crt/blob/208b1c213d85a0651986f0c4824de272925e4eed/docs/acceptance/sdk_split_acceptance.md#linuxx86_64-sdk-compile-policy-for-the-first-editor-workflow-2026-11-25)
already qualifies the signed SDK for C99/C++17 in all four native modes;
no provider patch, CRT source/build-tree input or extension edit is needed.

| Input | Exact identity |
| --- | --- |
| Public bootstrap CLI | `v0.1.0-preview.20261125.linux-x86_64.1`; archive `942a45365cb3906754e5663a1c401ba223201cab9f025aa23d30e2b285a42289` |
| Public Dev SDK | `v0.0.1-preview.20261125.linux.x64.1`, clean producer `c89ae1ac720b258e7ee0e52829f900636283cb67` |
| SDK archive / manifest | `e455d08c31d1bf5d3cb470c4b28f09fffa228545350dc04ed099e132da85744b` / `6adfa744c4739f25b5c7275316913081a644b3bfc6a185dc8ac2a8f774f2cbbe` |
| Installed SDK content / native policy | `d2edb36c64f8f7c6d67603763e07a83d0cb4c2858c99e7957e00dde2d54dc921` / `54bb35e014c59af90c1e2b73c3f544ebc7815edc958e704e15bef097dbedd2d1` |
| Host and tools | Native Linux/x86_64, kernel `7.0.0-43-generic`, Ubuntu LLVM 21.1.8; tools declared by absolute path |

The CI bootstrap verifies the selected signed public bytes before native
use. The per-host artifact/signature/publisher identities are unchanged;
only `pr_tests` adds `editor-test`. Public Previews do not contain this new
API until a separately authorized future publication.

## Initial Linux native results (2026-11-25)

The final source snapshot (include/src/crt.json file-digest ledger) is
`860953a4d807f87558859926197a7ebecc5c6c3b0f2e324eba9caafc425db346`.
It identifies the initial P0 source later committed as `8fcc05e` and its fresh
native export. The follow-ups below retain their own changed inputs and evidence.

| Gate | Result |
| --- | --- |
| `editor-test`, public installed SDK | **374/374**, eight C/C++ entries equal actual cold compiler argv |
| Network-denied/source-hidden consumer, read-only queries/export and failed-write preservation | Pass; 374 native checks plus positive read-only query/export and structured unwritable-output refusal |
| SDK/LLVM/project-free capability query | Pass; exact capability fixture under empty HOME/PATH and hidden inputs |
| Actual compiler working directories | All eight object DWARF compilation directories match their exported `directory`; inspected with test-host readelf |
| Existing local fast suites | Native **949**, archive **221**, SDK store **114**, store locks **9**, recipe **147** |
| CI host receipt/classification helpers | **20/20**, including missing/invalid editor receipt refusal and retention after source re-export |
| Fresh self-build | Stage-1 and stage-2 cold **22 executed / 0 reused**; warm **0 / 22** |
| Bare execution/import audit | Version/help/doctor pass with empty HOME/PATH; no `DT_NEEDED`; declared OS interpreter `/lib64/ld-linux-x86-64.so.2` |

| Consumer artifact / receipt | SHA-256 |
| --- | --- |
| Final fresh self-built CLI | `f6b29209a5cae56b0ae050067473942a77c84dfdc85cdb412bd6a1cc08821c6a` |
| Separately source-hidden consumer CLI | `878f1e6efdda7661a0614bd8dc3dc8ab05ae7bb2a7ae212335afc4de17e12818` |
| Source-hidden native test executable | `73475112fc9cbb8b5d8fec7b2ac9cca6d15464f88b5c29c8ca2b0f77885798b8` |
| Final full local CI summary | `05146d021ba39763613ca4225886bbba6279c48f72d9533ce56d74a6533e0225` |

The retained post-test stage-1 CLI equals stage 2 byte for byte in the final
loop. The summary's initial stage-1 hash precedes tests: the first new-driver
test rebuilds the CLI once because compile argv/cache keys and the compiler
working directory now differ from the old published bootstrap's behavior.
The retained binaries, cold/warm receipts and initial hash are preserved
separately; this is not a general byte-reproducibility claim.

Evidence is retained in ignored `out/editor-api-20261125/`: `source-inputs.json`,
`verification.json`, `ci-summary.json`, the retained CI/native `editor-api-result`
receipts, source-hidden result/argv, SDK-free capability controls and host-test
log. Full native work and trace directories are under
`/home/syszone/crt-cli-editor-p0-20261125/`: final full loop `build-r3/`, focused
receipt-retention check `editor-final-receipt-check/`, and separate isolated
consumer `isolated-r4/` / `isolated-work-r4/`. The two development bootstrap
loops and earlier isolated iterations remain development records.

Failed preparation controls remain separate: an initial isolated launch
overlapped CI's source re-export and found no local binding before executing
the CLI; an initial SDK-free probe tried to chdir under a hidden checkout.
Stable retained inputs and an external empty HOME fix those harness
preconditions. Neither failed launch counts as a passing API refusal.

## Exercised contract and focused regressions

`editor-test` is CRT-native C++ and uses the real selected CLI, installed SDK
and compiler. Its checked-in `src/test/data/editor_api.json` supplies the
capability and final-envelope success/error/child-failure wire fixtures, plus
version/usage negatives. It is a contract fixture, not a claimed editor run.

- Capability discovery and unknown-version refusal work without selecting a
  project or SDK. The existing final JSON envelope stays format 1; legacy
  `sdk list` and `port list` retain their collections.
- A source-only project reports unresolved and cannot export until explicitly
  bound. Bound state reports exact SDK/policy/tool and lock-byte identities;
  target paths/existence are planning metadata, not runtime/debug acceptance.
- An eight-source C/C++ fixture covers static/shared executables and
  static/shared libraries. Every exported argv equals one actual cold-build
  compiler invocation, including language, SDK headers/macros and per-mode
  options; no link/startup command appears. Compiler working directory is
  explicitly the same project root the database reports.
- Static/shared C++ consumers run. Warm builds execute nothing; re-export is
  byte-stable. Project manifest, lock, local binding and outputs are preserved
  by queries/exports. They never acquire SDKs or source ports.
- SDK-content/policy lock mutations, missing sources, unsupported manifests
  and missing port items are reported as incompatible without repinning,
  rebuilding or replacing an existing database. Changed local defaults cannot
  migrate the binding.
- Protected/symlink/unrelated output files and absent/unwritable output
  parents fail closed. A failed application reports child exit 7 in workflow
  outcomes; its stdout is on stderr and the final JSON remains parseable.

The CI driver retains `artifact/editor-api-result.json` before the second
source export removes stage-1 outputs. Its report gate refuses a missing,
malformed, failed or mismatched-count receipt; focused host tests cover this
retention/failure behavior.

## Native replay procedure

Start with a fresh native source/output and the checked public pins:

```text
python3 tools/ci_bootstrap.py --work /absolute/fresh-ci --llvm-bin /usr/lib/llvm-21/bin
```

The default PR set now includes `editor-test`. The driver preserves the final
envelope/cold compiler evidence alongside its ordinary native, archive,
SDK-store, store-lock and recipe results. Cold/warm bootstrap, actual ELF
imports and SDK-free CLI execution retain their separate checks.

For a separate Linux consumer boundary replay, build `editor-test` in a
retained, configured source export with the tested CLI. Keep that export
stable while the replay runs; the CI bootstrap recreates its source directory
between stages. Then:

```text
python3 tools/replay_editor_linux.py --cli /absolute/crt-cli --test /absolute/editor-test --project /absolute/configured-source-export --home /absolute/private-home-with-installed-sdk --output /absolute/fresh-evidence --work /absolute/fresh-work --hide /absolute/crt --hide /absolute/crt-cli --hide /absolute/crt-vscode
```

Bubblewrap denies networking, clears PATH/environment and mounts the root/SDK
read-only. Original producer/CLI/extension roots and copied product source
headers are hidden; only the public contract fixture and binding metadata
remain visible. Explicit visibility controls precede native execution. The
same C++ test runs with disposable Unicode/space projects, followed by queries
on a read-only project, a successful export to the chosen external directory,
and a structured refusal for an unwritable output. Strace separately records
the host visibility controls; native execution admits only the selected CLI,
test binary, declared LLVM tools and the two built applications. Host readelf
checks all eight objects' actual debug compilation directories against the DB.
Python, Bubblewrap, strace and readelf are test-host orchestrators/auditors,
never CLI/app prerequisites.

## Windows CI static-only SDK library fixture (2026-11-25)

PR [#53](https://github.com/webos21/crt-cli/pull/53), source `8fcc05e`, failed in
`editor-test` on both Windows hosts in run
[`45579708676`](https://github.com/webos21/crt-cli/actions/runs/45579708676).
All four Linux/macOS jobs passed; the Windows native/archive/SDK-store/store-lock/
recipe suites and remaining bootstrap/audit stages passed. The shared failure
was the fixture's `ready before build` assertion, before compile export began.

Both public Windows policies were downloaded and checked against their
unchanged CI contract pins:

| Host | SDK policy SHA-256 | Declared zlib artifacts |
| --- | --- | --- |
| Windows/x64 | `7762f572b4614b3068e1ea99694ce19360e213d31c5bd01019e4ad6ce005b726` | `static: lib/libz.a`; no shared variant |
| Windows/arm64 | `7c7027800b5099454ca1d67847943459064362fa65204d5ca17d6a3c8e1b721b` | `static: lib/libz.a`; no shared variant |

The original fixture declared zlib on all targets but left `sdk_library_linkage`
implicit. Its shared-runtime executable/library therefore asked for shared
zlib, which those SDKs do not declare. The read-only planner correctly refused
that request. SDK library linkage and CRT runtime linkage are independent,
already implemented contracts; this was not a missing Windows export feature
or permission to invent an SDK variant.

The fixture now explicitly selects static SDK zlib. Static/shared CRT runtime
and all four C/C++ compile modes remain exercised; no host, target or suite is
skipped. A missing SDK library must report an actionable incompatible-state
diagnostic and leave project/database bytes unchanged. A policy declaring no
shared zlib also exercises an explicit shared request and requires its refusal,
with no fallback or database replacement. Ready-state assertion failures now
include the actual query response so future provider mismatches are visible
in job logs.

Fresh focused Linux validation passes **381/381** against the original public
SDK. An explicitly test-only policy copy removes the Linux provider's shared
zlib entry to reproduce the Windows declaration, keeping the public SDK and
original policy untouched. The implicit shared request reports
`the SDK contract declares no shared variant of library 'z' for this target`,
and export fails with `project_incompatible` before writing a database.
Explicit static selection restores a ready plan, and the complete corrected
fixture passes **388/388**, including the extra absent-variant negatives.
This constrained copy is a local failure control, not a producer policy
promotion or a replacement for native Windows CI.

Local work is retained under `/home/syszone/crt-cli-editor-ci-fix-20261125/`:
`local/` has the fresh source/test export and 381-check receipt;
`static-only-control-r2/` has the restricted policy, request/refusal logs and
388-check receipt. The first control setup assumed legacy `application`
instead of `new`'s current `targets` manifest and stopped before configure;
that setup failure remains separate. Initial hosted logs/artifacts and both
verified Windows policies are under ignored `out/editor-api-ci-fix-20261125/`.

The [required PR checks](https://github.com/webos21/crt-cli/pull/53/checks) own
the corrected native Windows replay and full six-job gate. Production code,
API/lock formats, SDK/CLI artifact pins, publisher inputs and adjacent sources
are unchanged by this fixture correction.

## Windows compiler-argv path spelling (2026-11-25)

The static-library fixture correction at `0fff118` reaches cold compilation
on both Windows hosts, but run
[`45591705708`](https://github.com/webos21/crt-cli/actions/runs/45591705708)
then fails the exact exported-versus-executed argv assertion. All Linux/macOS
jobs pass. The failure is retained in both Windows artifacts, separately from
the original unsupported-library request.

Inspection constructed `Builder` with a raw `root / "out/native"` path while
build and port-lock callers supplied `inside(root, "out/native")`. Windows
therefore allowed different literal separator spelling in compiler object,
depfile and generated-header arguments. POSIX paths did not expose the split.

`Builder` now owns its canonical native build directory and applies native
preferred separators once. Callers cannot supply a second spelling. Ready
project metadata and generated-header paths use the same builder directory.
The regression still compares literal argv arrays; it does not normalize away
differences or accept merely equivalent commands. Each compile output must
also use the reported build-directory spelling. Mismatch diagnostics include
both actual and exported argv for any future difference.

Fresh Linux source/output validation in
`/home/syszone/crt-cli-editor-ci-fix-20261125/canonical-build/` passes editor
**389**, native **949**, archive **221**, SDK store **114**, store locks **9**
and recipe **147**. `artifact/editor-api-result.json` retains the actual
eight-entry compiler comparison and the directory assertions. This local
POSIX result is not substituted for native Windows path validation.

The [PR checks](https://github.com/webos21/crt-cli/pull/53/checks) retain native
Windows x64/arm64 and the full required matrix; no SDK/artifact pins, native
compiler policy, runtime linkage or public schema changes are involved.

## Hosted P0 acceptance (2026-11-25)

[CI run 45596263040](https://github.com/webos21/crt-cli/actions/runs/45596263040)
passed all six native jobs and `CI gate`. The tested PR head was
`52603fbfa00b5856926d95c3de48084f0e8dda0a`, and each `ci-summary.json` records
the tested PR merge `49e34645f39d7eafc528a439625d32219a7b4d4c`. Its source tree
matches the tree of the [PR #53 merge commit](https://github.com/webos21/crt-cli/commit/3733aeb108bcad5ba45f9feec234330d5c4ddbc4)
on `main`. The workflow used the [per-host CLI and SDK pins at that revision](https://github.com/webos21/crt-cli/blob/3733aeb108bcad5ba45f9feec234330d5c4ddbc4/.github/ci/pins.json).

| Native runner | Editor checks passed |
| --- | ---: |
| Linux/x86_64, Ubuntu 26.04 | 389 |
| Linux/x86_64, Ubuntu 24.04 | 389 |
| Linux/aarch64, Ubuntu 26.04 | 389 |
| macOS/arm64 | 413 |
| Windows/x64 | 424 |
| Windows/arm64 | 432 |

The six downloaded `artifact/editor-api-result.json` files match their
summary counts. In each receipt, all eight exported C/C++ argv arrays match
actual cold compiler invocations exactly, with both static and shared CRT
runtime coverage. Both Windows receipts include refusal of the unsupported
shared-zlib request. The native regression suites and import audits also passed.
The downloaded artifacts and `canonical-hosted-verification.json` are retained
under ignored `out/editor-api-ci-fix-20261125/`.

## Limits and downstream handoff

The standalone source-hidden/read-only consumer workflow is qualified on
Linux/x86_64. The hosted matrix above exercises the CLI contract with the
recorded PR source and per-host pins. No clangd binary or Extension
Development Host was run. `crt-vscode` must adopt API/version discovery and
these final-envelope fixtures through its V0 queue, then exercise V1-08/09/10.
The existing help-substring guard is still provisional extension code.

This adds no job-event stream, debugger configuration, generalized project
schema, SDK resolver, download default, port build, lock migration, release or
publisher-key change. Generated project headers remain a normal build output;
export reports their paths but does not create them. D5/D6/D7 gates beyond the
bounded P0 API stay in their existing queues.
