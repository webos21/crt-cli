# GitHub CI Acceptance

This record owns what the CI driver and workflow have actually been run against. The design and the
procedure are in the [development guide](../guides/development.md#continuous-integration); open work is in
[TODO](../../TODO.md#github-ci). A passing local rehearsal is not a GitHub-hosted run.

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

## Not run

- Ubuntu 24.04's full native suite remains blocked by the producer-side loader
  gap above; Ubuntu 26.04 passes at the recorded combination.
- The Linux, macOS and Windows driver paths ran on hosted images up to the points recorded above; Python 3.12, `actions/*` at
  their pinned commits, the LLVM 21 apt install and artifact upload worked on Linux.
- Input-gated port suites, redirect/transport replays, Linux/aarch64 and Windows/arm64 are not covered.
- Publisher authentication stays `not_verified`: pins authenticate the bytes, the runner's system roots only protect transport.
