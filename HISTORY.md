# HISTORY: CRT-CLI Completed Work

Reverse-chronological, dated record of completed work. Current and planned
steps belong to [TODO](TODO.md). Designs, procedures, and measured acceptance
keep their own detailed records under [docs](docs/README.md).

Dates identify when the work was completed or documented. Add a span when work
crosses days; include commit/reproduction references when available without
inventing commits or test results. Older months may later be summarized under
`docs/history/` with links to their original Git records.

## 2026-10-31

### Add `fetch`, measure Windows Unicode paths and replay against a release-asset SDK

On Windows/arm64: the CLI gained a minimal `fetch` command (explicit CA file, always-on verification, size cap, pinned SHA-256,
staging then rename, no overwrite) because it had no way to download; CRT's new `tools/test_cli_fetch_https.py` passes 17 of 17
positive and negative cases against controlled loopback endpoints, and the suite has 769 checks. A Unicode-path probe
shows that Hangul works as ANSI (CP949) bytes through the C library and libc++ `filesystem`, UTF-8 bytes fail, a character
outside the code page is silently best-fit, and CLI `build` fails in a Hangul directory because clang writes its dependency
file in UTF-8; nothing was patched. A local `crt-dev-release` rehearsal (no tag, nothing uploaded) collected the
Windows/arm64 Dev SDK ZIP, whose two symlinks become file copies, and the CLI replay passes against its extraction.
`bin/win-arm64/crt-cli.exe` is refreshed. D1-07 stays open (no signed-metadata policy, publisher or SDK-hidden run) and no
SDK was promoted. [Acceptance](docs/acceptance/bootstrap_acceptance.md#windowsarm64-https-fetch-unicode-paths-and-release-asset-sdk-2026-10-31).

## 2026-10-30

### Replay the sources on Windows/arm64 and add the first win-arm64 binary

First native run of this host pair: CLI `ec187d5` against a Development SDK built on the arm64 host from a clean CRT
`67f9516`. Two producer-tool gaps blocked it and were fixed in CRT: the toolchain exporter and the Windows replay driver
accepted only Windows/x64 (the driver hard-coded the probe architecture), and the exporter's argv recorder was a hosted
program that needs a C runtime library this host's Visual Studio Build Tools lack for ARM64; it is now freestanding and
matches the previous output byte for byte on ASCII arguments. No CLI code changed. The replay passes 748 native checks,
cold 8/0 and warm 0/8 cache, a 361-process inventory without forbidden images and the bare-runtime audit (kernel32 and
one API-set import); a separate, unsandboxed stage1/stage2 loop is identical apart from the PE link timestamp.
`crt-c-test` passes 166/166 with the Linux/aarch64 `fcntl.h` change compiled in. `bin/win-arm64/crt-cli.exe` is added.
The CRT changes and the new recorder are not replayed on Windows/x64, and CLI HTTPS and Unicode paths remain open.
[Acceptance](docs/acceptance/bootstrap_acceptance.md#windowsarm64-native-replay-2026-10-30).

### Replay the sources on Linux/aarch64 and add the first aarch64 binary

First native run of this host pair: CLI `50bda2e` against a Development SDK built
on the aarch64 host from CRT `641b034` plus two uncommitted producer changes. The
CLI toolchain exporter now accepts Linux/aarch64 (it rejected it by design), and
CRT's `include/fcntl.h` now uses the arm64 kernel values for `O_NOFOLLOW` and
`O_DIRECTORY`; the x86_64 values had silently disabled `O_NOFOLLOW`, which broke
imported libc++ `remove_all` on dangling symlinks and failed replay r2 at its final
cleanup line. A regression test fails on the old header and passes on the new one.
No CLI code changed. Replay r3 passes 672 native checks, cold 8/0 and warm 0/8
cache, a clean execve audit and the bare-runtime audit; a stage1/stage2 bootstrap
loop is byte-identical. `bin/linux-arm64/crt-cli` is added. The header change was
not replayed on Linux/x86_64, macOS or Windows, and eight unrebuilt `crtmedia`
tests fail or time out for reasons not investigated.
[Acceptance](docs/acceptance/bootstrap_acceptance.md#linuxaarch64-native-replay-2026-10-30).

### Replay the merged sources on macOS/arm64

Native macOS replay of CLI `754ca29` against a Development SDK built from CRT
`74ea992`: 700 native checks, cold 8/0 and warm 0/8 cache, a bootstrap loop from
the previous committed binary and a bare-runtime/import audit pass without CLI
changes. Stage1 and stage2 match in code and data and differ only in `LC_UUID`
and `__LINKEDIT`, so the macOS fixed-point criterion is segment identity. The
committed `bin/macos/crt-cli` is refreshed.
[Acceptance](docs/acceptance/bootstrap_acceptance.md#macos-revalidation-after-windows-and-linux-changes-2026-10-30).

### Revalidate the Windows-merged sources on Linux and refresh the test binary

The merged Windows changes affected shared process creation, hashing, output,
HTTP and build/cache code, so the earlier Linux record was insufficient. A fresh
producer SDK/replay reproduced SETPGROUP ENOTSUP before compiler execution.
The CRT PAL fix and regression resolve it without CLI code changes. Fresh core,
imported C++/Dev consumer tests and the isolated native replay pass; exact
candidate/source/patch identities, 660 assertions, cold/warm cache, bare-runtime
and bootstrap-loop results belong to
[Linux revalidation](docs/acceptance/bootstrap_acceptance.md#linux-revalidation-after-windows-changes-2026-10-30).

Refreshed `bin/linux-x64/crt-cli` from the passing bare executable and the ignored
local `.sdk.json` from its verified handoff. Preserved the old binary/config,
failed candidate and earlier results. The root lock stays unresolved; this is
an explicit dirty local SDK, not promotion/public release. Current-source macOS
replay and the remaining installer/HTTPS/upgrade/lifecycle gates stay open.
STATUS is unchanged; no commit/push was performed in this verification turn.

## 2026-10-29

### Run the native contract on Windows/x64 and make the sources portable to it

The revised PAL-only contract now builds, self-builds and passes its native suite (714 assertions) on Windows/x64 against a
clean local Dev SDK candidate, with the OS-recorded process inventory standing in for execve tracing, and the bare CLI imports
only `kernel32` and one API-set DLL. The replay found twelve consumer-side defects (fork opt-in, native-child stdio hint,
libc++ path type vs the curl consumer options, wchar stream constructors, inventory order, self-overwrite of a running exe,
pipe drain, `fread` cost, child `PATH` for shared runtimes, vacuous edit probe, non-UTF-8 OS text, `/tmp` and `;` fixtures); the
CLI `execute()` is now `posix_spawn` on every host. Unicode paths remain a producer gap. Linux/macOS replays of the changed
sources are still to do. See
[Windows acceptance](docs/acceptance/bootstrap_acceptance.md#native-json-builder-on-windowsx64-2026-10-29).

### Add the local `.sdk.json` and verify the Linux/x86_64 bootstrap loop

`configure` reads defaults from an ignored `<project>/.sdk.json` (or `--sdk-config`);
explicit flags override per key ([format](docs/reference/bootstrap_contract.md#local-sdkjson)).
The Linux values came from CRT's passing `replay-l4` handoff (CRT `d054a66` Dev SDK,
dirty, unpromoted). Loop on Linux/x86_64: the earlier seed built the current sources
(stage1), stage1 rebuilt them from a clean `out/native` (stage2) with a byte-identical
SHA-256, a cold build ran 7 steps and an unchanged rebuild reused 7, and the native tests
pass (660 checks). `bin/` is split into `macos`, `linux-x64`, `linux-arm64`, `win-x64`,
`win-arm64`; the Linux/x86_64 binary is placed under `bin/linux-x64/` (ELF imports and hidden-SDK bare run checked, not an execve replay).

### Reuse intermediate build outputs and commit host binaries

Builds keep objects, archives and `-MD` dependency files under the ignored
`out/native/` and skip steps whose command, SDK/tool identity and input content are
unchanged. The macOS/arm64 executable is committed under `bin/macos/` for hands-on
testing (Linux and Windows pending). See
[acceptance](docs/acceptance/bootstrap_acceptance.md#incremental-build-cache-and-committed-binary-2026-10-29).

### Remove host macros from the CLI and add typed build targets

CLI code now reaches platform information, native-executable detection, the
user home and loaded images through CRT PAL APIs instead of `__linux__`/
`__APPLE__`, `/proc/self/maps` or dyld. Headers moved to `include/`, `crt-cli new`
creates `include/`, SDKs install under `$USER_HOME/.crt/sdks` on all hosts, the
vendored JSON/SHA-2 sources are replaced by Development-SDK ports, and targets
may be executables, static or shared libraries. macOS replay:
[PAL-only contract](docs/acceptance/bootstrap_acceptance.md#pal-only-native-contract-on-macosarm64-2026-10-29-revised).

### Replay native bootstrap on macOS/arm64

Added bounded macOS native-target/Mach-O inspection, reused the CRT POSIX child
backend and made generated target metadata follow the selected SDK. CRT's
separately hashed provider supplies the native Xcode OS SDK/link policy; CLI
still executes LLVM argv directly, without wrappers/Python/shell/CMake/Ninja.
The unresolved self-project request no longer fixes a Linux-candidate SDK
version, while configured locks and version-mismatch rejection remain exact.

The external SDK-only self-build and 474 native assertions pass, including
C/C++ generation/workflows, hostile paths, exit-7 failures, timeout and bounded
stdout. OS-enforced probes deny producer trees and interpreted/build tools;
bare help/version/doctor additionally pass without SDK/source/compiler access.
Mach-O directly imports only libSystem and has no RPATH. Public dyld maps and
dladdr-based allocation ownership distinguish OS-loaded C++ images from the
CLI's own static runtime. SDK inventories and original unresolved lock remain
unchanged; earlier failing attempts are retained, not relabeled.

Only the macOS D0 sub-gates are closed. Windows, actual CLI HTTPS, full D1,
installation/upgrades, complete external tool attestation and promotion remain
open. Both STATUS files are unchanged. Exact inputs, results and raw evidence:
[macOS acceptance](docs/acceptance/bootstrap_acceptance.md#native-json-builder-on-macosarm64-2026-10-29).

### Commit the native builder and reconcile producer/consumer documentation

Committed the native implementation as `1270695ee8e1ab53b661c4540af859cb2463d840`. Its 19
recorded code/config/import inputs match the accepted Linux export; the original
SDK, binary, source-export and failed-attempt identities remain unchanged.
Recorded CRT exporter/contract revision `17de3510c37cdb07e3c5c98583b22ebe4e1463ec` separately from the
`64b447b` SDK build baseline in the compatibility ledger. This is source
reconciliation, not a new build, promoted lock or public release.

Aligned README/TODO, current `new --language` examples, native bootstrap guide,
contract/reference/index and partial lifecycle evidence. Full macOS/Windows,
installer, upgrade and shipping gates remain open. Rechecked documentation
links/fences, dependency/exporter/binary hashes and version/bootstrap doctor;
retained the prior native assertion/core-test results at their original date.
Imported license bytes remain exact, including upstream whitespace. STATUS
remains unchanged. Details are in
[source reconciliation](docs/acceptance/bootstrap_acceptance.md#committed-source-reconciliation-2026-10-29).

## 2026-10-28

### Replace the CMake prototype with a native JSON compiler backend

Applied the owner's subsequent choices: `crt.json`, separate native modules in
`src/main/`, and C/C++ tests in `src/test/`. The CLI directly executes LLVM argv
for its own manifest and generated multi-source C/C++ projects. Python, shell,
CMake/Ninja and SDK wrappers are absent from the app/control/self-build path.
A disposable hosted C++ seed shares the backend and is never shipped. SDK ABI,
startup and link order remain producer-owned through a separately pinned
experimental policy captured from installed wrappers.

Native filesystem validation found the producer's Linux getdents64 type-offset
bug. CRT fix `64b447b` and a fresh clean-source Dev SDK resolve it without a
scanner workaround, old-artifact mutation, host runtime substitution or upstream
patching. Earlier build/tool/timeout/probe attempts remain retained separately.

Linux isolated native self-build, C/C++ fixtures and 429 assertions pass with
interpreters/build generators and producer trees unavailable. The self-built
binary matches the seed-built bytes, has no ELF DT_NEEDED/RPATH and runs bare
with only the declared OS loader mapped. Original SDK/provider inventories and
19 non-Markdown source inputs were rechecked. Exact hashes, commands and limits
belong to [native acceptance](docs/acceptance/bootstrap_acceptance.md#native-json-builder-on-linuxx86_64-2026-10-28).
The repository lock remains unresolved. No full D1/D5/D6, other-host replay,
SDK acquisition/updater, authenticated CLI integrity or public release is claimed.

### Retained checkpoint: first Linux bootstrap through SDK wrappers/CMake

Recorded the owner's four decisions: crt-cli naming, shared immutable SDK
references, first-configure/build acquisition and single-executable policy with
OS/data/cache/certificate/previous-version/temporary-helper allowances and no
permanent launcher. Established the initial source/tools/tests/CMake layout,
actual help/version/bootstrap doctor, result/input records and experimental
format-0 project/lock fixtures. The root lock remains unresolved; no stable
project resolver or promoted SDK pin is claimed.

The existing clean Dev SDK is consumed through a separately extracted handoff
and its public wrappers. Configure verifies the exact SDK content before
loading the toolchain; build rechecks it. SDK-owned static/exception/RTTI/hosted
controls resolve the initial exception-default failure without CRT ABI changes.
Actual Linux source/SDK-hidden and Unicode relocation replay passes, including
seven specific configure rejections and actual ELF/mapped-image evidence.
Harness/tool-selection failures and prior attempts remain identified separately.

The binary is a local development result, not a public release: full HTTPS,
trusted self-integrity, installer/new/upgrade and macOS/Windows replay remain
open. Numerical results, hashes, source snapshot, commands and limits belong to
[bootstrap acceptance](docs/acceptance/bootstrap_acceptance.md#linuxx86_64-d0-bootstrap-2026-10-28).
The [implemented contract](docs/reference/bootstrap_contract.md) separates
actual commands/formats from the remaining design. No full CRT/WebKit rebuild
or source/runtime SDK relabeling was required.

### Define initial structure, single-executable and project lifecycle goals

Reordered D0 with explicit D0.1-D0.7 items aligned to CRT Tranche 1.1-1.7:
product/initial source-layout review precedes SDK handoff, fixtures and minimal
CLI implementation. The generated-project/public build contract is shared;
internal module directories may evolve without a private self-build exemption.

Adopted single-executable distribution as a goal, replacing the prior dynamic
bundle default without retroactively changing producer evidence. Defined
self/environment/project doctor, planned verified CLI self-upgrade with
candidate health/activation/recovery, and project-specific SDK version/profile
pins independent of physical storage and build modes. Command naming, detailed
resources/helpers, storage default and new/install/first-build acquisition timing
remain explicit review decisions rather than selected implementation defaults.

Added [lifecycle acceptance](docs/acceptance/lifecycle_acceptance.md) for future
upgrade/project/package gates, extended bootstrap criteria without renumbering
existing case IDs, and linked the compatibility record, guide, index, README
and contributor rules. The reviewed baseline commits remain those in the
existing local workflow review, with these additional working-tree documents.
No product sources/build targets, command execution, runtime pass, lock adoption
or public release were introduced.
Documentation checks cover 226 local paths/anchors across the 14 changed/new
producer/consumer Markdown files, fences, numbering and whitespace. Original
bootstrap case IDs/Not run ledgers and both STATUS files are preserved; remote
URLs were not network-validated.

### Define joint development using local unreleased Dev SDK candidates

Coordinated CRT Tranche 1 and CLI D0 planning: one authorized session can
develop both local repositories before a public Dev SDK release. The producer
creates/verifies a candidate; CLI consumes its immutable copied/extracted public
SDK outside producer source/build trees. The development guide owns the
handoff/fix/rebuild/retest procedure; D0 acceptance owns source-tree-unavailable
validation and exact source/build/artifact identities. Failed candidates remain
separate from rebuilt ones and from final promoted-lock adoption.

Recorded the pre-change repository baselines and ownership in
[compatibility](docs/compatibility.md#local-candidate-workflow-review-2026-10-28),
preserving original design provenance and the Not run runtime ledgers. Updated
TODO, contributor guidance, design and the documentation index. SDK/CLI versions
and public-release times remain independent; promote/revalidate the producer
artifact before final CLI pin adoption and public delivery. No CLI source,
build entry point, candidate test, artifact promotion or public release was
performed by this documentation change.
Documentation checks pass for 123 local paths/anchors across the changed
producer/consumer documents, fences, numbering, review revisions and whitespace;
STATUS and the Not run runtime ledger are unchanged. Remote URLs were not
network-validated.

## 2026-10-21

### Connect producer contracts, bootstrap acceptance and Studio adoption

Reviewed CRT `a23a173`, CLI `a76d8ad` and Studio `d3793e3` against the supplied
cross-repository assessment. Added [Contract Compatibility and Adoption](docs/compatibility.md)
with exact review revisions, independent existing manifest/recipe formats,
draft/unassigned contract versions and evidence requirements. The original
design provenance remains pinned; no runtime combination is approved.

Added D1-07 SDK-free HTTPS metadata/artifact verification, shipped-runtime and
trust rejection cases; D2 still owns SDK installation transactions. Clarified
that JSON drafts begin in D0, D6 stabilizes automation, and formal D7 approval
does not block SDK-only Studio S0 or real earlier capability experiments.
Updated TODO, index, design and agent rules to use the adoption record and
producer-owned shared-change procedure. Automated checks remain planned.
This is documentation work only: CLI sources/build targets and native
bootstrap acceptance are still absent, with runtime ledgers left Not run.
Documentation validation checks local links/anchors, English text, fences,
conflict markers and whitespace; external URLs were not network-validated.

## 2026-10-20

### Establish the documentation layout and initial work queue

Created the user-facing README, documentation index, local CLI design,
development guide, and D0/D1 bootstrap acceptance contract. The local design
adopts the CRT cross-repository baseline at
`d21430ddeb59aea76cf6abd58c15a69f55cfda52`, records its origin, and becomes the
owner of subsequent CLI-specific design decisions. SDK and Studio contracts
remain linked to their producer-owned documents instead of being copied here.

Initialized TODO with D0 consumer contracts/bootstrap definition as the current
focus and D1-D7 as ordered plans. The README distinguishes intended command
examples from available features; the guide does not invent CMake presets or
test commands, and acceptance contains no fabricated runtime pass results.
Documentation checks cover local links/anchors, English text, code fences,
and whitespace. No CLI implementation, SDK build, runtime replay, commit,
publication, or release approval is claimed by this documentation work.

### Define repository agent guidelines

Created [AGENTS.md](AGENTS.md) from the pinned CRT CLI design. It establishes
CRT-native C/C++ and SDK-only consumption, Windows CRT host tools/rootfs,
source-first ports, immutable identities/installations, structured command and
process contracts, staged verification, and English documentation ownership.
The initial repository has no product build or executable yet; the guidelines
explicitly distinguish proposed contracts from implemented functionality.
