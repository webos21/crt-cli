# HISTORY: CRT-CLI Completed Work

Reverse-chronological, dated record of completed work. Current and planned
steps belong to [TODO](TODO.md). Designs, procedures, and measured acceptance
keep their own detailed records under [docs](docs/README.md).

Dates identify when the work was completed or documented. Add a span when work
crosses days; include commit/reproduction references when available without
inventing commits or test results. Older months may later be summarized under
`docs/history/` with links to their original Git records.

## 2026-11-11

### Publish the Linux/aarch64 CLI Preview

Published prerelease `v0.1.0-preview.20261111.linux-arm64.1` (tag target `6a00885`, not marked latest): a static `crt-cli` built from the clean commit by the public
Linux/aarch64 Dev SDK `v0.0.1-preview.20261110.linux.arm64.2` alone (cold 21/0, warm 0/21, native 913, 32 distinct executed paths, no `DT_NEEDED`), packed deterministically with notices and a pinned
30-file inventory. The extracted package acquires the SDK from GitHub through the `github-release` redirect policy and passes C/C++ `new`/first `build`/`test`/`run`/warm build (12/12); the unauthenticated public
download matches the sums byte for byte. Not claimed: publisher authentication, the 54-case redirect replay and a sandboxed SDK/source-hidden run on this host, other glibc versions.
[Record](docs/acceptance/preview_acceptance.md#linuxaarch64-cli-preview-release-2026-11-11).

## 2026-11-10

### Consume the Linux/aarch64 Dev SDK Preview

A CLI built from `5d148ba` by the new Linux/aarch64 SDK alone (cold 21/0, warm 0/21; native 913, archive 221, SDK-store 114, port-recipe 142) passes the new bubblewrap
`tools/replay_preview_public_linux.py` 30/30 on both the pre-publication handoff and the actual public bytes of `v0.0.1-preview.20261110.linux.arm64.2` (pinned install, C/C++ first build without configure, test, run, warm build,
unchanged locks). Glibc 2.43 only; the executable is not committed and no Linux/aarch64 CLI Preview is published.
[Record](docs/acceptance/preview_acceptance.md#linuxaarch64-public-sdk-promotion-and-consumer-adoption-2026-11-10) · [ledger](docs/compatibility.md#published-linuxaarch64-dev-sdk-adoption-2026-11-10).

### Adopt the repaired Linux Dev SDK and restore old-LTS CI coverage

Adopted producer-promoted `v0.0.1-preview.20261110.4` after local SDK-only and
public-byte consumer qualification. Required Ubuntu 24.04 and 26.04 jobs now
consume the same immutable SDK pin; both pass the complete bootstrap/self-build,
913/221/114/142 suites, stage-2/warm/import/SDK-free gates in run `42805951580`.
Stage 1/2 bytes match within each host. The original `.3` calloc/loader failure
is retained, rather than treated as solved by switching runners. Driver units
pass 7/7. Product/test sources, other-host SDK/bootstrap pins and existing
project locks are unchanged; no replacement CLI executable is published.
[CI acceptance](docs/acceptance/ci_acceptance.md#linux-loader-repair-sdk-adoption-2026-11-10)
and [adoption](docs/compatibility.md#linux-loader-repair-sdk-adoption-2026-11-10)
own exact artifacts, loader/compiler versions, migration limits and receipts.

### Reduce CI latency with isolated native case processes

The native harness runs independent project/variant cases in two owned CRT
processes, while environment/store mutation stays serial. Eleven timed sections
must preserve the native check total; host-side report negatives and native
child failure/deadline controls guard against partial passes and leaked jobs.
Product code, release pins, hashing and cold/warm/import gates are unchanged.

Hosted `f49c279` run `42794932909` passes all three required legs with the
original counts (Linux 901/221/114/142, macOS 941/221/114/142, Windows
991/214/113/142). The longest job falls from 1049 to 885 seconds; Windows and
macOS driver times fall 49.2% and 13.5% in this comparison. Linux's hosted trial
is slower, so no across-host or repeat-run speed guarantee is claimed. A
same-binary local comparison preserves 901 checks and reduces 163.98 to
114.98 seconds (29.9%). Failed Windows threaded prototypes remain recorded;
the retained runner uses process isolation. Detailed scope and observations:
[CI performance acceptance](docs/acceptance/ci_acceptance.md#hosted-process-worker-qualification).

### Fix Windows CI fixtures and qualify all three required hosted legs

Passed the selected vendor OS SDK explicitly through native fixture configure,
store and first-build inputs. Pinned the reviewed recipe fixture to LF checkout
bytes and added a regression that rejects CRLF bytes; the production trust gate
and reviewed digest are unchanged. Test failures now include the structured
child message. Product code and published SDK/bootstrap pins are unchanged.

Hosted run `42782205090` at `7c9f228` passes Windows/x64 (991/214/113/142 checks),
Linux/x86_64 (901/221/114/142) and macOS/arm64 (941/221/114/142), with all three
legs required. Cold/warm rebuilds, imports and standalone execution pass.
Windows/macOS stage bytes differ and are not claimed reproducible. Exact
failure causes, identities, receipts and remaining CI limits belong to
[CI acceptance](docs/acceptance/ci_acceptance.md#windows-hosted-qualification-2026-11-10).

### Qualify Linux CI on Ubuntu 26.04 and keep it required

Moved Linux/x86_64 to explicit `ubuntu-26.04` and Ubuntu's LLVM 21 packages,
removing the noble-specific apt.llvm.org setup. Hosted run `42779857557` at
`f3d8a58` passes all four suites (901/221/114/141), both bootstrap stages,
cache reuse, import audits and standalone execution. Stage 1 and stage 2 are
byte-identical. Linux failures now fail CI; the passing runner setting is kept.
Product code, SDK/bootstrap pins and Windows/macOS configuration are unchanged.
Ubuntu 24.04's producer-side loader compatibility gap remains open. Exact
runner/tool identities, receipts and limits belong to
[CI acceptance](docs/acceptance/ci_acceptance.md#ubuntu-2604-hosted-linux-qualification-2026-11-10).

### Add GitHub CI bootstrapped from the published Preview and remove `bin/`

The repository no longer commits executables. `.github/workflows/ci.yml` (PR, `main`, manual) runs
`tools/ci_bootstrap.py`: a Preview pinned by SHA-256 in `.github/ci/pins.json` acquires the pinned public Dev SDK,
builds the checked-out sources (stage 1), the four deterministic suites run on that executable, stage 1 rebuilds
them (stage 2) and the imports and an SDK-free run are audited. The five input-gated port suites stay out because they
print `skipped` and exit 0 without their inputs. A local Linux/x86_64 rehearsal against the real downloads passes
(901/221/114/141 checks, stage 1 equals stage 2 byte for byte, no `DT_NEEDED`).
[Inputs, results, fixed driver faults and what is not run](docs/acceptance/ci_acceptance.md).

### Diagnose the first GitHub-hosted CI run

Run `42773306405`: macOS/arm64 passes every stage (941/221/114/141 checks) and becomes the required leg. Windows/x64 failed
in the driver (`TMPDIR` was not set, so the suite's scratch root fell back to the missing `/tmp`), fixed. Linux/x86_64 fails
the native suite on Ubuntu 24.04: the host glibc 2.39 loader needs `GLIBC_2.2.5` malloc-family symbols that CRT's `libc.so`
does not version, reproduced without Docker from the official ubuntu-base image. That is a producer-side gap, recorded with its
reproducer and not worked around here. The driver now continues past a failed suite and records host tool and glibc versions.
[Run record, causes and reproducer](docs/acceptance/ci_acceptance.md#first-github-hosted-run-2026-11-10).

### Publish the Windows/x64 CLI Preview with GitHub SDK acquisition

Qualified the common restricted redirect implementation on Windows (54/54), built the
CLI against the public Dev SDK alone, and published the deterministic 30-file Windows
archive. SDK-free package checks pass 12/12 and real packaged GitHub acquisition plus
C/C++ first-build/test/run/cache/lock checks pass 39/39. Public bytes match the pins;
refreshed the Windows binary and owner-requested README/STATUS snapshots in both repositories.
[Exact inputs, native results, retained driver failures and limits](docs/acceptance/preview_acceptance.md#windowsx64-cli-preview-release-2026-11-10).

### Publish the macOS/arm64 CLI Preview with restricted GitHub redirects

Replayed the restricted HTTPS redirect policy on macOS (54/54 cases over the real SDK bytes, native 942 checks) and published `v0.1.0-preview.20261110.macos-arm64.1`: a 30-file, deterministic
archive of the SDK-built executable with notices and a pinned inventory. The packaged executable acquires the macOS Dev SDK Preview directly from GitHub (three redirects, pins matched)
and builds C and C++ projects under Seatbelt; unauthenticated public bytes match. Unsigned, not notarized, no publisher authentication.
[Acceptance](docs/acceptance/preview_acceptance.md#macos-cli-preview-release-2026-11-10).

### Publish the Linux/x86_64 CLI Preview

Prerelease `v0.1.0-preview.20261110.linux-x86_64.1` from `dc796f7`, built against the public SDK `.3` acquired over the new GitHub redirect policy; the packaged executable passes acquire and C/C++ first build/test/run with an empty `PATH`. Unsigned; [record](docs/acceptance/preview_acceptance.md#linuxx86_64-cli-preview-release-2026-11-10).

### Restricted HTTPS redirects for GitHub release assets (Linux/x86_64)

`fetch` and source-driven `sdk acquire` can opt into a bounded redirect policy
(`--redirect-policy github-release` or an explicit authority list). Default remains
refusal; libcurl never follows, so the CLI re-checks every hop (HTTPS only, no
credentials/fragment/downgrade, allow-listed authority, at most 5 hops, no loops) and
reports only authorities, never a signed URL. 54/54 loopback replay cases over the real
promoted `.3` bytes, the 901-check native regression and a real GitHub `sdk acquire`
pass; see [the record](docs/acceptance/preview_acceptance.md#restricted-https-redirects-and-github-acquisition-2026-11-10).
macOS/Windows replay and the CLI executable release are not done.

### Qualify the Windows final CLI package

Assembled the committed Windows executable (`3e8c0491...`) with its pinned notices from the actual public SDK bytes of `v0.0.1-preview.20261110.windows.1` and packed them into a deterministic 30-file
`.tar.xz` (5360820 bytes, `5c9ef404...`); `package_cli_archive.py` now identifies native images by content instead of the missing Windows execute bit. The new restricted-token
`replay_cli_package_windows.py` runs the extracted package SDK- and source-free (12/12, negative controls included). Nothing is published. [Acceptance](docs/acceptance/preview_acceptance.md#windows-final-cli-package-2026-11-10).

### Qualify and adopt the published Windows Development SDK Preview

Built the CLI from `9a5272d` by the clean Release Dev SDK `v0.0.1-preview.20261110.windows.1` alone (21 sources cold 21/0, warm 0/21; native 972, archive 214, SDK-store 113, recipe 141), committed it as
`bin/win-x64/crt-cli.exe` (`3e8c0491...`) and added `tools/replay_preview_public_windows.py`, a restricted-token adaptation of the macOS consumer driver. It passes 31/31 over the local handoff and,
after the producer published the prerelease, over the actual unauthenticated public bytes. [Acceptance](docs/acceptance/preview_acceptance.md#windows-public-sdk-promotion-and-consumer-adoption-2026-11-10).

### Qualify the macOS final CLI package

Assembled the committed macOS executable with its pinned notices from the actual public SDK bytes, packed them into a deterministic 30-file archive
(1222100 bytes, `f1a20b50...a876`) and verified a safe extraction against the manifest. The extracted package runs `--version`, `--help` and `doctor --scope self`
with resources under Seatbelt with the SDK, sources and compilers hidden; wrong pins, a changed notice and an extra file are refused. The new
`tools/package_cli_archive.py` has four unit tests. Nothing is published and the archive is a local, unsigned test aid.
[Acceptance](docs/acceptance/preview_acceptance.md#macos-final-cli-package-2026-11-10).

### Consume the published macOS Dev SDK Preview

The committed macOS executable installs the actual public bytes of `v0.0.1-preview.20261110.macos.1` from the pinned manifest and builds C and C++ projects without configure under Seatbelt
(30/30). The relocation of the private lock helper into `include/crtcli/` that the self-build needed is part of the same checkpoint. See
[acceptance](docs/acceptance/preview_acceptance.md#macos-public-sdk-promotion-and-consumer-adoption-2026-11-10).

### Adopt the published Linux SDK for native manual-install consumers

After producer publication, downloaded public assets match the qualified `.3`
SDK/policy. The exact existing CLI passes pinned install/verify and C/C++ first
build/test/run/cache from those bytes in source/SDK-hidden private state.
[Public adoption evidence](docs/acceptance/preview_acceptance.md#public-sdk-promotion-and-consumer-adoption-2026-11-10)
owns the pins and receipts. The reviewed trust boundary is manual repository
pins and explicit verified HTTPS/CA; publisher authentication stays not verified.
Current redirect denial leaves GitHub automatic acquisition as a separate CLI
increment. No root/default SDK migration, executable publication or routine
STATUS update accompanies this scoped producer promotion/adoption.

### Qualify the clean Linux Dev SDK / CLI Preview inputs

Selected the clean producer `0ab8f8c` SDK `.3` after retaining and fixing the
Release/context and C++ source-fixture failures. Rebuilt committed CLI product
`024de89` from a fresh public-SDK-only export, assembled a complete one-executable
resource package and actual archive, then replayed native projects/ports,
SDK-free control, HTTPS acquisition/first build and transport/authentication
failures on that exact executable. [Preview acceptance](docs/acceptance/preview_acceptance.md#linux-local-candidate-2026-11-10)
owns the final pins, gate counts, raw receipts and limitations. No default/project
lock or committed binary changes. Source publication, official trust and producer
promotion remain the next gate; other hosts, Full/D4 and self-upgrade are separate.

### Complete the bootstrap notice closure for current CLI inputs

The Preview rehearsal found that `create_bootstrap_resources.py` predated
liblzma linkage and omitted xz's notices/recipe and the CLI's own license.
Include both in the existing format-1 complete inventory, retaining one
executable and explicit-at-fetch or optional explicit-CA data policy. Missing
xz notices refuse publication; four producer-only packaging regressions pass.
This changes data assembly, not product code, SDK compiler policy or resource
format. [Preview acceptance](docs/acceptance/preview_acceptance.md#linux-local-candidate-2026-11-10) owns final artifact evidence.

## 2026-11-09

### Adopt the bounded D3 zlib adapter contract

Reviewed the synchronized macOS affected-change evidence and the existing
Linux/Windows-x64 records against CRT `dfb83c4` / CLI `8ad11dd`. CRT records
explicit approval of the fixed zlib recipe/table/field/key policy and CLI adopts
it in [compatibility](docs/compatibility.md#d3-bounded-adapter-adoption-2026-11-09).
Only the publication-time review marker changes in product code. Added focused
regression for verification/idempotent reuse of a disposable pending manifest;
retained real pending items and a previously pinned consumer also remain usable
without manifest/lock migration. Exact new CLI/input hashes, suite results and
old-item evidence live in [acceptance](docs/acceptance/sdk_install_acceptance.md#d3-review-metadata-and-existing-item-reuse-2026-11-09).

D3 is closed for its exercised Linux/x86_64, macOS/arm64 and Windows/x64 scope;
Windows/arm64 and Linux's unendorsed C shared SDK-provider control are not added.
The current queue moves to clean Dev SDK + CLI Preview input qualification;
D4 and Full remain independent. Existing dirty developer fixtures, root locks,
committed binaries and STATUS are preserved. No public artifact or production
pin is promoted by the technical approval.

### Close the native macOS D3 common-change replay

Replayed the Linux-verified `22720f7d...bee16` product/test snapshot with the retained clean macOS rehearsal.7 SDK and
independent host tools. A fresh source-hidden build passes 29/29 gates: all six input-enabled D3 suites, 921 native
checks, SDK/source-free CLI, actual publish/reuse/verify, warm cache and unchanged original/copied inputs. Actual
provider maps pass 2/2 and binding audit passes on nine images; macOS's SDK control maps its SDK libz, unlike the
explicitly unendorsed historical Linux control. No product/recipe/SDK change, binary refresh, root lock, STATUS or
promotion. The driver adds Linux-equivalent ledger and command/bare checks with seven fast host fixtures.
[Acceptance](docs/acceptance/sdk_install_acceptance.md#macos-d3-common-change-closure-replay-2026-11-09) owns exact receipts and limits.
Final CRT approval/adoption and global D3 closure remain separate decisions.

### Replay the latest D3 common changes on Linux and prepare native macOS handoff

Fresh public-SDK-only CLI build and seven input-enabled suites pass: recipe
141, build 35, store 87, project 81, command 33, oracle 23 and native 881 checks.
The isolated driver passes 29/29 gates including actual publish/reuse/verify,
warm cache, original-tree denial, bare CLI/import and unchanged input inventories.
Reused the identified dirty developer SDK/local host tools without relabeling
provenance or refreshing binaries/root locks. The old SDK C shared control
maps host libz; this remains explicitly unendorsed, not positive SDK evidence.
[Acceptance](docs/acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09)
owns exact inputs/source/binary/results and the owner's macOS continuation.
No product, SDK or recipe change was required; formal review/closure stays pending.

### Adopt D3 closure, preview boundaries and the native handoff sequence

At the owner's request, refreshed README/TODO/design and STATUS against CRT
`21b9d0e` / CLI `2ae74d4`. Current implementation/host evidence is separated
from pending producer approval and latest common-change proof. Dev SDK + CLI
preview is separate from Full delivery; D4 amalgamation/dependency work does
not enlarge D3 or block the first preview. Added an explicit Linux/macOS
handoff procedure. Existing failures/approvals and root/artifact pins remain
unchanged; actual replay results are recorded separately in acceptance.

### Windows/x64 native zlib adapter

Implemented CRT-shell/make tool-path handling, Windows layout/OS-SDK key inputs, PE audit,
store relocation and project DLL selection. Fresh isolated contract replay and all six
input-enabled D3 suites pass, including static/shared C/C++ provider/lock proof and the
packaged oracle. Refreshed the committed Windows/x64 executable from the qualified image.
Retained earlier failures; SDK/recipe/root lock and STATUS stay unchanged.
[Exact inputs, results and limits](docs/acceptance/sdk_install_acceptance.md#source-port-d3-windowsx64-implementation-and-replay-2026-11-09).

### Windows/x64 replay of the Linux/macOS D3 increment

Reused the identified clean rehearsal.10 SDK after confirming the CRT delta is documentation only. Current product
sources match the macOS inventory: native 972, archive 214, SDK-store 113 and recipe 141 checks pass; cold/warm self-build,
SDK-free commands and PE imports pass. Fixed only Windows fixture path/home handling and added a bounded replay driver.
All five input-enabled D3 suites compile but fail at the unsupported Windows executor/layout/PE boundary; no source port,
lock or oracle equivalence is accepted. SDK/host-tools inventories, committed binaries and STATUS stay unchanged.
[Exact inputs, retained failures and continuation receipts](docs/acceptance/sdk_install_acceptance.md#source-port-d3-replay-windowsx64-2026-11-09).

### D3 macOS adapter and native source-port replay

Added OS layouts, a relocatable zlib dylib install name, declared ar/ranlib overrides and explicit OS SDK delivery;
Mach-O load-command audit and the macOS SDK-settings build-key input; project port-library selection by OS. Linux key
serialization is preserved. Native macOS source builds run through pinned CRT host tools into a separate store; both
C/C++ linkage modes, actual provider mapping, lock pins and the packaged oracle are exercised with original trees hidden.
The oracle keeps exact 15/15 static-object comparison; the documented install-name/RPATH differences are asserted.
Nine suites pass (including 921 full-native checks); provider maps pass 2/2 and binding audit passes on nine images.
The replay tool retains failures and supports input-checked test-only continuation. SDKs, upstream files, recipe bytes,
root lock, committed executables and STATUS are untouched. Exact inputs, per-suite counts, launcher/Seatbelt corrections
and remaining-host/formal-review limits: [acceptance](docs/acceptance/sdk_install_acceptance.md#source-port-d3-replay-macosarm64-2026-11-09).

### D3: `crt.lock` port pins

`crt-cli port lock [--project]` writes `ports: [{name, build_key, manifest_sha256, sdk_content_sha256}]` into `crt.lock` for every provider-`port` dependency (re-verifying each item first; removes the list when there are none). A lock with `ports` is enforced at every `build`/`test`/`run`: the declared set must equal the pinned set, keys must match, the item's `manifest.json` digest (which lists every file hash) must be the pinned one and the SDK identity must be the selected one; a lock without `ports` keeps the earlier behavior and the receipt says `pinned: false`. `configure` keeps pins for the same SDK and drops them, visibly (`port_pins`), for another. A consistently rewritten item manifest passes the store's own verification but is refused by the pin. `port-project-test` 81 checks (30 new). Experimental CLI-owned field of lock format 1. Record: `docs/acceptance/sdk_install_acceptance.md`.

### D3: `crt-cli port build|list|verify|recover`

`port build <name> --archive <file> --host-tools <dir> --host-tools-sha256 <hex> [--project <dir>]` reads the selected SDK's own `porting/recipes/<name>.json` (accepted only through the reviewed adapter table), requires the archive to be the recipe's pinned source, computes the build key first and, if that item exists, re-verifies and reports it without building; otherwise it extracts the archive, runs the executor in `ports/work/<pid>-<tick>/`, publishes, removes the work directory and prints the key and the `ports` declaration to put in `crt.json` (`--json` carries it as `declaration`). A failed build keeps its job directory and names it. `list`, `verify <key>` and `recover` act on the store. `port-command-test` 33 checks, plus the real binary run twice (published, then already published). Experimental CLI-owned surface. Record: `docs/acceptance/sdk_install_acceptance.md`.

## 2026-11-08

### D3 step 6: oracle differential

`port-oracle-test` (environment-gated) builds the reviewed zlib recipe with the native executor and with the SDK's own `tools/crt-port-build.py --sdk-root ... --test` on the same pristine source, then compares. Both install the same 8 paths with identical kinds and symlink targets; `zlib.h`, `zconf.h`, `zlib.3` are byte-identical; `libz.a` is byte-identical; `zlib.pc` is identical apart from the install prefix; both recipe tests print the same line (`compressed=3476 decoded=131072 version=1.3.1`). The only binary difference is expected: `libz.so.1.3.1` carries `$ORIGIN:<sdk>/lib` in both, and the oracle appends its own install `lib` (the `CRT_PORT_RPATH_DIR` difference recorded in step 4). The SDK is byte-identical after both builds. 23 checks. Record: `docs/acceptance/sdk_install_acceptance.md`.

### D3 step 5: explicit provider choice and consumer proof

Targets may declare `ports` with provider `sdk` or `port` (+ build key); the build re-verifies the store item, requires it to match the SDK and target, refuses one project/closure that can reach both providers, and links the item's headers and library in place of the SDK's. Path evidence: linker map, compiler dependency file and the process's own memory map name the item (and, in the SDK-provider control, not the item). The first shared run exposed that the link recipe silently dropped an extra argument (the host's libz was loaded); `Toolchain::invoke` now refuses that. `port-project-test` 51 checks. Record: `docs/acceptance/sdk_install_acceptance.md`.

### D3 step 4: binary audit, build key and port store

`port_store.h/.cpp`: in-process ELF audit (`DT_NEEDED` must resolve in the SDK or the item, runpath inside the item/SDK, SONAME aliases, relative in-item symlinks, no job paths, expected files from the adapter entry), a build key over recipe/source/adapter/SDK/toolchain-binary/host-tools/environment identities, and `PortStore` (staged copy, audit, manifest format 0, atomic rename under a per-key lock, `verify`, `recover`). The first real audit refused zlib because `crt-cc` stamps `CRT_PORT_RPATH_DIR` into every link; the executor now sets it only for the unpublished recipe tests. Lock helpers moved to `store_lock.h`. `port-store-test` 86 checks including concurrent publication and tamper detection; nothing in the CLI calls the store yet. Record: `docs/acceptance/sdk_install_acceptance.md`.

### D3 step 3: port executor

`port_build.h/.cpp` build the reviewed zlib recipe (configure, patch, make, install, both recipe tests) in a new job through the host-tools bundle and the SDK's `crt-cc` via job wrappers that name the linker; `strace` shows no host `sh` was executed. `port-build-test` 35 checks with a negative control; nothing in the CLI calls the executor yet. Record: `docs/acceptance/sdk_install_acceptance.md`.

### D3: `crt-cc` under the host-tools bundle's `mksh`

`crt-cc` compiles, links and runs under the bundle's `mksh`/`make`/applets with a bundle-only `PATH`; two executor requirements surfaced: the linker must be named explicitly (`--ld-path`, not `PATH`) and `make`'s `SHELL` cannot contain spaces. No code changed; Linux/x86_64 only. Record: `docs/acceptance/sdk_install_acceptance.md`.

## 2026-11-07

### D3 step 2: recipe subset reader and seeded adapter table

`port_recipe.h/.cpp` parse the bounded zlib-recipe subset fail-closed (digest gate first, duplicate keys, unknown keys/placeholders/OS names, env-name denylist, test sources pinned by the table, roles from the table) and reproduce the oracle's environment semantics. `port-recipe-test` 137 checks on Linux/x86_64 with a negative control; nothing calls the reader yet and CRT's review of the table is pending. Record: `docs/acceptance/sdk_install_acceptance.md`.

### D3 step 1: hardened `tar.gz` reading

`extract_archive` reads `tar.gz` (single gzip member, CRC/length trailer verified after the tar end marker, zero padding only, no trailing bytes) with the same entry rules as `tar.xz`; SDK install still refuses it. `archive-test` 221 checks, `native-test` 881, `sdk-store-test` 114 on Linux/x86_64, with a negative control. Observed and not fixed: the `tar.xz` reader does not verify the xz stream after the tar end marker. Record: `docs/acceptance/sdk_install_acceptance.md`.

## 2026-11-06

### D3 native source-port adapter: design record

Added `docs/design/source_port_adapter.md`: native executor with `crt-port-build.py` as differential oracle only, fail-closed recipe subset, roles from a CRT-reviewed table keyed by recipe digest, one provider (SDK or port) per dependency, SDK-owned `crt-cc` as `CC`, host tools only from the pinned bundle, build key and atomic store publish, and path-resolution (not content) as the consumption proof. Review found that the archive module reads only `xz`/`zip` while zlib ships `.tar.gz`, so hardened `tar.gz` reading is the first increment. Also fixed the stale TODO claim that the Windows/x64 concurrent-install crash is open. Design only: nothing implemented, no producer review of the adapter table yet.

### Windows/x64 concurrent-install SIGSEGV fixed at its root

The pinned libunwind copied only general registers into its cursor, so a catch resumed with indeterminate XMM6-XMM15; the CLI's lock-wait loop, which throws and catches per poll, then built
`std::wstring` objects from stack garbage. Fixed in the CRT libunwind recipe (CRT `05b5678`). A CLI self-built against the rebuilt SDK (`v0.0.1-rehearsal.10`) passes the installer driver 101/101 three times
(both concurrent installs exit 0 each time), selection 68/68, first build 86/86 and pinned HTTPS 74/74. The committed `bin/win-x64/crt-cli.exe` is refreshed. [Acceptance](docs/acceptance/sdk_install_acceptance.md#windowsx64-concurrent-install-sigsegv-fix-2026-11-06).

### Selection, first build and pinned HTTPS replay on Windows/arm64

Replayed the receipt/repair, full-identity selection, first-build, pinned-HTTPS and release-envelope increments on Windows/arm64 with a CLI self-built from `160a4a6` against the existing rehearsal.7 SDK (CRT `0631131`; the delta to `a00c6be`
is host-tools and documentation only): installer/receipt driver 101/101, selection 68/68, first build 86/86, HTTPS 74/74, all first run, with the existing Windows drivers unchanged. No CRT or CLI product defect. Fifteen repeated
concurrent installs of the SDK all succeed without a crash (supporting evidence only; the Windows/x64 crash stays open); the throwaway stress script's own parsing defect is recorded. The committed `bin/win-arm64/crt-cli.exe` is refreshed.
[Acceptance](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsarm64-2026-11-06).

### Windows/x64 selection, first build and pinned HTTPS replay

The Linux/macOS increments (full-identity selection, first build without `configure`, explicit pinned HTTPS acquisition, release envelope without top-level `format`) replayed on Windows/x64 with a CLI self-built from `ea9b8cd`
against the existing rehearsal.8 SDK (CRT `436b4f2`): selection 68/68, first build 86/86, HTTPS 74/74, installer/receipt driver 101/101. No CRT or CLI product defect; four harness findings (working-directory DLL search,
locked-file publication needing a no-share-delete handle, fixture timeout for the 146 MB archive, OpenSSL config) fixed in the new Windows drivers. The concurrent-install crash did not reproduce in one run; it stays open.
The committed `bin/win-x64/crt-cli.exe` is refreshed. [Acceptance](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsx64-2026-11-06).

### Release-envelope correction reproduced with an original Linux release

A fresh `crt-dev-release` of committed CRT `4db4c85` (`v0.0.1-rehearsal.9`; development SDK test, `crt-c-test` 147/147 and release all exit 0) has an envelope without top-level `format`. The CLI before the
correction (`1dc3c03`) fails the HTTPS acquisition replay 46/53 with `key 'format' not found`; the corrected CLI passes 73/73, the failure/retry continuation 12/12 and 881 native, 158 archive and 114 store checks.
Linux only, developer-built CLI, no committed binary change. [Acceptance](docs/acceptance/sdk_install_acceptance.md#release-envelope-correction-with-original-producer-assets-linuxx86_64-2026-11-06).

### Close the macOS host-tools private-dylib scenario

The unchanged corrected CLI consumes a labeled bundle containing actual AWK
rebuilt with the public SDK's shared C runtime. Private closure replay passes
27/27: relocation, real dyld load evidence, a working poisoned-library control,
CLI child filtering, copied aliases and missing/changed dylib refusal before
job creation. CRT fixes canonical private-runtime links beside copied aliases;
the ordinary static bundle's complete payload stays identical. No original
SDK, upstream source or CLI product implementation change is needed for this slice.

Apple sandbox-exec strips DYLD variables before its child starts. The test-only
native bridge now injects them inside the boundary; selection/first-build
replays pass 66/66 and 86/86 again with that real hostile environment, and five
policy-generation unit tests pass. Earlier TLS/native receipts remain distinct
and are not relabeled as reruns. The [consumer record](docs/acceptance/host_environment_acceptance.md#macosarm64-private-dylib-host-tools-replay-2026-11-06)
links producer-owned hashes, failures and the scoped closure. Windows private
DLL/arm64, production trust/promotion, root locks/defaults and STATUS are unchanged.

### Replay Linux selection, first-build and HTTPS increments on macOS/arm64

Replayed `8788b59`, `4f55a52` and `1dc3c03` against CRT `82466be`, retaining
clean CRT `784389b` Dev and original rehearsal.6 release assets. The corrected
SDK-only candidate passes selection/collisions 66/66, first-build 86/86,
HTTPS acquisition 73/73 and acquisition-failure retry 12/12, with actual
static/shared consumers and four clean Mach-O binding audits. Shared cases
now have native macOS Seatbelt adapters and four policy-generation unit tests.
The corrected candidate and portable/vendor-aware native fixture pass all
922 native regressions.

Found and corrected a common consumer contract bug: release envelopes have
no required top-level `format`; archive `install.format` is the existing
version-1 contract. Unsupported install metadata still fails before asset GETs.
Also repaired a native fixture's machine-specific versus portable-policy
assumption without weakening product checks or fabricating vendor SDK support.
[Acceptance](docs/acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-macosarm64-2026-11-06)
owns exact before/after inputs, measured gates and retained failures. Other-host
replay of the correction, Windows blockers, public trust/promotion and broader
profiles remain open; no SDK bytes, committed binary, lock/default or STATUS changed.

### Acquire caller-pinned SDK inputs over HTTPS and reuse installed identities

Added consumer-owned SDK source format 0, `sdk acquire --source` and first-build
`--sdk-source`/local `sdk_source`. The native flow verifies explicit CA/metadata/
SDK/contract pins, delegates transport and installation to existing components,
and downloads nothing for an exact verified installed identity. PID-owned
network staging uses existing live/dead recovery rules. Source-only new and
success-only project pin publication remain unchanged.

[Acceptance](docs/acceptance/sdk_install_acceptance.md#pinned-https-sdk-acquisition-linuxx86_64-2026-11-06)
records SDK-only build, 70 TLS/acquisition/recovery checks, 12 focused first-build
failure/retry checks and 881 native regressions. It preserves exact source/binary/
fixture identities and the dirty local developer scope; official trust/default
URLs, producer promotion and macOS/Windows replay remain separate gates. No
SDK bytes, root/default lock, committed binary or STATUS is refreshed.

### Build unresolved projects without a separate configure

First `build` reads explicit LLVM flags or `.sdk.json`, selects the fitting
installed SDK and publishes exact format-1 pins after successful compilation.
Resolved/partial bindings are not reselected; failed validation, compilation or
metadata publication preserves the original binding. Native regressions pass 881
checks and isolated workflow cases pass 85/85, including rollback and keyed retry. The
[acceptance record](docs/acceptance/sdk_install_acceptance.md#first-build-without-configure-linuxx86_64-2026-11-06)
owns exact source/binary/test identities and the retained developer-SDK scope.
ELF review found that the retained provider's pure C shared policy links static
inputs; it remains a producer follow-up, not a C shared-runtime pass. C static
and actual C++ shared consumers are distinguished. Remote acquisition and
macOS/Windows replay remain open; no SDK/binary/default/lock promotion or STATUS
refresh is performed.

## 2026-11-05

### Enforce complete SDK identity references and replay Linux selection collisions

Fixed store selection so a 13–64-digit content reference cannot match only its
first 12 digits. Native regressions pass 855 checks; reusable selection cases
and the bounded Linux replay pass 65/65 with competing SDKs, exact selection,
lock preservation, hostile tool/header/runtime paths and static/shared consumers.
[Acceptance](docs/acceptance/sdk_install_acceptance.md#installed-sdk-selection-and-search-path-collisions-linuxx86_64-2026-11-05)
owns exact source/binary identities, fresh SDK-only build, test results and
retained product/fixture failures. Reused the explicitly dirty developer SDK;
no producer runtime/schema, root lock, committed binary or STATUS changed.
Affected-host matcher/collision replay and the existing Windows native crash
remain open; automatic first build/acquisition remains the next workflow slice.

### Replay Windows/x64 receipts and retain the concurrent-install native failure

Built CLI `51f1dc0` from the retained clean CRT `436b4f2` Dev SDK and original
rehearsal.8 inputs. Cold/warm self-build, 113 Windows store checks, 14 producer
fixtures and a real symlink-receipt probe pass. The isolated replay records
93 passes before one concurrent child reports SIGSEGV; a continuation passes
the remaining nine checks. The [record](docs/acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-windowsx64-2026-11-05)
preserves both outcomes and five non-reproducing lock-owner diagnostics.
Fixed Windows replay reporting so a crashed/empty reply fails the case rather
than raising TypeError before saving native evidence; four focused reporting
fixtures pass. No product/PAL fix, SDK rebuild or full Windows closure is
claimed. Native crash investigation and Windows/arm64 delta replay remain
in TODO; committed binaries, defaults, root locks and STATUS are unchanged.

### Replay installation receipts and pinned repair on macOS/arm64

Rebuilt CLI `502992e` from the retained clean CRT `784389b` Development SDK,
under the producer handoff reviewed at `c3daf08`. SDK-only self-build/cache
checks, 114 store checks and 87 isolated installation/repair/project cases
pass; the producer metadata/exporter fixtures pass 14/14. The product/test
source snapshot matches the Linux receipt increment. No product/PAL fix or
SDK rebuild was needed. [Acceptance](docs/acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-macosarm64-2026-11-05)
owns exact identities, the hidden-tree log-FD harness failure and passing
evidence. Native Windows receipt replay and installed-SDK collision checks
remain open. Committed binaries, defaults, root locks and STATUS are unchanged.

### Preserve installation-time contract pins and repair legacy metadata explicitly

Implemented CLI-owned SDK receipts, required/absent contract state and fresh
configure/verify pin enforcement. Added `sdk repair` from an independently
pinned original manifest/archive/contract: metadata is restored while SDK
identity and project locks remain unchanged. A known pin cannot be replaced or
downgraded, and `recover` cleans only dead receipt/contract writers.

Linux store/native tests and the isolated install/project/repair replay pass;
[acceptance](docs/acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-linuxx86_64-2026-11-05)
owns exact source/binary/input identities, counts and retained driver/fixture
failures. Existing SDK bytes were reused with explicit dirty-local provenance;
this is not a clean release or five-pair qualification. Native macOS/Windows
delta replay, installed-SDK collisions and automatic acquisition remain open.
Committed binaries/defaults, root locks and STATUS are unchanged by this increment.

### Adopt the first-user-flow sequence and refresh README/STATUS

Reviewed CLI `3978ced` and CRT `433a7d6` with the owner. Reordered the queue to
bounded local-store closure, installed-SDK first build and pinned acquisition,
with D3 source-port work enabled in parallel on fixed contracts. Deferred
self-upgrade/general updates and Full profile work beyond that first flow.
The [design](docs/design/crt_cli.md#execution-priorities-adopted-2026-11-05)
owns the change-impact gates and declared-host preview scope;
[compatibility](docs/compatibility.md#first-user-workflow-planning-review-2026-11-05)
records the shared review. Refreshed the usable local README workflow and
created STATUS at the owner's explicit request. Updated contributor/development
guidance and linked original evidence without new test or runtime claims.
No code, binary, SDK identity, lock, trust key or release artifact changed.

### `sdk verify` covers the stored toolchain contract

The known Stage 2 gap is closed at unit level: `sdk verify` now also checks the contract kept beside the SDK (regular file, supported format, no machine path,
`sdk_content_sha256` equal to the SDK's identity). Six new `sdk-store-test` checks (89 total) pass on Linux/x86_64 together with `native-test` 829 and `archive-test` 158. Not replayed on
the other hosts and the committed binaries are not refreshed; the contract's own digest stays a lock-only property. All five native pairs have now passed the Stage 2 replay, so the D2 project
references item is checked off. See [acceptance](docs/acceptance/sdk_install_acceptance.md#sdk-verify-covers-the-stored-contract-linuxx86_64-2026-11-05).

### D2 Stage 2 on Windows/x64: toolchain contract and project SDK selection

Last native pair for the Stage 2 work: a rehearsal `crt-dev-release` of committed CRT `436b4f2` (`v0.0.1-rehearsal.8`) ships the portable toolchain contract (`os_sdk.kind` `windows-um-import-libraries`), `sdk install`
keeps it beside the SDK, and a project selects the only installed SDK with no `--sdk-contract`. The self-built SDK-only CLI (cold 16/0, warm 0/16; 919 native, 151 archive and 83 SDK-store checks) passes the
Windows restricted-token replay 83/83 (62 installer plus 21 contract cases). Nothing needed fixing on this host. The committed `bin/win-x64/crt-cli.exe` is refreshed. See
[acceptance](docs/acceptance/sdk_install_acceptance.md#toolchain-contract-and-project-sdk-selection-replay-stage-2-windowsx64-2026-11-05).

### D2 Stage 2 on Windows/arm64: toolchain contract and project SDK selection

First Windows execution of the Stage 2 work: a rehearsal `crt-dev-release` of committed CRT `0631131` ships the portable toolchain contract (`os_sdk.kind` `windows-um-import-libraries`, collected
with the Windows SDK import libraries found through `CRT_WINDOWS_SDK_LIBPATH`), `sdk install` keeps it beside the SDK, and a project selects the only installed SDK with no `--sdk-contract`. The
self-built SDK-only CLI (cold 16/0, warm 0/16; 932 native, 151 archive and 83 SDK-store checks) passes the Windows restricted-token replay 83/83 (62 installer plus 21 contract cases); only the OS SDK kind
is locked, its location is local state. One CLI test defect was fixed (`b03d127`): `sdk-store-test` assumed the new SDK was `list()[1]`, but the store sorts by content hash, which is target dependent. The
committed `bin/win-arm64/crt-cli.exe` is refreshed. See
[acceptance](docs/acceptance/sdk_install_acceptance.md#toolchain-contract-and-project-sdk-selection-replay-stage-2-windowsarm64-2026-11-05).

### D2 Stage 2 on macOS/arm64: toolchain contract and project SDK selection

Replayed the Linux Stage 1/2 work natively: a rehearsal `crt-dev-release` of committed CRT `784389b` ships the portable
toolchain contract (`os_sdk.kind` `macos-xcode-sdk`), `sdk install` keeps it beside the SDK and a project selects the only
installed SDK with no `--sdk-contract`. The self-built SDK-only CLI passes the D2 Seatbelt replay 69/69 (48 installer plus 21
contract cases); the OS SDK is discovered with `xcrun` inside the sandbox and only its kind is locked. One driver defect (a hidden
working directory) was fixed. The committed `bin/macos/crt-cli` is refreshed. See
[acceptance](docs/acceptance/sdk_install_acceptance.md#toolchain-contract-and-project-sdk-selection-replay-stage-2-macosarm64-2026-11-05).

### D2 Stage 2 replay on Linux/aarch64

Replayed the toolchain-contract and project-selection cases natively on the aarch64 guest, from a release of committed CRT `78ed80f` and a CLI self-built
from this checkout's latest sources (`9d711d2`; cold 16/0, warm 0/16; 841 native, 158 archive and 83 SDK-store checks). The unchanged Linux
bubblewrap driver passes 69/69 on its first run. No CLI code changed; `bin/linux-arm64/crt-cli` is replaced by the self-built executable.
[Record](docs/acceptance/sdk_install_acceptance.md#toolchain-contract-and-project-sdk-selection-replay-stage-2-linuxaarch64-2026-11-05).

## 2026-11-04

### D2 Stage 2 replay scripts (Linux/x86_64 executed; macOS and Windows written, not run)

The shared cases live in `tools/replay_sdk_contract_cases.py`; the three replay scripts call them with their own isolation
(bubblewrap, Seatbelt, restricted token), copy the contract asset next to the archive (without it `sdk install` rightly
refuses a release that names one) and keep the store at the sandbox `$HOME/.crt/sdks`. Linux/x86_64 with the new release
passed 69/69 ([acceptance](docs/acceptance/sdk_install_acceptance.md#toolchain-contract-and-project-sdk-selection-replay-stage-2-linuxx86_64-2026-11-05)).
The development `seed` binary was lost to a temporary-directory cleanup, so the CLI for this replay was rebuilt from the
sources with the committed `bin/linux-x64/crt-cli`. macOS and Windows scripts only parse; they have not run.

### D2 toolchain contract stored with the SDK (Stage 2, Linux/x86_64)

A release archive entry may name its portable toolchain contract (`contract`, another asset of the release, produced by CRT
`export_cli_toolchain.py --portable`). `sdk install --release-manifest` checks it against the manifest's digest and size,
the document type, the absence of any machine path and the installed content identity, then keeps it at
`<store>/.contracts/<directory>.json` (prepared before the SDK is published, renamed right after; an SDK may gain a
contract later, a stored contract never changes, a dead owner's temporary file is a `recover` leftover). `configure` with
a store SDK then needs no `--sdk-contract`/`--contract-sha256`: the lock records `contract_source: store` and the
contract's digest, `.crt/local.json` keeps no SDK or contract path, and a tampered or missing stored contract fails. A
contract with `os_sdk` (`@OS_SDK@` in its argv) is completed on this host (`xcrun`, `CRT_WINDOWS_SDK_LIBPATH`, or
`--os-sdk`); the location is local state and the lock records only its kind. Verified: native 829 checks (29 new:
stored contract used, wrong digest, tampered, missing, `--os-sdk` where none is needed, unknown kind, substitution in
the compile command, vanished OS SDK), store 83 (17 new: stored, idempotent, immutable, another SDK's contract,
machine-path/format/digest refusals leaving nothing, added later, dead/live temporary files), archive 158; CRT
exporter tests 9. Real flow from a `crt-dev-release` rehearsal (`v0.0.1-rehearsal.3`, scratch clone, nothing uploaded,
uncommitted code): `sdk install` stored the contract (403 KB), reinstall was idempotent, `new` + `configure` with only
the tool paths selected the single installed SDK, then `build`, `test` and `run` printed "Hello from CRT". Not verified:
macOS/Windows (the `@OS_SDK@` discovery is implemented from the same rules as the exporter but exercised on Linux only
through the `--os-sdk` override and a synthetic contract), a release from committed code, explicit updates.

### D2 project SDK selection from the store (Stage 1, Linux/x86_64)

`configure --sdk <ref>` selects an SDK published by `sdk install` by directory name or an identity prefix (>= 8 hex);
without a named SDK the only installed dev SDK for the host and the manifest's `sdk.version` is used, and none or
several fail with the candidates listed (no moving default). The directory name must equal the version and content
identity that `inspect_sdk` proves. `crt.lock` is now format 1 and records `source` (`store` + `sdk_directory`, or
`handoff`); a store selection leaves no `sdk_root` in `.crt/local.json` and is found again through the store by its locked
identity. Format 0 locks are refused with an instruction to run `configure` again and are never migrated. Verified
with the dev-loop CLI: native tests 800 (33 new: selection by name/prefix/automatic, handoff copy and short or unknown
refs refused, `--sdk` with `--sdk-id`, two fitting SDKs, renamed directory, format-0 lock untouched, locked SDK
uninstalled, then restored), archive 158, store 66. Not done: the toolchain contract is still an external
`--sdk-contract` file (Stage 2: host-independent contract with an `@OS_SDK@` placeholder, shipped in the release and
kept beside the installed SDK), macOS/Windows replays, DLL/path collisions, explicit updates.

### D2 local SDK installer on Windows/arm64

Replayed the installer natively on the Windows/arm64 VM with a self-built, SDK-only CLI (cold 16/0, warm 0/16; 870 native, 151 archive and 66 SDK-store
checks) against a rehearsal `crt-dev-release` of clean CRT `b20a460`. The unchanged Windows restricted-token driver passes 62/62 (pinned-manifest install,
immutability, tampered inputs, fifteen hostile archives, interruption and recovery, concurrency, read-only store, tar.xz of the installed SDK). The first
producer exposed a CRT toolchain defect, not a CLI one: with `-fhosted`, clang's default asynchronous unwind tables make LLVM 22.1.8 omit the CFA adjustment
after the `__chkstk` probe on aarch64-w64-mingw32, so exceptions thrown through `extract_archive` (a ~149 KB frame) terminated; CRT now compiles Windows/aarch64
with synchronous tables. No CLI code changed; `bin/win-arm64/crt-cli.exe` is replaced by the self-built executable.
[Record](docs/acceptance/sdk_install_acceptance.md#windowsarm64-isolated-replay-2026-11-04).

### D2 local SDK installer on Linux/aarch64

Replayed the installer natively on the aarch64 guest with a self-built, SDK-only CLI (cold 16/0, warm 0/16; 779 native, 158 archive and 66 SDK-store
checks) against a rehearsal `crt-dev-release` of clean CRT `ffd0405`. The unchanged Linux bubblewrap driver passes 48/48: pinned-manifest install with the
promised identity, immutability, tampered inputs, hostile archives, interruption and recovery, concurrency, read-only store and zip. The 17 symlinks of the
SDK survive installation. No CLI code changed; `bin/linux-arm64/crt-cli` is replaced by the self-built executable.
[Record](docs/acceptance/sdk_install_acceptance.md#linuxaarch64-isolated-replay-2026-11-04).

### D2 local SDK installer on Windows/x64

`sdk install|list|verify|recover` replayed natively on Windows/x64 with the new restricted-token driver
`tools/replay_sdk_install_windows.py`: 62/62 checks against a rehearsal `crt-dev-release` zip (install metadata format 1, 2,917 entries)
of clean CRT `1ee0379`, using the self-built SDK-only CLI (857 native, 151 archive and 66 SDK-store checks, cold 16/0, warm 0/16). Tamper,
fourteen hostile-archive (including drive-letter, reserved-name and stream names), kill-during-extraction, concurrency, read-only-store and tar.xz-of-a-zip cases behave as on
Linux and macOS, and `kill(pid, 0)` lock-owner liveness works on CRT's Windows PAL. The replay exposed three Windows defects in the CRT producer (the xz DLL
was recorded as `lib/lzma.dll`, the shared consumer never linked liblzma, install metadata raised `OSError` for an escaping symlink), fixed in CRT, and two CLI items
(`ifstream` was given a `path`, and the archive test asserted POSIX mode bits). `bin/win-x64/crt-cli.exe` is the self-built executable. Network
denial is not claimed on Windows. [Acceptance](docs/acceptance/sdk_install_acceptance.md#windowsx64-isolated-replay-2026-11-04).

### D2 local SDK installer on macOS/arm64

Replayed `sdk install|list|verify|recover` natively on macOS/arm64 with the new Seatbelt driver
`tools/replay_sdk_install_macos.py`: 48/48 checks against a rehearsal `crt-dev-release` of clean CRT `53b1a80` (install
metadata format 1, 2,786 entries, `liblzma` in the SDK), using the self-built SDK-only CLI. Tamper, hostile-archive,
interruption, concurrency, read-only-store and zip cases behave as on Linux, including refusal of case-colliding names on
the case-insensitive default volume. The committed `bin/macos/crt-cli` is the self-built executable of that replay. See
[acceptance](docs/acceptance/sdk_install_acceptance.md#macosarm64-isolated-replay-2026-11-04). No CLI code changed.

## 2026-11-03

### D2 local SDK installer on Linux/x86_64 (tar.xz and zip)

`sdk install --archive --archive-sha256 [--content-sha256]`, `sdk list`, `sdk verify <dir>` and `sdk recover` work on
a store (`--store` or `$USER_HOME/.crt/sdks`). `archive.cpp` decodes tar.xz itself (liblzma from the Dev-SDK `xz`
port, capability `archive-xz-1`) and rejects traversal, absolute/drive/reserved names, case collisions, escaping
symlinks, links to devices/hard links and size/entry abuse (100 checks). `sdk_store.cpp` verifies the archive digest,
extracts into same-filesystem staging, checks stage/target/profile, derives `<version>-<content12>` from the inventory
hash, serializes on a per-identity lock directory (dead owners are taken over), publishes with one rename, never
replaces an installed SDK and removes dead transactions (34 checks). zip (zlib raw inflate) is read from the central directory, checks CRC-32 and local/central agreement, keeps Unix
symlinks and exec bits, and refuses zip64, multi-disk, encrypted and non-stored/deflate entries; a zip and a tar.xz of one SDK
have the same identity. Verified through the seed dev loop only (native 767 + archive 158 + store 35 checks, a CLI
smoke, and mutation checks on the CRC rule). `sdk install --release-manifest <file> --release-manifest-sha256 <hex>` takes the archive digest, size and CRT install
metadata from a pinned manifest, bounds extraction by it and requires the extracted counts, bytes, root, identity,
version, stage, profile and target to match (66 store checks; the real 186 MB dev SDK installs and verifies). Linux isolated bubblewrap replay: 48/48 checks ([acceptance](docs/acceptance/sdk_install_acceptance.md)). Not yet verified: macOS/Windows
(including the `lzma` import-library name and `kill(pid,0)` lock-owner semantics).

### Complete the bounded D1 native replay on Windows/x64

Fresh producer candidates and a new SDK-only self-build pass 857 native
checks, 13 cold/13 cached work items, bare PE/ownership/UTF-8 checks and the
previous-binary bootstrap. Exact Windows r5 tools pass 32 isolated
environment/resource cases; selected package CA passes all 32 HTTPS/signature
cases. Explicit project diagnosis/shell preserves SDK and project pins.

Producer fixes address LF fetch, async status, directory search/TMPDIR,
private rootfs environment transport and first-make UTF-8 installation. Retain
failed/intermediate identities; the final CLI uses the repaired runtime.
Correct two Python-only fixture assumptions (native executable suffix and
POSIX policy paths); six fixtures pass. Product sources, committed binaries,
locks/defaults, releases and STATUS are unchanged. The local three-primary-host
D1 scope closes; next is D2. Exact inputs and limits belong to
[Windows D1 acceptance](docs/acceptance/host_environment_acceptance.md#windowsx64-independent-host-environment-replay-2026-11-03) and the
[compatibility review](docs/compatibility.md#independent-host-tools-windows-replay-2026-11-03).

### Replay independent D1 host tools and resources on macOS/arm64

Consumer `095afbc` replays against the retained clean `8db69f4` public Dev SDK
and a new independent bundle assembled by clean CRT `518953f`. Fresh self-build
passes 13/0 cold, 0/13 warm and 807 native checks; the previous-binary bootstrap
has identical code/data, with UUID/link metadata differences retained. Imports
and CRT symbol ownership pass, without private CRT dylibs or foreign bindings.

Added the macOS Seatbelt test-host driver and portable policy fixtures. It
passes 31 environment/resource/cleanup cases, plus explicit SDK project
diagnosis/shell. CRT's HTTPS driver gains macOS isolation and canonical work
paths; 32 packaged-CA HTTPS/signature cases pass after retaining an initial
policy-path failure. Six resource/policy fixtures pass. No product/PAL/upstream
code, committed binary, local SDK default, production pin or STATUS changed.
Windows/x64 replay remains open. Exact receipts, source/SDK/tool/package hashes
and native limits belong to [D1 acceptance](docs/acceptance/host_environment_acceptance.md#macosarm64-independent-host-environment-replay-2026-11-03)
and the [compatibility review](docs/compatibility.md#independent-host-tools-macos-replay-2026-11-03).

## 2026-11-02

### Implement D1 independent tools, scoped doctor and bootstrap resources on Linux

Adopted CRT's separately pinned host-tools bundle and added `env --profile
port-build`, bounded non-interactive `shell`, and self/environment/project doctor.
Job rootfs/cwd and SDK identities stay distinct; the shared process runner now
stops owned background descendants on normal completion as well as timeout.
Virtual make SHELL paths handle spaces/Unicode without upstream patches or a
permanent helper. Added producer-only executable/notices/CA packaging and
explicit resource pin/signature checks; plain doctor keeps unavailable checks
honest. Native application builds still execute no shell/Python/CMake.

The final clean-SDK build passes 767 native checks, 31 isolated tool/resource
cases and 32 isolated HTTPS/signature cases with the selected package CA.
Three packaging fixtures pass. Refreshed the local Linux test binary with a
backup. Original failures and the exact uncommitted source/artifact combination
remain in [D1 acceptance](docs/acceptance/host_environment_acceptance.md) and
[compatibility](docs/compatibility.md#independent-host-tools-and-d1-linux-adoption-2026-11-02).
Native Windows/macOS replay remains in TODO; D1 is not closed. No production
lock, artifact promotion/publication, official trust policy or STATUS change.

### Close D0.7 with the canonical common-source technical candidate

CRT repaired libc++abi patch/cache validation and regenerated common Linux
02/03 source assets at clean `49879bf`, local version
`v0.4.0-sourcefix.20261102.5`. Actual isolated source builds, C++ and Development
consumers plus the same-asset aarch64 cross-build belong to
[producer acceptance](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#common-source-candidate-selection-2026-11-02).

Unchanged CLI product inputs at `7cab48e` pass the new SDK-only Linux replay:
754 native checks, cold 11/0 and warm 0/11, strict bare-runtime/import and
execve audits. The previous committed binary builds current sources with the
new clean SDK; stage1/stage2 are byte-identical under the same isolation
boundary, with dirty opt-in disabled. Exact provider/SDK/source/binary/tool
identities and scope remain in
[acceptance](docs/acceptance/bootstrap_acceptance.md#common-source-candidate-replay-2026-11-02)
and [compatibility](docs/compatibility.md#common-source-candidate-selection-2026-11-02).

Selected the local technical combination, refreshed the Linux test binary and
ignored local SDK defaults with backups, and closed D0.7. No production lock,
public release or SDK promotion is adopted; new native ARM/macOS/Windows
qualification and full D1 stay separate. D1's independent host-tools/port-build
contract is next. STATUS and CLI product sources are unchanged.


### Close the recorded D0.5/D0.6 technical bootstrap gates

Reviewed the five native pairs' existing startup/link, bounded diagnosis,
one-executable and relocated SDK-only build/run/ABI/import evidence; marked
work-queue D0.5/D0.6 complete at their recorded local/unpromoted combinations.
The [technical review](docs/acceptance/bootstrap_acceptance.md#technical-bootstrap-gate-review-2026-11-02)
keeps exact identities, isolation limits, skips and prior failures in the original
records. This is not new execution or complete D0/D1 acceptance.

Reviewed CRT `1537a9183419f238d03d6307aa991fc3e0959a0e` and accepted its
common version/OS/stage source-asset naming policy in
[compatibility](docs/compatibility.md#technical-bootstrap-gates-and-common-source-contract-review-2026-11-02).
D0.7 remains open for producer repair/regeneration, affected validation and final
selection before promotion/pin adoption. No CLI code, committed binary, SDK
payload, lock, schema or STATUS change.

### Replay the Windows/arm64 changes on Windows/x64

CLI `1499a7b` replays on native Windows/x64 against a fresh clean CRT `b4c5b5a` Development SDK (contains the `d12e210` spawn-environment fix; `crt-c-test` 168 of 169, one skip). All 844 native checks pass (cold 11/0, warm 0/11), the bare PE/doctor audit and all 32 restricted-token HTTPS/signature cases; CRT's mksh, toybox, awk and make start from a Unicode SDK path. Windows/x64 does not add `PROCESSOR_ARCHITECTURE` (measured with Python), so the arm64 test tolerance is not exercised there and no CLI source changed. `bin/win-x64/crt-cli.exe` is refreshed. The bootstrap loop for this binary and ports in Unicode directories were not run. [Acceptance](docs/acceptance/bootstrap_acceptance.md#windowsx64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02).

### Replay the latest sources on Windows/arm64 and fix the spawn-environment leak

The first replay of CLI `f36e858` against a clean `ee0b5e3` SDK failed the shared child-environment check. The OS adds
`PROCESSOR_ARCHITECTURE=ARM64` to ARM64 processes (a CRT-free program shows it), so the test now allows that one name with the host's value; and the
Windows PAL left its `CRT_*` bootstrap variables in every CRT child's environment, fixed in CRT `d12e210` with a regression test that fails on the old
runtime. Against a clean `d12e210` SDK the replay passes 845 native checks (cold 11/0, warm 0/11), the bare PE/doctor audit and all 32
restricted-token HTTPS/signature cases (new CRT runner), and CRT's mksh, toybox, awk and make start from a Unicode SDK path.
`bin/win-arm64/crt-cli.exe` is refreshed. Windows/x64 has not run the fix; the bootstrap loop for this binary and ports in Unicode
directories were not run. [Acceptance](docs/acceptance/bootstrap_acceptance.md#windowsarm64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02).

### Revalidate latest Linux/aarch64 sources against a clean Dev SDK

Fresh clean CRT `c4e19f3` passes 147 core tests, imported-C++ static/shared
smokes and static/shared transport consumers. CLI `9e0110f` passes 766 isolated
native checks, cold/warm self-build, bare ELF/mapping audits, SDK-free env
control/redaction and 32 disposable-identity HTTPS/signature cases. The previous
aarch64 binary bootstraps a byte-identical stage1/stage2 fixed point; stage2
passes its suite and warm rebuild. No product implementation change was needed.

Refreshed the local aarch64 binary and ignored SDK config, preserving previous
bytes and exact source/SDK/provider identities. No production lock, SDK
promotion, full Tranche 1/D1 completion or public release is claimed. Windows/
arm64 process-fix and asset-coherence gates remain open. Evidence:
[Linux/aarch64 acceptance](docs/acceptance/bootstrap_acceptance.md#linuxaarch64-latest-source-revalidation-2026-11-02).

### Share child environments and expose bounded native diagnostics

Implemented experimental `env --profile control|app-build|app-run` through the
same environment builder as actual child execution. Control needs no SDK;
project profiles validate exact existing bindings without changing locks or
building targets. Diagnostics print fixed values and inherited names only.
Independent host-tools/port-build and interactive shell remain unsupported.

A fresh Linux/x86_64 export passes 754 native checks, cold/warm self-build
11/0 and 0/11, bare/import/execve audits, SDK-hidden control/redaction and
32 disposable-identity HTTPS/signature cases. Refreshed the Linux binary,
retained its previous bytes/config, and kept the root lock unresolved. Other
hosts require replay of this increment; no producer schema, SDK promotion,
official trust or complete D1 acceptance is assigned. Evidence:
[Linux environment acceptance](docs/acceptance/bootstrap_acceptance.md#linux-child-environment-diagnostics-2026-11-02).

## 2026-11-01

### Revalidate current macOS/arm64 sources with a clean Dev SDK

A fresh clean CRT `8db69f4` Development SDK passes its producer gates; unchanged
CLI `a9d43c6` passes 732 isolated native checks, cold/warm self-build 10/0
and 0/10, the previous-binary bootstrap loop and SDK-hidden bare/import audits.
All 32 disposable-identity HTTPS/signature cases and a real pinned-repository
HTTPS probe pass. Unicode-path clean-SDK selection also passes without dirty opt-in.

Refreshed the local macOS binary/config and retained previous bytes, failed
attempts and exact identities. CRT host-tool probes retain make's direct
space-containing SHELL failure and the passing explicit symlink bridge; complete
D1 and production promotion remain open. Product implementation and root lock
are unchanged; SDK promotion, release and STATUS updates remain separate. Evidence:
[macOS acceptance](docs/acceptance/bootstrap_acceptance.md#macos-latest-source-revalidation-2026-11-01).

### Fix Windows/x64 owned-descendant timeout cleanup and refresh the local binary

CRT now manages new owned spawn groups with Job Objects before child resume,
retains group/leader identity after waitpid and prevents PID reuse. A fresh
local development SDK passes 167 core tests with one skip and static/shared
transport consumers. The SDK-only CLI self-build/cache audit and all
788 native checks pass, including the previously failing
descendant marker; 32 SDK-free HTTPS/signature cases pass again.

Refreshed the Windows/x64 binary and ignored local SDK config, preserving the
old binary, SDKs and failed attempts. CLI product sources and the unresolved
root lock are unchanged. Exact inputs, producer delta, commands and limits:
[Windows fix acceptance](docs/acceptance/bootstrap_acceptance.md#windowsx64-owned-descendant-timeout-fix-2026-11-01).
Windows/arm64 and full D1/job-control gates remain open. The producer fix and
documents are committed as `8db69f404dcfa70b5aa0569cae51a962fd0615d2`; its source
inputs match the tested delta. Existing dirty SDK/binary identities are retained,
without SDK promotion or a public release.

### Revalidate current Windows/x64 sources and retain the process-tree failure

Remote and local main agree at CRT `fa5dc30` and CLI `e66abf2`. A fresh clean
Dev SDK passes its static/shared transport consumers; core has 166 passes and
one skip. Latest CLI build/self-build and cold/warm cache checks pass in a
Unicode path, but the native suite exits 1: an owned descendant survives its
timeout and writes the marker. CRT's Windows process-group kill remains
`ENOSYS`; no assertion was disabled or full regression pass claimed.

Independent SDK-hidden help/version/doctor, Unicode C++ new/configure/build/
test/run, CRT host-tool startup probes and all 32 disposable-identity HTTPS/
signature cases pass. Retained test-host findings include Python's CP949
JSON default and OpenSSL's missing default config/non-ASCII extension path.
Exact identities, commands, failed attempts and limits are in
[Windows/x64 acceptance](docs/acceptance/bootstrap_acceptance.md#windowsx64-latest-source-revalidation-2026-11-01).
Original producer sources, committed binaries, local SDK config and root lock
are unchanged. Full Windows acceptance needs a producer fix and fresh replay;
no SDK promotion, trust registration, commit or publication was performed.

### Select disposable test identities for Linux authentication validation

The owner selected test certificates instead of supplying official publisher
keys/URLs and retained Linux-only execution, with Windows/macOS Not run.
Replayed committed CLI `bd5359e` using CRT test driver `fa5dc30`, a newly
generated test CA/server certificate and an independent RSA publisher test key.
All 32 TLS/integrity/signature cases pass with `/home` hidden except the fresh
test work directory; original repositories, SDK/provider and source export are
unavailable. Test certificate/key paths include spaces and Hangul. Successful
execve paths are only the test Python/OpenSSL and CLI.

Exact public fingerprints, inputs, argv and receipt hashes are in
[the test-identity acceptance record](docs/acceptance/bootstrap_acceptance.md#linux-test-identity-replay-2026-11-01).
Official trust registration remains separate; private test keys stay outside
tracked sources. The binary, SDK, project lock and implementation are unchanged,
so the earlier 692-check native replay is retained without a redundant rebuild.
The remaining Linux D1 host-tools/env/shell/resource gates remain open.

## 2026-10-31

### Add experimental publisher signatures and audit the D1 gates

Added paired explicit-public-key/detached-signature fetch options, verified
through CRT's Mbed TLS using fixed RSA-PSS/SHA-256 and 32-byte salt before
publication. Invalid keys/signatures fail closed; unsigned mode remains
explicitly unverified. The public key is selected by the caller, not adopted
as an official release anchor. Native Linux replay passes 692 checks (including
positive-control/timeout descendant cleanup), cold/warm 10/0 and 0/10, an execve
audit and SDK-hidden loader/doctor checks. The final binary passes all 32
controlled HTTPS/signature cases with SDKs/source/providers hidden. A real CRT
repository HTTPS probe and a local notices/CA resource fixture also pass.

The host-tools audit retains a real failure: make splits a space-containing
SHELL path; a declared temporary symlink bridge to the same CRT mksh passes.
Updated the implemented contract, guide, per-gate D1 audit, compatibility and
work queue; refreshed the Linux binary/config with backups. Exact identities
and limitations are in [D1 acceptance](docs/acceptance/bootstrap_acceptance.md#d1-publisher-and-host-environment-audit-2026-10-31).
Complete D1 remains open: official key/release-metadata policy, independent
host-tools/env/shell, remaining failure/resource qualification and current
Windows/macOS native gates are not closed. No test private key, official trust
setting, production pin or release artifact was published.

### Revalidate the latest Linux/x86_64 CLI with a clean Dev SDK

Replayed CLI `36c6f65` (fetch and narrow-encoding changes, plus a scope-only TODO
edit) against a fresh Dev SDK from clean CRT `f826397`; no implementation fix
was needed. Producer core 147/147, imported C++ smokes and static/shared Dev
transport/trust consumers pass. Isolated CLI replay passes 684 native checks,
cold 9/0 and warm 0/9, execve and SDK-free runtime/import audits. The previous
Linux binary builds the new CLI against the new SDK, which rebuilds byte-identically
and passes 684 checks itself. All 17 controlled HTTPS cases pass with SDK,
source and provider hidden; publisher authentication/full D1-07 remain open.
Exact identities, commands, receipts and limits are in
[Linux acceptance](docs/acceptance/bootstrap_acceptance.md#linux-latest-source-revalidation-2026-10-31).

Refreshed `bin/linux-x64/crt-cli` and the ignored `.sdk.json` to the passing
handoff with dirty-SDK opt-in disabled, retaining the prior binary/config and
all older candidates. Updated the work queue, maturity and compatibility links.
The tracked root lock remains unresolved; no SDK promotion or public release
is claimed. Other native hosts still need current-source replay.

### Declare the CRT-CLI project license

Added the Apache License, Version 2.0 text in the root `LICENSE` and declared
`Apache-2.0` for project-owned source code and documentation in the README,
unless otherwise noted. CRT and other third-party components retain their own
licenses and redistribution notices. Verified the adjacent CRT repository's
`LICENSE.md` at `4e8606c`: its project-owned default is BSD-style with two
conditions, and imported components retain their original licenses; CRT is
not licensed project-wide under Apache-2.0. This change applies to CRT-CLI only.

### Implement the Windows UTF-8 narrow-string model

Adopting the manifest option (B), CRT now ships `share/crt/windows/utf8.manifest`, its `crt-cc`/`crt-c++` wrappers and in-tree
executables embed it, `crt_get_narrow_encoding()` reports the result and the Development profile gains the
`crt-narrow-encoding-1` capability (`crt-c-test` 167/167). The CLI takes the manifest from the provider, so it carries no
flag: `doctor` reports `narrow_encoding`, non-ASCII or invalid arguments and compiler-reported dependency paths are checked on
Windows, and the native suite runs a Hangul/`é` fixture. Against an SDK extracted from a local release-asset ZIP the replay
passes 780 checks (cold 9/0, warm 0/9), `new` to `run` pass in `한글 é 공백`, and the 17 `fetch` cases pass again;
`bin/win-arm64/crt-cli.exe` is refreshed. The Windows Unicode-path requirement is met on Windows/arm64 only; Windows/x64, other
hosts (they need an SDK with the new capability), Windows before 1903, host tools in Unicode directories and a promoted SDK
remain open. [Acceptance](docs/acceptance/bootstrap_acceptance.md#windows-utf-8-narrow-strings-implemented-2026-10-31).

### Design the Windows narrow-string model and prototype it

The Hangul-path `build` failure traced to two narrow encodings on Windows (ANSI for `argv`, environment, file calls and
libc++; UTF-8 for `mbstowcs`, `nl_langinfo`, the PAL image paths and clang). CRT's new
[design document](https://github.com/webos21/crt/blob/main/docs/design/windows_narrow_strings.md) compares keeping ANSI, an
`activeCodePage=UTF-8` manifest and wide calls inside CRT, and recommends the manifest (Windows 10 1903+) with the wide-call
rewrite as the fallback. A prototype linked the CLI through a provider copy that adds the manifest: `new`, `configure`,
`build`, `test` and `run` pass in a Hangul, `é` and space directory, and the native suite passes 777 checks with a Unicode
fixture, with no CLI source change; invalid UTF-8 is silently mapped to U+FFFD, so callers must validate. Nothing was
implemented in the SDK and the fixture change was not committed. The owner decisions are open.
[Acceptance](docs/acceptance/bootstrap_acceptance.md#windows-narrow-string-model-prototype-2026-10-31).

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
