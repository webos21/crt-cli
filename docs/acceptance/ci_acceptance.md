# GitHub CI Acceptance

This record owns what the CI driver and workflow have actually been run against. The design and the
procedure are in the [development guide](../guides/development.md#continuous-integration); open work is in
[CI monitoring](../../TODO.md#ci---monitoring-and-pin-promotion).
See the [hosted signing gate](#hosted-sign-before-publishing-gate-2026-11-25)
for the completed release qualification. A passing local rehearsal is not a GitHub-hosted run.

## Current Hosted Coverage (2026-11-11)

As of 2026-11-20, all five host pairs have passing pins, redirect and port jobs in the
[15-job integration run](#windowsarm64-hosted-integration-expansion-2026-11-20). The Windows/arm64 workflow Preview is
[published and signed](preview_acceptance.md#windowsarm64-workflow-preview-release-2026-11-20).

Integration update 2026-11-19: the [Windows/x64 port adoption record](#windowsx64-hosted-port-integration-2026-11-19) qualifies the 11-job
configuration across Linux/x64, Linux/arm64, macOS/arm64 and Windows/x64, with five executed port suites on every host. The subsequent
[redirect expansion](#windowsx64-hosted-redirect-expansion-2026-11-19) adds the Windows redirect row and records its scoped verification.
Main/first-scheduled-run status stays separate from the original required-CI evidence below; required native coverage remains six rows across five host pairs.

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
[native integration driver](../guides/development.md#native-integration-driver).

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
[hosted result](#hosted-windows-11-arm-result-of-the-win-arm64-pins-2026-11-18)). Step 4 was completed by the
[rehearsal](#windowsarm64-releaseyml-rehearsal-2026-11-19) and [signed publication](preview_acceptance.md#windowsarm64-workflow-preview-release-2026-11-20).

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
[CI-03](../../TODO.md#ci---monitoring-and-pin-promotion) rather than treated as a failure of the first run. A red scheduled run remains a regression or a changed external input, not a known gap.

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
[first Preview record](#first-windowsarm64-preview-pins-and-the-ciyml-row-2026-11-18) and the hosted [result](#hosted-windows-11-arm-result-of-the-win-arm64-pins-2026-11-18).

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

## Linux/x86_64 SDK publisher signature replay (2026-11-18)

The eleven `sdk repair` cases that the [Windows/arm64](#windowsarm64-sdk-publisher-signature-replay-2026-11-18) and [Windows/x64](#windowsx64-sdk-publisher-signature-replay-2026-11-18) records left unrun on Linux were run natively on
Linux/x86_64 (Ubuntu 26.04.1, kernel 7.0.0-43, OpenSSL 3.5.5 as test-host signer, Python 3.14.4) with no CLI source change. The consumer is the executable of the pinned, signed bootstrap Preview
`v0.1.0-preview.20261116.linux-x86_64.1` (archive SHA-256 `52fbe5d1...fdca` equals the `pins.json` pin; executable SHA-256 `a6b33cbd...6039`). The SDK is the pinned, signed Dev SDK `v0.0.1-preview.20261118.linux.x64.1`
(manifest `b76aea28...a641`, contract `602e7a03...63d0`, archive signature `a15eb945...e50b`, content `3008315d...7a79`). The release files were taken from an earlier local download and compared with the pins before use; the one
download made for this record is the GitHub acquisition below. Every case ran with a cleared environment and a private home.

**Test keys, loopback HTTPS (`tools/replay_publisher_acquire_linux.py`): 37/37**, result SHA-256 `0fda58df...8c64`. These are the cases of the Windows records: 15 acquisition, 11 `sdk install` and 11 `sdk repair`. The repair group damaged
a signed install by a valid-looking edit of the stored contract; `sdk verify` reported "toolchain contract digest mismatch"; repairs with a signature by another key, a signature over different bytes and a key that did not sign were refused with
"publisher signature verification failed", a mismatching fingerprint pin with "publisher key does not match the pinned key fingerprint", a key without a signature and a signature without a key with the usage error (exit 2), and a missing
signature file with "publisher input must be a regular file". Each refusal left the damaged contract bytes and the original receipt untouched with no staging or lock files. The repair with the valid signature restored the original contract
and receipt, and `sdk verify` then passed (2618 files). Each refusal message was read; none is a connection or setup failure.

**Real key, real bytes, real signature (`tools/replay_publisher_real.py --host linux-x64`): 10/10**, result SHA-256 `977a58e3...29fa`, key SPKI `59218576...3fa2`. A real GitHub `sdk acquire` (the `github-release` redirect policy and the pinned
signature digest) is `signature_verified` with the pinned content; `sdk install` with a one-bit-changed copy of the real signature (`3b85c409...5a38`) is refused and publishes nothing; with the real signature it is `signature_verified`;
after the stored contract is damaged, `sdk repair` with the changed signature leaves the damaged contract and the original receipt untouched, `sdk repair` with the real signature restores both, and `sdk verify` passes.

**Not covered.** macOS/arm64 is recorded in its own [repair case closure](#macosarm64-sdk-repair-case-closure-2026-11-18); the isolation of the bubblewrap-based port jobs is not exercised here (cleared environment, test keys and loopback only); an acquisition refused for a
real signature (the signature URL is fixed by the pin). Linux/arm64 was not run there; it is recorded in [its own section](#linuxarm64-sdk-publisher-signature-replay-2026-11-19). Receipts: `out/linux-x64-sigreplay-20261118/` (`test-keys/`, `real-key/`, the release inputs and the extracted CLI).

## Linux/arm64 SDK publisher signature replay (2026-11-19)

The publisher-signature replay that the [Linux/x86_64 record](#linuxx86_64-sdk-publisher-signature-replay-2026-11-18) left unrun on Linux/arm64 was run natively on this host (Ubuntu 26.04.1, kernel 7.0.0-43 aarch64, OpenSSL 3.5.5 as test-host
signer, Python 3.14.4) at `main` `3aa7030` with no CLI source change. The consumer is the executable of the pinned, signed bootstrap Preview `v0.1.0-preview.20261116.linux-arm64.1`: archive SHA-256 `311bc451...77aa`
(5,063,404 bytes) equals the `pins.json` pin, executable SHA-256 `4b5109e9...3d3b`. The SDK is the pinned Dev SDK `v0.0.1-preview.20261110.linux.arm64.2` (manifest `ba77b26e...67be3`, contract `a3df2fbd...34d4`,
archive signature `25287c3f...f08f`, content `cd2ab657...0cc6`). All six inputs (CLI archive and signature; SDK manifest, contract, archive and signature) were downloaded for this record and compared with the pins before use.

**The consumer is the genuine signed Preview.** The CLI's own `fetch` with the pinned key (SPKI SHA-256 `59218576...3fa2`, equal to `key_spki_sha256` in `pins.json`) and the `github-release` redirect policy downloaded the Preview archive
from its public URL and reported `signature_verified`; a copy of the signature with one bit changed was refused with "publisher signature verification failed" and left no output file.

**Test keys, loopback HTTPS (`tools/replay_publisher_acquire_linux.py`): 37/37**, result SHA-256 `920880ff...b15f`. These are the cases of the other hosts' records: `sdk acquire`, `sdk install` and `sdk repair` with a signature by another key, a
signature over different bytes, a key that did not sign, a mismatching or malformed fingerprint pin, a key without a signature and a signature without a key, a missing signature file, an oversized or `http` signature URL and a missing
signature asset. The repair group damaged a signed install by editing the stored contract, saw `sdk verify` report it, and every refused repair left the damaged contract and the original receipt untouched with no staging or lock files; the
repair with the valid signature restored both and `sdk verify` then passed. The refusals were read from the recorded responses, not inferred from exit codes: of the 37 cases, 17 carry a signature or key refusal text ("publisher signature verification
failed" 9, "publisher key does not match the pinned key fingerprint" 2, "--publisher-key and --signature must be supplied together" 4, "publisher input must be a regular file" 2), and none carries a connection, certificate or process-start failure. This
record does not split the 37 into acquire/install/repair counts: a split derived from case names here (14/12/11) differs by one case from the earlier records' (15/11/11), so the earlier split is neither confirmed nor corrected.

**Real key, real bytes, real signature (`tools/replay_publisher_real.py --host linux-arm64`): 10/10**, result SHA-256 `9140277b...9d0e`, key SPKI `59218576...3fa2`. A real GitHub `sdk acquire` (the `github-release` redirect policy and the pinned signature
digest) is `signature_verified` with the pinned content; `sdk install` with a one-bit-changed copy of the real signature (`ac23f0a9...4fda`) is refused and publishes nothing; with the real signature it is `signature_verified`; after the stored contract is
damaged, `sdk repair` with the changed signature leaves the damaged contract and the original receipt untouched, `sdk repair` with the real signature restores both, and `sdk verify` passes. Both refusals carry "publisher signature verification failed".

**Not covered.** An acquisition refused for a real signature (the signature URL is fixed by the pin); the isolation of the bubblewrap-based port jobs (cleared environment, test keys and loopback only); other glibc versions or distributions (Ubuntu
26.04, glibc 2.43 only); a bootstrap Preview built by `release.yml`. The CLI Preview used is the signed `20261116` one pinned in `pins.json`, not a newer one. Receipts are on this host under `/home/cmjo/GitLocal/crt-cli-sigreplay-20261119/`
(`test-keys/result.json`, `real-key/result.json`, the logs, the verified release inputs and the extracted CLI).

## Windows/arm64 `release.yml` rehearsal (2026-11-19)

`dry_run: true` rehearsal of `release.yml` for `win-arm64` on the hosted `windows-11-arm` runner, the step that the [first Windows/arm64 Preview](#first-windowsarm64-preview-pins-and-the-ciyml-row-2026-11-18) could not take (a host's first Preview has no bootstrap pin for the workflow).
[Run 44425099335](https://github.com/webos21/crt-cli/actions/runs/44425099335) on `main` at `d72a2a5` (the `CI gate` of that commit succeeded), date `20261119`, serial `92` (a test serial that no release uses), notes `.github/release/preview-notes.md`.
Runner image `windows-11-vs2026-arm64 20261112.217`. `package` (23:57 to 00:10 UTC), `verify` (00:10 to 00:13) and `publish` all succeeded.

**Inputs the workflow used.** Bootstrap `v0.1.0-preview.20261118.windows-arm64.1` and Dev SDK `v0.0.1-preview.20261118.windows.arm64.1` (content `3b251637...621f`), both from `pins.json`; this is the first time a Windows/arm64 archive was built by the workflow from those pins rather than by hand.

**`package`.** `CI PASS (win-arm64)`; stage 1 and stage 2 differ byte-wise, as on the other hosts (functional equivalence is checked separately); the archive is 5,233,476 bytes, SHA-256 `2dc514a6...5f79`, package manifest `ea74bf9b...d918`, 30 files.
**`verify`**, on a separate `windows-11-arm` runner and from the artifact only: `--version`, self `doctor` with the manifest pin, a wrong pin refused, a real GitHub `sdk acquire`, then C and C++ `new`, `build`, `test`, `run` and a warm build, all `exit=0`; cold 3 and warm 0 commands for both projects, project lock `2e3b7014...376c`.
The executable (`6cafc190...fc34`) is a PE with machine `0xAA64` importing only `api-ms-win-core-synch-l1-2-0.dll` and `kernel32.dll`, equal to the `allowed_imports` pin.

**Approval.** `publish` entered `waiting` on the `release` environment after `package` and `verify` had succeeded, and the approval is recorded as `approved` by `webos21` with the comment `OK`. The state before approval and the person who gave it are therefore both in the record, unlike the [2026-11-11 dry run](#release-workflow-2026-11-11), whose approval author is not known.

**Draft.** The draft `v0.1.0-preview.20261119.windows-arm64.92` was a prerelease at `d72a2a5` with exactly the archive and `SHA256SUMS-windows-aarch64`. The downloaded archive and sums file are byte-identical to the `package` job's artifacts and the sums file verifies. The notes carry the real digest, size and commit with no leftover token. The archive was extracted and its `crt-cli.exe` equals the executable digest of `release.json`; the PE header read directly (not through `llvm-readobj`) gives `0xaa64`. No tag existed.
The draft was then deleted; the nine published CLI Previews remain and no `windows-arm64.92` tag exists.

**Not covered.** A real publication by the workflow (`dry_run: false`) for `win-arm64`, the owner's offline signature of such an archive and the signature replay on it, and replacing the hand-built Preview in `pins.json` and in crt's `sdk-pins.json` (each a reviewed change in both repositories). The rehearsal's archive was not executed outside the hosted `verify` job.

## macOS/arm64 integration expansion (2026-11-19)

**Candidate on `codex/macos-host-tools-integration`; not adopted on main yet.** The common integration driver and all three workflow
matrices (`pins`, `redirect`, `ports`) now include macOS/arm64 on `macos-15`. This is build-host tooling only: no CLI product, public SDK,
runtime ABI, recipe, root/project lock, publisher key, default or STATUS changes. Windows integration remains unimplemented.

### Inputs and implementation

The new `integration-pins.json` entry selects the published signed host-tools Preview `v0.1.0-hosttools.20261119.macos-aarch64.1`:
archive 583176 bytes, SHA-256 `90c7585a92fc020e05ea69e7bcd752a4df67f060706c87f81822cb11b0c676a1`, manifest
`27fac5efbb6f846041b471d8b0be8e8004608ef5839ce4a2ef23c62aae8e07b8`, detached signature 256 bytes, SHA-256
`15c2af47b2de52ccf876a0d1c335ba187a92ff92de8b14c2fb52521b0ac3d451`. Producer evidence predates this pin:
[CRT public replay](https://github.com/webos21/crt/blob/84eec172a9268f58f1d61016f08d10b9cdb368bc/docs/acceptance/host_tools_acceptance.md#macoswindows-host-tools-publication-signatures-and-macos-replay-2026-11-19).
SDK/CLI pins are unchanged: `v0.0.1-preview.20261110.macos.1` (content `e3dc7fb1...bac8`, contract `d5a100fd...e464`) and signed bootstrap
`v0.1.0-preview.20261117.macos-arm64.1`. The host-tools and bootstrap signatures are verified before extraction/execution; SDK archive,
manifest, installed content and compiler contract are checked against their existing pins.

macOS resolves Xcode tools via the existing CI bootstrap helper and uses Seatbelt, not Linux's LLVM/Bubblewrap/strace path. The oracle
Python is explicitly selected and SHA-256-checked before launch, inventoried and permitted only for the differential test. macOS suite
parsing now uses the same strict outcome helper as Linux: exactly one named positive check count, zero exit and no skip on stdout/stderr.
The driver accepts the existing macOS ledger shape only with exactly five requested/reported passing suites, matching input pins,
complete passing gates and `allow_dirty_sdk: false`. Earlier local receipts lacking these fields are not relabeled as integration passes.

### Local verification

Local macOS 27.2.1/arm64, Xcode 27.2, base CLI `2e4c4d1e8fa7df25bdebe1358587b469fce7752e` plus this tooling-only candidate:
integration pin gate passes; 34 integration unit tests, four macOS parser/input tests, five Seatbelt policy tests, 17 bootstrap tests,
11 pin-review tests and 21 release-driver tests pass (92 total). The workflow parses with Ruby/Psych and every run block parses with
macOS `/bin/bash`. The public-download `redirect` entry point passes **54/54** cases with the pinned CLI and SDK.

The complete public-download `ports` entry point exits 0: five input-enabled suites, **300 checks, 26/26 adapter gates**, no skip.
Counts: build **40**, store **105**, oracle **41**, command **33**, project **81**. Both static/shared recipe tests and C/C++ consumers run.
The real port command publishes once, reuses the exact key, then verifies the item. Cold self-build executes 21/reuses 0; warm executes 0/reuses 21.
The CLI imports only libSystem; bare version/doctor, source/SDK/tool denial probes, no-undeclared-executable policy and unchanged original/copied
SDK/bundle inventories pass. Only the differential oracle gets its labeled host-tool exception. The source fingerprint remains
`b54ea39ae1bc663dcea6da1a7b72c659998ce289fc621b155642182e4402ec33`; native CLI SHA-256 is
`eddb433570034bf169d91b7a5913201554757b0ac91c9fce8109c561e9a5d1aa`.

Local oracle is Python 3.15.2 (workflow selects Python 3.12), executable SHA-256
`6cc907e4a022fefc843bcd93954eaf5a2cbfc77a095640bfadeeed96154190c5`. Its digest is checked by both common driver and macOS adapter,
recorded in the input/result ledgers and used for the oracle invocation. `port-inputs.json` records `input_mode: download` and
`publisher_authentication: signature_verified` for the host-tools archive; staging's `native_validation: not_run` is not the final replay result.
The SDK install directory, installed contract and content match the pinned public SDK; it is never rebuilt or patched by integration.

Retained evidence in CRT's ignored `out/macos-hosted-integration-20261119/` includes local logs, `local-ports-r2/replay/{inputs,commands,result}.json`,
the common `port-inputs.json`/`replay-command.json` and install logs, plus `local-redirect-r1/redirect-replay.json`.
Port result SHA-256 `9bde4b7729386314297091df7e95bca844a7b345caa43c05329760129a9c5047`;
redirect result SHA-256 `bd1e398e6dd30f2cf54fd50b82d6b23b26603b1f82084c542960823d9477a74d`.
The full fresh work directories are `/private/tmp/crt-cli-integration-macos-20261119-ports-r2` and `...-redirect-r1`.
The first ports attempt was intentionally stopped during development after review found an oracle variable-name collision; its logs and
workspace remain, not counted as a pass. The corrected driver uses fresh `ports-r2` outputs and unchanged public assets.

After the linker-helper correction, a fresh local workspace again passes **300 checks and 26/26 gates**, using the same authenticated
public inputs (no new download or SDK mutation). The complete retained replay is `local-classic-fresh/`, result SHA-256
`942672b35a6d7ca73236508de756df5559f22c5c60ecfe80f277e0e50e950f8e`; driver SHA-256
`679e0264b949c06c33a87245fd6bc38612afe5d63aec3357c88a2a15f8be9b27`, CLI SHA-256
`e64710033d39499751e07020552408dfa696ddf4ef6f73b10fd37a1bf5f7af42`. Xcode 27.2 needs no legacy helper; its helper inventory is empty.
The focused regression set is now **93 tests** (one additional exact-helper fixture), no skips. An earlier retained-workspace retry
passes the five suites but only 25/26 gates: its store already contains the item, so the fresh-publication assertion fails. It is not
counted as a full pass or used as cold-publish evidence; the fresh replay above supplies that evidence.

### First hosted attempt and linker-dispatch correction

Draft [PR #30](https://github.com/webos21/crt-cli/pull/30) at `cbba4792b6eb6506b4dc4ddcd7a9047fc3c61b5b` has a passing
[native CI run](https://github.com/webos21/crt-cli/actions/runs/44463523022): all six native rows and `CI gate` succeed.
The first manual [integration run](https://github.com/webos21/crt-cli/actions/runs/44463529315) passes all three hosts' `pins` and
`redirect` jobs and both Linux `ports` jobs, but fails macOS `ports` before any port suite runs (2/4 replay gates, zero suites).
Its retained artifact is `ports-macos-arm64-cbba4792b6eb6506b4dc4ddcd7a9047fc3c61b5b`, ID `14501171915`.
The clean SDK configure succeeds; fresh CLI linking reports `can't exec ld64` after a legacy-option warning on Xcode 16.4/macOS SDK 15.5.
The initial correction looked for a sibling named `ld64` only. In the second [run](https://github.com/webos21/crt-cli/actions/runs/44473654095)
at `2bf4dfd`, the helper inventory is empty and the same macOS link fails: that inferred filename was not the installed helper.
Apple also names its legacy linker `ld-classic`; the corrected resolver checks both exact names. The second run's Linux/x64 job separately
refuses a 12,103-byte zlib download against the unchanged 1,512,791-byte pin before any suite runs; no digest/size check is relaxed.
After preserving both failures, that obsolete run was canceled while Linux/arm64 was still running, releasing the concurrency slot
for the corrected run. Its canceled Linux/arm64 leg is not a pass.

The correction permits only existing executable `ld64`/`ld-classic` files next to the selected canonical linker, not the Xcode directory. It records
the helper path/digest in `inputs.json`; focused policy fixtures cover its absence, non-executable state and exact-file permission.
The artifact now also retains individual step stdout/stderr and the CLI import audit for diagnosis. The passing fresh rerun below
does not erase or relabel the earlier failures.

### Hosted qualification on the PR branch

Manual [integration run 44474145070](https://github.com/webos21/crt-cli/actions/runs/44474145070), commit
`25591d8699a59b90e08acc1d7a72a054d8f09d4f`, passes **all nine jobs**: `pins`, `redirect` and `ports` on macOS/arm64,
Linux/x86_64 and Linux/arm64. The macOS port job runs on macOS **15.8.2** (`24H232`), image `macos-15-arm64 20261112.0539.3`,
Xcode **16.4**, SDK **15.5** and Python **3.12.10**. The explicit oracle executable SHA-256 is
`d4f152f2a753c94e0e7935c8ebbe6b2609979e1df7898422b577d0076383d08b`.

macOS reports **300 checks / 26 of 26 gates**, no skip: build 40, store 105, oracle 41, command 33, project 81. Fresh public-download
inputs match the pins above; static/shared zlib and C/C++ consumers, oracle comparison, publication/reuse/verify, the import boundary,
source-hidden probes and unchanged original/copied SDK/tool inventories all pass. Redirect is separately **54/54**.
`external_linker_helpers` confirms the actual helper is
`/Applications/Xcode_16.4.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ld-classic`, SHA-256
`d3077803de1f49624e93723a4102aec9ae4893e04f2fe1530ce7e5c74dd9fbb5`; no `ld64` file is present.
The port driver digest matches the final local replay above. Hosted CLI SHA-256 is
`921552ae20614ddcd800f8b150419fd158bdd2830bc0ce6de97e2716d7380fed` (no cross-path byte-reproducibility claim).

Each Linux port job preserves its **262 checks / 25 of 25 gates** baseline (35/90/23/33/81), no skip. The transient wrong-size
zlib response from the previous attempt is not accepted or substituted; this run authenticates the unchanged pinned source bytes.
[PR CI 44474145670](https://github.com/webos21/crt-cli/actions/runs/44474145670) at the same commit passes all six native matrix rows
and `CI gate`. The final follow-up changes only documentation and the host-neutral helper fixture (POSIX executable-bit semantics and
the simulated Seatbelt prerequisite); the 93-test local set passes without skips. No CLI product or SDK input changes after this replay.

Evidence is retained in CRT's ignored `out/macos-hosted-integration-20261119/hosted-44474145070/` plus the original macOS log.
Hosted artifact IDs: macOS ports `14506132689`, redirect `14505354447`, Linux/x64 ports `14506626758`, Linux/arm64 ports `14505808312`.
Result SHA-256 values: macOS ports `f1905bd238d129748e34c6d849e014862fda030e319d2fe16f73dff8a5547c36`,
macOS redirect `6f3bd16abba36cdfe40d88b2198cbfae6683b0e942e7b233fd7796896c6d74cf`,
Linux/x64 ports `4c94ef100b1d83b868993e432293f51558008fd036d5cd02e035d502552ccf1a`,
Linux/arm64 ports `35d246e32704b8c6c7c5689b5d692b7b5ab9129d3d54959d2c63512dc8b2c9b9`.

**Adoption remains pending.** PR #30 is unmerged; the owner authorized a branch/PR and hosted verification, not a main merge.
The macOS row reaches the main daily schedule only after reviewed adoption. Windows integration, Windows/arm64 D1 (31/32),
private-DLL qualification, release publication and STATUS are unchanged. Unrelated CRT/GFX/Web stages are not gates for this
build-host change.

## Windows/x64 public host-tools integration pin (2026-11-19)

**Input pin and native preparation qualified; hosted Windows integration remains open.** Both checkouts were clean and equal to
their fetched `origin/main` before this increment: CLI `b9168c514c3b7244458413467b8fa1c4312643fa`, CRT
`bc195a1915b6ee21ff98957af9eb2003cf334e6e`. Host: native Windows/x64, OS build `10.0.26100`, test-host Python 3.11.9.
The preceding [Windows public-byte D1/D3 record](host_environment_acceptance.md#windowsx64-published-host-tools-consumer-replay-2026-11-19)
already qualifies this artifact's consumer behavior. No CLI product or SDK bytes changed here; those suites were not rebuilt or relabeled as new runs.

The public [host-tools Preview](https://github.com/webos21/crt/releases/tag/v0.1.0-hosttools.20261119.windows-x86_64.1) is not a draft.
Its annotated tag resolves to `56fc914ff4029a5ef0db8d7050418725a9dafc4b`, matching the record's clean packer and assembler.
Four assets were downloaded afresh without credentials and agree with GitHub's asset digests, the checksum line and the record's `consumer_pin`:

| Asset | Bytes | SHA-256 |
| --- | ---: | --- |
| `crt-v0.1.0-hosttools.20261119.windows-x86_64.1.tar.xz` | 784724 | `bd65a2b8d1a5b7865110233bb47a1d13ea56724ad76593333cc349bb0980a210` |
| Archive `.sig` | 256 | `72f522291148d0e5fae696d2783eb32153b4513c8c45498d557ed60326896695` |
| `host-tools-release-record-windows-x86_64.json` | 3446 | `5f7b05861e9d0c2e2d7808a955224317d6a5c054f3123f7005546609bd6b4841` |
| `SHA256SUMS-host-tools-windows-x86_64` | 121 | `f7aaceb3305b2d2d4c32b240178957ba4029ae54328fdd2d16ad36316c24cf9b` |

The manifest SHA-256 is `cd6cb75e91b8f9202cda82876de9898e19fb4aeb01be981ebcfde9eabb7b7e00`, content
`dc106db47ed8bcbb56bdd6da0415dce527653fcb42b0c54bdc1402dca74c35fe`, host `windows/x86_64`, promotion `preview`.
The manifest and release record agree on assembler identity and source SDK: public Dev SDK `v0.0.1-preview.20261110.windows.1`,
source `b31276a6acccf0a89731f7bb0aecbdcdc9b45cda`, content `5bb55372ec10e3cfa24c84de981a7130f56aeb57ef0d43d5da22a65036dec7ef`,
release manifest `b0fca4c81d29b8eee28e0d9fb7d6e26950b094a8be4f2c091e43922366a46976`, matching the existing SDK pin.
The [immutable producer record](https://github.com/webos21/crt/blob/0d9d573b93a7c9edb00af18f2d1351d9e774b6a8/docs/acceptance/host_tools_acceptance.md#windowsx86_64-host-tools-public-signature-and-native-replay-2026-11-19)
owns the earlier independent signature/native-image audit.

`integration-pins.json` now includes `win-x64`, including its detached-signature URL, size and digest. The existing pinned publisher key
has SPKI SHA-256 `59218576956e493476bc36f7385d8e01a03ff9b92946a531b013ad3937d43fa2`; the stdlib RSA-PSS verifier authenticates
the downloaded archive before extraction. The release record and checksum are unsigned supporting metadata; this is not Authenticode or
a new independent key-distribution check. The existing signing-policy trust limit still applies.

The common driver supports `pins --host win-x64` and `ports --host win-x64 --prepare-only`. Preparation downloads the unchanged pinned zlib,
archive and signature, validates size/digest/signature, and checks manifest host/version, clean assembler and Preview promotion.
Its receipt reports `publisher_authentication: signature_verified`, `native_validation: not_run` and no `passed` field.
Without `--prepare-only`, Windows `ports` fails with `Windows port integration is not wired`; Windows `redirect` likewise fails before
creating work or downloading inputs. No Windows row was added to `integration.yml`.

Validation:

- Focused `test_ci_integration.py`: **36 tests pass, no skips**, including Windows identity, signed preparation, wrong host and unwired-command refusal.
- Six controls using the public input bytes pass: wrong archive digest, wrong signature digest, a bit-changed signature with a matching digest
  (cryptographic refusal), missing signature, wrong manifest digest and wrong host. None publishes a prepared receipt or final bundle.
- The exact prepared bundle passes `replay_host_tools_windows.py` **18/18**, including full inventory/native-image validation and execution of
  every declared executable/alias, shell, awk, GNU make and basic applets from a relocated space/Hangul path with empty `PATH`.
  This producer-style replay does not deny reads of original sources or network access and is separate from hosted port-suite acceptance.

The first online preparation failed at `host-tools.extracting` -> `host-tools` rename with `WinError 5` (access denied), after extraction.
Its staging is retained without a success receipt. A later diagnostic could rename that staging and restore its original name without
changing attributes or ACLs. A fresh `online-prepare-r2` succeeds with identical pins; the cause of the first failure is unconfirmed.
No retry or security-setting change was added to hide it.

Reproduction (Python is a test-host dependency; substitute fresh output paths):

```text
python -m unittest discover -s tools -p test_ci_integration.py
python tools/ci_integration.py pins --host win-x64
python tools/ci_integration.py ports --host win-x64 --prepare-only --work <fresh-work>
python <crt-checkout>/tools/replay_host_tools_windows.py --bundle <fresh-work>/host-tools/crt-host-tools-windows-x86_64 --output <fresh-native-evidence>
```

Retained evidence: CLI ignored `out/win-host-tools-pin-20261119/`, including `review_public.py`, `release-downloads/`, `pin-review.json`,
`online-prepare/` (failed staging), `online-prepare-r2/port-inputs.json`, `native-replay/host-tools-replay.json` and logs.
`pin-review.json` SHA-256: `cbdc805ec4cbd09a4289a94dbf8879d12d262fa057a41de72a7737548b029df0`;
prepared receipt: `5818c95f5681ec947cc1e90de1f94f1024fb3055127427b3d820306e7e40ce6d`.
Native tool receipt: `4ad149e9d3938b4697a54bd61069e9b9db9d7013641a1e46093f3cc22d6a3913`;
`preparation-attempts.json` records the failed and successful preparation attempts separately.
Windows hosted adapter/job wiring, real-symlink prerequisite and five-suite result qualification remain separate D2 work;
Windows/arm64 D1, private DLLs, SDK/bootstrap pins, root/project locks, release publication and STATUS are unchanged.

## Windows/x64 hosted port integration (2026-11-19)

**Adopted on main after hosted qualification on `codex/windows-hosted-integration`.** The signed host-tools pin above is committed on main
as `2473b545e2e7ecc6112a10cfff67e359cf60a3bc`. This increment connects its common input driver to a fresh Windows restricted-token
adapter and a `windows-2022` job. Windows `pins` joins the shared pin matrix; the separate Windows port job checks out into a newly created,
private directory so hosted `Users` grants do not make the original source visible through the restricted token. Existing directories and
global machine policy are not changed. Windows redirect integration remains separate.

The only producer test-host helpers are downloaded from public immutable CRT commit `bc195a1915b6ee21ff98957af9eb2003cf334e6e`:

| Helper | Bytes | SHA-256 |
| --- | ---: | --- |
| `crt_windows_external.py` | 11781 | `f01379e0aaed7da3c1009b3eee286a8d705608f475a68209f3839837a64e9236` |
| `crt_windows_trace.py` | 10792 | `35355c9e63b7abc69223655ea8cfe142981763b6c965e88ab13d408b93fb986a` |

`integration-pins.json` pins their exact URLs, names, sizes and hashes; both the common driver and the adapter check them before import.
These are build-host acceptance helpers, never SDK/product build inputs or a sibling checkout dependency. Their source trust is the reviewed
immutable commit plus digest pins; there is no new signature claim for the helper files.

`replay_port_windows_ci.py` copies the source export, public SDK, contract, bootstrap executable and signed host tools into a fresh per-SID
workspace. It probes original input denial and real CRT symlink creation before building the current CLI. Missing symlink support is a
failure with a Developer Mode diagnostic; the workflow never enables it. The selected LLVM/OS SDK remain explicitly declared host tools.
The adapter inventories LLVM executables/helpers and pins a declared oracle Python, copied into the workspace and admitted only for
`port-oracle-test`. It records every job-object process image; an unknown or undeclared image fails acceptance. This is selected file-read
denial and process accounting, not network/executable denial.

The five required suites are `port-build-test`, `port-store-test`, `port-oracle-test`, `port-command-test`, `port-project-test`.
Every result must execute positive checks, report no skip and match the prepared archive/manifest/contract identities. Windows also requires
explicit symlink, original/bare input denial, PE and unchanged-inventory cases. Cold/warm build, SDK-hidden bare help/version/doctor,
real `port build zlib`, static/shared consumers and a PE x64/OS-import audit are adapter gates. Results and failure logs are retained as artifacts.

Local validation: focused tests **40/40**, Python syntax, `git diff --check` and actionlint 1.7.12 pass. Actionlint's obsolete built-in
runner list needs the existing `ubuntu-26.04`/`ubuntu-26.04-arm` unknown-label warning excluded; no other warning is suppressed.
The first native prototype preflight is retained at `C:\Users\Lee\s13\p` (orchestration log in CLI ignored
`out/win-host-tools-pin-20261119/local-hosted-adapter.log`): all five suites pass **270 checks** (36/97/21/33/83), real `port build zlib`,
PE audit and unchanged inventories pass, cold build 21 executed / 0 reused and warm 0 / 21. It predates the integrated separate-bare-SID
gate; the matching CLI separately passes all four hidden-input/help/version/doctor checks at `C:\Users\Lee\s13\bare-evidence`.
The committed driver's complete hosted result below is the qualification evidence. Local CLI SHA-256:
`df8a608f630bb850d697cf38e357a5fd01afba663194d8fa2a5301c28e4eac11`; path/debug differences are not a reproducibility claim.

### Retained first hosted failure and fixture correction

[Integration 44489721254](https://github.com/webos21/crt-cli/actions/runs/44489721254), at `b4e21ea`, fails the Windows `pins` job:
five signature-fixture cases cannot sign the archive at the Unicode path with the runner's `C:\Program Files\OpenSSL\bin\openssl.EXE`.
The port jobs consequently do not run; this is not a passing or skipped-suite acceptance result. The original Windows job log is retained as
`out/win-host-tools-pin-20261119/hosted-first-windows-pins.log`.
`9a417fc` signs the same archive bytes through OpenSSL stdin/stdout; Python writes the detached signature into the original Unicode directory.
The Unicode staging and verification checks remain, and no product, publisher key or signed input bytes change. All 40 tests then pass on
each of the four hosted pin jobs, including Windows.

### Hosted Windows/x64 qualification

[Integration 44490048419](https://github.com/webos21/crt-cli/actions/runs/44490048419) runs exact commit
`9a417fc52073b977dc8e36d7c8c978cd09dbb6f9`. The `windows-2022` port job passes in **14m37s**, with no setting change for Developer Mode
or symlinks. Its private checkout and restricted-token visibility probe pass, real CRT symlink creation passes, and the five suites execute:

| Suite | Checks | Result |
| --- | ---: | --- |
| `port-build-test` | 36 | passed, no skip |
| `port-store-test` | 97 | passed, no skip |
| `port-oracle-test` | 21 | passed, no skip |
| `port-command-test` | 33 | passed, no skip |
| `port-project-test` | 83 | passed, no skip |

The complete replay passes **29/29 gates**, including static/shared consumers, original and separate-bare-SID input denial,
SDK-hidden help/version/doctor, `port build zlib`, x64 PE with only declared OS DLL imports, unchanged original/copied SDK/tool inventories
and source inputs. Cold build is 21 executed / 0 reused; warm is 0 / 21. All **4121 process records** have declared images, no unknown
image or timeout. Host-tools download reports `signature_verified`; the source export, recipe inputs and compiler contract match the pins.
The oracle alone admits its copied test-host Python: original `C:\hostedtoolcache\windows\Python\3.12.10\x64\python.exe`, SHA-256
`4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a`; copied under the native workspace with the same digest.
CLI SHA-256: `4c1e90ed5ec3cde8e59be9a800251da48b1de06db784db6570532ac100915579`.

Downloaded Windows evidence passes an independent `check_hosted.py` review: common result judging, exact five-suite counts and native
boundary cases, source export, pins, committed driver (LF/CRLF form), oracle identity, cold/warm cache and every process record.
Evidence: CLI ignored `out/win-host-tools-pin-20261119/hosted-r2/`, plus `check_hosted.py` and `hosted-r2-artifacts.json`.
Windows result SHA-256: `ecf758914ae6a4cfe90ca88e88270996071994fb39b7bd8a84c05485ff649219`.
Artifact ID: `14513750774` (`ports-win-x64-9a417fc52073b977dc8e36d7c8c978cd09dbb6f9`), ZIP digest reported by GitHub:
`e08dc07a44e6db571a208549be7b9ac88fe76c90b030016387b3b9da2fcdec7d`.
All **11 integration jobs pass**. The other downloaded port results also pass the common judge: Linux/x64 **262 checks / 25 gates**,
Linux/arm64 **262 / 25**, macOS/arm64 **300 / 26**, no skipped suite. `all-ports-verification.json` retains their result hashes.
[CI 44490052761](https://github.com/webos21/crt-cli/actions/runs/44490052761) at the same commit passes all six native rows and `CI gate`.
The first scheduled Windows run is not yet claimed here. Windows redirect, Windows/arm64 D1 and private-DLL qualification
remain separate; no release publication, root/project lock, SDK or STATUS changes.

### Main adoption of Windows integration

After the owner's instruction to merge and push, latest `origin/main` (`2473b54`) was merged into the validation branch without conflicts
or additional changes. Main fast-forwards to `f06a036c01c1e71d14df87c1f082b1df816c33c6`, which differs from fully qualified `9a417fc`
only in five documentation files. Both branches are pushed; the runtime, workflow, adapter and pin inputs are identical to the passing
11-job integration and six-row native CI. The main push's [CI 44493533992](https://github.com/webos21/crt-cli/actions/runs/44493533992)
passes all six native rows and `CI gate`; its full run metadata is retained as `out/win-host-tools-pin-20261119/main-ci-run.json`.
The existing daily `23 3 * * *` schedule now includes Windows `pins` and `ports`;
at adoption the first scheduled Windows run was unobserved. The later
[scheduled-run observation](#first-scheduled-windowsx64-integration-run-2026-11-19) closes that TODO. No additional schedule, release dispatch or security
setting change is needed for this adoption.

## Windows/x64 hosted redirect expansion (2026-11-19)

**Adopted on main after hosted qualification on `codex/windows-redirect-integration`.** The existing common redirect replay already contains
Windows environment and hidden-process handling. Removing the Windows dispatch refusal enables its real native **54/54** cases with the
pinned public Windows Preview and SDK; no CLI product, SDK, archive pin, publisher key, host-tools input or runtime contract changes.
The historical `replay_redirect_linux.py` filename is retained for existing callers; its documented native scope now includes Windows.

`redirect` gains a `win-x64`/`windows-2022` matrix row. The workflow resolves the temporary work path through Python's environment rather
than shell-specific `$RUNNER_TEMP` syntax, and retains the result plus runner stdout/stderr. The common driver checks native host matching,
exactly 54 distinct named passing cases, no skip, the selected CLI executable hash and recorded command exits limited to 0/1/2. Missing,
partial, duplicate, failed, skipped, foreign-CLI or crashed-process reports fail. OpenSSL key/certificate commands have a 60-second bound;
the outer replay has a 1500-second bound and retains partial logs on timeout. Processes remain hidden and non-interactive on Windows.

The replay downloads the selected public release bytes with their existing pins, authenticates the bootstrap signature, then serves the
exact SDK manifest/contract/archive behind controlled loopback TLS origins. It checks allowed redirects and acquisitions, denied authorities,
ports, suffix look-alikes, credentials, fragments, HTTP downgrade, invalid/missing/duplicate Location, loops/hop limits, wrong-certificate TLS,
404/oversize/interrupted bodies, digest mismatch, argument refusals, query/header redaction, provenance and installed-cache reuse.
This uses a cleared environment and private home/store; it is not restricted-token source denial or network/executable sandboxing.
Real GitHub CLI acquisition has its own earlier acceptance; these 54 controlled cases are not relabeled as real remote redirect origins.

Native inputs are unchanged from `.github/ci/pins.json`: bootstrap `v0.1.0-preview.20261117.windows-x86_64.1`, archive
`02396e04cd4775cb7cfd65bd5c0e2c8975c4fa5b4ad18cfa039f65bf5cdd706f`, executable
`feb9779109892cf73235307617398e8dfe70bdadcc86d3dff22ba04642c84df1`; Dev SDK `v0.0.1-preview.20261110.windows.1`, manifest
`b0fca4c81d29b8eee28e0d9fb7d6e26950b094a8be4f2c091e43922366a46976`, archive
`c71bb91e9f2c623d44e8154c7798a446b850781bdc1cf515eb9675dc04137be1`, contract
`7762f572b4614b3068e1ea99694ce19360e213d31c5bd01019e4ad6ce005b726`.

Local evidence: `C:\Users\Lee\s14\r` (initial native run) and `C:\Users\Lee\s14\r2` (new common result gate and log capture), each **54/54**;
second result SHA-256 `0f734f50a1bb0e1573c926cee3577860a49b8a596cfe498cbae0805617ff24f8`. CLI ignored
`out/redirect-win-x64-20261119/` holds logs; focused fixtures pass **43/43**, Python syntax and actionlint 1.7.12 pass (excluding only its
obsolete Ubuntu 26.04 label warnings). These fixtures do not substitute for the real native cases.

Manual `scope=redirect` selects the four pin jobs and four redirect jobs; port jobs are intentionally outside that verification scope.
Omitted/default `scope=all`, unknown scopes and scheduled events keep the complete port matrices. The schedule stays `23 3 * * *`.
Scoped success does not qualify unexecuted port suites; their unchanged source/pin evidence remains the preceding adopted integration.
Reproduce: `python tools/ci_integration.py redirect --host win-x64 --work <fresh-short-work>`; hosted:
`gh workflow run integration.yml --ref <review-branch> -f scope=redirect`.

Producer documentation synchronization is separately committed as CRT `dae4d141abe56d1f8fe5b298aff657df60cbdffd`, with
[its documentation CI](https://github.com/webos21/crt/actions/runs/44504270139) passing. It links the prior observed hosted handoff rather
than claiming a new producer replay. The first scheduled Windows run, ARM64 D1 and private-DLL qualification remain separate.

### Hosted redirect qualification

[Integration 44505928361](https://github.com/webos21/crt-cli/actions/runs/44505928361), `scope=redirect`, runs exact consumer commit
`c97d2a73ee25c98e9dda6df18e60c7e93b176098`. All four `pins` jobs pass the 43 focused tests and pin validation. All four native `redirect`
jobs pass **54/54 cases** and record **48 CLI invocations** each. The two port job records (Windows plus the Linux/macOS matrix) are
intentionally skipped by this requested scope; this is not a new full 12-job/port-suite acceptance run.

Downloaded artifacts independently pass `judge_redirect_result` and match executable hashes re-derived from newly downloaded,
signature-authenticated public bootstrap packages. Foreign-host executables are hashed, never run on this Windows reviewer machine.

| Native hosted replay | Cases | Result SHA-256 |
| --- | ---: | --- |
| Linux/x64 | 54/54 | `5ad1691374fdfb60076128a2937e05455f3c3421a39c9351f19a5cc7cd25c3aa` |
| Linux/arm64 | 54/54 | `266681c8da6718fc5b5e27c256b84f8b93e04e7fcada13acb73917bbee812ea6` |
| macOS/arm64 | 54/54 | `0118de9ccbb19149d8c41599bb57b5c8ed41864a56cc49def707eea29eadadf6` |
| Windows/x64 | 54/54 | `fe529b6a3801c262f6fcdf30eaf670bb48746e959a607e4e2b18a99c74d19196` |

The Windows result uses the same `feb97791...84df1` executable as both local runs. Artifact ID `14518234349`, ZIP digest reported by GitHub
`f188ba330b09e1bb6a555bba6a3ee3f0f4cb64a7447a17707fa0c118fa6cb493`. Evidence is retained in CLI ignored
`out/redirect-win-x64-20261119/`: `hosted/`, `hosted-run.json`, `hosted-artifacts.json`, `hosted-verification.json`, `check_hosted.py` and
identity-only bootstrap downloads. Product/test C++ sources, manifests, bootstrap/SDK/host-tools pins and port execution are unchanged from
adopted `858c04b`; the earlier qualified port artifacts remain their evidence.

### Main adoption of Windows redirect

Latest `origin/main` was merged into the validated branch without additional source changes; main fast-forwards and is pushed to
`495840f59dd6d58fc06bfbe30e51a405d5961274`. Its delta from qualified `c97d2a7` is four documentation files only.
[Main CI 44507080338](https://github.com/webos21/crt-cli/actions/runs/44507080338) passes all six native rows and `CI gate`.
The full run metadata is retained in `out/redirect-win-x64-20261119/main-ci-run.json`.
The default daily/manual configuration now has four pin, four redirect and four port legs. The scoped verification above proves only its
requested eight jobs, while port inputs and their prior qualified source remain unchanged. First scheduled Windows observation was still open
at this adoption; the [later review](#first-scheduled-windowsx64-integration-run-2026-11-19) records the actual scheduled result.

## First scheduled Windows/x64 integration run (2026-11-19)

**Pass, observed retrospectively on 2026-11-20.** GitHub API identifies
[Integration 44518672154](https://github.com/webos21/crt-cli/actions/runs/44518672154) as `event: schedule`, attempt 1,
branch `main`, exact source `495840f59dd6d58fc06bfbe30e51a405d5961274`. Its commit ancestry includes Windows integration adoption
`f06a036c01c1e71d14df87c1f082b1df816c33c6` and Windows redirect adoption. It is the earliest scheduled run after that adoption in the
complete returned schedule history (three runs). Created/started **2026-11-19 11:52:24 UTC (20:52:24 KST)**,
completed and updated **12:08:44 UTC (21:08:44 KST)**. This is an existing scheduled run, not a new manual dispatch.

All **twelve jobs pass**: `pins`, `redirect` and `ports` on Linux/x64, Linux/arm64, macOS/arm64 and Windows/x64. No job is skipped.
Windows uses `windows-2022`; pins pass, redirect completes at 11:53:18 UTC, and ports run from 11:52:45 through 12:08:43 UTC.
Downloaded Windows artifacts are independently checked with the existing `judge_port_result` and `judge_redirect_result`, in addition to
the workflow conclusion. The ports review also checks signed public host-tools/download provenance, exact SDK/source/driver inputs,
oracle-Python scope, command exit/timeout/undeclared-image records and cache reuse.

| Windows port suite | Passed checks |
| --- | ---: |
| `port-build-test` | 36 |
| `port-store-test` | 97 |
| `port-oracle-test` | 21 |
| `port-command-test` | 33 |
| `port-project-test` | 83 |

The five suites total **270 checks**, alongside **29 native replay gates**, with **3710 declared owned processes**, no timeout or undeclared
image. Cold self-build executes 21 sources/reuses zero; warm self-build executes zero/reuses 21. Its source-built CLI is
`9246a6346fc34224a9eab6fe42a266325e56d1bf52021ce051eb96565de1c6f9`.
Oracle Python is `C:/hostedtoolcache/windows/Python/3.12.10/x64/python.exe`, SHA-256
`4d6f5f81a4bca11191c4c7c6b43632694d0a4ce74e068619d8fdc161d469859a`, copied into the isolated oracle view for `port-oracle-test` only.
Windows redirect separately passes **54/54 cases**, 48 CLI invocations, with the unchanged published executable
`feb9779109892cf73235307617398e8dfe70bdadcc86d3dff22ba04642c84df1`; its digest is independently compared to the retained public Preview copy.
These are different CLI build identities and are not relabeled as one artifact.

Artifact ZIP digests below are reported by GitHub metadata; extracted result files are independently hashed and judged.

| Downloaded Windows evidence | Identity |
| --- | --- |
| Ports result SHA-256 | `5d3dbfddb7595d323336546ebbe4fc7da1ae5a32d8a3b7e569a5ad7d3cc5357f` |
| Redirect result SHA-256 | `7d464a5b24d9c8e3f6fd871f3c12b7899c26339594eb200eeef6e6ea215f4a57` |
| Ports artifact ID / ZIP bytes | `14527553680` / `176076` |
| Ports artifact ZIP digest | `43a7c711e4d442c4f5557ff81e7222fa8828a077561317cf9b9cc3c642ad7787` |
| Redirect artifact ID / ZIP bytes | `14527006101` / `5300` |
| Redirect artifact ZIP digest | `bf892703e139b72ebdd9b4ea1575a4b56ede01c7e87c39172a1eb283e21667c7` |

Original API metadata, downloaded artifacts, result-judge receipts and reviewer script are retained in ignored
`out/scheduled-win-x64-20261120/`: `run.json`, `schedules.json`, `artifacts.json`, `hosted/`, `verification.json` and `verify_schedule.py`.
The existing Windows port-artifact reviewer is `out/win-host-tools-pin-20261119/check_hosted.py`; invocation supplies the retained artifact
directory and exact `495840f` source revision. Source/driver/workflow and integration/bootstrap pin files are unchanged between that run
and review baseline `04ec435`. This observation does not constitute a fresh current-source native matrix run.

**Cadence remains open.** Configured cron is `23 3 * * *` (03:23 UTC / 12:23 KST). API creation times for the three recorded schedule
events are 2026-11-17 12:04:01 UTC, 2026-11-18 11:55:40 UTC and 2026-11-19 11:52:24 UTC; all three runs succeeded.
The Windows run was created 8h29m24s after the nominal daily clock time. The event metadata proves actual scheduling and execution,
but does not establish why the start differs from cron or guarantee future daily delivery. No timing cause or service outage is inferred.
The first Windows observation TODO is checked; the separate cadence item stays unchecked. No source, workflow, pin, release, schedule,
global/security setting or STATUS change is made.

## Windows/arm64 hosted integration expansion (2026-11-20)

`integration.yml` selects `windows-11-arm` for `pins`, `redirect` and `ports`. The signed public host-tools archive is
`v0.1.0-hosttools.20261119.windows-aarch64.1` (697,220 bytes, archive SHA-256
`c74ad50ea5f5008d515e1aac3581fa5d73d7acba276ae990a9d3df84599706ee`, manifest SHA-256
`6f7225ac4ffafe4c5651bf025b47d7b58df0939c520dbf40b7a029aa2892b6f1`, signature SHA-256
`2956f38c0621feae16a0c6f02ca8346cc77423c33f8adcc108d92f7d8d46443f`). The port driver re-downloads and authenticates it,
checks the signed 20261118 CLI seed and ARM64 Dev SDK pins, and checks the build against the native PE machine and declared OS imports.
The x64 and ARM64 jobs share the authenticated test-host helpers at producer revision `bc195a1915b6ee21ff98957af9eb2003cf334e6e`.
ARM64 uses native Python, hash-checked LLVM 22.1.8 and Windows SDK `um/arm64`.
[Producer's public-byte replay](https://github.com/webos21/crt/blob/9867851515950749e83a3c2c59759a23f8bcf444/docs/acceptance/host_tools_acceptance.md#windowsaarch64-host-tools-public-signature-and-native-replay-2026-11-19).

Local Windows/arm64 pin checks and all 43 integration driver tests passed. `ci_integration.py redirect --host win-arm64`
passed **54/54** with the pinned public CLI (`132c8b5a...3db8`) and SDK. Its receipt in
`C:\Users\cmjo\w\ia64r\redirect-replay\redirect-replay.json` has SHA-256
`4e82d2ebbaa369eef397fe5107af3cc239b8af2de2a947ad3e429b7b1fba61bc`.

The first local `ports --host win-arm64` run failed in `port-store-test` while two threads in one process published a single key.
`file_sha256` reported a read error on the copied SDK's `LICENSE.md`. A separate C++17 program built against the same public SDK,
without any port or CLI code, ran two threads opening, reading and closing distinct read-only one-byte files (20,000 iterations each).
It exited 1. The two readers reported 1/1 read errors, 3/2 wrong bytes and 1/1 close errors. Its source is retained in
`out/arm64-fd-concurrency.cpp` (SHA-256 `3a6a84738390f8aa376e5e1d4b2f26133d354c43f4cbc112849c71047bd6c5ff`), and the first
suite streams and command ledger are under `C:\Users\cmjo\w\ia64p\replay\`. The pinned producer's descriptor allocator is a starting
point for the remaining CRT investigation. [Pinned CRT source](https://github.com/webos21/crt/blob/e7c763094d96ccba1b524077b24b04b876a91d5b/libc/src/arch/windows/common/syscall.c#L3533).

The CLI's publication test now spawns two independent processes against one fresh shared store and the same completed build and key.
It requires both workers to finish, one to create the item, the other to reuse it, and the resulting item, staging area and locks to
pass the existing checks. This exercises the competition between actual CLI processes, including distinct owner PIDs. The isolated
producer thread reproducer remains failing on the SDK in use.

The initial [hosted run 44660444614](https://github.com/webos21/crt-cli/actions/runs/44660444614) reproduced the in-process failure
on `windows-11-arm` and `windows-2022`, both in `port-store-test`. The ARM64 artifact `14596012931` records read failures while
hashing libc++ headers. Both failed Windows artifacts are retained under `out/gha-44660444614-arm/` and `out/gha-44660444614-x64/`.

The fresh native ARM64 rerun at `C:\Users\cmjo\w\ia64q\replay\` passed all five suites and 29 gates. Its `result.json` SHA-256 is
`1dce728cdf93eba3bf52199288f9d4f841452443811c5e3719eaea55fe998056`.
[Hosted integration 44662115742](https://github.com/webos21/crt-cli/actions/runs/44662115742) at
`e6dc40acddfaee43e67b9691a813f0aa3cd3dc3e` passed all 15 jobs. The ARM64 redirect artifact matches the pinned public executable
and records **54/54**. The ARM64 port artifact reports **270** checks (build 36, store 97, oracle 21, command 33, project 83),
**29** replay gates and **5,023** declared process records. The cold self-build executed 21 actions and the warm self-build reused
all 21. The driver checked real symlinks, the hidden-input boundary, the ARM64 PE machine, the OS import allowlist, and unchanged
source, SDK and host-tool inventories. The independent artifact review in `out/gha-44662115742-arm/verification.json` records ARM64
port and redirect result SHA-256 values `7d111e4983cfcebb4da04eb67869b313b839af15028b9e94957b79b8dc00011a` and
`9c331a5e6490b3aaec3615da3ffb7412ed5cf801ede101ee7d2f0420270ef679`.

All six required native CI jobs and `CI gate` passed for the same commit. [PR CI 44662121328](https://github.com/webos21/crt-cli/actions/runs/44662121328).
The tested commit entered `main` through [PR #34](https://github.com/webos21/crt-cli/pull/34) as merge `aa06fb3`, whose
[main CI 44664407453](https://github.com/webos21/crt-cli/actions/runs/44664407453) also passed.

## Windows/arm64 workflow Preview candidate (2026-11-20)

[Release run 44664411493](https://github.com/webos21/crt-cli/actions/runs/44664411493) was dispatched from `main` with `host: win-arm64`,
`dry_run: false`, date `20261120`, serial `1`, `.github/release/preview-notes.md` and tested source `e6dc40acddfaee43e67b9691a813f0aa3cd3dc3e`.
Native `package` and `verify` passed on separate `windows-11-arm` runners. The package and verify receipts agree on the archive digest.

| Input or candidate | Exact identity |
| --- | --- |
| Bootstrap CLI | Signed `v0.1.0-preview.20261118.windows-arm64.1` |
| Dev SDK | `v0.0.1-preview.20261118.windows.arm64.1`, content `3b251637845621ba28d6ba1d9c2f6a5e097f6b328f57054f7e7cb5c5eb52621f` |
| Tag | `v0.1.0-preview.20261120.windows-arm64.1` |
| `crt-cli-0.1.0-dev-windows-aarch64-preview.20261120.1.tar.xz` | 5,234,232 bytes, SHA-256 `de7efa11662833bf3790a23ffa4b24ef4dfe503de2de4cf6e53f87562709d8a0` |
| Package manifest | `53d91b8e62bf3eda43a086a72cec531ead57eda7c41f278f97038cba236b6725` |
| `crt-cli.exe` | `e15efbb25b3ea9e4b0ccdb8327ed58b23c6b85c7e5a6b7b068ac16cadeae394b`, PE `0xAA64`, imports `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll` |

The hosted package verifier exercised version, self doctor, incorrect manifest-pin refusal, real HTTPS SDK acquisition, and C and C++
creation, build, test, run and byte-identical lock reuse (3 cold commands, 0 warm for each language). The workflow archive and verify
receipts are retained at `C:\Users\cmjo\w\ar20\pack\` and `C:\Users\cmjo\w\ar20\verify\`. The workflow moved to the protected
`release` environment review after those jobs passed. Publisher signing follows the owner's offline procedure.
[Package artifact](https://github.com/webos21/crt-cli/actions/runs/44664411493/artifacts/14597532662).

The authenticated `webos21` reviewer account approved `release` for the requested publication after comparing the package and verifier
digests. The `publish` job passed and the prerelease tag resolves to the tested source commit. The owner's offline detached signature
was verified and attached to the published archive. The native CLI verified it against a fresh public download.
[Public-byte verification](preview_acceptance.md#windowsarm64-workflow-preview-release-2026-11-20).


## Scheduled Integration history and the macOS runner failure (2026-11-24)

The complete `schedule` history of `integration.yml` was read through the GitHub API on 2026-11-24: seven runs, all attempt 1 on `main`. Configured cron is `23 3 * * *` (03:23 UTC). Times are UTC.

| Run | Created | Source | Result | Cause |
| --- | --- | --- | --- | --- |
| [44079658878](https://github.com/webos21/crt-cli/actions/runs/44079658878) | 11-17 12:04:01 | `d0efcaa` | success | |
| [44299098232](https://github.com/webos21/crt-cli/actions/runs/44299098232) | 11-18 11:55:40 | `3aa7030` | success | |
| [44518672154](https://github.com/webos21/crt-cli/actions/runs/44518672154) | 11-19 11:52:24 | `495840f` | success | [Windows record](#first-scheduled-windowsx64-integration-run-2026-11-19) |
| [44731342833](https://github.com/webos21/crt-cli/actions/runs/44731342833) | 11-20 11:54:07 | `1ad5e57` | success | |
| [44898897532](https://github.com/webos21/crt-cli/actions/runs/44898897532) | 11-21 11:23:42 | `8374470` | **failure** | `ports (win-arm64)`: concurrent store-lock race in the CLI, fixed ([record](sdk_install_acceptance.md)) |
| [45018368929](https://github.com/webos21/crt-cli/actions/runs/45018368929) | 11-22 11:39:10 | `44fbf24` | success | first scheduled run after the lock fixes, all 15 jobs |
| [45181292472](https://github.com/webos21/crt-cli/actions/runs/45181292472) | 11-23 13:10:31 | `1ab78e1` | **failure** | macOS/arm64 runner never assigned (below) |

**2026-11-23 failure: infrastructure, not a CLI result.** The run was created 9h47m after the nominal clock time. `pins (macos-arm64)` and `redirect (macos-arm64)` (label `macos-15`) were created at 13:10:32, never received a runner
(`runner_name` is empty, zero steps) and were cancelled at 13:25:33, 15 minutes later. The check-run annotation reads "The job was not acquired by Runner of type hosted even after multiple attempts", next to GitHub's notice
"Due to capacity constraints, jobs targeting macOS arm64 runners may experience longer queue times." Both `ports` job definitions (`ports` and `ports-windows`, each `needs: pins` for the whole `pins` matrix) were skipped, and their names show the unexpanded `${{ matrix.host }}`, so **no port suite ran on any host**: one cancelled
`pins (macos-arm64)` leg suppresses the port suites of Linux and Windows too. Only `pins` and `redirect` ran, and the other eight jobs passed: `pins` and `redirect` on linux-x64, linux-arm64, win-x64 and win-arm64. No macOS check of any kind ran.

Between the last green scheduled source `44fbf24` and `1ab78e1` no file under `src/`, `include/` or `.github/` changed; four replay scripts under `tools/` were added and none is used by `integration.yml`. That makes a regression in the
tested inputs unlikely but does not prove it: the macOS result for `1ab78e1` is **unobserved**, not passing. The last macOS scheduled results are the 11-22 pass (all 15 jobs, `44fbf24`) and the 11-21 run in which macOS passed. Whether the
failure repeats was not tested; no re-run or manual dispatch was started for this record.

**Cadence and closure.** Creation times fall 8h00m to 9h47m after 03:23 UTC on all seven days (11:23 to 13:10 UTC), never near the configured time. The cause was not established. The owner closed the cadence observation on
this evidence: the scheduled trigger demonstrably fires and runs the workflow (seven runs, five green, two red for stated reasons), while the start time and the allocation of hosted runners are governed by GitHub's scheduling policy and
runner capacity, which this repository cannot control. A late start or an unassigned macOS runner is therefore not a defect to fix here. It still has a visible cost, recorded so a reader does not misjudge a red run: a run can fail on runner
availability alone and looks like a regression at the run level; a macOS capacity failure leaves macOS unverified and, through `needs: pins`, every host's port suites unrun for that day. A red scheduled run is triaged by reading the
job annotations first (a runner that was never assigned is infrastructure; a failed step is a regression or a changed external input). Nothing tracks this item any more: a macOS-only gap is repaired by a manual dispatch, and making
`ports` wait only for its own host's `pins` would be a separate, reviewed workflow change.

No source, workflow, pin, schedule, release or setting change is made; the TODO cadence item (CI-03) and its pointers in STATUS were removed at the owner's request once this record and HISTORY held the result.

## Hosted sign-before-publishing gate (2026-11-25)

The `release` environment permits only `main` and requires an owner
approval for each stage and finalization. [PR #47](https://github.com/webos21/crt-cli/pull/47)
and [post-merge CI](https://github.com/webos21/crt-cli/actions/runs/45510253214)
passed at the signing workflow change. The hosted Linux/x86_64 runs were:

| Run | Result |
| --- | --- |
| [Dry-run stage](https://github.com/webos21/crt-cli/actions/runs/45510380183) | Package, verify and stage passed; no finalization receipt was uploaded. |
| [Dry-run finalization](https://github.com/webos21/crt-cli/actions/runs/45512751143) | `plan` refused the missing receipt; `finalize` skipped. |
| [Real stage](https://github.com/webos21/crt-cli/actions/runs/45513708388) | Native package and verification passed; attempt 1 retained a receipt for the exact non-public draft. |
| [Unsigned finalization](https://github.com/webos21/crt-cli/actions/runs/45518151962) | `plan` passed; the separately approved `finalize` refused the missing signatures and left the draft non-public. |
| [Signed finalization](https://github.com/webos21/crt-cli/actions/runs/45521362159) | After offline signing of both files, the approved workflow retried with the same stage receipt, published a non-latest prerelease and verified its downloads. |

The CLI's [release evidence](preview_acceptance.md#linuxx86_64-sign-before-publishing-preview-2026-11-25)
records independent public verification and its SDK input. The
[CRT producer record](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#sign-before-publishing-qualification-2026-11-25)
covers the SDK and joint gate. Incorrect signatures, changed/extra assets
and retry safety also have local controls in `tools/test_release_signing.py`;
these are not additional hosted publication runs. P1-01 is closed. CI pins
and consumer locks did not change; other hosts were not published through
this gate for this record.

## CI monitoring and pin promotion review (2026-11-25)

Review baselines after `git pull --ff-only`: CRT `9460fc87`, CLI `2c5e2e2`
and extension `6ee57fe`. Each repository matched its remote `main`. The latest
CRT/CLI records close sign-before-publication qualification on Linux/x86_64.
The CLI and extension records also distinguish completed PAL prerequisite
replays from the pending editor API and Extension Development Host gates.
The [CI work group](../../TODO.md#ci-monitoring-and-pin-promotion) led
In Progress at this review. Execution results are recorded below in the
[pin adoption](#qualified-ci-pin-adoption-ci-01-2026-11-25),
[Pi compatibility](#raspberry-pi-arm64-native-compatibility-ci-02-2026-11-25) and
[required-check rejection](#non-bypass-required-check-rejection-ci-04-2026-11-25) records.

### CI-01 candidate review and promotion order

`python3 tools/check_ci_pins.py` passed for `linux-arm64`, `linux-x64`,
`macos-arm64`, `win-arm64` and `win-x64`, including archive signatures under
the committed publisher key. The reviewed `.github/ci/pins.json` SHA-256 is
`02b15b9f5fcc8004ac9cab2ff6239465ee221055b152bea483753ddcf0a52078`.
The comparison below uses the [existing promotion policy](../guides/development.md#ci-pin-promotion).

| Host/input | Retained pin | Reviewed signed candidate | Evidence and next gate |
| --- | --- | --- | --- |
| Windows/arm64 CLI | `v0.1.0-preview.20261118.windows-arm64.1` | `v0.1.0-preview.20261120.windows-arm64.1` | The [public release record](preview_acceptance.md#windowsarm64-workflow-preview-release-2026-11-20) includes native package and D1 replay. The release's `CI gate` and package/verify/publish run passed. Prepare a Windows/arm64 bootstrap promotion PR with the retained SDK combination and require the affected native leg plus the complete PR `CI gate`. |
| Linux/x86_64 CLI | `v0.1.0-preview.20261116.linux-x86_64.1` | `v0.1.0-preview.20261125.linux-x86_64.1` | The [new release](preview_acceptance.md#linuxx86_64-sign-before-publishing-preview-2026-11-25) was packaged and verified natively with the retained November 18 SDK. A separate CLI promotion PR must exercise both x86_64 Ubuntu legs, the complete gate and stage receipts with that exact combination. |
| Linux/x86_64 SDK | `v0.0.1-preview.20261118.linux.x64.1` | `v0.0.1-preview.20261125.linux.x64.1` | The [producer qualification](https://github.com/webos21/crt/blob/9460fc87/docs/acceptance/release_qualification.md#sign-before-publishing-qualification-2026-11-25) includes native C/C++ smoke with the November 16 CLI. A new SDK promotion PR needs the full CLI bootstrap/test receipts and affected integration redirect replay. The November 25 CLI release consumed the November 18 SDK, so the two November 25 artifacts still need their own combination run. |

This review independently downloaded the two candidate CLI archives and
their signatures, checked their release-record sizes and SHA-256 values,
and verified both signatures. It also checked the November 25 SDK manifest
against its producer-record pin and verified the manifest-listed archive
and policy digests and their detached signatures. The publisher SPKI SHA-256
is `59218576956e493476bc36f7385d8e01a03ff9b92946a531b013ad3937d43fa2`.
Existing macOS, Windows/x64 and Linux/arm64 input pairs retain their accepted
records. Later unsigned PAL/CI SDK artifacts keep their producer-owned release
and signature prerequisites.

Execute Windows/arm64 CLI promotion first, then the Linux/x86_64 CLI review,
then the Linux SDK combination gate. A Linux host-specific PR may adopt the
CLI and SDK together when its complete input tuple has the required evidence.
Each promotion PR quotes a fresh
all-host pin check, the exact archive/signature and SDK identities, the
release evidence and its affected CI results. After merge, record the `main`
run and stage-1/stage-2 outcome. Preserve the preceding pin tuple in Git
history and use it if promotion fails. CI-01 remains open for these adoption
steps.

Existing work supplies the [promotion procedure](#ci-pin-promotion-2026-11-11),
the [Linux bootstrap adoption](#linux-bootstrap-pin-moved-to-the-20261116-preview-2026-11-17),
[macOS/Windows bootstrap adoption](#macos-and-windows-bootstrap-pins-moved-to-the-20261117-preview-2026-11-17)
and [Linux SDK adoption](#linuxx86_64-sdk-pin-moved-to-the-first-automated-preview-2026-11-18).
Reuse those records and the candidate release checks above. The remaining
adoption evidence must come from runs that actually consume the selected new
pin tuple. Candidate publication tests remain valid for their recorded inputs.

### CI-02 native compatibility handoff

CRT's [native compatibility P1-01](https://github.com/webos21/crt/blob/9460fc87/TODO.md#p1---native-compatibility-beyond-hosted-runners)
still requires a real Raspberry Pi ARM64 device and explicitly identified
Linux/glibc/loader/LLVM combinations. The retained CLI/SDK ARM64 pair has
Ubuntu 26.04/glibc 2.43 hosted evidence. The next producer/consumer record
must identify the device, OS image, loader, tool paths/versions, exact SDK/CLI
inputs, native C/C++ build/test/run results and actual runtime dependencies.
Link that record before broadening the advertised platform scope. CI-02
remains open for native device and distribution evidence.

The existing [Linux/x86_64 SDK adoption](#linuxx86_64-sdk-pin-moved-to-the-first-automated-preview-2026-11-18)
already records Ubuntu 24.04/glibc 2.39 and Ubuntu 26.04/glibc 2.43. The
[producer loader-repair closure](https://github.com/webos21/crt/blob/9460fc87/docs/acceptance/sdk_split_acceptance.md#required-hosted-replay-closure)
links the corresponding full hosted x86_64 runs. These exact x86_64 bounds,
the ARM64 26.04 bound and the producer ownership link are reusable completed
evidence. Raspberry Pi and additional ARM64 loader combinations still require
their own native records.

### CI-04 ruleset observation

A fresh GitHub API read confirms ruleset `27002916` is active and requires
`CI gate` from GitHub Actions integration `15368`. Repository administrators
still have bypass mode `always`. The authenticated account is `webos21`,
the repository owner. The [accepted passing and blocked PR observations](#current-hosted-coverage-2026-11-11-1)
retain their scope. A second account without bypass must supply the direct
push or failing-check merge rejection, including its actor, target SHA,
check state and actual GitHub rejection. Revisit bypass settings with the
second maintainer. CI-04 remains open for that observation.
The existing ruleset configuration and `CLEAN`/`BLOCKED` PR observations are
completed sub-items. Only the actual non-bypass rejection needs a new actor
and execution record.

Local review artifacts are retained in `out/ci-monitoring-review-20261125/`:
the current pin-check log, retained pin snapshot, candidate downloads and
verification JSON, release-run/CI summaries and ruleset snapshot. These are
read-only input and policy checks. Promotion PRs, native compatibility runs
and non-administrator enforcement remain the next execution gates.

### Windows/arm64 bootstrap promotion proposal (2026-11-25)

The host-specific proposal updates only the Windows/arm64 bootstrap tuple to
`v0.1.0-preview.20261120.windows-arm64.1`. The archive is 5,234,232 bytes,
SHA-256 `de7efa11662833bf3790a23ffa4b24ef4dfe503de2de4cf6e53f87562709d8a0`.
Its 256-byte signature SHA-256 is
`b4b5eab4a7ac9f0315099a8c17091fd331d0958067b319f30f5a397118c80c63`.
The paired SDK is the retained `v0.0.1-preview.20261118.windows.arm64.1`.
The all-host public pin checker passes on this proposal, and the 11 pin-review
and 17 bootstrap-driver tests pass. The PR `CI gate` and main adoption receipts
are the remaining gates for this proposal.

## Non-bypass required-check rejection (CI-04, 2026-11-25)

The [observation run](https://github.com/webos21/crt-cli/actions/runs/45530662933)
passed on the isolated test branch at `03e132d`. It used only the job's
repository `GITHUB_TOKEN` with `contents: write`. This is a GitHub App
installation token, the documented [Actions authentication model](https://docs.github.com/en/actions/concepts/security/github_token).
The workflow initiator was `webos21`; the authenticated write principal was
the Actions app, outside the ruleset's administrator-role bypass. The test
commit is attributed to `github-actions[bot]`.

The app created an unprotected control reference successfully, proving its
repository write permission. It then attempted a fast-forward update of
`refs/heads/main` through the Git references API to commit
`e59dc1b9a3b4ccac24b13fbdc1a3e09f5e0572f6`, whose file tree equals its
parent on main. That commit had zero checks. GitHub refused the update with
HTTP 422 and the exact reason:

```text
Repository rule violations found

Required status check "CI gate" is expected.
```

Main stayed at `2c5e2e24f767ba9d2e4da686837a4f09eaa970bd` before and after
the request. The test removed its control reference. The temporary workflow
was confined to its test branch. This supplies actual non-bypass rejection
in addition to the earlier `CLEAN`/`BLOCKED` PR observations. Administrator
bypass remains `always`; a second maintainer's arrival can trigger its policy
review. The observed actor and API operation are recorded explicitly.

Raw candidate/check/update/result JSON and its run log are retained in
`out/ci-monitoring-review-20261125/ci04-observation/`; the workflow source is
`out/ci-monitoring-review-20261125/ci04-observation-workflow.yml`. The observation
branch was deleted after archiving the result. `result.json` SHA-256 is
`d871785dd1392b4ce9d4e633456080af67699e200cbf216fbb16a0a77a64009c`.
CI-04 is complete for the observed non-bypass app/API operation.

## Raspberry Pi ARM64 native compatibility (CI-02, 2026-11-25)

A physical Raspberry Pi 3 Model B Plus Rev 1.3 ran the retained public
Linux/arm64 pair natively. Host: 64-bit Debian 13.7 (trixie), aarch64,
kernel `6.18.50+rpt-rpi-v8`, glibc 2.41, Debian Clang/LLD 19.1.7 in
`/usr/lib/llvm-19/bin`. The interpreter `/lib/ld-linux-aarch64.so.1` resolves
to `/usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1`, SHA-256
`32f8643735b7bbc6375bdae3198376ef0e130be0006150fb1524f04c3528e07a`.
This adds an actual device and an older ARM64 host libc to the existing
Ubuntu 26.04/glibc 2.43 hosted evidence. The [CRT producer handoff](https://github.com/webos21/crt/blob/1bafd9fffccf146e3501f12759b1cc7a73e6cd09/docs/acceptance/release_qualification.md#raspberry-pi-arm64-native-compatibility-2026-11-25)
closes its bounded native-compatibility task; broader distribution/device
support still requires its own qualification.

Inputs were downloaded on the Pi using a copy of the public CI tooling at
CLI `f8f2b3493a95ca779e24202ebca4d6dbb2c6975f` (its ARM64 tuple is unchanged).
No producer source/build tree was used. Each archive passed size, digest and
publisher signature checks under SPKI SHA-256
`59218576956e493476bc36f7385d8e01a03ff9b92946a531b013ad3937d43fa2`.

| Input | Exact public identity |
| --- | --- |
| CLI | `v0.1.0-preview.20261116.linux-arm64.1`, `crt-cli-0.1.0-dev-linux-aarch64-preview.20261116.1.tar.xz`, 5,063,404 bytes, SHA-256 `311bc451b0c20402f6298ba469e596326bef2c5ae526333ef861edff860e77aa` |
| CLI executable | `4b5109e99568fcf0f80d62f5e496748aec471d31dd8ea7ddc5e6235e339b3d3b` |
| SDK | `v0.0.1-preview.20261110.linux.arm64.2`, producer `e6a94ab9e69b05497b3804328971d24d6071a53d`, 7,632,648-byte archive SHA-256 `5c99dc59b747cb31cb623fa057264c3a4e7d5715f960203073dc1b34f664a1c4` |
| SDK manifest | `ba77b26edfa0413df5e8c17309506d7ae6ecf134ae7e9908e3742bc562067be3` |
| SDK content / policy | `cd2ab657615c0fa52a1804f301a57dfa21b1067e3e5ed5a9f9a07d86ae050cc6` / `a3df2fbd6b4d14d316b691a1d2831d0692aaba133e3b168076ee0ec7cb6434d4` |

The existing package verifier was invoked with the following argv, using a
fresh work directory and the downloaded archive/checksum files:

```text
python3 tools/ci_release.py verify --host linux-arm64 --date 20261116 --serial 1 --release-dir /home/pi/crt-ci02-20261125.BRxhDLr6/public --work "/home/pi/crt-ci02-20261125.BRxhDLr6/native space 한글/verify" --llvm-bin /usr/lib/llvm-19/bin
```

Its 30-file package verification, initial SDK-free version/doctor, SDK
acquisition and doctor/wrong-pin rejection passed. Generated C and C++
projects each passed cold build (3 commands), test, run and warm build
(0 commands), preserving the exact project lock. Project child environments
used a private HOME/TMPDIR and `PATH=/nonexistent`; external tool paths were
explicit. Python, curl and readelf are test-host tools, not CLI prerequisites.

Two additional fresh C++17 projects selected `--runtime-linkage static` and
`shared`. Their actual manifest `main.cpp` was replaced with a probe using
vector, iostream, `std::runtime_error` throw/catch and `/proc/self/maps` output.
The driver asserts that source is present in the compiler argv, requires the
`C++ runtime and maps passed` marker, and hashes every mapped file. Both
passed build/test/run (cold 3/0) and warm reuse (0/3), with unchanged locks
SHA-256 `9e0638840466c1d70a6e4c7a70cc647c48ceb2aa67f0a34e83a28e88c00635e3`.

| Probe | Executable SHA-256 | Observed runtime |
| --- | --- | --- |
| C++ static | `982b69980002e6fbe0ca568b11340f2cb99de7c478e4f952ef3778b9c9c5b23c` | Executable plus declared OS loader only; no `DT_NEEDED` |
| C++ shared | `6bc111553a939c804d7bc3bdf783d5e5d7f3f5014748d3c30e3e1c596ddd1bd2` | Selected SDK libc, libm, libdl, libc++, libc++abi and libunwind plus OS loader; `DT_NEEDED` agrees with maps |

Neither probe mapped host glibc, libstdc++ or libgcc_s. The CLI ELF also has
no `DT_NEEDED`; its interpreter is the declared OS loader. These results
qualify this public pair on the identified Pi/OS/LLVM combination. They do
not claim 32-bit Pi OS, other glibc versions, Full SDK/device/graphics suites,
the later pure-C shared-provider contract, or current-source acceptance.
No pin or existing project lock was changed.

The initial extra runtime script wrote an unused `main.cxx`; its additional
probe results are excluded. The existing package verifier results remain
valid. Corrected projects were created afresh and checked against their
manifest/compiler argv and runtime marker as described above.

Artifacts are in `out/ci-monitoring-review-20261125/pi-native/remote-results/`
and the remote job `/home/pi/crt-ci02-20261125.BRxhDLr6`. `host.json` SHA-256 is
`7d3576bff46ac9ad982088a9185c13205cf062014745cf9143fabfbd3a652097`;
`runtime-cpp-verified/result.json` SHA-256 is
`b0d7538a8b6f235da3e2dedbb9d4f0d0ffecd73d4f86886612d1f351d3c86417`.
The retained corrected `runtime-probe.py` SHA-256 is
`687491f63928ac45dfd2f7b5484158dd63df437cda72c4da1c69dcc8e05a7cbf`.
These receipts include tool hashes, all commands, outputs, source/executable
hashes, ELF audits and mapped-image hashes. CI-02 is complete for the
explicit native compatibility scope.

## Qualified CI pin adoption (CI-01, 2026-11-25)

The preceding candidate review was executed as two host-specific promotion
PRs with no product source or project-lock changes. Final public pin checking
passes for all five hosts after combining the changes. The previous working
pairs remain in Git history; the root `crt.lock` remains unresolved.

| Promotion | PR gate and merge | Consumed public pair |
| --- | --- | --- |
| Windows/arm64 | [PR #49](https://github.com/webos21/crt-cli/pull/49), [six-leg PR CI](https://github.com/webos21/crt-cli/actions/runs/45529621118) passed; merge `39910afe6c670a3edee6314fa380b24f6f864832` | CLI `v0.1.0-preview.20261120.windows-arm64.1`; retained SDK `v0.0.1-preview.20261118.windows.arm64.1`, content `3b251637845621ba28d6ba1d9c2f6a5e097f6b328f57054f7e7cb5c5eb52621f` |
| Linux/x86_64 | [PR #50](https://github.com/webos21/crt-cli/pull/50), [six-leg PR CI](https://github.com/webos21/crt-cli/actions/runs/45529997970) passed; merge `efb8dae777ee83aa13a915deec82b6871f3de1a6` | CLI `v0.1.0-preview.20261125.linux-x86_64.1`; SDK `v0.0.1-preview.20261125.linux.x64.1`, content `d2edb36c64f8f7c6d67603763e07a83d0cb4c2858c99e7957e00dde2d54dc921` |

The Windows CLI archive/signature digests are recorded in the proposal above.
The Linux CLI archive is 5,295,152 bytes, SHA-256
`942a45365cb3906754e5663a1c401ba223201cab9f025aa23d30e2b285a42289`,
with signature SHA-256
`63e482b7fea39e5be10824a032d153f4600a455918ef65065821298b666a3131`.
The new Linux SDK manifest SHA-256 is
`6adfa744c4739f25b5c7275316913081a644b3bfc6a185dc8ac2a8f774f2cbbe`;
archive SHA-256 is
`e455d08c31d1bf5d3cb470c4b28f09fffa228545350dc04ed099e132da85744b`;
policy SHA-256 is
`54bb35e014c59af90c1e2b73c3f544ebc7815edc958e704e15bef097dbedd2d1`.
All bootstrap and SDK authentication receipts say `signature_verified`.

The SDK change also passed [Integration redirect replay](https://github.com/webos21/crt-cli/actions/runs/45529992501)
on the PR head `f8f2b3493a95ca779e24202ebca4d6dbb2c6975f`. All five pin and
redirect jobs passed; the affected Linux/x64 result contains 54 distinct
passing cases and the selected CLI executable SHA-256
`827d974bfcb399254efcd9325fcf7df99cebc69b9a6ada7f335f777fc5ad01d5`.
Port suites were intentionally outside this redirect-only dispatch, with no
new port-suite acceptance claimed.

PR native receipts:

| Affected leg | Native / archive / SDK-store / store-lock / recipe checks | Stage 1 / stage 2 SHA-256 |
| --- | --- | --- |
| Windows/arm64 | 1040 / 214 / 113 / 9 / 147 | `b75a31c55398ac2ef7860eb5f0a9b3ca523dab2cbf128acfaf7a91b5e24ada44` / `664035c234eb2134488418da69474c39c7b497c2bf2b2ee03ee02f6a48f603c9` |
| Linux/x64 Ubuntu 26.04 | 949 / 221 / 114 / 9 / 147 | both `c3727a08bb343a98875364db5ef9a46d06fecea7e011531a8534c56effd52e47` |
| Linux/x64 Ubuntu 24.04 | 949 / 221 / 114 / 9 / 147 | both `50f09b644239a7d41b90ebc267b0b4dad946fd8ca6127e45c6de9a35aaa35320` |

Every affected stage executes 21 cold commands; stage 2 warm executes zero
and reuses 21. Windows imports only kernel32 and the declared synchronization
API-set DLL; Linux has no `DT_NEEDED` and uses `/lib64/ld-linux-x86-64.so.2`.
Bare version/help/doctor checks pass for both stages.

The Windows stage compiler/linker argv arrays are identical, but the PE
contains a wall-clock timestamp: stage 2 has `1795572678` (02:11:18 UTC),
within its 02:10:09-started build and long after stage 1 started at 02:00:53.
The selected LLVM 22.1.8 [MinGW driver](https://github.com/llvm/llvm-project/blob/ca7933e47d3a3451d81e72ac174dcb5aa28b59d1/lld/MinGW/Driver.cpp#L396)
only forces zero for `--no-insert-timestamp`; the [COFF default](https://github.com/llvm/llvm-project/blob/ca7933e47d3a3451d81e72ac174dcb5aa28b59d1/lld/COFF/Driver.cpp#L1959)
uses the current time when no timestamp/reproducibility setting is supplied.
The recorded link argv has no such override. This identifies a source of
byte differences across sequential Windows links, while both stages pass
functional and dependency gates. No claim is made that timestamp is the only
byte difference, or that Windows byte reproducibility is achieved. Linux
stage equality is measured within each runner; cross-runner equality is not
claimed.

Post-merge verification is in progress; CI-01 and its TODO group remain open until both main gates and their affected receipts are recorded.
