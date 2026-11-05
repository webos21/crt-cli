# D2 Local SDK Installer Acceptance

Contract: [CLI design section 3](../design/crt_cli.md) (immutable store, transactional install) and the CRT
[install metadata](https://github.com/webos21/crt/blob/main/docs/design/sdk_development_contract.md#install-metadata-format-1-crt-31)
(format 1). Scope of this record: local archives only; no network acquisition, updates, project-lock migration or CLI
self-upgrade.

## Linux/x86_64 isolated replay (2026-11-03)

`tools/replay_sdk_install_linux.py` runs every CLI call in bubblewrap: user/ipc/uts/net namespaces (no network), read-only
root, source and producer trees hidden, cleared environment with an empty PATH, private `/tmp`, one writable directory
(store + sentinel tree). The PID namespace is shared on purpose: lock/transaction owners are PIDs, and a fresh PID
namespace per run would recycle small PIDs. Inputs: the dev archive and `release-manifest-dev-linux-x86_64.json` of a
`crt-dev-release` **rehearsal** (tag `v0.0.1-rehearsal.1`, uncommitted code in a scratch clone; not a release candidate),
CLI = the seed dev-loop binary (not a self-built, SDK-only CLI). Result: **48/48 checks**, evidence retained outside the
repositories (`result.json`, work tree).

| Area | Checks |
| --- | --- |
| Install from pinned manifest | identity equals the promised one; one SDK directory; `.staging`/`.locks` empty; `list`, `verify`; reinstall idempotent; host recomputation of the identity equals the manifest's |
| Immutability | flipped byte in an installed file: `verify` fails, reinstall refuses ("immutable"); restoring the byte passes |
| Tampered inputs | flipped archive byte, extra trailing byte, wrong pinned manifest digest: refused, nothing published |
| Hostile archives (digest supplied) | traversal, absolute path, absolute/escaping symlink, write-through symlink, hard link, device node, case collision, not an SDK: refused; no SDK, no staging, no locks; nothing outside the store; sentinel tree unchanged |
| Interruption | SIGKILL during extraction: nothing published, leftovers present, `recover` removes them, next install and `verify` pass |
| Concurrency | two parallel installs with `--lock-wait`: both succeed (one new, one already installed), one SDK, bookkeeping empty |
| Read-only store | refused |
| Zip | a zip of the same SDK installs with the identical identity; `verify` passes |

The first run exposed a harness error (killing only bwrap's process group left the CLI running, so `recover` correctly
kept its live transaction); the replay now kills the CLI by its command line. The harness failing there is also evidence
that `recover` does not remove a live owner's transaction.

## macOS/arm64 isolated replay (2026-11-04)

`tools/replay_sdk_install_macos.py` is the Seatbelt counterpart of the Linux driver. Every CLI call runs under a profile
with no network, writes denied everywhere except one scratch directory (the SDK store plus a sentinel tree) and a private
temporary directory, the CRT and CLI checkouts and the producer worktree denied for reading and writing, and a cleared
environment (empty `PATH`). Seatbelt cannot make the root read-only or give each run a private `/tmp`, so the write
boundary is the deny rule and escapes are also searched for afterwards. macOS has no PID namespaces; lock and transaction
owners are ordinary PIDs. A read-only store is a store whose directory the profile denies writing. The interruption case
finds the CLI by its command line (`ps`) and kills it with SIGKILL.

Inputs: a **rehearsal** `crt-dev-release` of clean CRT `53b1a80` (`feat(sdk): ship xz port and install metadata in
Development SDK releases`) built natively in a detached worktree with tag `v0.0.1-rehearsal.2` (not a Git tag, not a release
candidate). Its `release-manifest-dev-macos-aarch64.json` (SHA-256 `fed6e5db...e52b`) carries install metadata format 1 for
`crt-v0.0.1-rehearsal.2-macos-aarch64-dev.tar.xz` (6,673,896 bytes, SHA-256 `7446cb5e...aa20`): 2,786 entries (2,598 files,
178 directories, 10 symlinks), 116,934,449 uncompressed bytes, identity `80bfe046...e1da`. The SDK ships `liblzma`
(static and shared). The CLI is **self-built and SDK-only**: the bare executable of the macOS contract replay of the same SDK
(`f1d9f925...d688`, 3,876,216 bytes, imports only `libSystem`, no RPATH): 16 sources, cold self-build 16 executed / 0
reused, unchanged rebuild 0 / 16, previous-binary bootstrap loop (stage1 and stage2 identical in code and data; only
`LC_UUID` and `__LINKEDIT` differ), native suite 807 checks plus 158 archive and 66 SDK-store checks, SDK-hidden
help/version/doctor.

Result: **48/48 checks**, the same cases as the Linux table above, evidence in `result.json` and the retained work tree
(outside both repositories). macOS-specific points: the case-collision archive (`A` and `a`) is refused although the default
APFS volume is case-insensitive; an interrupted install leaves recoverable staging that `recover` removes, after which a new
install and `verify` pass; two parallel installs give one new and one already-installed result with one SDK directory and
empty bookkeeping; a zip of the installed SDK reproduces the identity.

An earlier invocation of the driver stopped at an argument error (its work directory was inside a hidden checkout); no CLI
call had run. Nothing in the CLI changed for this replay.

## Windows/x64 isolated replay (2026-11-04)

**Pass, native Windows/x64; local/unpromoted.** `tools/replay_sdk_install_windows.py` is the Windows counterpart of the Linux and
macOS drivers. Every CLI call runs through CRT's `crt_windows_trace`: a restricted token naming a per-run SID, an empty `PATH`,
redirected stdio and a job object that records every process the call starts. The work directory (the store, a sentinel tree,
hostile archives; Hangul and spaces in its name) is private to that SID, and a native probe built with the SDK under test
proves the CRT checkout, the CLI sources, the producer clone and the SDK/source of the CLI build unreadable under it. Windows has
no network namespace and no read-only root, so **network denial is not claimed**: the `sdk install` command has no network path,
and the recorded images are only `crt-cli.exe` and `conhost.exe`. A read-only store is a directory whose ACL gives the run SID read
access only. The interruption case reads the transaction's recorded PID from `.staging/*/txn.json` and kills it with `taskkill /F /T`.

### Inputs

| Input | Identity |
| --- | --- |
| Host | Windows 11 Pro 10.0.26100 x64, LLVM/Clang 22.1.8, Windows SDK imports `10.0.28000.0/um/x64`, Developer Mode on (not changed) |
| Producer | clean CRT [`1ee0379`](https://github.com/webos21/crt/commit/1ee037923ef6a3460c6f99c6e426d294fe5b7025) (CRT `a48000c` plus the three Windows fixes below), fresh clone, `CRT_RELEASE_TAG=v0.0.1-rehearsal.3` (not a Git tag, nothing uploaded) |
| Producer gate | `crt-development-sdk-test` exit 0; `crt-c-test` 168 of 169 pass, one skip (`fork_heap_snapshot_test_runs`), 317 s; `crt-dev-release` exit 0 |
| Release asset | `crt-v0.0.1-rehearsal.3-windows-x86_64-dev.zip`, 146,453,507 bytes, SHA-256 `fc3eeb74...810b` |
| Release manifest | `release-manifest-dev-windows-x86_64.json`, SHA-256 `fe86e1d1...5886`; install metadata format 1: `zip`, root `dev`, 2,917 entries (2,736 files, 181 directories, 0 symlinks), 402,317,041 bytes, identity `075d5eb3...2c4d` |
| CLI | the **self-built, SDK-only** executable of the Windows contract replay of that SDK: SHA-256 `9948bcd9...fe2d`, 29,552,640 bytes, imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; source `2df5610` plus the edits described below (source export `90e1ec49...4ad1`) |
| CLI build | 16 sources; cold self-build 16 executed / 0 reused, warm 0 / 16; native suite 857 checks, archive test 151 and SDK-store test 66 checks; 503 recorded processes, no forbidden image; SDK-hidden help/version/doctor |

### Result: 62/62 checks

The same cases as the Linux and macOS tables, with Windows-specific hostile names, and the archive formats exchanged (the Windows
release asset is a zip, so the variant is a tar.xz of the installed SDK): install from the pinned manifest gives the promised
identity (`v0.0.1-rehearsal.3-075d5eb36d10`), one SDK directory and empty bookkeeping, list/verify pass, a reinstall is idempotent, and
the host recomputation of the identity (CRT's `crt_sdk_install_metadata`) equals the manifest's; a flipped byte in an installed file
is detected by `verify` and a reinstall refuses to replace it; a flipped archive byte, a trailing byte and a wrong pinned manifest
digest are refused with nothing published. Fourteen hostile archives are refused with no SDK, staging or lock left and nothing outside
the store: path traversal, absolute path, drive-letter path, reserved device name (`con.txt`), alternate data stream (`file.txt:stream`),
absolute and escaping symlinks, write through a symlink, hard link, device node, case collision, not-an-SDK, and the zip forms
(backslash traversal, drive letter, `NUL.txt`). Killing the installing process mid-extraction publishes nothing and leaves
recoverable leftovers; `recover` removes them, then a new install and `verify` pass. Two parallel installs with `--lock-wait`
give one new and one already-installed result, one SDK and empty bookkeeping. A read-only store is refused with nothing written, and
a tar.xz of the installed SDK installs with the identical identity. The input files are unchanged by every run.

**Windows lock-owner semantics.** Lock and transaction owners are PIDs checked with `kill(pid, 0)`. On CRT's Windows PAL it
reports a killed owner as dead (`recover` and the next install take over its leftovers) and a live installer as alive (the second
parallel install waits and then reports the SDK as already installed). The `lzma` link name resolves through the `lzma.lib` alias
the xz port installs, and the runtime is `bin/liblzma-5.dll`.

### What the replay found

Three **producer** defects stopped the Windows Development SDK before any CLI call, so they are fixed in CRT with commits
[`f9dc911`](https://github.com/webos21/crt/commit/f9dc911) and [`1ee0379`](https://github.com/webos21/crt/commit/1ee037923ef6a3460c6f99c6e426d294fe5b7025): `tools/build_dev_sdk.py` recorded the nonexistent
`lib/lzma.dll` instead of `bin/liblzma-5.dll` as the xz runtime artifact, so `verify_dist` rejected the SDK ("missing xz shared
runtime: liblzma"); the Windows shared consumer lane never linked `liblzma.dll.a`; and `crt_sdk_install_metadata` let Windows raise
`OSError` instead of `ValueError` for an escaping symlink target. Two **CLI** items were needed: `archive.cpp` and the tar fixture passed a
`std::filesystem::path` to `std::ifstream`/`std::ofstream`, which the imported libc++ rejects on Windows (the code now passes
`path.string()`, as `core.cpp` already did), and `archive_test` asserted POSIX mode bits that CRT does not report on Windows
(those checks now run on the other hosts only; the product still sets the bits where the host has them). Retained, not adopted:
a producer build that reused an older libc++ source cache (rejected by CRT as non-canonical), and two CLI replays that stopped at the compile
error and at the mode-bit assertion. The first two full installer runs were 61/62 because the driver's "nothing escaped" search
matched the legitimate libc++ header `escaped_output_table.h` of the installed SDK; the driver now looks for the exact hostile name.

### Evidence and limits

Raw evidence (logs, `result.json`, per-call stdout/stderr, process images, the failed attempts) is under the ignored
`out/windows-d2-20261104/`; `bin/win-x64/crt-cli.exe` is replaced by the self-built executable above and the previous
bytes are kept there. Windows/x64 only. This is a rehearsal release asset, not a published release. Mode bits and
execute permissions have no Windows counterpart in this record. The positive symlink path is covered only by `archive-test` (Developer
Mode is on); the Windows Development SDK contains no symlinks. Not run: Windows/arm64, a restricted-token network boundary, disk-full and
power-loss behavior, Full-stage archives and the path where Developer Mode is off.

## Linux/aarch64 isolated replay (2026-11-04)

**Pass, native Linux/aarch64; local/unpromoted.** The unchanged `tools/replay_sdk_install_linux.py` runs every CLI call in
bubblewrap as on x86_64: user/ipc/uts/net namespaces (no network), read-only root, the CRT checkouts and the CLI sources hidden,
cleared environment with an empty `PATH`, private `/tmp`, one writable directory (store plus a sentinel tree), shared PID namespace.
Nothing in the CLI changed for this replay.

### Inputs

| Input | Identity |
| --- | --- |
| Host | Ubuntu 26.04 aarch64 QEMU guest (Linux 7.0.0-42), 4 CPU, Clang/LLD/llvm-ar 21.1.8 |
| Producer | clean CRT [`ffd0405`](https://github.com/webos21/crt/commit/ffd0405924e0660b2ed775e2c15b9d6f44e47532) in a detached worktree, preset `linux-host-ninja-debug`, `CRT_RELEASE_TAG=v0.0.1-rehearsal.4` (not a Git tag, nothing uploaded) |
| Producer gate | `crt-development-sdk-test` exit 0; `crt-c-test` 147/147, no skips; `crt-dev-release` exit 0 |
| Release asset | `crt-v0.0.1-rehearsal.4-linux-aarch64-dev.tar.xz`, 9,065,788 bytes, SHA-256 `4058bcae...b9dd` |
| Release manifest | `release-manifest-dev-linux-aarch64.json`, SHA-256 `0cd62bf7...9cbe`; install metadata format 1: `tar.xz`, root `dev`, 2,797 entries (2,602 files, 178 directories, 17 symlinks), 206,258,306 uncompressed bytes, identity `0d11df55...8c48` |
| CLI | the **self-built, SDK-only** executable of the Linux/aarch64 contract replay of that SDK: SHA-256 `fc7842f3...172c`, 15,974,328 bytes, no `DT_NEEDED`, interpreter `/lib/ld-linux-aarch64.so.1`; source `02c6735` |
| CLI build | 16 sources; cold self-build 16 executed / 0 reused, warm 0 / 16; native suite 779 checks, archive test 158 and SDK-store test 66 checks; 24 distinct executed paths in the strace audit, none forbidden; SDK-hidden help/version/doctor; SDK inventory unchanged |

### Result: 48/48 checks

The same cases as the x86_64 table: the install from the pinned manifest gives the promised identity (`v0.0.1-rehearsal.4-0d11df55b226`),
one SDK directory and empty bookkeeping; list/verify pass; a reinstall is idempotent; the host recomputation of the identity equals the
manifest's; a flipped byte in an installed file is detected by `verify` and a reinstall refuses to replace it; a flipped archive byte, a trailing
byte and a wrong pinned manifest digest are refused with nothing published; the hostile archives (traversal, absolute path, absolute and
escaping symlinks, write through a symlink, hard link, device node, case collision, not an SDK) are refused with no SDK, staging or lock left and
nothing outside the store; killing the installer during extraction publishes nothing, `recover` removes the leftovers and a new install and
`verify` pass; two parallel installs with `--lock-wait` give one new and one already-installed result with one SDK directory; a read-only store is
refused; and a zip of the installed SDK installs with the identical identity. The SDK ships 17 symlinks and they survive the
install (the Windows record has none).

### Evidence and limits

Raw evidence (`replay/result.json`, `d2/result.json`, logs, per-call output) is under the ignored `out/linux-arm64-d2-20261104/`;
`bin/linux-arm64/crt-cli` is replaced by the self-built executable above and the previous bytes are kept there. This is a rehearsal release asset, not
a published release. The producer commit contains the aarch64 `faccessat` access-mode fix (`a48000c`). Not run: Windows/arm64, Full-stage archives,
disk-full and power-loss behavior, a published release.

## Windows/arm64 isolated replay (2026-11-04)

**Pass, native Windows/arm64; local/unpromoted.** The unchanged `tools/replay_sdk_install_windows.py` runs every CLI call under CRT's
`crt_windows_trace` as on Windows/x64: a restricted token naming a per-run SID, an empty `PATH`, redirected stdio and a job object that records every
process. The private work directory (Hangul and spaces in its name) holds the store, a sentinel tree and the hostile archives, and a native probe built
with the SDK under test proves the CRT checkout, the CLI sources and the producer clone unreadable under the run SID. As on x64, network denial is not
claimed. No CLI code changed for this replay.

### Inputs

| Input | Identity |
| --- | --- |
| Host | Windows 11 arm64 VM (QEMU, 4 vCPUs, 10.0.26300), LLVM/Clang 22.1.8, Windows SDK imports `10.0.28000.0/um/arm64`, Developer Mode on (not changed); native arm64 shell and Python (an x64 Git Bash configures the x86_64 sources) |
| Producer | clean CRT [`b20a460`](https://github.com/webos21/crt/commit/b20a4604cc131362c2bc564d2263d1676b3a0c5d) in a detached worktree at a short ASCII path, `CRT_RELEASE_TAG=v0.0.1-rehearsal.6` (not a Git tag, nothing uploaded) |
| Producer gate | `crt-development-sdk-test` exit 0 (1,669 s); `crt-c-test` 169 of 169, one skip (`fork_heap_snapshot_test_runs`); `crt-dev-release` exit 0 |
| Release asset | `crt-v0.0.1-rehearsal.6-windows-aarch64-dev.zip`, 141,785,359 bytes, SHA-256 `7b63ccdd...7bec` |
| Release manifest | `release-manifest-dev-windows-aarch64.json`, SHA-256 `488c2ac6...5f95`; install metadata format 1: `zip`, root `dev`, 2,917 entries (2,736 files, 181 directories, 0 symlinks), 398,216,734 bytes, identity `1239f18c...b043b37` |
| CLI | the **self-built, SDK-only** executable of the contract replay of that SDK: SHA-256 `3f587b71...2c52`, 29,425,664 bytes, imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; source is this repository at `13bba78` (driver source inventory `dcb82a50...a90a`) |
| CLI build | 16 sources; cold self-build 16 executed / 0 reused, warm 0 / 16; native suite 870 checks, archive test 151 and SDK-store test 66 checks, all three test executables exit 0; 471 recorded processes over the contract replay, only the declared native tools, the CLI and its generated executables (`clang`, `clang++`, `ld.lld`, `llvm-ar`, `conhost`); SDK-hidden help/version/doctor; SDK inventories unchanged |

### Result: 62/62 checks

The same cases as the Windows/x64 table: install from the pinned manifest gives the promised identity (`v0.0.1-rehearsal.6-1239f18c392c`), one SDK
directory and empty bookkeeping, list/verify pass, a reinstall is idempotent and the host recomputation of the identity equals the manifest's; a flipped
byte in an installed file is detected by `verify` and a reinstall refuses to replace it; a flipped archive byte, a trailing byte and a wrong pinned manifest
digest are refused with nothing published. Fifteen hostile archives are refused with no SDK, staging or lock left and nothing outside the store: path
traversal, absolute path, drive-letter path, reserved device name, alternate data stream, absolute and escaping symlinks, write through a symlink, hard link,
device node, case collision, not-an-SDK, and the zip forms (backslash traversal, drive letter, reserved device name). Killing the installing process
mid-extraction publishes nothing and leaves recoverable leftovers; `recover` removes them, then a new install and `verify` pass. Two parallel installs with
`--lock-wait` give one new and one already-installed result with one SDK and empty bookkeeping. A read-only store is refused with nothing written, and a
tar.xz of the installed SDK installs with the identical identity. The input files are unchanged by every run.

### What the replay found

One **producer** defect stopped the replay before the installer was reached, and it is fixed in CRT with commit
[`b20a460`](https://github.com/webos21/crt/commit/b20a4604cc131362c2bc564d2263d1676b3a0c5d). The contract replay of the first producer (CRT `e3deb32`,
`rehearsal.5`) self-built the CLI and passed 858 native checks, but `archive-test` and `sdk-store-test` aborted (exit 134) with `libc++abi: terminating due to
uncaught exception of type crtcli::Error` at the first `unsafe archive path` they were meant to catch. The throwing frame's caller was a bogus address: the
DWARF CFI of `extract_archive`, which allocates about 149 KB through `__chkstk`, still said `CFA=sp+96` after the `sub sp, sp, x15, lsl #4`. clang enables
asynchronous unwind tables by default for aarch64-w64-mingw32 once `-fhosted` is given (the CLI's native driver compiles programs that way), and LLVM 22.1.8
then omits the CFA adjustment for stack-probed frames; with `-fno-asynchronous-unwind-tables` it emits it, and x86_64 always does. The CRT wrappers now pass
that flag for Windows/aarch64, so the policy the exporter records for the CLI carries it. This is a toolchain issue rather than a CLI one: any CRT-native
program with a frame above the guard page and an exception passing through it was affected. Retained, not adopted: the first producer and its contract
replay (`contract-first-attempt-failed`), and a producer release that was refused because the earlier release output existed (moved aside, not replaced).

### Evidence and limits

Raw evidence (logs, `result.json`, per-call stdout/stderr, process images, the failed first attempt) is under the ignored
`out/windows-arm64-d2-20261104/`; `bin/win-arm64/crt-cli.exe` is replaced by the self-built executable above and the previous bytes (`6fce97e0...787c`) are kept
under `previous/`. Windows/arm64 only. This is a rehearsal release asset, not a published release. Mode bits and execute permissions have no Windows
counterpart in this record; the Windows Development SDK contains no symlinks, so the positive symlink path is covered only by `archive-test`. The CLI
executable is PE machine `0xAA64`; Strawberry Perl (an emulated x64 build) was on `PATH` for the producer builds. Not run: a restricted-token network
boundary, disk-full and power-loss behavior, Full-stage archives and the path where Developer Mode is off.

## Not verified

A release
candidate that is an actual published release; Full-stage archives; disk-full and
power-loss behavior; very large archives beyond the 186 MB dev SDK.

## Toolchain contract and project SDK selection replay (Stage 2, Linux/x86_64, 2026-11-05)

The replay scripts share the cases in `tools/replay_sdk_contract_cases.py`: the contract the release ships is stored beside
the SDK, a project selects the only installed SDK and its contract with no `--sdk` and no `--sdk-contract`, the toolchain
builds, tests and runs, and damage is caught. The store is now `$HOME/.crt/sdks` of the sandbox (HOME is writable
scratch), because `configure` looks there.

| Input | Value |
| --- | --- |
| Release | `crt-dev-release` rehearsal `v0.0.1-rehearsal.3` (scratch clone, nothing uploaded, **uncommitted code**), archive + toolchain contract + manifest |
| CLI | built from this checkout's sources by the committed `bin/linux-x64/crt-cli` against a local, dirty-marked Dev SDK (`--allow-dirty-sdk true`); not a CLI built from the SDK under test |
| Isolation | bubblewrap as above; the project workflow runs the host `clang`, `ld.lld` and `llvm-ar` read-only from the root |

Result: **69/69** (the earlier 48 plus 21 new cases), evidence outside the repositories. New cases: the installation kept the
contract byte for byte; `sdk list` reports it; `configure` selects the only SDK and records identity, contract digest and
`contract_source: store` with no SDK or contract path and no machine path in the lock; `build`, `test`, `run`; a modified
stored contract is refused by the lock and works again once restored; a missing one is reported; `--os-sdk` is refused where
the contract needs none; a release without its contract asset, with a flipped byte and with a trailing byte is refused with
nothing published. Known limit: `sdk verify` does not cover the stored contract (a changed contract is caught by a locked
project, not by the store); a project that runs `configure` again would lock the changed bytes.

### Other hosts

`replay_sdk_install_macos.py` and `replay_sdk_install_windows.py` run the same cases but were **not executed** on their hosts
in this change; they are written from the Linux script and checked only for syntax. Both now need the native tools
(`--cc --cxx --linker --ar`), and the release must come from a collector that had the OS SDK at collection time (Xcode;
`CRT_WINDOWS_SDK_LIBPATH`). On Windows the same variable must be set for the replay itself, and the process accounting
accepts images from the tools' directories and the scratch tree during project calls. The macOS case expects the
`macos-xcode-sdk` kind, the Windows case `windows-um-import-libraries`; discovery (`xcrun`, the variable) runs inside the
sandbox, so a Seatbelt or token restriction that blocks it shows up as a failed `configure`, not as a skipped case.
