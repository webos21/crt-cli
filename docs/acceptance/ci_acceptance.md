# GitHub CI Acceptance

This record owns what the CI driver and workflow have actually been run against. The design and the
procedure are in the [development guide](../guides/development.md#continuous-integration); open work is in
[TODO](../../TODO.md#github-ci). A passing local rehearsal is not a GitHub-hosted run.

## Current Hosted Coverage (2026-11-11)

The checked-in workflow has five required jobs across four host/architecture
pairs. The latest documented run,
[`42931003805`](https://github.com/webos21/crt-cli/actions/runs/42931003805),
is a successful manual dispatch of branch `ci-linux-arm64` at `f6a41a8`.
Its five job conclusions and event/source metadata were rechecked through the
GitHub API during the 2026-11-11 documentation review; artifacts and native
tests were not rerun by that review.

| Host/architecture | Required runner(s) | Qualified SDK input | Evidence |
| --- | --- | --- | --- |
| Linux/x86_64 | `ubuntu-24.04`, `ubuntu-26.04` | `v0.0.1-preview.20261110.4` | [Repaired-loader qualification and four artifact audits](#whole-required-matrix-and-artifact-verification) |
| Linux/arm64 | `ubuntu-26.04-arm` | `v0.0.1-preview.20261110.linux.arm64.2` | [Manual hosted run and ARM64 artifact verification](#linuxarm64-leg-2026-11-11) |
| macOS/arm64 | `macos-15` | `v0.0.1-preview.20261110.macos.1` | [Four-job qualification with its original host-specific SDK pin](#whole-required-matrix-and-artifact-verification) |
| Windows/x64 | `windows-2022` | `v0.0.1-preview.20261110.windows.1` | [Four-job qualification with its original host-specific SDK pin](#whole-required-matrix-and-artifact-verification) |

The ARM64 addition independently verifies that run's ARM64 artifact. The other
four job conclusions pass; their earlier independent artifact checks retain
their recorded source revisions rather than being relabeled as fresh audits.
The workflow enables PR and main-push triggers for non-documentation changes,
but those events have not yet been observed for the ARM64 leg. Ubuntu 24.04
arm64, other ARM64 glibc versions,
Windows/arm64, input-gated port/redirect integration and release automation
remain outside this CI qualification. The `.3` Ubuntu 24.04 failure below is
historical; the later `.4` record closes it for the named x86_64 combinations.

## Documentation-only Trigger Exclusions (2026-11-11)

Pushes to `main` and PR updates now share CRT's documentation exclusions:
`**/*.md` and `docs/**`. Automatic CI is skipped only when every changed file
matches an excluded path; mixed source/documentation changes still run.
`workflow_dispatch`, the main-branch push restriction, required matrix and all
job steps are unchanged. Code, manifests, tools, tests, CI pins and workflow
configuration remain eligible for automatic CI.

Local validation parses the workflow YAML, compares all non-trigger settings
with the committed workflow, and checks documentation-only, mixed and
code/configuration path cases for both events. This is configuration validation,
not a new hosted event or native replay. The retained hosted records above and
below keep their exact source revisions; the new path-filter behavior still
needs observation after publication.

## Linux/x86_64 local rehearsal of the driver (2026-11-10)

Scope: `tools/ci_bootstrap.py` on one native host with the real public downloads, no workflow engine.

| Item | Exact identity |
| --- | --- |
| Source under test | CLI `0cae8c8bc1fee02a31baf230aba703460fe1afa3` plus this change's uncommitted CI files and the `bin/` removal (product, test and manifest sources are unchanged) |
| Bootstrap CLI | Published `v0.1.0-preview.20261110.linux-x86_64.1`, archive SHA-256 `ad43afbf...1383` (matches the release `SHA256SUMS`), executable `a8fb76a4...a43a` |
| SDK | Public `v0.0.1-preview.20261110.3`, acquired through `github-release` redirects; installed content `36623957...2b3f` equals the pin |
| Host tools | Ubuntu 26.04.1, Ubuntu clang/LLVM 21.1.8 at `/usr/lib/llvm-21/bin`, GNU readelf 2.46, Python 3.14.4 |
| Command | `python3 tools/ci_bootstrap.py --work <new-dir> --llvm-bin /usr/lib/llvm-21/bin` (private `HOME` inside `--work`) |

| Gate | Result |
| --- | --- |
| Pin check, version, `sdk acquire` of the pinned SDK from GitHub | Pass (3.8 s) |
| Stage 1 (bootstrap CLI, explicit `configure` to the installed SDK + its stored contract) | 21 executed / 0 reused (84 s) |
| Suites on the stage-1 executable: native / archive / sdk-store / port-recipe | 901 / 221 / 114 / 141 checks pass, none skipped (same counts as the published Preview record) |
| Stage 2 (stage-1 copy, first build with `--sdk`, rebuilt at stage 1's path) | 21 executed / 0 reused; unchanged rebuild 0 executed |
| Stage 1 vs stage 2 bytes | Identical, SHA-256 `7f622894...ca61` |
| Import audit | No `DT_NEEDED`, `RPATH` or `RUNPATH`; interpreter `/lib64/ld-linux-x86-64.so.2` only |
| Executable alone (no SDK store, empty `PATH`, fresh home): `--version`, `--help`, `doctor --bootstrap` | Pass for both stages, identical checks |
| Negative controls: wrong bootstrap digest; test unclassified in `pins.json`; test in both lists | Each fails with its own message, exit 1 |

Findings fixed during the rehearsal (the first two attempts failed, both in the driver, not the product):

- `native-test` reads `sdk_root` and `sdk_contract` from the project's `.crt/local.json` because it drives explicit
  `--sdk-root`/`--sdk-contract` flows. A first-build store binding deliberately keeps no machine path, so stage 1 is bound by
  `configure` against the store directory and the contract stored beside it.
- `__FILE__` and `-g` debug info embed the absolute source path, so stages built in different directories never match
  byte for byte. Stage 2 is therefore rebuilt at stage 1's path; the earlier "different bytes" result was this, not drift.
- The bare-run check reused one directory for both stages; it is now per stage.

## First GitHub-hosted run (2026-11-10)

Workflow run [`42773306405`](https://github.com/webos21/crt-cli/actions/runs/42773306405) on commit `d6578cc50c7e4812a66d9cba5c566a6e614c54ee`
(images `ubuntu-24.04` 20261101.360, `macos-15`, `windows-2022`). Logs and receipts were downloaded from the uploaded artifacts.

| Leg | Result |
| --- | --- |
| macOS/arm64 | **Pass, every stage.** Public macOS Dev SDK `v0.0.1-preview.20261110.macos.1` (content `e3dc7fb1...bac8`) acquired from GitHub; suites native 941 / archive 221 / sdk-store 114 / port-recipe 141, none skipped; stage 2 cold 21/0 and warm 0; imports `libSystem` only; SDK-free run passes. Stage 1 and stage 2 differ byte-wise (cause not investigated; the Preview record saw identical code and data), which the driver reports without gating. `native-test` took 841 s |
| Linux/x86_64 | Bootstrap, LLVM 21 apt install, SDK acquisition, stage 1 (21/0) pass. `native-test` fails in `variant-cxx-shared-library`: `greeting-test: symbol lookup error: undefined symbol: calloc, version GLIBC_2.2.5`. Later stages did not run (the driver stopped at the first failed suite; it now continues) |
| Windows/x64 | Bootstrap, LLVM 22.1.8 choco install, Windows SDK discovery, SDK acquisition, stage 1 (21/0) pass. `native-test` aborts at start: `mkdtemp failed` |

**Windows cause (driver).** The suite takes its scratch root from `TMPDIR`, then `TEMP`, then `/tmp`; the CLI forwards only `TMPDIR` to
children, and the driver set `TMPDIR` on POSIX hosts only. Windows has no `/tmp`.
The driver now sets it everywhere; the later hosted diagnosis below records
the subsequent fixture failures and their resolution.

**Linux cause (producer side, reproduced locally).** The failing binary is not host-specific: the executable built on the Ubuntu 26.04 rehearsal
host (glibc 2.43, where it passes) fails identically under Ubuntu 24.04's glibc 2.39 loader. Its dynamic section needs only the SDK's
`ld-linux-x86-64.so.2` stub, `libc.so` (soname of CRT's libc, versions `CRT_1.0`), `libc++`, `libc++abi`, `libunwind`, `libm` and `libdl`; there is no
`GLIBC_*` version need. `LD_DEBUG=symbols,bindings` shows the host loader itself looking up `calloc` across those SDK libraries, and CRT's `libc.so`
defines only `calloc@@CRT_1.0`. This is consistent with the glibc loader re-binding its malloc family to the first C library in scope by the version
`GLIBC_2.2.5` (a reading of glibc behavior, not verified against its source here). The C variants, which run before the failing one in the suite, pass on 24.04; the Preview record notes that `link_c` uses static CRT libraries,
so the C++ shared-runtime path is the one that loads CRT's `libc.so` beside the host loader (inferred, not separately confirmed). This is the same area as the open "Linux C shared SDK mode" exclusion
and belongs to the CRT producer; the CLI is not changed to hide it. A producer-side fix needs its own reproducer and review.

Reproducer without Docker (needs `bwrap`; `ubuntu-base` SHA-256 from the release `SHA256SUMS`):

```text
curl -fsSL https://cdimage.ubuntu.com/ubuntu-base/releases/24.04/release/ubuntu-base-24.04.5-base-amd64.tar.gz -o base.tar.gz
echo "e77b6f10c2590cef872b33ee9f635a0e3fd1f57fb074c0e52b5c7f56147a0c86  base.tar.gz" | sha256sum -c -
mkdir root && tar -xzf base.tar.gz -C root --no-same-owner --exclude='dev/*'
ln -sf ../lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 root/lib64/ld-linux-x86-64.so.2
# build any C++ shared-runtime project with the Linux Dev SDK, then run its executable inside glibc 2.39:
bwrap --unshare-all --bind root / --proc /proc --dev /dev --ro-bind <work> <work> --clearenv --setenv PATH /nonexistent <work>/variant/out/native/greeting-test
```

The reproducer project is `crt-cli new variant --language cxx --kind shared-library`; its `greeting-test` is the failing executable. The same
executable runs on glibc 2.43.

## Ubuntu 26.04 Hosted Linux Qualification (2026-11-10)

**Pass, native Linux/x86_64.** The owner selected migration to Ubuntu 26.04,
keeping it if the hosted run passed. Run
[`42779857557`](https://github.com/webos21/crt-cli/actions/runs/42779857557),
Linux job [`129707007529`](https://github.com/webos21/crt-cli/actions/runs/42779857557/job/129707007529),
tests commit `f3d8a58ddf59225826bcf41dd1c99687d7f11747` and concludes success.

| Item | Measured identity/result |
| --- | --- |
| Runner | `ubuntu-26.04`, OS 26.04.1, image `20261101.186.1` |
| Host runtime/tools | glibc `2.43-2ubuntu2.4`; Ubuntu Clang 21.1.8 (`6ubuntu1`); LLD 21.1.8; LLVM ar 21 |
| Bootstrap / SDK | Same published Preview and SDK `.3` pins as the preceding Linux run; SDK content `36623957fbdd5a384cc173991e20253ff83148028ab8072298dcd434a55a2b3f` |
| Suites | Native **901**, archive **221**, sdk-store **114**, port-recipe **141** checks pass; none skipped |
| Stage 1 / stage 2 | Each cold build **21 executed / 0 reused**; warm rebuild **0 / 21** |
| Stage equality | Byte-identical, SHA-256 `e58e9c0487b20b169c7cf19b863a9cb06b6116bcf3634faa68d8d399417d5785` |
| Import audit | No `DT_NEEDED`, `RPATH` or `RUNPATH`; interpreter `/lib64/ld-linux-x86-64.so.2` |
| Standalone execution | Both stages pass version/help/bootstrap doctor with fresh home, no SDK store and empty PATH |

The failing C++ shared-library case from Ubuntu 24.04 now passes within the
unmodified native suite. Product/test/manifest sources, the CI driver and
bootstrap/SDK pins are identical to run `42777416265` (`4897690`). Only Linux's
runner, LLVM provisioning, failure gating and documentation change. LLVM comes
from the Ubuntu archive; the Ubuntu 24.04/noble apt.llvm.org repository is no
longer added. Linux is a required leg (`experimental: false`), so failures
cannot be masked by `continue-on-error`. The owner-selected passing setup stays.

Downloaded job metadata/logs, artifact listings, complete Linux artifacts and
the prior 24.04 failure remain in `out/ci-ubuntu2604-20261110/`. Independent local
checks verify the downloaded executable against SHA256SUMS and both stage
receipts, all recorded step exits, test counts, warm reuse and ELF dynamic tags
(`verified-result.json`). The hosted native suite takes 200.9 seconds.

This qualifies the pinned combination on this Ubuntu 26.04 image. Ubuntu 24.04
and other loader versions remain unqualified for this shared-runtime path;
no CRT fix or broader glibc support range is claimed. Input-gated port suites,
transport replays and release publication remain separate. Windows/macOS keep
their existing workflow settings; this Linux result does not qualify Windows.

## Windows Hosted Failure Diagnosis (2026-11-10)

Run [`42777416265`](https://github.com/webos21/crt-cli/actions/runs/42777416265),
Windows job [`129699079264`](https://github.com/webos21/crt-cli/actions/runs/42777416265/job/129699079264),
tests `4897690` after the driver-level TMPDIR fix. The native suite now starts,
but two suites fail. Archive and SDK-store suites, both builds, warm reuse,
standalone execution and PE import audits pass. Two fixture defects explain
the failures:

- **Native configure inputs:** the parent CI binding explicitly selects the
  Windows SDK `um/x64` import-library directory, but nested fixture configure
  commands and `.sdk.json` defaults omit `local.os_sdk`. The portable contract
  needs this location; the deliberately filtered child environment does not
  inherit `CRT_WINDOWS_SDK_LIBPATH`. The fixture must pass the recorded
  `--os-sdk` input, including store selection and first-build cases. Failure
  diagnostics previously printed only `expected command exit: configure`;
  the helper now also includes the child exit code and structured message.
- **Reviewed recipe bytes:** the recipe's LF SHA-256 is
  `dde944984d799197c0af1553ac7cf38139edc7b2e4e673ef9b4c17dd8c1a349b`.
  Replacing LF with CRLF yields exactly the failing digest
  `87d71bc7e02525dce5df204a09e2f1f3180b59a0519903b44ed32d552f08dd0f`.
  `.gitattributes` now sets `text eol=lf` for this one reviewed fixture. A
  fresh Git checkout with `core.autocrlf=true` keeps those recipe bytes while
  a control text file converts to CRLF. A new native recipe regression still
  rejects CRLF bytes as unreviewed; the production digest gate is unchanged.

The fix changes tests, the fixture checkout policy and CI gating. Product
code, the reviewed adapter digest and published bootstrap/SDK pins are
unchanged. Failed artifacts and the checkout reproduction receipt remain in
`out/ci-windows-20261110/`.

## Windows Hosted Qualification (2026-11-10)

**Pass, native Windows/x64; complete three-host workflow passes.** Run
[`42782205090`](https://github.com/webos21/crt-cli/actions/runs/42782205090)
tests `7c9f228800ce152de96e9be382bb0323037baa48`. Windows job
[`129714680727`](https://github.com/webos21/crt-cli/actions/runs/42782205090/job/129714680727)
and the overall workflow conclude success with all three matrix entries set to
`experimental: false`. Linux/macOS also execute the changed common tests.

| Item | Measured identity/result |
| --- | --- |
| Windows image | Windows Server 2022, `windows-2022` image `20261101.353.1` |
| External tools | LLVM/Clang/LLD 22.1.8; Windows SDK `10.0.26100.0/um/x64`, explicitly selected |
| Bootstrap | Unchanged Windows Preview `v0.1.0-preview.20261110.windows-x86_64.1`, archive SHA-256 `4bafc8464b729049840f1f21807f505bbcc09e7d5f7bc260e63d8f971ade75c0` |
| SDK | Unchanged public `v0.0.1-preview.20261110.windows.1`, content `5bb55372ec10e3cfa24c84de981a7130f56aeb57ef0d43d5da22a65036dec7ef`, contract `7762f572b4614b3068e1ea99694ce19360e213d31c5bd01019e4ad6ce005b726` |
| Windows suites | Native **991**, archive **214**, SDK-store **113**, recipe **142** checks pass, none skipped |
| Cold/warm builds | Stage 1 and 2 each **21 executed / 0 reused**; warm **0 / 21** |
| PE imports | Only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll` |
| Standalone execution | Both stages pass version/help/bootstrap doctor with fresh home, empty PATH and no SDK store |
| Stage-1 binary | SHA-256 `76ee07ffd0b52aafe7b64b2c22da217f27a77120ddf91fe6764e2f68829971d7` |
| Stage-2 binary | SHA-256 `1baa5eb7decdc7b3010c75a83f6ecabd70dd004045befe73a494b9f1b12238c8` |
| Linux regression | **901 / 221 / 114 / 142** checks; both stages, cache, ELF/import and standalone audits pass; stage bytes identical |
| macOS regression | **941 / 221 / 114 / 142** checks; both stages, cache, Mach-O/import and standalone audits pass; stage bytes differ |

The native suite now completes configure, project/store selection, first build,
static/shared consumers and vendor SDK fixtures. The recipe suite accepts the
unchanged reviewed LF fixture on Windows and rejects CRLF conversion. Product
source, manifest, CI driver and release pins match the failing run; fixes are
limited to test inputs, checkout policy, diagnostic text and workflow gating.

Full original-failure and new three-host artifacts, Windows job logs/metadata,
workflow result and local checkout proof are retained under
`out/ci-windows-20261110/`. `verified-result.json` checks each downloaded final
binary against its SHA256SUMS and stage-2 receipt, all step exits, suite counts
and warm reuse. An independent local `llvm-readobj-21 --coff-imports` confirms
the Windows artifact's two allowed OS DLLs. Windows native tests take 658.3 s.

Windows/macOS stage byte differences remain reported, not gated or claimed
byte-reproducible; their cause is not investigated in this fix. This is the
deterministic PR suite, not the five input-gated port integrations, transport
replays, ARM matrix or release qualification. No artifact is published.

## CI Latency Investigation (2026-11-10)

The owner requested lower CI latency with the existing checks retained. The
comparison baseline is passing run `42782205090` (`7c9f228`); its driver times
exclude runner/tool setup:

| Host | Native suite | Whole driver |
| --- | --- | --- |
| Linux/x86_64 | 123.7 s | 272.9 s |
| macOS/arm64 | 804.0 s | 1000.2 s |
| Windows/x64 | 658.3 s | 927.9 s |

### Retained Failed Thread Experiment

`0fd003a` adds eleven timed native sections, strict coverage/count checks and
bounded parallel project/variant cases. Linux and macOS pass at run
[`42789198623`](https://github.com/webos21/crt-cli/actions/runs/42789198623).
The macOS native suite takes 525.0 s and its whole driver 746.0 s, but Windows
fails the C++ fixture's read-only binding/output assertion during concurrent
threaded execution. Diagnostic revision `6f54282`, run
[`42791013803`](https://github.com/webos21/crt-cli/actions/runs/42791013803),
also passes Linux/macOS but Windows instead receives empty JSON input from
both concurrent configure cases. These Windows failures remain unresolved as
threaded-runtime observations; their precise producer/consumer root cause is
not assigned, and no producer fix is claimed.

The retained implementation (`f49c279`) uses independent CRT case processes
instead. The parent launches serially and manages at most two cases at once,
each with its own fixture root, result/error files and owned process group.
Environment/store mutation stays serial. Workers have a 600-second deadline;
the manager bounds logs/results, validates each case's identity/count/phase,
waits for its other children after a failure and stops timed-out owned jobs.
Native negative controls exercise a failed child alongside a completing child
and a timed-out delayed-marker writer. Seven host-side tests reject missing,
duplicate, incomplete and malformed section reports and mismatched counts,
including CRLF report input. Normal native execution defaults to one worker;
the CI driver writes test-only format-0 local settings for two workers.

All original cases, SDK/tool/inventory hashing, both cold builds, warm reuse,
import and standalone gates remain enabled. No product code, SDK/bootstrap
pin, validation cadence, registry schema or release policy changes. A passing
process-isolated run does not qualify Windows threaded spawning/file I/O.

Evidence, local comparisons and failed Windows artifacts remain under
`out/ci-speed-20261110/`. Original serial baseline artifacts remain under
`out/ci-windows-20261110/`. Hosted final outcomes are recorded below after
qualification; the failed experiments are not acceptance.

### Same-binary Local Comparison

The final native test executable, SHA-256
`9186751247ef8a16365996d0a70aa31cdb0c15289f6e3296a35d664828de4b84`,
is compiled with the installed public SDK's recorded compiler/link vectors
from the fresh Linux rehearsal, substituting the current native-test source.
It runs sequentially on the same local host with
jobs 1 and 2, using private scratch/home state and the same explicit binding:
**163.98 s / 114.98 s**, a **29.9%** reduction. Both report **901 checks**,
eleven complete phases and identical phase check counts. Both also execute
the pool failure/deadline controls. The focused result excludes test compilation
and the two application self-builds; it is not a hosted-run duration.

`compare-final.py`, the exact executable, stdout/stderr and
`final-local-comparison.json` remain under the evidence root. The earlier
threaded and initial process prototypes retain separate identities/results.

### Hosted Process-worker Qualification

**Pass on all three required hosts.** Run
[`42794932909`](https://github.com/webos21/crt-cli/actions/runs/42794932909)
tests `f49c279a010fba94e5a50ea86a5b254843447a96`. All seven report-gate tests,
eleven native sections, native pool failure/deadline controls, original suites,
both cold builds (21/0 each), warm reuse (0/21), imports and standalone checks
pass. Native counts stay **901 / 941 / 991** on Linux/macOS/Windows; their other
suite counts match the serial baseline exactly.

| Host | Native seconds before -> after | Driver seconds before -> after | Observed driver change |
| --- | --- | --- | --- |
| Linux/x86_64 | 123.7 -> 143.3 | 272.9 -> 399.7 | 46.5% longer |
| macOS/arm64 | 804.0 -> 614.0 | 1000.2 -> 865.6 | 13.5% shorter |
| Windows/x64 | 658.3 -> 300.5 | 927.9 -> 471.2 | 49.2% shorter |

The longest hosted job, including setup, drops **1049 -> 885 seconds**
(17m29s -> 14m45s, 15.6%). These are one-run observations on separate hosted
VMs, not a guaranteed average. Linux's unchanged application cold builds also
grow from 48.9/48.0 to 84.7/86.0 seconds, so this trial does not establish a
hosted Linux latency improvement. The same-host, same-test-binary comparison
above separately measures the retained case parallelism.

Downloaded final executable SHA-256 values:

- Linux: `e58e9c0487b20b169c7cf19b863a9cb06b6116bcf3634faa68d8d399417d5785`.
- macOS: `d052facef0f86ec37f892b79a5a1a97c7f5b636e86da5a98c028123036b5ec92`.
- Windows: `78ee3a178490bbf108e7620c9cca0d4090908140f398ccb36a60f6c5f2ba6341`.

`verified-final-result.json` independently checks each artifact's SHA256SUMS
and stage-2 receipt, all step exits, exact counts, eleven completed sections,
phase totals and cold/warm results. Local ELF and PE readers confirm no Linux
DT_NEEDED/RPATH/RUNPATH and only the two allowed Windows OS DLLs. Native macOS
audit still permits libSystem only. Linux stage bytes remain identical;
Windows/macOS differences remain reported and unqualified for byte identity.
Run/job metadata, full artifacts and comparisons are retained in the evidence
root. No test tier is removed or deferred and no nightly workflow is added.

## Limits at the Original Three-host Checkpoint

These limits describe the preceding process-worker run with Linux SDK `.3`.
The later [loader-repair adoption](#linux-loader-repair-sdk-adoption-2026-11-10)
and [ARM64 qualification](#linuxarm64-leg-2026-11-11) supersede the corresponding
host gaps for their exact inputs. Current remaining scope is summarized above.

- At that checkpoint, Ubuntu 24.04's full native suite was blocked by the
  producer-side loader gap with SDK `.3`; Ubuntu 26.04 passed that combination.
- The Linux, macOS and Windows driver paths ran on hosted images up to the points recorded above; Python 3.12, `actions/*` at
  their pinned commits, the LLVM 21 apt install and artifact upload worked on Linux.
- Input-gated port suites, redirect/transport replays, Linux/aarch64 and
  Windows/arm64 were not covered by that three-host run.
- Publisher authentication stays `not_verified`: pins verify byte identity;
  the runner's system roots protect TLS transport.

## Linux loader-repair SDK adoption (2026-11-10)

**Linux hosted qualification passes on both required Ubuntu versions.** The producer published
Linux Dev SDK `.4` at clean CRT `448fa32349ae697f589f0ad8fc60cfdf7a7e195a`,
then unauthenticated public re-download matched all six frozen assets.
[Producer evidence](https://github.com/webos21/crt/blob/main/docs/acceptance/sdk_split_acceptance.md#linux-loader-compatibility-repair-2026-11-10)
owns exact payload hashes, TLS descriptor/CRT destructor policy and old/new
loader tests. [Adoption](../compatibility.md#linux-loader-repair-sdk-adoption-2026-11-10)
owns the new CI pins and rebuild/migration limits. Product/test sources at CLI
`65b20c4` are unchanged by this consumer CI/pin/document change.

Local SDK-only CLI self-build is 21/0; native 913, archive 221, SDK-store 114,
port-recipe 142 checks pass, none skipped. Warm 0/21 and SDK-free import/run
checks pass. The local wrapper's incorrect return-value unpacking failed only
after all four suite exits were zero; its separate corrected postcheck retains
that evidence and does not count a product failure as passing.

The existing published Linux CLI (SHA-256 `a8fb76a4` prefix, exact full pin in
producer public-consumer receipt) passes 14/14 isolated checks on public `.4`
files: native install/verify, C/C++ new/first build/test/run/warm reuse, immutable
SDK/download inventories, and C++ shared-library tests. The same generated
`greeting-test` passes the pinned Ubuntu glibc 2.39 interpreter and the host
2.43 interpreter. Public consumer has producer/local SDK/source trees hidden,
empty PATH, private HOME/tmp, and no network. This qualifies that generated
consumer, not a whole Ubuntu 24.04 hosted suite.

Required CI now includes both Ubuntu versions, LLVM 21 and the same `.4` SDK.
The noble LLVM signing-key fingerprint and provisioning come from the earlier
working 24.04 baseline; 26.04 retains its Ubuntu archive. Existing macOS/Windows
pins are unchanged. Completed hosted results follow below; no production/default
lock change or native aarch64/Full/Web replay is implied.

### Completed Linux hosted qualification

Run [`42805951580`](https://github.com/webos21/crt-cli/actions/runs/42805951580),
CLI source `5d148bacb1ee52436b2080475f04d51a035e2a38`, has successful required
Linux jobs on the same promoted SDK `.4` (content `22107183...2ca81`).
Both uploaded binaries/receipts were downloaded and independently verified.

| Required host | Exact host runtime/compiler | Suites native/archive/store/recipe | Stage-2 executable SHA-256 |
| --- | --- | --- | --- |
| Ubuntu 24.04 | glibc `2.39-0ubuntu8.9`; LLVM/Clang 21.1.8 from apt.llvm.org | 913 / 221 / 114 / 142, none skipped | `d13999591696a81feffaf3874d0d84d4ae0c7039c1011b2b306b3de36986517f` |
| Ubuntu 26.04 | glibc `2.43-2ubuntu2.4`; Ubuntu LLVM/Clang 21.1.8 (`6ubuntu1`) | 913 / 221 / 114 / 142, none skipped | `25f3f3a75986e0e89aa85dea46b87697d8b236f012a5902a22ab6bbaa68718ed` |

For each host, public GitHub SDK acquisition, both cold self-builds (21/0),
unchanged rebuild (0/21), byte equality of stages 1/2, empty ELF dependency/
rpath audit and both SDK-free executables' version/help/bootstrap-doctor checks
pass. Interpreter is `/lib64/ld-linux-x86-64.so.2`; no host libc/C++ imports
are added. The previously failing `variant-cxx-shared-library` runs inside the
unmodified native suite on actual Ubuntu 24.04, not a substituted-loader test.
This closes the old `.3` producer gap for these exact `.4` combinations.

Evidence is retained in `/home/syszone/GitLocal/crt-preview-20261110-tlsdesc/`:
`hosted-linux-artifacts/` and `hosted-linux-verification.json`. Source/SDK pins,
summary result/test counts/cold caches, stage equality and binary SHA-256 agree.
The original `.3` failure remains above. Additional host/loader versions and
Linux/aarch64 remain unqualified; fallback lifecycle and pure-C shared SDK
limits are unchanged. No replacement CLI package or existing-project migration.

### Whole required matrix and artifact verification

The same run subsequently concludes **success, all four required jobs**:
Ubuntu 24.04, Ubuntu 26.04, macOS/arm64 and Windows/x64. All four artifact
summaries and stage-2 executable digests were downloaded and independently
checked against source `5d148ba`, result `pass`, cold-build receipts and suite
results (`hosted-verification.json`, `hosted-artifacts/`). Both Linux stage
pairs are byte-identical. macOS/Windows retain their own original SDK pins and
reported byte-difference limitations; this adds no other-host `.4` SDK claim.

## Linux/arm64 leg (2026-11-11)

`ci.yml` gains a required `native (linux-arm64)` leg on the hosted `ubuntu-26.04-arm` runner. It bootstraps from the published Linux/aarch64 CLI Preview
(`v0.1.0-preview.20261111.linux-arm64.1`, executable `9fbef825...e1db`), which acquires the public Linux/aarch64 Dev SDK `v0.0.1-preview.20261110.linux.arm64.2`
(content `cd2ab657...0cc6`) with the `github-release` redirect policy; both pins in `.github/ci/pins.json` were checked against the public bytes before use. Only 26.04 is
required: that SDK was exercised on glibc 2.43 only, so no `ubuntu-24.04-arm` leg is claimed. The driver now recognises Linux/aarch64 (`host_key`, unit-tested) and applies the glibc probe and the ELF
`DT_NEEDED`/`RPATH`/`RUNPATH` and interpreter audit to every Linux host; no product or SDK change.

**Local replay first.** `tools/ci_bootstrap.py --host linux-arm64` on the native aarch64 development machine passes: native 913, archive 221, SDK-store 114, port-recipe 142, stage 1 and stage 2 byte-identical
(cold 21 executed / 0 reused each), no imports, interpreter `/lib/ld-linux-aarch64.so.1`.

**Hosted run.** `workflow_dispatch` run `42931003805` on the branch `ci-linux-arm64` (source `f6a41a8`) concludes **success on all five jobs**: Ubuntu 24.04 and 26.04 x86_64, `linux-arm64`, macOS/arm64 and Windows/x64.
The `linux-arm64` artifact summary was downloaded and checked: result `pass`, glibc 2.43, LLVM/LLD 21.1.8, the same four suite counts, stage 1 and stage 2 byte-identical, bootstrap executable `9fbef825...`, SDK pin as above. The other four legs
are unchanged inputs and were not re-verified artifact by artifact here.

Not covered: pull-request and `push` events on `main` for this leg (the first run was a manual dispatch from the branch; a runner or image change fails closed), Ubuntu 24.04 arm64, other glibc versions, and the `integration.yml` and `release.yml` queues.
