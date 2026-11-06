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
nothing published. Known limit (closed the same day, see "`sdk verify` covers the stored contract" below): `sdk verify` did not cover the
stored contract (a changed contract is caught by a locked project, not by the store); a project that runs `configure` again would lock the changed bytes.

### Other hosts

`replay_sdk_install_macos.py` and `replay_sdk_install_windows.py` run the same cases but were **not executed** on their hosts
in this change; they are written from the Linux script and checked only for syntax. (Update 2026-11-05: the macOS script was
then executed, see the macOS Stage 2 record below.) Both now need the native tools
(`--cc --cxx --linker --ar`), and the release must come from a collector that had the OS SDK at collection time (Xcode;
`CRT_WINDOWS_SDK_LIBPATH`). On Windows the same variable must be set for the replay itself, and the process accounting
accepts images from the tools' directories and the scratch tree during project calls. The macOS case expects the
`macos-xcode-sdk` kind, the Windows case `windows-um-import-libraries`; discovery (`xcrun`, the variable) runs inside the
sandbox, so a Seatbelt or token restriction that blocks it shows up as a failed `configure`, not as a skipped case.

## Toolchain contract and project SDK selection replay (Stage 2, Linux/aarch64, 2026-11-05)

**Pass, native Linux/aarch64; local/unpromoted.** The unchanged `tools/replay_sdk_install_linux.py` (with the shared
`tools/replay_sdk_contract_cases.py`) runs in bubblewrap exactly as on x86_64; the store is the sandbox `$HOME/.crt/sdks`, and the
project workflow runs the host `clang`, `ld.lld` and `llvm-ar-21` read-only from the root. Nothing in the CLI changed for this replay.

