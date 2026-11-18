# GitHub CI Acceptance

This record owns what the CI driver and workflow have actually been run against. The design and the
procedure are in the [development guide](../guides/development.md#continuous-integration); open work is in
[TODO](../../TODO.md#ci-platform-expansion). A passing local rehearsal is not a GitHub-hosted run.

## Current Hosted Coverage (2026-11-11)

Updated 2026-11-18: the checked-in workflow has six required jobs across five host/architecture pairs; the Windows/arm64 leg (`windows-11-arm`) was added that day and has
[its own hosted record](#hosted-windows-11-arm-result-of-the-win-arm64-pins-2026-11-18). The rest of this section is the 2026-11-11 state of the other four pairs. The latest documented run,
[`42931003805`](https://github.com/webos21/crt-cli/actions/runs/42931003805),
is a successful manual dispatch of branch `ci-linux-arm64` at `f6a41a8`.
Its five job conclusions and event/source metadata were rechecked through the
GitHub API during the 2026-11-11 documentation review; artifacts and native
tests were not rerun by that review.

| Host/architecture | Required runner(s) | Qualified SDK input | Evidence |
| --- | --- | --- | --- |
| Linux/x86_64 | `ubuntu-24.04`, `ubuntu-26.04` | `v0.0.1-preview.20261118.linux.x64.1` | [First automated Dev SDK adoption](#linuxx86_64-sdk-pin-moved-to-the-first-automated-preview-2026-11-18); earlier [repaired-loader qualification](#whole-required-matrix-and-artifact-verification) used `.4` |
| Linux/arm64 | `ubuntu-26.04-arm` | `v0.0.1-preview.20261110.linux.arm64.2` | [Manual hosted run and ARM64 artifact verification](#linuxarm64-leg-2026-11-11) |
| macOS/arm64 | `macos-15` | `v0.0.1-preview.20261110.macos.1` | [Four-job qualification with its original host-specific SDK pin](#whole-required-matrix-and-artifact-verification) |
| Windows/x64 | `windows-2022` | `v0.0.1-preview.20261110.windows.1` | [Four-job qualification with its original host-specific SDK pin](#whole-required-matrix-and-artifact-verification) |
| Windows/arm64 | `windows-11-arm` | `v0.0.1-preview.20261118.windows.arm64.1` | [Hosted result of the win-arm64 pins](#hosted-windows-11-arm-result-of-the-win-arm64-pins-2026-11-18) |

The ARM64 addition independently verifies that run's ARM64 artifact. The other
four job conclusions pass; their earlier independent artifact checks retain
their recorded source revisions rather than being relabeled as fresh audits.
The workflow enables PR and main-push triggers for non-documentation changes;
those events were not yet observed for the Linux/arm64 leg when this section was written and
have been since: it passed in the pull-request and `main` push runs of 2026-11-18
([44244028130](https://github.com/webos21/crt-cli/actions/runs/44244028130) and
[44245882492](https://github.com/webos21/crt-cli/actions/runs/44245882492)). Ubuntu 24.04
arm64 and other ARM64 glibc versions remain outside this CI qualification; input-gated
port/redirect integration and release automation have their own records below. The `.3` Ubuntu 24.04 failure below is
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

Not covered by this dispatch run: pull-request and `push` events for the leg (observed afterwards, see [Current hosted coverage](#current-hosted-coverage-2026-11-11)), Ubuntu 24.04 arm64, other glibc versions, and the `integration.yml` and `release.yml` queues.

## Current hosted coverage (2026-11-11)

**Events.** The `linux-arm64` leg (`ubuntu-26.04-arm`) has now passed on every event path that triggers `ci.yml`, each time with all other legs green:

| Event | Run | Source | Result |
| --- | --- | --- | --- |
| `workflow_dispatch` (branch) | `42931003805` | `f6a41a8` | success, 5 jobs |
| `push` to `main` | `42932786276` | `6201aa6` | success, 5 jobs |
| `pull_request` | `42946566272` | `47c3a03` (PR 1) | success, `changes`, 5 native jobs and `CI gate` |
| `push` to `main` (merge of PR 1) | `42948410361` | `0cf5a77` | success, `CI gate` reported by `github-actions` |

**Change classification.** `paths-ignore` is replaced by a `changes` job and an always-running `CI gate` (see the [development guide](../guides/development.md#continuous-integration)):
a workflow that never starts leaves a required status check pending, which would block every documentation-only pull request. The classifier was exercised locally on a documentation-only range
(`code=false`), a code range, a mixed range, an unreachable `before` and a manual dispatch (all `code=true`, i.e. the full matrix). Unclassifiable input runs the matrix; skipping requires a proven documentation-only change.

**Ruleset.** Repository ruleset `27002916` "main requires CI gate" (`active`) targets `refs/heads/main`: required status check `CI gate` from the GitHub Actions app (`integration_id` 15368, not strict), no deletion, no non-fast-forward.
Repository administrators bypass `always` (chosen so that the maintainer's direct documentation commits keep working); pull requests from anyone else cannot merge unless `CI gate` passes.
Two verification pull requests were opened as drafts and closed without merging, with their branches deleted:

| Pull request | Change | `CI gate` | Merge state |
| --- | --- | --- | --- |
| 2 | one HTML comment in `docs/README.md` | success; `changes` success, the matrix `SKIPPED` | `CLEAN` / `MERGEABLE` |
| 3 | a deliberately failing test in `tools/test_ci_bootstrap.py` | failure; all five native jobs failed | `BLOCKED` |

**Limits.** The rule's effect on a non-administrator cannot be observed from the administrator account used here: the administrator bypass means a direct push to `main` or a merge of a failing pull request would still be accepted,
so the `BLOCKED` state is evidence of the rule, not of a rejected push. No second account tried to push. Bypass is `always`, not `pull_request`; tightening it later removes direct pushes for the maintainer too.
A pull request needs no review (no approval rule is configured). Required checks apply to `main` only; tags and other branches are unprotected. A runner label or image change fails the leg closed rather than skipping it.

## Integration workflow (2026-11-11)

`integration.yml` (manual `workflow_dispatch`; not a required check, no write permission, actions pinned by SHA like `ci.yml`) runs three jobs on `ubuntu-26.04` and `ubuntu-26.04-arm`. Inputs are pinned in
`.github/ci/integration-pins.json`; `tools/ci_integration.py` checks size and SHA-256 of every download and treats an unpinned input as a failure, because a skipped suite looks like a pass (7 unit tests, including each
empty field, a non-https URL, a malformed digest, a wrong size and the committed pin file).

| Job | What it proves | State |
| --- | --- | --- |
| `redirect` | The pinned published Preview (`.github/ci/pins.json`) against the pinned public SDK bytes behind two loopback TLS origins: the 54-case restricted-redirect replay | passes on both hosts |
| `pins` | Every integration input is pinned | fails on both hosts: the independent host-tools bundle is not pinned |
| `ports` | The five input-gated port suites (`port-build\|store\|oracle\|command\|project-test`) | not run (`needs: pins`); the driver step fails explicitly and is not wired to a replay yet |

**First run.** `workflow_dispatch` run `42957820373` on `main` (`da487ec`): `redirect (linux-x64)` and `redirect (linux-arm64)` succeed with **54/54** cases each; both `pins` jobs fail with
"unpinned url, name, sha256, size, manifest_sha256, version"; `ports` is skipped. The workflow conclusion is therefore `failure`, and that is the intended state.

**What blocks the port suites.** (1) The CRT independent host tools (mksh, make, toybox applets) have no public release: the retained records call them local-only, and CRT's release list has only Dev SDK Previews.
Pinning a URL, size, archive digest and manifest digest requires CRT to publish a per-host bundle first; this repository neither builds it nor references a producer tree. (2) `tools/replay_port_linux.py` is a one-off
driver that takes the source tree, SDK, contract, host tools, archive and a seed CLI as arguments and uses Bubblewrap and strace; it has to be adapted for a hosted runner and cannot be exercised until (1) exists. The zlib
source is pinned (`zlib-1.3.1.tar.gz`, 1,512,791 bytes, SHA-256 equal to the recipe pin and measured against the public bytes).

**Schedule.** There is no `schedule:` trigger yet. A daily run that is red for a known reason that cannot be fixed here would teach readers to ignore red. The commit that fills the host-tools pin adds the trigger.

Not covered: macOS and Windows integration jobs, the oracle's pinned Python, the port suites themselves, and any repository secret (none is used).

## Linux integration driver preparation (2026-11-16)

Scope: prepare the CLI-owned driver while the independent public host-tools
bundles remain unavailable. This is separate from the 2026-11-11 hosted run
above. CRT issue [#2](https://github.com/webos21/crt/issues/2) is still open,
with no publication response as checked on 2026-11-16; the committed x64/arm64
host-tools fields remain null. No workflow, schedule, release, SDK/project
format or product-source change is made by this increment.

**Input and result interface.** `tools/ci_integration.py` owns online downloads
or explicit local input copies, size/SHA-256 checks, fresh work directories,
host-tools archive staging and final result judging. Extraction rejects
traversal/absolute/drive paths, escaping links, hard links, duplicate/case
collisions and member/expanded-size limits. The pinned manifest must have the
requested host/version and clean assembler provenance. Failed staging never
creates `port-inputs.json` or publishes the `host-tools` directory. A retained
`.extracting` tree belongs only to that failed disposable work directory.

`ports --prepare-only` writes an input record with `native_validation: not_run`
and no suite-pass result. An explicit `--pins` file and `--input-dir` allow
local test-only bundles without changing committed pins; byte verification
is identical to online preparation. Synthetic fixture executables are never
run or described as a native CRT bundle.

The native path downloads the existing public bootstrap CLI/SDK separately,
installs the SDK into a private HOME, then invokes the Linux adapter against a
fresh source copy. Bubblewrap, visibility probes and strace remain exclusively
in `tools/replay_port_linux.py`; it is not a universal runner. The adapter
records and verifies the explicitly selected oracle Python executable digest,
and labels the oracle's separate host-tool allowance. Other suites continue
to reject host shell/interpreter execution. Only an exact report of all five
required suites, each with a positive integer check count and no skip, all
passing cases, clean SDK policy and matching source/host-tools/contract input
identities is accepted by the common driver.

**Fast checks.** All 44 `test_ci_*.py` tests pass, including 21 integration-driver
tests. A synthetic archive exercises local and simulated-download paths,
manifest/host/version/provenance refusals, unsafe/bounded extraction, stale-work
refusal, missing/tampered bytes, pinned oracle validation and incomplete,
skipped, zero-check, failed or wrong-input result rejection. Separate real CLI
invocations prepare a valid synthetic fixture without a native result, reject
same-size tampering and a missing input, accept restored inputs in fresh work,
and reject both committed missing
host-tools pins before creating work. These are test-host checks, not five
native port-suite passes.

**Native local rehearsal: pass, 25/25 adapter checks, no skipped suite.** On
Linux/x86_64 `7.0.0-43-generic` with Ubuntu LLVM 21.1.8, the common driver's
full native path consumes the existing public x64 CLI and `.4` SDK pins plus
an explicitly local-only bundle archived from the earlier immutable handoff.
It installs into a fresh private store, builds a new CLI source copy, runs the
five suites, checks SDK/source-free CLI execution and ELF imports, probes
denied original-tree access, and checks input inventories and native
shell/interpreter traces. Actual port publication, exact-key reuse, verification
and a zero-command warm CLI build also pass.

| Suite | Checks passed |
| --- | ---: |
| `port-build-test` | 35 |
| `port-store-test` | 90 |
| `port-oracle-test` | 23 |
| `port-command-test` | 33 |
| `port-project-test` | 81 |

| Local rehearsal input / receipt | Exact identity |
| --- | --- |
| Review baseline (plus this driver preparation) | CLI `154ef31813d73d8effef9b604d848614e37d0ce5`; product/test source fingerprint `2e2cbdc60dcb60d9d3613bbabaf2f82cdf5c5652de1f92e7ba67e17ce008aca0` |
| Local-only host-tools manifest | `v0.0.1-hosttools.20261110.3`, SHA-256 `922c76155957a55d095f902077cb73bbc120ae2c2453e9c1c000f5e58acdefa9`; clean assembler CRT `0ab8f8cfbbde19e14dd5861ba176fb8881e06182` |
| Test-only host-tools archive | 1,184,348 bytes, SHA-256 `f32949c86706c35440c6527e90329c75cde107b20251662e743bfc58a5ffb897`; no public URL or promotion |
| Public SDK content / stored policy | `22107183e5127d44e979f328fa78aaf2a436db802aa9f8821a864a39d492ca81` / `369dae4fbe6c220a5f451431b2bbc76de947a7d618184abe0520424c4db84cfb` |
| Zlib source archive | `9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23` |
| Explicit oracle Python | `/usr/bin/python3.14`, executable SHA-256 `52e0a13e60a981d8c4b6478be2ba5176f69da07948a056bf49cf6f077e30cb41` |
| Fresh SDK-only CLI | SHA-256 `752fd57c99e660a909423261a83e672e35f413ac555e60c80e8248c16e530982` |
| Adapter `result.json` | SHA-256 `1dbad400b25db913d1a0f52a90d69081a6bd4196bfd9f95c2c6e9ee5b679a754` |

The local host-tools fixture comes from
`/home/syszone/GitLocal/crt-preview-20261110/handoff-r4/host-tools/`; it is copied
through a test archive, never rebuilt or modified here. Its `promotion` remains
`local-only`. This rehearsal is not evidence of a published bundle and adds no
Linux/aarch64 hosted acceptance. The project's existing SDK-provider C shared
control still prints host `libz`; it remains explicitly unendorsed under
[CRT 6.4](sdk_install_acceptance.md#isolation-scope-and-observed-control-limitation),
not accepted SDK shared binding. The oracle still reports differing shared
library/pkg-config bytes; its existing semantic comparisons pass and do not
claim byte reproducibility.

Evidence is retained under `out/ci-integration-prep-20261116/`, including
`host-tests.log`, `synthetic-cli-results.json`, `committed-pin-refusals.json`
and the explicit local-test pin file/command, copied `native-result.json`,
`native-inputs.json`, `native-commands.json`, `native-port-inputs.json` and
`verification.json` (driver/adapter hashes and unchanged-boundary checks).
Full logs and traces remain at
`/home/syszone/crt-cli-ci-integration-20261116/native-r1/`, with native scratch
`native-work/d3-linux-jecfcxuc/`. Reproduction and ownership:
[Linux integration driver](../guides/development.md#linux-integration-driver).

**Still required for D1:** publish/promote acceptable producer bundles for both
Linux hosts, review/pin those public bytes, add the hosted invocation arguments
and `schedule:` in the activation commit (including declared LLVM, isolation
prerequisites and the selected per-host oracle Python identity), then record all five suites with no
skip on both hosted runners and the scheduled execution. Local test-only input
success cannot close this gate. macOS/Windows integration remains separate D2;
actual CLI Preview publication remains an independent release gate.

## Release workflow (2026-11-11)

`release.yml` is started by hand (`workflow_dispatch`: host, a full commit SHA, date, serial, a notes template path, `dry_run` defaulting to true). Three jobs, driven by `tools/ci_release.py` (14 unit tests):

| Job | Permission | Does |
| --- | --- | --- |
| `package` | read | refuses a commit that is not on `main` or has no successful `CI gate`; builds exactly that commit with the pinned published CLI and public SDK through `ci_bootstrap.py`; assembles the notices; packs twice and requires identical bytes |
| `verify` | read | on a separate runner: archive digest against its sums file, `package_cli_archive.py verify`, `--version`, self `doctor` with the manifest pin, a wrong pin refused, a real GitHub `sdk acquire`, then C and C++ `new`/`build`/`test`/`run`/warm build |
| `publish` | `contents: write` | waits for the `release` environment; re-checks the digest, renders the notes template, stages a draft, downloads it and compares byte for byte, then publishes a non-latest prerelease (or stops at the draft) |

The `release` environment requires reviewer `webos21` and deploys from `main` only. `prevent_self_review` is off because a one-person repository could not publish otherwise; the approval is therefore a deliberate pause, not independent review.

**A design flaw found before the first run.** `publish` required the notes to contain the archive SHA-256, which only exists after the build, so a note committed in advance could never pass. The notes are now a reviewed template with explicit tokens
(`{{ARCHIVE_SHA256}}`, `{{ARCHIVE_BYTES}}`, `{{COMMIT}}`, `{{TAG}}`); a template without the digest token or with an unknown token is refused, and a digest typed by hand does not stand in for the token.

**Local run first.** `ci_release.py package` on the native aarch64 machine produced the archive, with a first attempt (`release-ci-check-r1-failed`) stopping on a bare `StopIteration` because the artifact is named `crt-cli-<host>`; the executable is now located by
its verified digest and the SDK by the identity the summary records. `verify` passed all steps against that archive.

**Hosted dry run.** Run `42963992535` (`main`, `8063bbf`, `linux-arm64`, serial 9, `dry_run: true`): `package`, `verify` and `publish` succeeded. The draft `v0.1.0-preview.20261111.linux-arm64.9` targeted `8063bbf`, held the 5,036,724-byte archive and the sums file,
and its notes carried the real digest (`67d2074d...d703`) with no leftover token; the archive, the sums file and the notes agreed after an independent download. The draft was then deleted; no tag was created and no release other than the four published ones remains.
**An approval record exists for that run** (`approved`, `webos21`) that the assistant did not create; who pressed it and how is not known from the record. Future runs should add an approval comment.

**Rejections.** Run `42965008964` with `6a00885`, which has no `CI gate` check, stopped in `package` ("CI gate on ...: none"); `verify` and `publish` were skipped and nothing was created. Locally, the publish driver also refused a branch name instead of a SHA, notes without the digest,
a tampered archive and an already-published tag (checked against the real serial-1 release, read only).

Not covered: an actual publication (`dry_run: false`), the other two hosts, the environment's behaviour for a second reviewer, and workflow-level tests for `actions/download-artifact` across a failed upload.

## First real publication and its signatures (2026-11-17)

`release.yml` ran with `dry_run: false` on `main` at `eab525c` for both Linux hosts (runs `43857798919` linux-x64, `43857804937` linux-arm64, serial 1, date 20261116). `package`, `verify` and `publish` succeeded for both; `publish` waited for the `release` environment and the approval was recorded as `approved` by `webos21` **with an empty comment**, so the target commit, tag, digest and reviewer are not written into an approval comment as the TODO item asked. As before, one maintainer approving their own release is a deliberate pause, not independent review.

External consumption, from the public URLs in a fresh directory: both archives matched their release sums file and the digests in the notes (x64 `52fbe5d1...7fdca`, 5,261,236 bytes; arm64 `311bc451...77aa`, 5,063,404 bytes); both tags resolve to `eab525c`; the extracted x86_64 executable reports `crt-cli 0.1.0-dev`. The arm64 archive was not executed here (no arm64 machine in this session); its draft-stage `verify` job ran on a hosted arm64 runner.

Signatures: the owner signed both archives offline; the files arrived named `...preview.20261116.1.sig` and were stored as `<archive>.sig` (256 bytes each). Both verified with `openssl` and were then attached to their releases (no existing asset changed). From the public URLs, `crt-cli fetch --publisher-key --signature` returned `signature_verified` for both, and the x86_64 signature against the aarch64 archive is refused with `publisher_signature_invalid`.

Not covered: macOS and Windows Previews (the workflow supports Linux only), executing the arm64 archive from the public asset, and the CI bootstrap pin, which still names the older Preview.

## Windows/x64 Preview workflow preflight (2026-11-17)

`release.yml` accepts `win-x64` for separate `package` and `verify` jobs on
`windows-2022`. Both use the same LLVM 22.1.8 installation as Windows CI.
Verification shares Windows SDK import-library discovery with bootstrap CI,
uses the Windows root certificate store, and checks the extracted `crt-cli.exe`
with `llvm-readobj` before launch. It records the PE machine and imports in
`verify.json`. The common `main`/`CI gate` check, test set, artifact transfer,
and approval-gated `publish` job are unchanged.

**Native package replay.** On Windows/x64 build 26100, LLVM 22.1.8, Python
3.11.9 and Windows SDK `10.0.28000.0/um/x64`, the unchanged `package` command
built a clean clone at `71682966a7512f42196e6e7a2f5c216a295e6963` with
`--host win-x64 --date 20261117 --serial 91 --llvm-bin 'C:/Program Files/LLVM/bin'`.
It authenticated the pinned public Windows bootstrap archive, then acquired
the pinned Windows Dev SDK `v0.0.1-preview.20261110.windows.1` (content
`5bb55372ec10e3cfa24c84de981a7130f56aeb57ef0d43d5da22a65036dec7ef`)
through the older bootstrap's digest-based SDK acquisition.

Native, archive, SDK-store and recipe tests passed **991/214/113/142** checks.
Both cold self-builds executed 21 commands; stage 2 reused 21 on a warm build.
The executable bytes differ between the two stages; both pass the standalone
help/version/doctor and direct-import checks. Two packings of the stage-2
package produced identical 5,459,168-byte archives. This proves deterministic
packing of one build, not byte-identical self-builds.

| Local rehearsal identity | Value |
| --- | --- |
| Label only, no tag or release created | `v0.1.0-preview.20261117.windows-x86_64.91` |
| Archive SHA-256 | `a651b9626a1d7be5a36968a520a204a56a45b0ea7e194768c8edd33e1e9cb944` |
| Executable SHA-256 | `dccfa15b735e123c07d4ebb81315cf0497e080b76f06fc6df7c7e83ea217a076` |
| Package manifest SHA-256 | `e83c1624000cd9b61966c51fb8b0b58098773a8fe519235305a5f48b2767e7c2` |
| Verification receipt SHA-256 | `315bceaf57cd5ece0a40a86888e1327f7ee7c5e150aec2df5f538dfb9cf2c30b` |

**Separate native verification.** Ran the updated workflow's Bash `verify`
step on this device in a fresh workspace with spaces and Korean text, using
the copied archive and checksum. All 30 package files verified. The extracted
PE is x64 (`0x8664`) and imports only `kernel32.dll` and
`api-ms-win-core-synch-l1-2-0.dll`. Version, self doctor, wrong resource pin
refusal, public GitHub SDK acquisition and both C/C++ create/build/test/run
flows pass. Both first builds executed three commands and both warm builds
executed zero, with unchanged project locks. The updated verifier also passes
on the existing public Windows Preview. A one-byte change to that archive is
refused before extraction with the original checksum.

**Draft comparison and limits.** A local fixture substitutes only the `gh`
operations of the unchanged `publish --draft-only` driver. It checks the
commit, tag and digest in the rendered notes and compares the archive and
checksum bytes after copying them back; it never invokes `gh release edit`.
This is an offline check of the shared draft logic. No Actions run, GitHub
draft, actual publication, new signature, or pin adoption is claimed.
Hosted rehearsal still requires the change on `main`, a passing `CI gate` for
the selected commit, and approval on the `release` environment.

All 100 Python tool tests pass in the native Windows test environment, and
`actionlint` 1.7.12 passes. An initial test run in the restricted app shell
failed at real-symlink fixtures; the native run passed without changing those
tests. Logs and receipts are retained under `out/wrel17/`, including
`pkg/ci/artifact/ci-summary.json`, `pkg/release/release.json`,
`fresh runner 한글/verify-work/verify.json`, and `draft-replay.json`.

### Hosted Windows/x64 draft rehearsal (2026-11-17)

The [required CI run](https://github.com/webos21/crt-cli/actions/runs/44004872940) passed all five native jobs and `CI gate` for
`b6cc5af00220578839f6e173eccd8e89c809ee99`. The [release run](https://github.com/webos21/crt-cli/actions/runs/44012191646)
(`main`, `host: win-x64`, that commit, date `20261117`, serial `9`, `dry_run: true`) passed `package` and `verify` on separate
`windows-2022` runners and `publish` after `webos21` approved the `release` environment (comment `OK`). The run took 30 minutes
from dispatch to completion, including the approval. The verifier acquired the pinned public Windows Dev SDK `v0.0.1-preview.20261110.windows.1`
(content `5bb55372...c7ef`), verified all 30 package files and ran both C and C++ workflows: each first build executed three
commands, each warm build zero, with identical project-lock bytes (`bc10a55b...656e`). The extracted `crt-cli.exe` is PE machine
`0x8664` and imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`.

The draft `v0.1.0-preview.20261117.windows-x86_64.9` is a pre-release draft that targets `b6cc5af`. Its 5,459,460-byte archive has SHA-256
`cc41be6274726e36bca45f9ee56adacf3c0e037e2ecf22755f52acb19348cc33` (the executable is `bd6a9630...5420`) and was re-downloaded from GitHub
and compared with `cmp`: the archive and `SHA256SUMS-windows-x86_64` are byte-identical to the `package` job's artifacts, and the digest
matches the sums file. The package consumed the bootstrap CLI `v0.1.0-preview.20261110.windows-x86_64.1` after a verified publisher signature.
This record is the second Windows build of the workflow's inputs: the local preflight above (serial 91, no GitHub release) packed a different commit and is not reused.

Receipts, the downloaded draft and the run artifacts are retained under `out/windows-release-workflow-20261117/hosted-44012191646/`.
Limits at that point (all closed by the sections below: publication, signature, public replay and draft cleanup): nothing was published, no `.sig` was attached (the owner signs offline, as for the other hosts), and the draft is **not deleted**; delete it by hand
(or sign and publish a separate serial) as the workflow header says. Windows/x64 is therefore at the same gate as macOS/arm64 was after its draft rehearsal;
the first hosted publication (`dry_run: false`), the signature replay and a public-asset replay remain open.

### First hosted Windows/x64 publication (2026-11-17)

The [release run](https://github.com/webos21/crt-cli/actions/runs/44015918992) used `main` workflow revision `ec0f105` and built the
reviewed commit `b6cc5af00220578839f6e173eccd8e89c809ee99` (the commit of the draft rehearsal above, `CI gate` green) with `host: win-x64`,
`dry_run: false`, date `20261117` and serial `1`. `package` and `verify` passed on separate `windows-2022` runners; `webos21` approved the
`release` environment with the comment `OK`, and `publish` passed. The pre-release
[`v0.1.0-preview.20261117.windows-x86_64.1`](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261117.windows-x86_64.1) was published at
2026-11-17T04:34:34Z (not a draft, not the latest release). The Git tag resolves to `b6cc5af`.

The public 5,458,816-byte archive has SHA-256 `02396e04cd4775cb7cfd65bd5c0e2c8975c4fa5b4ad18cfa039f65bf5cdd706f` (executable `feb97791...84df`). It was downloaded
from the public release and compared with `cmp`: the archive and `SHA256SUMS-windows-x86_64` are byte-identical to the `package` job's artifacts, and the digest matches the
sums file and `release.json`. The hosted verifier ran on those same bytes: 30 package files, the x64 PE (`0x8664`) importing only `kernel32.dll` and
`api-ms-win-core-synch-l1-2-0.dll`, public Windows Dev SDK `v0.0.1-preview.20261110.windows.1` acquisition, and both C and C++ workflows (cold 3, warm 0 commands,
lock `bc10a55b...656e`). Receipts and the public download are retained under `out/windows-release-workflow-20261117/hosted-44015918992/`.

Not covered at publication time: the owner's offline `.sig` was not attached yet (see the next section) and the earlier draft `...windows-x86_64.9` still existed; no pin adopts this release.

### Windows/x64 Preview signature and public replay (2026-11-17)

The owner signed the archives offline and placed the two 256-byte detached signatures on this device. Before anything was uploaded, `tools/publisher_signature.py`
verified each against the exact archive bytes with the repository's pinned public key (`.github/ci/crt-release-publisher.pub`, SPKI SHA-256
`59218576956e493476bc36f7385d8e01a03ff9b92946a531b013ad3937d43fa2`, identical to CRT's `docs/security/crt-release-publisher.pub`):

| Asset | Signature SHA-256 | Result |
| --- | --- | --- |
| `...preview.20261117.1.tar.xz` (public) | `cfad3442...5587` | verifies |
| `...preview.20261117.9.tar.xz` (rehearsal draft) | `836682bb...8a39` | verifies |
| each signature over the other archive | | refused |

Both `.sig` files were then uploaded as **new** assets of their releases (`gh release upload`, no replacement; the archive and `SHA256SUMS-windows-x86_64` are unchanged,
and the draft stays a draft). From a fresh download of the public `.1` release into a new workspace the signature still verifies, and `tools/ci_release.py verify`
(unchanged) ran on the public bytes natively on this device (Windows 11 build 26100, LLVM 22.1.8, a work path with spaces and Korean): archive SHA-256 `02396e04...706f`
matches the checksum file, all 30 package files verify, the PE is x64 importing only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`, public Windows Dev SDK acquisition
works and the C and C++ workflows build, test and run (cold 3, warm 0 commands, lock `bc10a55b...656e`). Receipts: `out/windows-release-workflow-20261117/` (`signatures/`, `public-replay/`).

Limits: the signature is checked with this repository's Python verifier, not with `crt-cli fetch --publisher-key` on the public download; the `release.json` used for the public replay is the
package artifact's, not a file served by GitHub; the `.9` draft was signed only because the owner supplied that signature (it was deleted afterwards, see below); no pin adopts this release.

### Rehearsal draft cleanup (2026-11-17)

At the owner's request the two serial-9 rehearsal drafts, `v0.1.0-preview.20261117.windows-x86_64.9` (with its attached `.sig`) and
`v0.1.0-preview.20261117.macos-arm64.9`, were deleted with `gh release delete`. Both were drafts; no Git tag existed for either. Afterwards the repository
lists no draft release, and the published `.1` releases (assets, signatures and tags `b6cc5af` and `b51d103`) are unchanged. The rehearsal run records, receipts under
`out/` and the digests recorded above remain as evidence; a rehearsal draft carries no authority once deleted, and the next rehearsal uses a new serial.

## macOS/arm64 Preview workflow preflight (2026-11-17)

`release.yml` now accepts `macos-arm64` for `package` and `verify` on separate
`macos-15` runners. Package uses the shared public bootstrap and Dev SDK pins.
Verify selects the Xcode tools with `xcrun`, exports the macOS system CA roots,
checks the extracted Mach-O architecture and records the SDK identity and C/C++
build results in `verify.json`. The approval-gated `publish` job uses the same
archive, checksum and draft comparison path as Linux.

**Native preflight.** On macOS 27.2.1 (`26B131`), Xcode 27.2 (`27B52`),
Apple clang 21.0.0, `ci_release.py package --host macos-arm64` ran from a
clean checkout of `9a2de21847f9a3fe938a439b40fa1beade291079`, with date
`20261117` and serial `1`. The bootstrap archive signature verified and the
CLI consumed the pinned public macOS Dev SDK `v0.0.1-preview.20261110.macos.1`
(content `e3dc7fb1810a3f78ab8ef33882225ccb562d21f6f5977a2037ab92819326bac8`).
The native, archive, SDK-store and port-recipe suites passed 941/221/114/142
checks. Both cold self-builds executed 21 commands, with different executable
hashes. Stage 2 reused all 21 outputs on a warm build. The Mach-O imports only
`libSystem`, has no `LC_RPATH`, and declares minimum OS 27.2 in this local build.

Two packings of that stage-2 package produced identical 1,234,032-byte
archives (SHA-256 `fc2c77c7a4d26286c1cf0e310de8e98b8e7b6c241834fad5b02d599d1bb78bc2`).
The updated `ci_release.py verify --host macos-arm64` used a separate fresh
workspace and passed ARM64, inventory, resource pin, wrong-pin refusal, real
GitHub SDK acquisition, and both C/C++ create/build/test/run flows. Each first
build executed three commands and each warm build executed zero, with identical
lock bytes across the two builds. The same verifier passed against the existing
published macOS Preview after checking its size, digest and detached signature.

**Regression checks.** All 97 Python tool tests and `actionlint` pass. Both
published `20261116` Linux CLI Previews pass the updated `verify` command in
Ubuntu 26.04 containers on the test Mac (aarch64 and x86_64 with emulation for
x86_64), using fresh SDK stores and LLVM 21. The records are local evidence.

Local receipts and logs are retained in `out/macos-release-workflow-20261117/`.
The [release procedure](../guides/development.md#publish-a-cli-preview)
describes the shared inputs and evidence for each gate.

### Hosted macOS/arm64 draft rehearsal (2026-11-17)

The [required CI run](https://github.com/webos21/crt-cli/actions/runs/43994193784)
passed on all five runners for `b51d103e7d0cf3b1816e0d3671963811ea7bf07e`.
The [release run](https://github.com/webos21/crt-cli/actions/runs/43995986848)
then passed `package` and `verify` on separate `macos-15` runners with
`dry_run: true` and tag `v0.1.0-preview.20261117.macos-arm64.9`. The verifier
acquired the pinned public SDK and ran the C and C++ workflows. Each first
build executed three commands and each warm build executed zero with unchanged
lock bytes. The ARM64 Mach-O declares minimum macOS 15.0 and SDK 15.5, imports
only `libSystem`, and has no `LC_RPATH`.

`webos21` approved the `release` environment with the comment `Good to go`.
The commit, tag and digest are recorded here. `publish` passed and stopped at
the draft. An independent download confirmed that the 1,248,552-byte archive
and `SHA256SUMS-macos-aarch64` are byte-identical to the package artifacts.
The SHA-256 in the package receipt, verification receipt and draft notes is
`39a6250fd122f76df9099f1617b5df2a9f70a466419f46f2250c0e20aafbb1a6`.
The GitHub release record confirms draft status and the exact target commit.
Hosted receipts are retained under `out/macos-release-workflow-20261117/hosted-43995986848/`.
The owner's offline `.9.tar.xz.sig` was subsequently attached to this draft.
The detached 256-byte signature re-downloaded from GitHub verifies against
the draft archive. Its SHA-256 is
`b7bcf141a067d173fde02228ab66497e1f8381f670e3eb50279d88e9bfd13027`.

### First hosted macOS/arm64 publication (2026-11-17)

The [release run](https://github.com/webos21/crt-cli/actions/runs/43998835624)
used `main` workflow revision `7f2381d447a1b54c4e029868aae8dd07c06ee52e`
and built the reviewed commit `b51d103e7d0cf3b1816e0d3671963811ea7bf07e`
with `dry_run: false`, date `20261117` and serial `1`. `package` and `verify`
passed on separate `macos-15` runners. `webos21` approved `release` with the
comment `OK`. The tag and full archive digest for that approval are recorded
in the [public asset evidence](preview_acceptance.md#github-actions-macosarm64-preview-release-2026-11-17).
`publish` passed and the Git tag resolves to the reviewed commit.

The public archive and checksum matched the hosted receipts, and the public
executable passed native macOS SDK acquisition and both C/C++ workflows.
Release, approval, tag, package, verification and public replay receipts are
retained in `out/macos-release-workflow-20261117/hosted-43998835624/`.
The owner signed the `.1` archive offline. After attachment, the detached
signature verified on a fresh public download and the signed archive passed
the native replay described in the public asset evidence above.

### macOS/arm64 SDK publisher signature replay (2026-11-17)

The CLI from the signed `.1` Preview and the pinned public macOS Dev SDK
`v0.0.1-preview.20261110.macos.1` were exercised on macOS 27.2.1/arm64.
With test-only keys and loopback HTTPS, `tools/replay_publisher_acquire_linux.py`
passes all 26 acquisition and local-install cases on this host. The cases
cover explicit authentication, unsigned input, installed SDK reuse without
rechecking, wrong keys and signatures, invalid pins, and failure without
publishing an SDK or leaving staging files. The result SHA-256 is
`fd8b9a3f3fc2b57f0d8a9c6a46d6fdef26c51144da30ad4eee5ec0f30cf06dbf`.

In separate fresh homes, the shipped executable acquired the SDK from
GitHub with `publisher_authentication: signature_verified`, and installed
the public SDK archive with the actual publisher key and `.sig`. A repair
with a modified signature preserved the damaged contract and original
receipt without leaving staging or locks. Repair with the valid published
signature restored the original contract and `sdk verify` passed. Receipts
are in `public-signed-v3hnxbd0/`, `sdk-publisher-replay.GvrKY50N/` and
`sdk-signed-repair.VRCPzgS4/` under the hosted receipt directory above.

## CI pin promotion (2026-11-11)

[The policy](../guides/development.md#ci-pin-promotion) fixes when a pin may move, what moves with it, the evidence a pull request must quote, what must pass and how a new host is added. `tools/check_ci_pins.py` re-downloads each host's published CLI archive and SDK manifest, archive and
contract and compares size, SHA-256 and the manifest's listing; it reported `OK` for `linux-x64`, `linux-arm64`, `macos-arm64` and `win-x64` against `.github/ci/pins.json`. Six unit tests make it report a changed bootstrap archive, a size-only change, a changed SDK archive or contract,
an archive missing from the manifest and a contract digest the manifest disagrees with, so that `OK` is not the only output ever seen.

Not covered: automatic detection that a newer Preview exists, a scheduled run of the checker, and any second reviewer.



## Publisher signatures for host-tools pins and `sdk acquire` (2026-11-16)

The publisher key (fingerprint `59218576...43fa2`, [CRT record](https://github.com/webos21/crt/blob/main/docs/security/publisher_signing.md)) signed the
two host-tools archives, five Dev SDK archives and four CLI archives. The consumer side now accepts and requires it where the pins are CLI-owned.

- **`sdk acquire` source** gains the optional `publisher` object ([contract](../reference/bootstrap_contract.md#explicit-pinned-https-sdk-source-experimental-format-0)). Native, no
  OpenSSL. Replay `tools/replay_publisher_acquire_linux.py` (loopback HTTPS, throw-away key, real published `.4` release): 15/15 - no `publisher`
  stays `not_verified`; the right key verifies with request order manifest, contract, signature, archive; a reused SDK is flagged not re-checked;
  a key file that did not sign, a valid signature by another key against the pinned fingerprint, a signature over other bytes, a signature digest
  pin mismatch (archive never requested), a missing signature asset (archive never requested), an oversized signature pin, a wrong fingerprint,
  a relative key file, an unknown field, an `http` signature URL and a missing signature object all fail with no SDK and no staging left.
  Live check with the real key through the GitHub redirect policy, public `.4` archive and its `.sig`: `signature_verified`, key `59218576...43fa2`.
- **CI pins** carry a top-level `publisher` (`.github/ci/crt-release-publisher.pub` and its SPKI SHA-256, which the driver recomputes and compares) and a
  `signature` pin (`url`, `name`, `size`, `sha256`) per host-tools entry. `ci_integration.py` fails if either is missing, downloads the signature under
  its pin, and verifies the archive with the pinned key (`openssl dgst`, a build-host tool) before anything is extracted; `pins` checks their presence.
  Local `--input-dir` fixtures are unpublished and record `publisher_authentication: not_verified`; a download-mode run records `signature_verified`
  and the key fingerprint in `port-inputs.json`. Tests: 27 `test_ci_integration` (a tampered signature, a missing signature pin, a missing key and
  every edit of the publisher pin fail). `ports --prepare-only` against the real public x86_64 bundle: `signature_verified`; a changed signature pin: fail.
- Full `crt-cli test` on a clean copy with this change, built against the public `.4` SDK with explicit SDK root/contract: 913 native checks, archive 221,
  SDK store 114, port recipe 142 pass; the five input-gated port suites were not run for this change (it does not touch that path).
- Not covered by that first step (see the next section for what followed): the zlib source (third-party, digest-pinned only), and any claim beyond "the holder of this key
  signed these bytes". The fingerprint's independent publication is still partial (repository and project site, both under the same GitHub account).

## Publisher signatures for the bootstrap CLI pin and `sdk install` (2026-11-16)

- **`sdk install` / `sdk repair`** take `--publisher-key --signature [--publisher-key-spki-sha256]` ([contract](../reference/bootstrap_contract.md#explicit-pinned-https-sdk-source-experimental-format-0)).
  The replay `tools/replay_publisher_acquire_linux.py` now has 26 cases (15 acquire + 11 install): the right key and fingerprint install and report
  `signature_verified`; no options keep the unchanged result; a signature by another key or over other bytes, a key file that did not sign, a
  mismatching fingerprint pin and a missing signature file install nothing and leave no staging; a key without a signature, a signature without a
  key, a fingerprint pin without a key and a malformed fingerprint exit 2. A live install of the public `.4` archive with its real `.sig` and the
  published key passes.
- **Bootstrap CLI pin and SDK pin.** `.github/ci/pins.json` carries the same `publisher` pin as the integration pins and a `signature` pin
  (`name`, `size`, `sha256`) for each host's bootstrap archive and SDK archive (all eight were signed on 2026-11-16). `tools/ci_bootstrap.py` downloads the
  `.sig` next to the bootstrap archive and verifies it with the pinned key *before* extracting or running the CLI; `ci_integration.py` does the same for its
  published CLI; a pin without a signature fails. The summary records `publisher_authentication: signature_verified` and the key fingerprint.
  `tools/check_ci_pins.py` (the review aid) now also compares each pinned `.sig` with the published bytes and verifies each bootstrap and SDK archive with the pinned
  key: OK on linux-x64, linux-arm64, macos-arm64 and win-x64 against the real public releases.
- **Verifier.** The Python side uses `tools/publisher_signature.py`, a stdlib-only RSASSA-PSS verifier (SHA-256, MGF1-SHA256, salt 32, RSA 2048-4096), so the
  macOS and Windows hosts need no OpenSSL. It verifies all nine real release signatures and the two host-tools signatures; `test_publisher_signature` (6 cases against
  OpenSSL-made keys: wrong key, bit flips, other salt/MGF/hash/PKCS#1 v1.5, wrong length, 1024-bit and EC keys refused), `test_check_ci_pins`, `test_ci_bootstrap`
  and `test_ci_integration` pass (70 tests with `test_ci_release`). The integration driver's earlier OpenSSL call was replaced by it.
- Full `crt-cli test` with these changes (clean copy, public `.4` SDK, explicit root/contract): 913 native checks, archive 221, SDK store 114, port recipe 142 pass; the five
  input-gated port suites were not run.
- **Still not covered (Linux superseded 2026-11-17, see the next-but-one section).** The SDK archive used by CI is still acquired by the pinned *bootstrap CLI*, which predates the `publisher` source field, so that download
  is digest-pinned in CI and only the review aid checks its signature; it becomes native once a CLI Preview with this change is pinned as the bootstrap.
  macOS/Windows native replays of the new options were not run here; the zlib source stays digest-only; the key's independent publication is still partial.

## Linux bootstrap pin moved to the 20261116 Preview (2026-11-17)

`.github/ci/pins.json`: `linux-x64` and `linux-arm64` now bootstrap with the Previews published by `release.yml` on 2026-11-17 (`v0.1.0-preview.20261116.linux-x86_64.1`, `...linux-arm64.1`; sizes, digests and signature pins as published). Their `sdk acquire` understands the source `publisher` object, so a bootstrap pin flagged `sdk_source_publisher: true` makes `ci_bootstrap.py` add the pinned key, its fingerprint and the pinned SDK-archive signature to the SDK source, and the run fails unless the acquisition reports `signature_verified`. `macos-arm64` and `win-x64` keep the older CLI and the unchanged source; an older CLI rejects unknown source fields, which is why the flag is per host.

Evidence: `tools/check_ci_pins.py` OK for all four hosts against the public bytes (including the new bootstrap signatures); 59 Python tests pass (3 new for the source shape); a full local `tools/ci_bootstrap.py --host linux-x64` run on Linux/x86_64 (LLVM 21) passed: bootstrap archive and the SDK archive both `signature_verified`, native 913, archive 221, SDK store 114, port recipe 142, stage 1 and stage 2 byte-identical. Summary: `publisher_authentication` for bootstrap and SDK.

Hosted: pull request 14 (`6ba425b`) passed `CI gate` with all five native legs; the downloaded `ci-summary.json` of linux-x64, linux-x64-ubuntu2404 and linux-arm64 record `v0.1.0-preview.20261116` as bootstrap with `signature_verified` for the bootstrap and the SDK archive (913/221/114/142, stages identical); macOS and Windows record `signature_verified` for their older bootstrap only. Merged as `1dcae3a`; the `push` run on `main` (`43989300254`) passed `CI gate` with every leg.

Not covered: macOS/Windows (still digest-only for the SDK download), and a second reviewer (the merge was a one-maintainer step).

## Host-tools pins and the wired `ports` job (2026-11-16)

CRT published the independent host-tools Previews ([crt#2](https://github.com/webos21/crt/issues/2),
[CRT record](https://github.com/webos21/crt/blob/main/docs/acceptance/host_tools_acceptance.md#linux-host-tools-preview-publication-2026-11-16)).
`.github/ci/integration-pins.json` now pins both Linux hosts from the CRT release records (clean tree `5fc1a7d`, `promotion: preview`); the
`consumer_pin` values are quoted verbatim and the downloaded bytes were compared with them.

| Host | `version` | Archive size / SHA-256 | `manifest_sha256` |
| --- | --- | --- | --- |
| linux-x64 | `v0.1.0-hosttools.20261116.linux-x86_64.1` | 626632 / `81174849...2155` | `9bd3ddf1...f192a` |
| linux-arm64 | `v0.1.0-hosttools.20261116.linux-aarch64.1` | 589644 / `7a4c26ad...c75f` | `2ea7bff5...b3c2` |

**Driver.** A download-mode pin must name a `promotion: preview` manifest (`extract_host_tools(require_preview=...)`); explicit local `--input-dir`
fixtures are unchanged. Tests: 45 `test_ci_*` (the committed-pin test now asserts published-release URLs, names and versions; a local-only bundle is
refused online and leaves no `host-tools` directory).

**Workflow.** `integration.yml`'s `ports` job installs LLVM 21, Bubblewrap and strace, relaxes the AppArmor user-namespace restriction, records the
oracle Python's real path and SHA-256 at run time (a recorded identity, not an independently chosen pin), runs the common driver and uploads the replay
result. `schedule:` (daily 03:23 UTC) is added in the same change, as the request required.

**Native preflight, Linux/x86_64 (this machine, real public pins): all five suites pass, adapter 25/25, none skipped.** `ci_integration.py ports`
downloaded the pinned published CLI, SDK `.4` and the published host-tools archive, installed the SDK into a private store, built a CLI from this
source, and ran: `port-build` 35, `port-store` 90, `port-oracle` 23, `port-command` 33, `port-project` 81. `replay/result.json` SHA-256
`0da497f76f099f2df079bfb969b110ffe44394ca91a3a2fb38b5ba3e01c585de`; inputs matched `9bd3ddf1...` (host tools) and `9a93b2b7...` (zlib).

**First hosted run: pass on both Linux hosts.** `workflow_dispatch` run [43820315250](https://github.com/webos21/crt-cli/actions/runs/43820315250) on `main`
at `b1b6e44` (2026-11-16): `pins`, `redirect` and `ports` succeed on `ubuntu-26.04` and `ubuntu-26.04-arm`. The uploaded `replay/result.json` of each host
was read, not only the job status: `passed: true`, `allow_dirty_sdk: false`, 25/25 cases, and all five suites ran with no skip.

| Host | `port-build` | `port-store` | `port-oracle` | `port-command` | `port-project` | Host-tools manifest | SDK contract |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| linux-x64 | 35 | 90 | 23 | 33 | 81 | `9bd3ddf1...` | `369dae4f...` |
| linux-arm64 | 35 | 90 | 23 | 33 | 81 | `2ea7bff5...` | `a3df2fbd...` |

Both runs used the zlib source `9a93b2b7...`. Linux/aarch64 is the first native aarch64 evidence for the five suites and for the published aarch64
host-tools bundle's use by the CLI; the earlier aarch64 evidence for that bundle was CRT's own hosted replay. Each job ran about 11 minutes. The oracle
Python is the runner's `setup-python` interpreter, whose path and SHA-256 the job records at run time; that is a recorded identity, not an independently
chosen pin. Bubblewrap needed the AppArmor user-namespace restriction relaxed on the runner. The scheduled trigger fires from the next 03:23 UTC; its first
result is recorded in [First scheduled integration run](#first-scheduled-integrationyml-run-2026-11-18). This closes the CLI-side D1 Linux integration gate; hosted macOS/Windows integration remains the separate D2 item.

## macOS and Windows bootstrap pins moved to the 20261117 Preview (2026-11-17)

`.github/ci/pins.json`: `macos-arm64` and `win-x64` now bootstrap with the Previews that `release.yml` published and the owner signed on 2026-11-17
([`v0.1.0-preview.20261117.macos-arm64.1`](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261117.macos-arm64.1), built from `b51d103`;
[`v0.1.0-preview.20261117.windows-x86_64.1`](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261117.windows-x86_64.1), built from `b6cc5af`).
Only the bootstrap pin changes (release, asset, size, SHA-256 and the signature pin, all as served by GitHub and matching the digests recorded in the publication
and signature records above); the SDK pins of both hosts are untouched. Both builds descend from `eab525c`, which added the source `publisher` object, so the
pins are flagged `sdk_source_publisher: true` as the Linux pins are, and `ci_bootstrap.py` now gives their SDK acquisition the pinned key and archive signature.

| Host | Bootstrap archive | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| macos-arm64 | `crt-cli-0.1.0-dev-macos-aarch64-preview.20261117.1.tar.xz` | 1,248,592 | `5084b2b3...0cb` |
| win-x64 | `crt-cli-0.1.0-dev-windows-x86_64-preview.20261117.1.tar.xz` | 5,458,816 | `02396e04...706f` |

Evidence: `tools/check_ci_pins.py` is `OK` for all four hosts against the public bytes, including both new bootstrap signatures and the unchanged SDK signatures, and
`test_check_ci_pins.py` passes. Hosted: [pull request 15](https://github.com/webos21/crt-cli/pull/15) (`5b39a8e`, merge-ref run [44023277756](https://github.com/webos21/crt-cli/actions/runs/44023277756)) passed `CI gate` with all five native legs (macOS 12m57s, Windows 14m18s, Linux 4m53s-6m58s). The downloaded `ci-summary.json` of `macos-arm64` and `win-x64` record the new `v0.1.0-preview.20261117` bootstrap with the bootstrap CLI authenticated by the pinned publisher key and the SDK archive `signature_verified`, as the Linux legs do for `20261116`.

Not covered: a local `ci_bootstrap.py` run on Windows/x64 or macOS/arm64 (the hosted legs ran the same driver), and a second reviewer. The move does not change the SDK pins; it is separate from the Windows/arm64 wiring.

## Windows/arm64 host wiring and local candidate preflight (2026-11-17)

**Local candidate and driver wiring only; nothing is published, no leg is required and no pin is adopted.** The
[host-adding policy](../guides/development.md#ci-pin-promotion) (item 7) requires, in order, a public Dev SDK for the host, a published CLI
Preview built by that SDK alone, a hosted runner and a local pass. At this date `webos21/crt` has no Windows/aarch64 SDK release and this
repository has no `win-arm64` Preview, so the `ci.yml` matrix row and the `pins.json` host are **deliberately not added**: the driver fails with
"no CI pins for host win-arm64" instead of skipping, and a row without pins would turn the required `CI gate` red on `main`.

### What is wired

| Area | Change |
| --- | --- |
| `tools/ci_bootstrap.py` | host key `win-arm64` (Python `ARM64`/`aarch64` on `win32`), `--host` choice, the Windows SDK `um/arm64` import-library directory for that host (`um/x64` for `win-x64`), a clear error for a host without pins |
| `tools/ci_release.py` | `win-arm64`: tag `v0.1.0-preview.<date>.windows-arm64.<n>`, asset `crt-cli-0.1.0-dev-windows-aarch64-preview.<date>.<n>.tar.xz`, checksum file `SHA256SUMS-windows-aarch64`; the PE audit checks machine `0xAA64` for it and `0x8664` for `win-x64`, each refusing the other |
| `.github/workflows/release.yml` | host option `win-arm64`, runner `windows-11-arm`, native ARM64 LLVM 22.1.8 installed from the LLVM release (`LLVM-22.1.8-woa64.exe`, SHA-256 checked, `clang` must report `aarch64-pc-windows-msvc`), `--llvm-bin` for every `win-*` host, `actions/setup-python` told `arm64` for it (empty, i.e. the runner default, for every other host) |
| Tests | new and updated cases: host key and unsupported hosts, a host without pins, the import-library directory per host, ARM64 and x64 PE identity (each refusing the other), `win-arm64` names |
| `tools/replay_cli_package_windows.py` | new `--sdk-version`: the check "compiled-in SDK input" hard-coded the first x64 Preview's SDK version and failed for any other SDK; the default keeps the old value |

All five CI driver test modules pass (bootstrap 17, release 21, pins 11, integration 27, publisher signature 6); both workflow files parse. The
hosted runner label `windows-11-arm` is generally available for public repositories and CRT's own `ci.yml` already builds on it with the same
installer and `um/arm64` libraries, but **no crt-cli job has run on it**.

### Local SDK candidate (producer)

Clean CRT `ded8e99ec77f60269adf56340af0123c24372345` in a fresh worktree, build root `out/preview/build`, local label
`v0.0.1-preview.20261117.windows.arm64.1` (no tag, nothing uploaded): `crt-development-sdk-test` passes, `crt-c-test` passes 168 of 169 with
`fork_heap_snapshot_test_runs` skipped, `crt-dev-release` collects the six public-format assets. The first `crt-dev-release` failed because
the toolchain-contract export needs `CRT_WINDOWS_SDK_LIBPATH` (CRT's workflows set it explicitly); its partial output was kept, not deleted, and
the release was repeated with the variable set. CRT's independent asset verifier (`tools/ci_sdk_release.py verify --os windows --arch aarch64`,
importing none of the producer's code) reports `OK`.

| Asset | Bytes | SHA-256 |
| --- | ---: | --- |
| `...windows-aarch64-dev.zip` | 141,779,092 | `5ed0692fa19951ab0eee1862bd4662644cfaa9b3354ce3cc7caf8bd0f767251a` |
| `...windows-aarch64-dev-toolchain.json` | 441,104 | `c420ec22c6430acbdd29d03f819bc61b8a8887a9e0d01b6bc42518505d96a228` |
| `release-manifest-dev-windows-aarch64.json` (manifest pin) | 1,848 | `547b055376eb8e53d557ac0dc5a3fecafefd40b04c3f5d3706c322d5a96e6b3a` |

SDK content identity `4e1c71663dd9ec52866f874faed482577c22497d525f41c8f0c1a677542f397b`. CRT `main` has moved on (`1c441e9`: an Apple-only compiler flag and a port-build
environment change), so the bytes for an actual release must be rebuilt from the final `main`; this candidate only proves the path.

### Local CLI candidate and package

The sources are the working tree of `ci/windows-arm64-wiring` (`main` `d0efcaa` plus the uncommitted wiring above; driver source SHA-256
`9488335022250feb985826fd912ddb82b20a8494623f91ef51c8459c0a44d472`), built against the extracted candidate SDK by CRT's native Windows replay
driver (CRT-hosted seed, then self-build, hidden CRT and CLI checkouts, per-run SID, empty `PATH`, job-object accounting, a Hangul/accented/space work path).
Result: every step exit 0, **1005 native / 214 archive / 113 SDK-store / 142 port-recipe checks**, cold self-build 21 executed / 0 reused, warm 0 / 21, 635 recorded processes,
bare doctor `narrow_encoding: utf-8`, imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; executable
`eb8b1d2b0e734b37e52e9380db1c7e8fef92eddbba3bc033ae76569354100c1c`, 46,423,552 bytes (built `-O1 -g`).

**First attempt failed on path length, not on the product.** With the work root `...\Temp\crt arm preflight 한글 é 20261117` the deepest installed libc++
header path was 262 characters, over Windows' 260 limit, because the public-format SDK directory name
(`v0.0.1-preview.20261117.windows.arm64.1-4e1c71663dd9`) is about 20 characters longer than the earlier local names (`v0.4.0-cli.20261102-...`); `copy_file` and the archive test's long-name case failed.
A shorter Hangul/accented/space root (237 characters) passes. Keep the hosted `RUNNER_TEMP` and any local work root short; the hosted Windows legs have not been measured against the new name length.

Package: `create_bootstrap_resources.py` and `package_cli_archive.py pack` over the self-built executable and the candidate SDK's notices: 30 files, manifest SHA-256
`7ce2c11c50e6b307261ae629b82876289cee8aeb55e8364031eeab4c958ae291`; archive `crt-cli-0.1.0-dev-windows-aarch64-preview.20261117.1.tar.xz`, 5,393,124 bytes, SHA-256
`38b8004a59238663bdbfd2b5076e97a4e6fe84f0f9121014ae67e576bb6b50e2`; two packings are byte-identical. Extraction verifies all 30 files; the PE is ARM64 (`0xAA64`) with only the two allowed imports.

| Replay | Result |
| --- | --- |
| Package run, restricted token (`replay_cli_package_windows.py`) | first attempt **11/12**: the tool expected the x64 Preview's SDK version; the product output was correct (`explicit-at-fetch`, compiled-in SDK `...windows.arm64.1`). With `--sdk-version`: **12/12** (`07b5fe50...50c8`) |
| Consumer of the candidate SDK (`replay_preview_public_windows.py`, local download directory) | **31/31**: manifest and asset digests, pinned install and idempotent reinstall, wrong pin refused, C and C++ first build without configure, test, run, warm build executes nothing, locks and inputs unchanged (`7357a0c0...71f3`) |

### Not covered

No signature, no Authenticode, no GitHub release or draft, no hosted `windows-11-arm` run of any crt-cli workflow, no `ci_release.py package`/`verify` run for `win-arm64` (they need the
pinned public bootstrap and SDK that do not exist yet; the local steps above reproduce their assemble, pack, extract, audit and consumer parts but not the real GitHub SDK acquisition), no HTTPS
fetch cases and no Preview-notes text for this host. The package replay and consumer replay ran against a local download directory. Evidence is in the ignored local directory `cw/preflight/`
(`replay2/`, `package-run2/`, `consumer-run/`, `sdk-verification.json`); the failed first attempts `replay/` and `package-run/` are retained.

### Remaining order

1. CRT: add a Windows/arm64 row to `sdk-integration.yml` and the planned `sdk-release.yml`; rebuild the SDK from the final `main`, publish it and attach the owner's offline signature.
2. Here: build and package the first `win-arm64` Preview from that published SDK (the first Preview of a host cannot use `release.yml` because it has no bootstrap pin; use the local path above, then publish and sign it).
3. Pin both in one reviewed pull request that also adds the `ci.yml` matrix row (`windows-11-arm`, `--llvm-bin 'C:/Program Files/LLVM/bin'`, the ARM64 LLVM step) and the `win-arm64` host key to the pin tests; dispatch it manually before treating the leg as required.
4. Dispatch `release.yml` with `host: win-arm64` and `dry_run: true`, then publish and sign a Preview built by the workflow.

Status 2026-11-18: steps 1 to 3 are done ([first Preview, pins and row](#first-windowsarm64-preview-pins-and-the-ciyml-row-2026-11-18) and its
[hosted result](#hosted-windows-11-arm-result-of-the-win-arm64-pins-2026-11-18)); step 4 is still open in [TODO](../../TODO.md#ci-platform-expansion).

## Linux/x86_64 SDK pin moved to the first automated Preview (2026-11-18)

`.github/ci/pins.json` `linux-x64` SDK: `v0.0.1-preview.20261110.4` -> [`v0.0.1-preview.20261118.linux.x64.1`](https://github.com/webos21/crt/releases/tag/v0.0.1-preview.20261118.linux.x64.1),
the first Preview built, verified and published by the producer's `sdk-release.yml` ([run 44088028759](https://github.com/webos21/crt/actions/runs/44088028759))
and signed by the owner afterwards ([producer record](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#first-automated-dev-sdk-preview-s4-2026-11-18)).
Manifest `b76aea28...a641` (1753 bytes), content `3008315d...7a79`, contract `602e7a03...63d0`, signature `a15eb945...e50b`. Bootstrap CLI and other hosts' pins are unchanged.

Evidence: `tools/check_ci_pins.py` is `OK` for all four hosts. [Pull request 17](https://github.com/webos21/crt-cli/pull/17) passed `CI gate` with all five native legs, and the
`main` push run [44235742285](https://github.com/webos21/crt-cli/actions/runs/44235742285) (`8f4b163`) is green on all of them. Both Linux/x86_64 legs
(`ubuntu-26.04`, glibc 2.43; `ubuntu-24.04`, glibc 2.39) report `sdk.publisher_authentication: signature_verified`, SDK content `3008315d...`, suites
native / archive / sdk-store / port-recipe = 913 / 221 / 114 / 142 (unchanged from the `.4` qualification), identical stage-1/stage-2 bytes and 0 reused on a cold cache.

Not covered: the other three hosts still consume their earlier SDKs; the producer states no intended SDK change from `.4`, so equal counts are expected and not proof of equivalence;
no second reviewer (single-owner repository).


## First Windows/arm64 Preview, pins and the `ci.yml` row (2026-11-18)

**Published and signed; pinned; a local `ci_bootstrap.py` pass. The hosted `windows-11-arm` result is recorded separately once it exists.**
This follows the [host-adding policy](../guides/development.md#ci-pin-promotion) (item 7) and the order of the
[preflight record](#windowsarm64-host-wiring-and-local-candidate-preflight-2026-11-17).

| Input | Identity |
| --- | --- |
| Dev SDK | CRT `v0.0.1-preview.20261118.windows.arm64.1`, built by `sdk-release.yml` as a first-host bootstrap (consumer smoke recorded as NOT RUN), signed by the publisher; manifest SHA-256 `354ab45f94540877dda619d7613937a8caff413ad64fc4aa3f1b88c0a3fceecc` (1,847 bytes), installed content `3b251637845621ba28d6ba1d9c2f6a5e097f6b328f57054f7e7cb5c5eb52621f`, archive signature SHA-256 `50ec4f51089a37dc760bdef6e9e58af72a5acb73a90a41f3790cbf5d9cf1c308` |
| CLI Preview | `v0.1.0-preview.20261118.windows-arm64.1` at `2a025858fa72bb93bdd155eb82c2c2fe2145e530`, archive `crt-cli-0.1.0-dev-windows-aarch64-preview.20261118.1.tar.xz`, 5,234,024 bytes, SHA-256 `c5ff710532c0ff9191ae373b48a8f8fe5ceb04bea70647989f9146e255e46e11`, package manifest `89e58853ec874ec2d7391773d1606701e874fc338934f3f3e87bce6341e68057`, signature SHA-256 `b72eb649d476673d784a6f4f84c090f73da9df505372cfc9005d19fa2a2da5e6` |

**The Preview was not built by `release.yml`** (a host's first Preview cannot be: it has no bootstrap pin) and no reviewer approval gate applied.
It was built by hand on native Windows/arm64: CRT's native contract replay (`replay_cli_contract_windows.py`; CRT-hosted seed, then self-build) against
the downloaded, signature-verified SDK, then `create_bootstrap_resources.py` and `package_cli_archive.py pack` (two packings byte-identical,
extraction verified). Replay: cold self-build 21 executed / 0 reused, warm 0 / 21, 601 recorded processes, native suite with archive 214, SDK store
113 and port recipe 142 checks, imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; executable
`132c8b5ae15f66ed9dd82fcb5a425160e4d682c008ab218353b7eed30f6d3db8`, 45,776,384 bytes. Package replay under the restricted token
(`replay_cli_package_windows.py --sdk-version ...windows.arm64.1`) 12/12 and the consumer replay of the published SDK bytes
(`replay_preview_public_windows.py`, local download directory) 31/31. The draft was uploaded by `ci_release.py publish --draft-only`, downloaded
and compared byte for byte, its notes rewritten to say that it is a first-host manual build, published as a non-latest prerelease after the user
confirmed, and the owner's detached signature (verified with the publisher key; 256 bytes) was attached and re-downloaded identically.

**Failed attempts, kept.** Three contract replays failed before the passing one, all through the replay inputs and not the product: a source export
that was not a git checkout; `lld-link.exe` passed as `--linker` (the contract's GNU-style link command needs `ld.lld.exe`); and a work root whose
installed-SDK path reached 263 characters, over Windows' 260 limit (the public-format SDK directory name is long). A root of 257 characters or less
passes; keep it short. The failed work directories remain under the ignored local `cw/out1` and temp roots.

**Pins and row.** `pins.json` gains `win-arm64` (bootstrap, SDK, `allowed_imports`, `sdk_source_publisher`); `ci.yml` gains the `windows-11-arm`
row, `actions/setup-python` told `arm64` for it, the SHA-256-checked native ARM64 LLVM 22.1.8 installer step and `--llvm-bin` for it.
`check_ci_pins.py --host win-arm64` re-derives every pin from the published bytes: `OK win-arm64`. The five driver test modules pass (bootstrap 17,
release 21, pins 11, publisher signature 6, integration 27). **Local `tools/ci_bootstrap.py --host win-arm64`: `CI PASS`**, the bootstrap CLI and the
SDK both `signature_verified`, cold stage 1 and stage 2 21 executed / 0 reused, native suite 1,004 checks, archive 214, SDK store 113, port recipe
142, imports audited; stage 1 and stage 2 differ byte-wise (as on the other hosts), functional equivalence checked.

**Not covered.** No hosted `windows-11-arm` run of any crt-cli job at the time of writing (the pull request's own run and a manual dispatch are the
next evidence), no `release.yml` `dry_run` for `win-arm64`, no HTTPS `sdk acquire` by the packaged executable for this host (the consumer replay used a
local download directory), no Authenticode. The `crt` repository still has `cli_preview: none` for `windows-aarch64` and no `sdk-pins.json` entry; that
is a separate reviewed change in `crt` and is also needed before its consumer smoke can run.

## Hosted `windows-11-arm` result of the win-arm64 pins (2026-11-18)

This closes the "no hosted `windows-11-arm` run of any crt-cli job" limit of the [first Windows/arm64 Preview record](#first-windowsarm64-preview-pins-and-the-ciyml-row-2026-11-18).
The row was added by pull request 19 (merge `00b6fda`), which was first run as an ordinary pull request and then, after the merge, as a push to `main`.

| Run | Event | Commit | `native (win-arm64)` | `CI gate` |
| --- | --- | --- | --- | --- |
| [44244028130](https://github.com/webos21/crt-cli/actions/runs/44244028130) | pull request | `32426d4` | success, 05:24:39-05:37:06 UTC (about 12.5 minutes) | success |
| [44245882492](https://github.com/webos21/crt-cli/actions/runs/44245882492) | push to `main` | `00b6fda` | success, 05:40:44-05:52:51 UTC (about 12 minutes) | success |

Both runs executed the whole job on `windows-11-arm`: the native ARM64 LLVM 22.1.8 installer step passed its SHA-256 and target checks (`clang version 22.1.8`,
`Target: aarch64-pc-windows-msvc`), and `tools/ci_bootstrap.py --host win-arm64` ended in `CI PASS (win-arm64)` (the pull-request run's steps carry
no failed or cancelled entry). The other jobs of both runs (`changes`, the five other native legs and the gate) passed. The earlier `DIRTY` state of the pull request (a conflict with `main`) produced no run at all,
so the first run is the one that followed merging `main` into the branch.

The step log reports the check results of the driver but not the test counts of the summary receipt, because the artifact `ci-summary.json` was not downloaded for
this record; the counts of the local pass in the earlier section are the ones recorded. Not covered by this record: a manual `ci.yml` dispatch of the leg on its own
(both runs are the ordinary events), the `release.yml` `dry_run: true` rehearsal for `win-arm64`, and a `release.yml`-built second Preview. The leg is
non-experimental, so it is part of the required `CI gate` from the merge on.

## First scheduled `integration.yml` run (2026-11-18)

The daily trigger (`cron: '23 3 * * *'`) has run once. [Run 44079658878](https://github.com/webos21/crt-cli/actions/runs/44079658878) (`schedule`, at `d0efcaa`) was created 2026-11-17 12:04:01 UTC and finished
12:15:35 UTC (about 11.5 minutes), so it started about 8 hours 41 minutes after the 03:23 UTC slot of that day: GitHub does not guarantee the start time of a scheduled run. All six jobs succeeded:
`redirect`, `pins` and `ports`, each on `linux-x64` and `linux-arm64`. The workflow is `active` (id 414171448). This is a record of one hosted scheduled run, not of a daily cadence.

**Open observation.** At 2026-11-18 09:44 UTC the workflow's run list holds no scheduled run for the 2026-11-18 03:23 UTC slot. Whether it is late, skipped or missing was not diagnosed. It is tracked in
[TODO](../../TODO.md#ci-operations) rather than treated as a failure of the first run. A red scheduled run remains a regression or a changed external input, not a known gap.

## Windows/arm64 SDK publisher signature replay (2026-11-18)

The same replay that was run on macOS/arm64 ([record](#macosarm64-sdk-publisher-signature-replay-2026-11-17)) was run natively on Windows/arm64 (Windows 11, build 26300): the executable of the signed first
Windows/arm64 Preview `v0.1.0-preview.20261118.windows-arm64.1` (SHA-256 `132c8b5ae15f66ed9dd82fcb5a425160e4d682c008ab218353b7eed30f6d3db8`) against the published, signed Dev SDK
`v0.0.1-preview.20261118.windows.arm64.1` (manifest `354ab45f...eecc`, content `3b251637...621f`), in fresh homes under a path with spaces and Korean. **Windows/x64 was not run:** this host is ARM64 and an emulated x64
run would not be native evidence.

**Test keys, loopback HTTPS (`tools/replay_publisher_acquire_linux.py`): 37/37.** Result SHA-256 `93f51afb...8cf7`. The driver gained, for this record, a Windows child environment (see below) and eleven `sdk repair` cases; its
26 acquisition and local-install cases are the ones run on macOS. Acquisition: no `publisher` field keeps `not_verified`; the pinned key and signature give `signature_verified` with the request order manifest, contract,
signature, archive; an installed SDK is reused without a network request and reported `not_verified`; refused, with nothing published and no staging or lock files left: a key that did not sign, a valid signature by a key other than the
pinned fingerprint, a signature by another key, a signature over different bytes, a signature digest pin mismatch (stops before the archive is requested), a missing signature asset (HTTP 404), an oversized signature pin, a wrong
fingerprint pin, a relative key file, an unknown publisher field, an `http` signature URL and a missing signature object. `sdk install`: the unchanged result shape without options, `signature_verified` with the options, and the same
refusals plus the usage errors (exit code 2) for a key without a signature, a signature without a key, a fingerprint without a key and a malformed fingerprint. **`sdk repair`:** a signed install is damaged by a valid-looking edit
of the stored contract; `sdk verify` reports the digest mismatch; a repair with a signature by another key, a signature over different bytes, a key that did not sign, or a mismatching fingerprint pin is refused and leaves the
damaged contract bytes and the original receipt untouched with no staging or locks; the usage errors and a missing signature file are refused; a repair with the valid signature restores the original contract bytes and the original
receipt, and `sdk verify` passes.

**Real bytes, real key (not test keys).** The published archive and the owner's real `.sig` with `.github/ci/crt-release-publisher.pub` (SPKI `59218576...3fa2`), 5/5, result SHA-256 `5a3b699e...3628`: `sdk install` with a one-bit-changed
copy of the real signature installs nothing; with the real signature it is `signature_verified`; after the stored contract is damaged, `sdk repair` with the one-bit-changed signature changes nothing (damaged contract and original receipt preserved,
no staging or locks); `sdk repair` with the real signature restores the original contract and receipt; `sdk verify` passes. A real GitHub `sdk acquire` with the `github-release` redirect policy, the real key and the real signature pin
(`50ec4f51...cf1c308`, 256 bytes) returned `signature_verified` with key `59218576...3fa2` and content `3b251637...621f` (reply SHA-256 `910aa19c...5e28`). The CI driver's accepting path on this SDK is recorded in the
[first Preview record](#first-windows-arm64-preview-pins-and-the-ciyml-row-2026-11-18) and the hosted [result](#hosted-windows-11-arm-result-of-the-win-arm64-pins-2026-11-18).

**First attempt, kept: 17/26, not evidence.** The driver passed the consumer only `HOME` and `PATH`. On Windows a process without `SystemRoot` cannot resolve `localhost` (shown directly: the same `fetch` reaches a loopback listener once `SystemRoot` is set and
fails with "Could not connect" before connecting when it is not), so every case that needed the network failed and several refusal cases could have passed for the wrong reason. It is a test-environment fault, not a CLI fault; the driver now adds `SystemRoot`,
`USERPROFILE`, `TEMP` and `TMP` on Windows (the POSIX environment is unchanged) and decodes the output as UTF-8. The second run's refusals were checked case by case for their intended reason (none is a connection failure).

**Not covered.** Windows/x64; the restricted-token isolation of `crt_windows_trace` (this replay uses a cleared environment, test keys and loopback, as on macOS); the eleven new `sdk repair` cases and the Windows environment on Linux and macOS (only the unchanged POSIX
environment is shared, and the repair cases have not been run there); an acquisition refused for a real signature (the signature URL is fixed by the pin). One observation, not changed here: the "missing signature object" case is refused with the raw JSON-library text
`[json.exception.out_of_range.403] key 'archive_signature' not found` rather than a diagnostic of the CLI's own; the refusal and its effect are correct. Receipts are in the ignored local directories `cw/o3` (37 cases), `cw/r1`, `cw/r2` and the failed first run `cw/o2`.

## Windows/x64 SDK publisher signature replay (2026-11-18)

The replays recorded for [macOS/arm64](#macosarm64-sdk-publisher-signature-replay-2026-11-17) and [Windows/arm64](#windowsarm64-sdk-publisher-signature-replay-2026-11-18) were run natively on
**Windows/x64** (Windows 11 Pro build 26100, OpenSSL 3.5.9 from Git for Windows as the test-host signer, Python 3.11.9) with no CLI source change. The consumer is the executable of the signed public Preview
`v0.1.0-preview.20261117.windows-x86_64.1`, extracted from its archive after the archive's signature verified (SHA-256 `feb97791...84df`, the executable the hosted `verify` job ran). The SDK is the published, signed Windows/x64 Dev SDK
`v0.0.1-preview.20261110.windows.1` (manifest `b0fca4c8...6976`, content `5bb55372...c7ef`, archive signature `4d9438a6...ad8b`), downloaded from the GitHub release. Every case ran with a cleared environment, a private home and a work path with
spaces and Korean (OpenSSL signs relative to its own working directory, so the non-ASCII path does not reach it).

**Test keys, loopback HTTPS (`tools/replay_publisher_acquire_linux.py`): 37/37**, result SHA-256 `c574b3e6...c7dbc7`. These are the cases of the Windows/arm64 run with the same grouping: acquisition (not-verified without options, verified with the pinned key and signature,
reuse of an installed SDK, a key that did not sign, a valid signature by a key other than the pinned fingerprint, another key, a signature over different bytes, a signature digest pin mismatch stopping before the archive request, a missing signature asset,
an oversized pin, a wrong fingerprint, a relative key file, an unknown publisher field, an `http` signature URL, a missing signature object), `sdk install` (unchanged result shape, verified, the same refusals and the four usage errors) and eleven `sdk repair` cases
(damage detected, four refusals that leave the damaged contract and the original receipt untouched, usage errors, a missing signature file, the verified repair and `sdk verify`). Each refusal was read for its reason: none is a connection failure and
none is the missing-`SystemRoot` fault of the first arm64 attempt; the child environment of the driver adds `SystemRoot`, `USERPROFILE`, `TEMP` and `TMP` on Windows.

**Real key, real bytes, real signature (`tools/replay_publisher_real.py`, new): 10/10**, result SHA-256 `6badd0b9...1bc4`, key SPKI `59218576...3fa2`. A real GitHub `sdk acquire` (the `github-release` redirect policy, the pinned signature digest `4d9438a6...ad8b`, the system roots as
explicit transport trust) is `signature_verified` and installs content `5bb55372...c7ef`. `sdk install` of the published archive with a **one-bit-changed copy of the real signature** (`89cdf88f...1cf9`) is refused ("publisher signature verification failed") and
publishes nothing and leaves no staging or locks; with the real signature it is `signature_verified`. After the stored contract is damaged, `sdk verify` reports the digest mismatch, `sdk repair` with the changed signature is refused and leaves the damaged contract and
the original receipt untouched, and `sdk repair` with the real signature restores the original contract and receipt and `sdk verify` passes. The driver is host-independent (`--host` selects the pin set in `.github/ci/pins.json`); this is its first run.

**First attempt, kept, not evidence.** The first run of the new driver failed at the refused-install case because the consumer ran in a different working directory than the driver and the relative `--release-dir` paths no longer resolved; the driver now resolves them. The refusal command was
then repeated by hand with the same inputs and gave the expected refusal. The second run, above, is the record.

**Not covered.** The restricted-token isolation of `crt_windows_trace` (this replay uses a cleared environment, test keys and loopback, as on macOS and Windows/arm64); the eleven `sdk repair` cases on Linux and macOS; an acquisition refused for a real signature (the signature
URL is fixed by the pin); verification of the real signature by `crt-cli fetch --publisher-key`. Receipts: `out/windows-x64-sigreplay-20261118/` (`test-keys/`, `real-key/`, the failed first real-key attempt, the downloaded SDK release and the extracted CLI).

## macOS/arm64 SDK repair case closure (2026-11-18)

**Pass, 37/37 test-key cases and 10/10 real-key cases** on native macOS
27.2.1 (`26B131`)/arm64. Review baseline is
`2e99c65077abb4d67f686f99ca139056bc947f67`. The consumer is the executable
from signed public Preview `v0.1.0-preview.20261117.macos-arm64.1`, built from
`b51d103e7d0cf3b1816e0d3671963811ea7bf07e`. The repair, SDK store/acquire and
publisher implementation files match those at the review baseline. This
extends the [earlier macOS signature evidence](#macosarm64-sdk-publisher-signature-replay-2026-11-17)
with the eleven shared repair cases added for Windows.

Fresh downloads of the CLI archive and signature, SDK manifest, every listed
SDK asset and the SDK signature matched their pinned sizes and digests. Both
archive signatures verified with the committed publisher key, SPKI SHA-256
`59218576956e493476bc36f7385d8e01a03ff9b92946a531b013ad3937d43fa2`.

| Input or result | Exact identity |
| --- | --- |
| Public CLI executable SHA-256 | `afcdf9b6a368e0c9c2efbfc9f2d8ac18945c310c361c627ac6e0c0d3e0df0073` |
| SDK release | `v0.0.1-preview.20261110.macos.1` |
| SDK manifest SHA-256 | `0981110c84e5d24a903dbd3965dbe92eb2fe405edfa89804e3086dfeb75983de` |
| SDK content SHA-256 | `e3dc7fb1810a3f78ab8ef33882225ccb562d21f6f5977a2037ab92819326bac8` |
| SDK signature SHA-256 | `58f8749aeb71e4382de7aa4c3aeecac62f78936c2668ace19940b472b2a4f6e1` |
| Test-key `result.json` SHA-256 | `c17abd621e4fc2317886b0842e1462ad224db46f43038ed636f36181f673c91e` |
| Real-key `result.json` SHA-256 | `582ce4052c7b94550057f9adc08772092de528c262139589487b375c35c7664f` |

The unmodified `tools/replay_publisher_acquire_linux.py` passes all 37 cases
using loopback HTTPS and throw-away RSA keys. Its repair group installs a
signed SDK and changes the stored compiler contract while keeping valid JSON.
`sdk verify` detects the digest mismatch. Repairs with another key's signature,
a signature over different bytes, a key that did not sign, or a mismatching
fingerprint are refused. A key without a signature, a signature without a key
and a missing signature file are refused as well. Every refusal preserves the
damaged contract bytes and the original receipt and leaves no staging or lock
files. The valid signature restores the original contract and receipt, and
the final `sdk verify` passes.

The unmodified `tools/replay_publisher_real.py --host macos-arm64` passes all
10 cases. A real GitHub acquisition reports `signature_verified` with the
pinned publisher fingerprint and SDK content. A one-bit-changed copy of the
actual SDK signature is refused for install and repair. The original public
signature installs successfully and restores the damaged contract, preserving
the original receipt. All expected refusals were read for their own diagnostic
and exit status. The local recording wrapper additionally requires a final
versioned JSON reply from every native CLI invocation, including failures.

Each replay uses fresh private homes and stores under paths containing spaces
and Korean, with a cleared consumer environment and `PATH=/nonexistent`.
Python 3.15.2 and OpenSSL 4.1.0 orchestrate the replay. Their identities and
the harness hashes are retained in `inputs.json`. This run uses the shared
drivers' process environment controls. Linux repair replay remains in TODO.

Reproduce using verified public inputs and fresh output paths:

```sh
python3 tools/replay_publisher_acquire_linux.py \
  --cli /absolute/package/crt-cli/crt-cli \
  --release-dir /absolute/public-macos-sdk \
  --output /absolute/fresh-test-key-results
python3 tools/replay_publisher_real.py \
  --host macos-arm64 --cli /absolute/package/crt-cli/crt-cli \
  --release-dir /absolute/public-macos-sdk \
  --output /absolute/fresh-real-key-results
```

Receipts are in `out/macos-sdk-repair-20261118/`, including verified public
inputs, `test-keys/`, `real-key/`, per-invocation stdout/stderr and argv/exit
records, `inputs.json`, the invocation wrapper and `verification.json`.
An initial download-preparation script counted the package directory as an
executable and stopped before either replay. Its failed log is retained as
`download.log`. After selecting regular files, input preparation passed in
`download-r2.log`; both native replays passed on their first run.