| Input | Value |
| --- | --- |
| Host | Ubuntu 26.04 aarch64 QEMU guest (Linux 7.0.0-42), 4 CPU, Clang/LLD/llvm-ar 21.1.8 |
| Release | `crt-dev-release` rehearsal `v0.0.1-rehearsal.5` of **committed** CRT [`78ed80f`](https://github.com/webos21/crt/commit/78ed80f20db967ddb5e9af90462705327fe61d35) (detached worktree, nothing uploaded): `crt-development-sdk-test` and `crt-dev-release` exit 0, `crt-c-test` 147/147. Assets: archive `crt-v0.0.1-rehearsal.5-linux-aarch64-dev.tar.xz` (9,059,308 bytes, SHA-256 `89f6437e...aa70`), contract `...-dev-toolchain.json` (403,527 bytes, SHA-256 `8dbb76b9...e9df`), manifest SHA-256 `cf246f85...7ca5`; install metadata format 1: 2,797 entries (2,602 files, 178 directories, 17 symlinks), identity `9f8f07ac...8191` |
| CLI | the **self-built, SDK-only** executable of the Linux/aarch64 contract replay of that SDK, built from this checkout (`9d711d2`): SHA-256 `69118ac1...bfef`, 16,352,720 bytes, no `DT_NEEDED`; cold self-build 16 executed / 0 reused, warm 0 / 16; 841 native, 158 archive and 83 SDK-store checks; 24 distinct executed paths, none forbidden; SDK inventory unchanged |

Result: **69/69** (the earlier 48 plus the 21 Stage 2 cases), first run: the installation kept the contract byte for byte; `sdk list`
reports it; `configure` selects the only SDK and records identity, contract digest and `contract_source: store`, with no SDK or
contract path and no machine path in the lock; `build`, `test` and `run` use the stored contract; a modified stored contract is
refused by the lock and works again once restored; a missing one is reported; `--os-sdk` is refused because the Linux contract needs
none; a release without its contract asset, with a flipped byte and with a trailing byte in it is refused with nothing published.
Compared with the Linux/x86_64 record, the CLI here is built from the SDK under test rather than by a committed binary against a
dirty-marked SDK, and the release comes from committed code. The limit noted there still holds: `sdk verify` does not cover the stored
contract.

Evidence (ignored): `out/linux-arm64-s2-20261105/` (`replay/result.json`, `d2/result.json`, `d2.log`). Not run here: the macOS and Windows scripts.

## Toolchain contract and project SDK selection replay (Stage 2, macOS/arm64, 2026-11-05)

**Pass, native macOS/arm64; local/unpromoted.** `tools/replay_sdk_install_macos.py` with the shared
`tools/replay_sdk_contract_cases.py` runs every CLI call under Seatbelt (no network, writes only to the scratch and private
temporary directories, the CRT and CLI checkouts and the producer worktree denied, cleared environment with an empty `PATH`).
The store is the sandbox `$HOME/.crt/sdks`; the project workflow runs the Xcode `clang`, `clang++`, `ld` and `ar` from the real
root, and `configure` discovers the OS SDK with `xcrun` inside the sandbox.

| Input | Value |
| --- | --- |
| Host | native arm64, macOS 27.2 |
| Release | `crt-dev-release` rehearsal `v0.0.1-rehearsal.6` of **committed** CRT [`784389b`](https://github.com/webos21/crt/commit/784389b) (detached worktree, nothing uploaded): archive (6,675,764 bytes, SHA-256 `bf0d92a1...c1a9`), toolchain contract `crt-v0.0.1-rehearsal.6-macos-aarch64-dev-toolchain.json` (402,864 bytes, SHA-256 `200d9a10...faec`, `os_sdk.kind` = `macos-xcode-sdk`), 02/03 stage-source assets; manifest SHA-256 `c08c25e8...156b`; install metadata format 1, 2,786 entries, identity `72d1b375...0b4c`; all four `SHA256SUMS` entries verify |
| CLI | the **self-built, SDK-only** executable of the macOS contract replay of that SDK, built from this checkout (`da3f3a9`): SHA-256 `acb4f7e1...5b62`, 3,927,832 bytes, imports only `libSystem`; cold self-build 16 executed / 0 reused, warm 0 / 16; bootstrap loop from the previous committed binary (stage1 and stage2 identical in code and data, differing only in `LC_UUID` and `__LINKEDIT`); 869 native, 158 archive and 83 SDK-store checks |
| Producer gate | `crt-development-sdk-test` and `crt-dev-release` exit 0; `crt-c-test` **146/146** (Linux counts 147; one test is host-conditional) |

Result: **69/69**, the same cases as the Linux records: the 48 installer cases and the 21 contract and project-selection cases.
On macOS `configure` selects the only installed SDK and its stored contract, records identity, contract digest, `contract_source:
store` and the OS SDK **kind** (`macos-xcode-sdk`) in `crt.lock`, and keeps the OS SDK **location** (the Xcode `MacOSX.sdk` path)
in the ignored local state only, with no machine path in the lock; `build`, `test` and `run` use the stored contract; a modified
stored contract is refused and works again once restored; a missing one is reported; a non-existent `--os-sdk` directory is
refused; a release without its contract asset, with a flipped byte and with a trailing byte is refused with nothing published.
The limit noted for Linux holds: `sdk verify` does not cover the stored contract.

The first invocation of the macOS driver (`install-replay-r1-cwd`) failed all nine project-workflow cases with `getcwd failed:
Operation not permitted`: the driver ran the CLI from the CLI checkout, which its own profile hides, and the project workflow now
reads the current directory. This is a driver defect, not a CLI one; the driver now starts every call from the work directory.
Nothing else was changed to reach 69/69, and no CLI or producer code changed for this replay.

Evidence (ignored): `out/s2-macos/` in the CRT checkout (`install-replay/result.json`, `replay-cli/result.json`, the failed
`install-replay-r1-cwd/`) and the producer worktree `crt-s2-784389b/out/s2-macos-20261105/`. Not run here: Full-stage
install metadata, a published release asset and promotion.

## Toolchain contract and project SDK selection replay (Stage 2, Windows/arm64, 2026-11-05)

**Pass, native Windows/arm64; local/unpromoted.** This is the first execution of `tools/replay_sdk_install_windows.py` with the shared
`tools/replay_sdk_contract_cases.py`, and the first Windows collection of the portable toolchain contract. Every CLI call runs under CRT's
`crt_windows_trace` as in the D2 record (restricted token naming a per-run SID, empty `PATH`, redirected stdio, job-object accounting; no network
denial is claimed). The store is `%USERPROFILE%\.crt\sdks` of the private work directory (Hangul and spaces in its name), because `configure` looks
there; project calls may start the native `clang`, `ld.lld` and `llvm-ar`, and `CRT_WINDOWS_SDK_LIBPATH` is passed for OS SDK discovery.

| Input | Value |
| --- | --- |
| Host | Windows 11 arm64 VM (QEMU, 4 vCPUs, 10.0.26300), LLVM/Clang 22.1.8, Windows SDK imports `10.0.28000.0/um/arm64`, Developer Mode on (not changed); native arm64 shell and Python |
| Release | `crt-dev-release` rehearsal `v0.0.1-rehearsal.7` of **committed** CRT [`0631131`](https://github.com/webos21/crt/commit/0631131dbced786efff8162817b3c8764ca5f57a) (detached worktree at a short ASCII path, nothing uploaded): archive `crt-v0.0.1-rehearsal.7-windows-aarch64-dev.zip` (141,785,345 bytes, SHA-256 `976150ba...6294`), contract `crt-v0.0.1-rehearsal.7-windows-aarch64-dev-toolchain.json` (441,104 bytes, SHA-256 `08ae2c51...1d68`, `os_sdk.kind` = `windows-um-import-libraries`), manifest SHA-256 `167f3260...2b4e`; install metadata format 1: `zip`, 2,917 entries (2,736 files, 181 directories, 0 symlinks), 398,216,734 bytes, identity `c4b35201...1233` |
| Producer gate | `crt-development-sdk-test` exit 0 (2,339 s); `crt-c-test` 169 of 169, one skip (`fork_heap_snapshot_test_runs`); `crt-dev-release` exit 0 (78 s) |
| CLI | the **self-built, SDK-only** executable of the Windows contract replay of that SDK, built from this repository at `b03d127`: SHA-256 `a366b26a...21f8`, 30,089,728 bytes, PE machine `0xAA64`, imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; cold self-build 16 executed / 0 reused, warm 0 / 16; 932 native, 151 archive and 83 SDK-store checks, all three test executables exit 0; 487 recorded processes, only the declared native tools, the CLI and its generated executables; SDK inventory unchanged |

Result: **83/83** on the first run of the corrected sources (the Windows installer cases are 62, plus the same 21 contract and project-selection cases as the 69 of
the other hosts): the installation kept the contract byte for byte beside the SDK (`.contracts/v0.0.1-rehearsal.7-c4b352012b42.json`); `sdk list` reports it; `configure`
selects the only installed SDK and its stored contract with no `--sdk` and no `--sdk-contract`, and records identity, the contract digest, `contract_source: store` and the OS SDK **kind**
(`windows-um-import-libraries`) in `crt.lock`, while the OS SDK **location** (`C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/arm64`) stays in the ignored local state and no
machine path is in the lock; `build`, `test` and `run` use the stored contract; a modified stored contract is refused by the lock and works again once restored; a missing one is reported; a
non-existent `--os-sdk` directory is refused; a release without its contract asset, with a flipped byte and with a trailing byte in it is refused with nothing published. The limit noted for
Linux holds: `sdk verify` does not cover the stored contract.

### What the replay found

One **CLI test** defect, fixed in `b03d127`. The first contract replay of the Windows SDK self-built the CLI and passed 932 native and 151 archive checks, but `sdk-store-test` failed
`an SDK without a contract installs`: the test looked at the newly installed SDK as `list()[1]`, and `SdkStore::list()` sorts by directory name, which ends in the content hash. The hash of
that fixture depends on the target, so on `windows/aarch64` the new SDK was evidently not the second entry and the check read another SDK (the sort order itself was not printed; the Linux and macOS
targets pass with the positional lookup). The test now finds the SDK by its identity (`content12`); no product code changed, and the corrected sources pass all 83 store checks, which is the confirmation. Retained, not adopted: the first replay
(`contract-first-attempt-failed`). No producer (CRT) change was needed on this host, and the Windows `@OS_SDK@` discovery worked without one.

### Evidence and limits

Raw evidence (logs, `result.json` files, per-call stdout/stderr, process images, the failed first attempt, the previous binary `3f587b71...2c52` under `previous/`) is under the ignored
`out/windows-arm64-s2-20261105/`; `bin/win-arm64/crt-cli.exe` is replaced by the self-built executable above. Windows/arm64 only: the Windows/x64 run of the same script is not part of this
record. This is a rehearsal release asset, not a published release. The Windows Development SDK contains no symlinks, and there is no network denial or read-only root on this driver.

## Toolchain contract and project SDK selection replay (Stage 2, Windows/x64, 2026-11-05)

**Pass, native Windows/x64; local/unpromoted.** The second Windows execution of `tools/replay_sdk_install_windows.py` with the shared `tools/replay_sdk_contract_cases.py`, run unchanged from the Windows/arm64 record.
Every CLI call runs under CRT's `crt_windows_trace` (restricted token naming a per-run SID, empty `PATH`, redirected stdio, job-object accounting; no network denial is claimed). The store is
`%USERPROFILE%\.crt\sdks` of the private work directory (Hangul and spaces in its name); project calls may start the native `clang`, `ld.lld` and `llvm-ar`, and `CRT_WINDOWS_SDK_LIBPATH` is passed for OS SDK discovery.

| Input | Value |
| --- | --- |
| Host | Windows 11 Pro 10.0.26100 x64, LLVM/Clang 22.1.8, Windows SDK imports `10.0.28000.0/um/x64`, Developer Mode on (not changed) |
| Release | `crt-dev-release` rehearsal `v0.0.1-rehearsal.8` of **committed** CRT [`436b4f2`](https://github.com/webos21/crt/commit/436b4f2e18db4ce2ac5d1eb56d6f2c7b37606296) (detached worktree at a short ASCII path, nothing uploaded): archive `crt-v0.0.1-rehearsal.8-windows-x86_64-dev.zip` (146,110,051 bytes, SHA-256 `2575deb4...f304`), contract `crt-v0.0.1-rehearsal.8-windows-x86_64-dev-toolchain.json` (440,297 bytes, SHA-256 `e9758d59...aeab`, `os_sdk.kind` = `windows-um-import-libraries`), manifest SHA-256 `2ce6c779...30f3`; install metadata format 1: `zip`, 2,917 entries (2,736 files, 181 directories, 0 symlinks), 396,523,706 bytes, identity `e80fa143...7159` |
| Producer gate | `crt-development-sdk-test` exit 0 (1,154 s); `crt-c-test` 169 of 169, one skip (`fork_heap_snapshot_test_runs`); `crt-dev-release` exit 0 (88 s) |
| CLI | the **self-built, SDK-only** executable of the Windows contract replay of that SDK, built from this repository at `f984387`: SHA-256 `c65c6e47...5e8a`, 30,184,448 bytes, imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; cold self-build 16 executed / 0 reused, warm 0 / 16; 919 native, 151 archive and 83 SDK-store checks; 559 recorded processes, only the declared native tools, the CLI and its generated executables; SDK inventory unchanged |

Result: **83/83** on the first run (the 62 Windows installer cases plus the same 21 contract and project-selection cases as the other hosts): the installation kept the contract byte for byte beside the SDK
(`.contracts/v0.0.1-rehearsal.8-e80fa143094e.json`); `sdk list` reports it; `configure` selects the only installed SDK and its stored contract with no `--sdk` and no `--sdk-contract`, and records identity, the
contract digest, `contract_source: store` and the OS SDK **kind** (`windows-um-import-libraries`) in `crt.lock`, while the OS SDK **location** (`C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64`) stays in the ignored
local state and no machine path is in the lock; `build`, `test` and `run` use the stored contract; a modified stored contract is refused by the lock and works again once restored; a missing one is reported; a non-existent
`--os-sdk` directory is refused; a release without its contract asset, with a flipped byte and with a trailing byte in it is refused with nothing published. The limit noted for Linux holds: `sdk verify` does not cover the stored contract.

### What the replay found

Nothing needed fixing: no CRT defect and no CLI defect on this host. The `b03d127` fix (the store test looked the SDK up by position) is what let this run pass the 83 store checks on a second Windows target. The only new input was the
pinned `xz-5.8.3.tar.gz`, downloaded with the owner's permission and equal to the recipe's SHA-256.

### Evidence and limits

Raw evidence (logs, `result.json` files, per-call stdout/stderr, process images, the previous binary `9948bcd9...fe2d` under `previous/`) is under the producer worktree's ignored `out/s2-windows-x64-20261105/`;
`bin/win-x64/crt-cli.exe` is replaced by the self-built executable above. Windows/x64 only. This is a rehearsal release asset, not a published release. The Windows Development SDK contains no symlinks, and there is no network
denial or read-only root on this driver. Full-stage install metadata, a published release asset and promotion are not run.

## `sdk verify` covers the stored contract (Linux/x86_64, 2026-11-05)

**Pass on Linux/x86_64 (unit level); not replayed on other hosts.** `sdk verify <dir>` used to recompute only the SDK's content identity. When
`.contracts/<dir>.json` exists it now also requires the stored file to be a regular file that parses as a supported `crt.experimental.native-toolchain`
format 1 contract, to name no machine path (`external_os_sdk`), and to carry `sdk_content_sha256` equal to the verified identity of that SDK. Each failure
is reported with the SDK directory and the reason ("unreadable", "not a supported contract", "names a machine path", "belongs to another SDK"). An SDK
installed without a contract verifies as before.

What this does **not** check: the contract's own digest. The store does not record it; the project lock does (`contract_sha256`), so a contract that is
valid for the SDK but different from the locked one is still caught only by `build`/`test`/`run` of the locking project, and a repeated `configure`
would lock the new bytes. Closing that needs the store to record the digest at install time, which is a later decision.

Evidence: `sdk-store-test` 89 checks (83 before; six new: intact contracts accepted, machine path, other SDK, unsupported format, unreadable, restored),
`native-test` 829 and `archive-test` 158, all green in a project build with the self-built Linux/x86_64 CLI against the current Development SDK
(`--allow-dirty-sdk true`, explicit contract; a developer check, not an SDK-only replay). The replay scripts were not changed or rerun for this
increment, and `bin/*/crt-cli` was not refreshed.

## Installation receipts and explicit repair (Linux/x86_64, 2026-11-05)

**Pass for the bounded Linux developer candidate.** This implements the next
local-store integrity increment after the first-user-flow planning review.
Original five-pair Stage 2 results keep their original clean SDKs/binaries;
this is neither a new common-source matrix nor production adoption.

### Inputs and provenance

Reviewed producer `6f2892d` and CLI `4aa9a8198c71fd04b4153c6304e9a4e61e485605`.
The implementation is an uncommitted CLI snapshot; `out/d2-receipts-20261105/candidate.json`
retains its sorted `[relative-path,SHA-256]` input list (`src`, `include`,
`crt.json`), compact JSON hash, artifact/provenance data and isolated results.

| Input | Identity |
| --- | --- |
| CLI source snapshot | `e380f275fffe299fcb6ab1fde1bea641e44ace9ef55c230554f51ed148f08694` |
| Tested CLI | `68f0535d03ec40d1ea719c8f9f214a1a884c7afeec30eff10bc0bc135cb553a7`, 16,661,360 bytes |
| Copied SDK content | `ecbb920a8e63e337095c0839b2d897f653f012162164a1b7b4e85a66486b32cd`, version `development`, native dev |
| Portable provider | `30e67672117ae9b8d3b3d9476f381c3ec4049291bb593208abe541966953d956` |
| Local fixture manifest | `14e77d7cdf3e43e2931eac61a365a0d4734f639b29614333007bbb162d21316a` |
| Fixture archive | `crt-development-linux-x86_64-dev.tar.xz`, 9,228,316 bytes, `76453fe2cbf8c82aeab5d750ec275b682a54c6ad309ff99950d9bd4d61643337` |

The already built SDK is copied from CRT's `out/linux-host-ninja-debug/dist/dev`
to an external handoff under `crt-cli-replay/d2-receipts-d2x4hb21/SDK space 한글/dev`.
Its producer/base/compile snapshot is `b6bf52be64047e718dc0f2870472a5ffece913f9`
with **working_tree_dirty: true**. Build and replay explicitly opt in to that
provenance; it is not relabeled clean. It already provides liblzma. No CRT
runtime/Web/GFX rebuild is needed for this consumer metadata change.

The archive/manifest are local regression fixtures assembled with existing
archive/install-metadata/export tools from those exact SDK bytes. They are not
`crt-dev-release` outputs, official publisher metadata, release candidates or
promoted assets. The provider is 402,956 bytes. Compiler/linker/archiver
identities and exact commands are retained in the configure/test receipts.
SDK/runtime, install metadata, native-provider and project-lock formats do not
change; [the receipt format](../reference/bootstrap_contract.md#installation-receipt-cli-owned-format-1)
belongs to CLI, outside the SDK inventory.

### Measured results

- `store-test-r4`: **114 checks pass**, covering original install/transaction
  behavior plus installation pins, required/absent contracts, valid-looking
  policy edits, explicit repair, unchanged payload identity, legacy receipts,
  attempted pin changes/downgrades, wrong repair identity, missing trusted
  metadata, unknown/symlinked receipts, interrupted contract addition and
  live/dead receipt writers.
- `native-test-r4`: **834 checks pass**, including a fresh configure rejecting
  valid modified policy bytes while preserving both project binding files.
  Synthetic provider/OS-SDK fixtures declare their own trusted receipts; these
  fixtures are not product migration or producer-compatibility evidence.
- Producer's existing metadata/exporter fixtures: **14 checks pass**. They
  retain the format-1 archive/identity/contract handoff, without a new SDK schema.
- `isolated-r1`: **84/84 pass** via `tools/replay_sdk_install_linux.py` and
  shared contract cases: actual install/list/verify, idempotence, stored-contract
  project build/test/run, receipt tamper/absence, explicit repair, preserved
  project pins, hostile archives, process interruption/recovery, concurrent
  installs, read-only store and zip identity. Rejections publish no accepted
  installation; incomplete metadata is not trusted automatically.
- `isolated-payload-delta`: **3/3 pass** under the same sandbox. Even with
  trusted release inputs, repair refuses a changed installed SDK payload and
  preserves its bytes plus receipt/contract; verify succeeds after the fixture
  restores the payload. These focused cases are also added to the shared replay
  helper for future hosts; the unchanged 84-case run was not repeated.
- ELF audit: no DT_NEEDED or RPATH/RUNPATH; interpreter is the declared OS
  `/lib64/ld-linux-x86-64.so.2`. No new runtime dependency was introduced.

The isolated work is `crt-cli-replay/d2-native-prb94y6e/`. Bubblewrap hides CRT,
CLI, the frozen producer worktree and the original build/SDK/provider/fixture
handoff, denies network, clears environment/PATH and exposes read-only inputs
plus private writable scratch. Installed SDKs are reconstructed from the copied
archive. A shared PID namespace preserves lock-owner liveness semantics.
`isolated-invocation-r1.json` records the full command, including explicit
`--allow-dirty-sdk`; `isolated-r1/result.json` retains that opt-in.

The source export is built with the committed CRT-native seed and the copied
SDK, then the resulting CLI explicitly configures lock format 1 and drives its
own tests. `store-test-r2` retains the old-seed format-0 binding refusal;
`store-test-r3` retains a wrapped-fixture identity-probe mistake, corrected in
the test helper before final execution. No product fallback or SDK mutation
was used to hide those failures. Root locks, ignored default SDK selection and
committed `bin/*` binaries are unchanged by this increment; use the tested
candidate at `source/out/native/crt-cli` in the handoff above.

### Remaining scope

The subsequent [macOS replay](#installation-receipts-and-explicit-repair-macosarm64-2026-11-05)
passes. Replay the receipt/repair delta on native Windows using existing
identified SDKs, especially metadata replacement and writer recovery. Do not
substitute the earlier Stage 2 runs for that new evidence. Installed-SDK host
path/tool/DLL collisions remain a separate D2 task. Automatic first build,
remote acquisition, generic profiles, official trust/promotion and self-upgrade
remain planned. This result closes the Linux receipt implementation/validation
slice, not all D2 or the first-user-flow preview.

## Installation receipts and explicit repair (macOS/arm64, 2026-11-05)

**Pass for the bounded native receipt/repair replay.** This exercises the Linux
implementation on macOS 27.2 (26B123), arm64, using existing identified SDK
bytes. It does not replace the original Stage 2 qualification or close all D2.

### Inputs and provenance

Reviewed CRT handoff `c3daf0888882a70883b713b38728c8e54d8a8f9d` and CLI
`502992e76e58f271114dc4abf97930cd5979d17d`. The sorted `src`, `include`,
`crt.json` input snapshot is
`e380f275fffe299fcb6ab1fde1bea641e44ace9ef55c230554f51ed148f08694`,
identical to the Linux receipt increment. No product/test source fix is needed.

| Input | Identity |
| --- | --- |
| Tested CLI | `4e5ac5236fe81dac2dc94565909c95cbdc9cda051c6e3e12f09d11b083cd558d`, 3,946,616 bytes |
| SDK | Clean CRT `784389b0aaad10545598eda4d2b7e3134e640a71`, `v0.0.1-rehearsal.6`, native dev |
| SDK content | `72d1b37593abb2f5dd9d58a8338f44191b0b2023d6cbf2621cf7cd688ce80b4c` |
| Original archive | `crt-v0.0.1-rehearsal.6-macos-aarch64-dev.tar.xz`, 6,675,764 bytes, `bf0d92a1948e8eb44b674bdf0cd6cf1c120fe2d2ede0735b231ad5712c26c1a9` |
| Original portable contract | `crt-v0.0.1-rehearsal.6-macos-aarch64-dev-toolchain.json`, 402,864 bytes, `200d9a105283deaa6c23b31e451555208a930de23aaa4990efc246d5f070faec` |
| Original release manifest | `release-manifest-dev-macos-aarch64.json`, 1,638 bytes, `c08c25e8abd3ba71d90db89690db40f5ccec8001402675f9d82a9991213d156b` |
| Explicit self-build provider | `6ffbf5560c4932e6cc509056feea11cb4d6734141f063618849674558641d2e7` |

The SDK is copied from CRT's retained `out/s2-macos/dev`, with symlinks
preserved, to an external `SDK space 한글/dev`. It is read-only during build;
both inventories remain unchanged. The SDK already provides liblzma. The
committed macOS seed (`acb4f7e1...15b62`) cold-builds the current CLI using
the SDK's explicit native policy. The self-build provider records a local
Xcode SDK path; the separate original release contract is portable, declares
`os_sdk.kind=macos-xcode-sdk`, and is what installation/repair pins.
No dirty-SDK opt-in, fresh runtime build or newly assembled release fixture is
used. Xcode Clang 21, `ld` and `ar` are explicit external tools; full paths,
SHA-256s and command arrays are retained in `candidate.json` and build receipts.

### Measured results

- SDK-only self-build: cold **16 executed / 0 reused**, warm **0 / 16**.
  `sdk-store-test` passes **114 checks**, including required/absent contract
  pins, immutable known pins, legacy and malformed/symlinked receipts, explicit
  repair, payload preservation and live/dead receipt/contract temporary writers.
- `tools/replay_sdk_install_macos.py` with the current shared contract cases:
  **87/87 pass**. Actual install/list/verify, stored-contract project
  new/configure/build/test/run, valid-looking policy edits, fresh-configure
  lock preservation, original-release repair and legacy recovery all pass.
  The payload refusal cases included in this run leave the changed payload and
  original metadata untouched; verify succeeds after fixture restoration.
  Hostile archives, interruption/recovery, concurrent installs, read-only-store
  rejection and ZIP identity also pass. This is the Linux 84-case replay plus
  its three later payload-preservation cases, now executed together.
- Existing CRT metadata/exporter fixtures: **14/14 pass**. No producer schema
  or SDK identity changes. Unchanged full native/archive/runtime suites are
  not rerun; the affected fresh-configure guards execute in the isolated cases.
- Mach-O audit: only `/usr/lib/libSystem.B.dylib` is imported, no RPATH,
  CRT-owned malloc and no foreign binding of audited CRT symbols. A bare
  doctor run succeeds with source/SDK/provider reads and compiler execution denied.

Self-build Seatbelt policies deny network, original CRT/CLI/producer-tree
access and SDK writes; execution is restricted to the declared tools, CLI,
boundary probe and generated native outputs. Boundary probes confirm source
reads and undeclared shell/Python/make/CMake/Ninja execution are denied.
The separate installation driver uses its existing Seatbelt profile: hidden
original inputs, no network, cleared environment/empty PATH and writes only to
scratch/private temp (plus `/dev`); it does not impose an exec allowlist.
Its private HOME supplies `$HOME/.crt/sdks`; external Xcode tools and OS-SDK
discovery are allowed. No real user SDK store or global environment is changed.

### Evidence, retained failures and limits

Evidence is in CRT's ignored `out/d2-receipts-macos-20261105/`:
`candidate.json`, `build-r2/`, `isolated-invocation.json`, logs and
`isolated/result.json` (SHA-256
`c580cd5b380b08131e03db6d0b79fd45a4237a66099f64619a2e25294487b39f`).
Self-build handoff:
`/private/var/folders/p3/z_1_279n4bq6kgrrs25d8bdc0000gn/T/crt-receipts-macos-qml94nju`;
installation work: `/private/tmp/crt-receipts-macos-20261105/d2-native-11s_4o3a`.
The tested candidate is `bare/crt-cli` in the self-build handoff, not a refreshed
committed `bin/macos` snapshot.

Initial local build-harness attempts failed `Clang resource directory query`
when stdout/stderr log FDs pointed into the denied original CRT tree. The same
policy succeeds with pipe-captured output or logs in the visible work tree.
The accepted continuation captures pipes in the host checker, then writes
evidence outside the child. Failed attempts and diagnostic comparisons are
retained; no sandbox rule, product code or SDK byte was relaxed to pass.

Native Windows receipt replay remains open, as do installed-SDK path/tool/DLL
collisions, automatic first build/acquisition and production trust/promotion.
This is neither a latest-CRT-runtime qualification nor a five-pair receipt
matrix. Root locks, defaults, committed binaries and STATUS are unchanged.

## Installation receipts and explicit repair (Windows/x64, 2026-11-05)

**Receipt/repair boundaries pass; the whole Windows replay remains open after
one concurrent-install native crash.** This is native Windows/x64, OS build
10.0.26100. It is not a five-pair receipt qualification or current-CRT runtime
rebuild. The first failed run remains failed even though a continuation passes.

### Inputs and identities

Reviewed CRT `dc62e8db883ad0218b32c442915d7d76fe7f5b5e` and CLI
`51f1dc092444b529cbddb01785727f8dbe9ff0b8`. Product/test input snapshot
(`src`, `include`, `crt.json`, normalized LF, sorted path/hash compact JSON):
`e380f275fffe299fcb6ab1fde1bea641e44ace9ef55c230554f51ed148f08694`,
identical to implementation `502992e` and the Linux/macOS increment.

| Input | Identity |
| --- | --- |
| Tested CLI | `086fba31a9beffb41faea65ac33b415d6c08a2e593b82064af04b1676b932cd8`, 30,440,960 bytes |
| SDK producer | Clean CRT `436b4f2e18db4ce2ac5d1eb56d6f2c7b37606296`, native dev, `v0.0.1-rehearsal.8` |
| Installed SDK identity | `e80fa143094eae0338716e0d1419bda07ea455ceadc21e0309c7e88622487159` |
| Original archive | `crt-v0.0.1-rehearsal.8-windows-x86_64-dev.zip`, 146,110,051 bytes, `2575deb4cacc3144dbd9ad03165a5a2b85f9a1e597abd61525bfeb44d42cf304` |
| Original portable contract | `crt-v0.0.1-rehearsal.8-windows-x86_64-dev-toolchain.json`, 440,297 bytes, `e9758d599f359949274ef69bd378b84a4973190970adfaf1fdf5984adbc9aeab` |
| Original release manifest | `release-manifest-dev-windows-x86_64.json`, 1,696 bytes, `2ce6c779b83a0174cd57e7cd2b5539b592278a38cfd28578067df8b9070730f3` |
| Explicit self-build provider | `aec144c03bbc3aa83cdbaa77d698096a54095117de910d9483167006ec4e7146` |

Reused the producer's retained
`C:/Users/Lee/s2/c/out/s2-windows-x64-20261105/build/dist/dev`
and original `build/release/dev` inputs. No source/SDK bytes, release inputs or
provenance were relabeled. The self-build provider has its local Windows SDK
path; installation and repair use the original portable release contract,
`os_sdk.kind=windows-um-import-libraries`. LLVM 22.1.8 is explicitly selected
under `C:/Program Files/LLVM/bin`; Windows import libraries are
`C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64`.
Exact tool/resource hashes and argv remain in the build receipts. No dirty-SDK
opt-in or newly assembled release fixture is used.

### Measured scope

- SDK-only CLI self-build: cold **16 executed / 0 reused**, warm **0 / 16**.
  `test --project <exported-source> --target sdk-store-test --json` passes
  **113 checks**, including required/absent pins, immutable known pins,
  malformed/legacy receipts, explicit repair, payload preservation and
  live/dead receipt/contract temporary writers. The Windows source branch
  excludes the symlink-receipt unit check, so 114 is not claimed.
- A separate real Windows symlink fixture passes **3/3**: regular receipt
  verifies, a host-created symlink to identical valid receipt bytes is refused,
  then the restored regular receipt verifies. The copied SDK inventory and
  link-target bytes remain unchanged. This uses the actual self-built CLI
  under a restricted token; host Python only prepares the fixture. No security
  setting or Developer Mode is changed.
- Producer metadata/exporter fixtures: **14/14**. Bare help/version/doctor and
  CRT allocation ownership pass with source/SDK/provider hidden. Imports are
  only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`. The scoped build
  records 101 processes, limited to declared tools, CLI/probe and generated
  executables. Original and copied SDK inventories are unchanged.
- The ordinary isolated replay records **93 passing checks** through
  interrupted-install recovery. These include stored-contract project
  new/configure/build/test/run, valid-looking policy edits, failed configure
  preserving locks, original-release repair, legacy recovery, payload-preserving
  repair refusal and hostile archives. It then fails in concurrent installation.
- A fresh continuation of phases 7–9 passes **9/9**: boundary probe, two
  concurrent installs (one new/one existing, one SDK, empty bookkeeping),
  read-only store refusal and tar.xz identity/verify. The probe overlaps the
  first run: there are **101 distinct cases covered across attempts**, not a
  single passing 101/101 run or a waived concurrency failure.
- Unchanged full native/archive/runtime suites are not rerun for this metadata
  delta. Earlier Stage 2 evidence keeps its own code/artifact identities.

Self-build uses a scoped copy of CRT's `replay_cli_contract_windows.py`, keeping
its restricted-token probes, process accounting, cold/warm builds and inventory
checks; only the selected native test and removal of dirty-SDK opt-in differ.
Installation uses `tools/replay_sdk_install_windows.py` with the exact rebuilt
CLI, original release directory, SDK-built probe, explicit LLVM tools and
`--hide` roots for both checkouts, producer worktree and self-build handoff.
Commands are retained in `status.json` and each phase's `commands.json`.

All consumer calls have empty PATH, private HOME/store/temp and redirected
stdio, with job-object process accounting. Original CRT/CLI/producer roots
are proven unreadable. Windows does not enforce per-image launch denial or
network denial here, and no read-only root is claimed. Installer HOME contains
spaces and Hangul; the self-build SDK path contains spaces. The original SDK
has zero symlinks. No real user SDK store or global environment is changed.

### Retained failure and next action

The first concurrent pair's `call-056.stderr` reports native `0xc0000005`
(SIGSEGV) at `0x00007ff75d2c4b83`, with empty stdout; its peer successfully
installs. The former checker then sorts `None` with a Boolean and raises
TypeError, so `isolated/result.json` is incomplete. **Do not mark it passed.**
The checker now retains raw concurrent traces, fails absent/non-Boolean values
without throwing, and includes timeout/stray-image failures. Four focused
reporting fixtures pass (success, empty crash, timeout, unknown process).
This changes reporting only; it does not fix the native failure.

The passing concurrent continuation does not establish a root cause. Five
small missing/empty/malformed/dead/live lock-owner diagnostic fixtures behave
as expected without a crash. A later base sample of the same failed executable
(`0x7ff75d190000`) would map its earlier fault to RVA `0x134b83`,
`free_unlocked` at `libc/src/malloc.c:431`. This is an **address candidate**,
not fault-time module/stack evidence or proof that the allocator is defective.
Capture a bounded concurrent reproduction with the actual module base, stack
and invalid-pointer caller before changing CLI/PAL or replacing SDK inputs.

Raw evidence stays in CRT's ignored `out/d2-receipts-windows-20261105/`:

| Result | SHA-256 |
| --- | --- |
| `build/result.json` | `e051ad19e152b16cdc3790c15d1a115192cf439b9f3aff5b3bc0d8e885286f26` |
| Failed/incomplete `isolated/result.json` | `b67427e63113e94f4a93b0a99826ca8989c8df5a280d01d5372fbc9b3f6d54e8` |
| `remaining-r2/result.json` | `a52cfd852879b7cdc9c8e41e3986aafc07da7ad6537585fa2f34a899af82626a` |
| `receipt-symlink/result.json` | `8d84db3009db54a0599838f1b964f0462c22bc90489cdd40166ad8a15382c92a` |

`candidate.json`, invocation/status/logs, scoped/continuation drivers,
`lock-owner-probe/`, `symbol-candidates.json` and `report-fixtures.json` are
retained there. Self-build handoff:
`C:/Users/Lee/AppData/Local/Temp/crt dev external lxutjgnb`;
first installation work:
`C:/Users/Lee/AppData/Local/Temp/crt receipts Windows 1793871858`;
continuation:
`C:/Users/Lee/AppData/Local/Temp/crt receipts Windows continuation 20261105 r2`.
The first local continuation omitted a helper and failed with NameError before
concurrent calls; it is preserved separately from the native failure and the
passing corrected continuation.

Windows/x64 native concurrency closure, Windows/arm64 receipt delta replay,
installed-SDK collisions and automatic acquisition remain open. No SDK/runtime,
committed binary, default selection, root/project lock, trust or STATUS refresh
or public artifact promotion is performed.

## Installed SDK selection and search-path collisions (Linux/x86_64, 2026-11-05)

This is the Linux slice of D2 / CRT 3.2. Reviewed CRT
`d856a009358dcf6391c89ee0af7390b54db2419b` and CLI base
`483e76d846808aa3391b329908c4b5883c815c66`. A content reference longer than
12 digits previously matched only the directory's first 12 digits. The bounded
reproducer accepts incorrect 13-, 32- and 64-digit references on the old CLI;
the fix checks every supplied digit against the installation receipt before
normal SDK/provider content validation. Receipt, provider, project-lock and
producer metadata formats are unchanged.

### Inputs and reproduction

| Input | Identity |
| --- | --- |
| Product/test source snapshot (`src`, `include`, `crt.json`, sorted path/hash compact JSON) | `af4627c71ee295c942ef858882a08b25753139ea385eb5b295725ace344b1268` |
| Fresh CLI | `f4b965d79326d567b12b8a9982795a5851ca1329fca4456103e0fda6dc5e9ef1`, 16,668,520 bytes |
| Reused SDK content | `ecbb920a8e63e337095c0839b2d897f653f012162164a1b7b4e85a66486b32cd` |
| Portable provider | `30e67672117ae9b8d3b3d9476f381c3ec4049291bb593208abe541966953d956` |
| Release-fixture manifest | `14e77d7cdf3e43e2931eac61a365a0d4734f639b29614333007bbb162d21316a` |

The SDK, archive and provider are the explicitly dirty local developer fixture
from [the Linux receipt increment](#installation-receipts-and-explicit-repair-linuxx86_64-2026-11-05),
not a clean release or promoted producer candidate. No runtime/source-stage
rebuild was needed. The exact SDK provenance and external LLVM/resource hashes
are retained in `candidate.json` and the self-build lock. The fresh source/SDK
handoff is `/home/syszone/GitLocal/crt-cli-replay/d2-selection-build-nesdfklg`;
its `source/out/native/crt-cli` is tested, not the committed binary.

Host-test entry point: `tools/replay_sdk_selection_linux.py` with `--cli` that
executable, `--release-dir` the receipt handoff's `release-fixture`, explicit
`--cc /usr/bin/clang --cxx /usr/bin/clang++ --linker /usr/bin/ld.lld
--ar /usr/bin/llvm-ar-21`, `--allow-dirty-sdk`, fresh `--output`/`--work-parent`,
and `--hide` for both repositories, the frozen worktree and both handoffs.
`tools/replay_sdk_selection_cases.py` owns reusable selection cases; other
native adapters still need to invoke them for their own collision acceptance.
Python, bubblewrap, readelf and the tiny host-built trap are test-host tools,
not application or CLI runtime dependencies.

### Measured scope

The SDK-only fresh CLI build executes **16 commands / 0 reused**.
`<fresh-cli> --json test --project <fresh-source> --target native-test` passes
**855 checks**, including correct/incorrect long identity references and
ambiguous-selection lock preservation (834 at the previous Linux baseline).
The bounded isolated selection replay passes **65/65**:

- Correct 8/12/13/32/64-digit references select the same exact identity;
  mismatching 13/32/64-digit references and too-short references are refused.
- Two SDKs of the same version but different real content identities both
  verify. Automatic selection refuses ambiguity, explicit selection reaches
  either identity, and the existing project builds from its original lock.
  The second SDK is a private resolver fixture: a copied payload plus a marker,
  matching inventory/provider/receipt. It is not a producer release.
- Missing/nonexistent declared compiler/linker/archiver, script substitution,
  modified locked tool bytes and ambiguous/incorrect SDK selection preserve
  both `crt.lock` and `.crt/local.json` byte for byte on refusal.
- Paths contain spaces and Hangul, including HOME/store and selected LLVM
  aliases. The working directory and PATH contain colliding native tool names;
  loader/include/library variables and CRT root variables point to fake
  runtime/header inputs. No trap executable runs. Static and shared consumers
  actually build, test and run with the selected tools and installed SDK.
- Original SDK file bytes, receipt/contract pins and release inputs remain
  unchanged. Final `sdk verify` passes.

Bubblewrap clears the environment then sets the hostile variables, denies
network access, hides source/handoff trees, exposes a read-only root and makes
only private scratch writable. Child environment filtering and explicit tool
paths are exercised; no general execution-denial or Windows DLL result is
claimed. ELF inspection finds no `DT_NEEDED` on the CLI/static consumer. The
shared consumer imports CRT `libc.so` and the declared host ELF loader, with
RUNPATH to the selected installed SDK; no glibc/GNU C++ runtime is imported.

### Evidence and limits

Ignored CLI `out/d2-selection-20261105/` retains `inputs.json`, `candidate.json`,
configure/build/native-test argv and logs, `after/result.json` and per-call
stdout/stderr/commands, ELF reports and consumer import identities. The
accepted scratch root is recorded in `after/result.json`. The accepted result
SHA-256 is `894acbd1c9f3c0a2df3a1f406db656ae8f502f99652c355f3165526eea592ad1`;
`candidate.json` binds command/import/reporting evidence digests as well.

Failed `before/` used an invalid generated-project basename; `before-r2/`
retains the three real identity-reference failures and three synthetic-fixture
failures caused by sorting generic path strings instead of CRT path components.
The fixture order was corrected without changing SDK inputs. After the passing
run, an exception-reporting guard was added to the driver; the deliberate
`harness-failure/` probe and `harness-failure-check.json` prove it emits a failed
result and exit 1. The accepted success-path driver hash and final reporting
probe hash are separately retained.

The matcher fix needs affected-host macOS/Windows replay before adoption there.
Windows/x64 concurrent-install fault-time module/stack investigation,
Windows/arm64 receipt replay and actual Windows tool/DLL collision checks remain
open. Automatic first-build binding/acquisition is the next workflow increment;
this record does not implement it or qualify default downloads. Root locks,
local defaults, committed binaries, STATUS and public artifacts are unchanged.
