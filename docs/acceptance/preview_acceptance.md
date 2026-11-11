# Dev SDK / CLI Preview Acceptance

This record owns bounded local Preview input and final-artifact qualification.
The [lifecycle contract](lifecycle_acceptance.md) keeps general shipping/update
requirements; [compatibility](../compatibility.md) owns contract adoption.

## Linux local candidate (2026-11-10)

Scope: native Linux/x86_64 Dev `02-cxx`, one CLI executable with pinned notice
resources, caller-supplied CA data, external LLVM and optional independent CRT
host tools. This section records the original local rehearsal; the public SDK promotion
is recorded below. The selected SDK version is
`v0.0.1-preview.20261110.3`; CLI product version remains `0.1.0-dev` with independent
source/binary identities. No Git release tag is created by the SDK parameter.

| Input | Exact identity |
| --- | --- |
| Clean CRT producer | `0ab8f8cfbbde19e14dd5861ba176fb8881e06182`; Release build, LLVM21, explicit x86_64 |
| SDK archive | `e858c1b8b22697e11551de44e1b5c3aba3cbdc93679787e33d7ae6241d6f2217` |
| SDK content | `36623957fbdd5a384cc173991e20253ff83148028ab8072298dcd434a55a2b3f` |
| Release manifest | `d4211d3298a764ff22f6e262073f834bbb36cb5d07c22e1ecb10f7f83f8fca60` |
| Portable native policy 1 | `90b3bb59aaac7df481becbad4e2b87473e2fefcb803123332689d7f1135e4e30` |
| CLI product source | `024de89d2a3e6dfbfe8b01db77262d091e2c13a7`; normalized product/test fingerprint `dcc249452758589253093f21ed48f98fbf65f94bdfb0d9d00e4362e73550d4e5` |
| Final CLI executable | `6ba046e5baeae24b86e407ad04353ec9d98b989c21921160413408ca0f71dab2` |
| Independent host-tools manifest | `922c76155957a55d095f902077cb73bbc120ae2c2453e9c1c000f5e58acdefa9`; version `v0.0.1-hosttools.20261110.3`, local-only |
| Bootstrap-resources 1 manifest | `03b526fc0f8d0801f076b6015a8ba8225093720f941196a724c0f379dca28b5f` |
| Resource assembler | `8ff1bce531c8904ef7acc030e25e9c97b8cf7c647cd67d3ce37e756f2268e6c4`; identified working-tree tool delta, separate from product source |

Producer [SDK/source-asset acceptance](https://github.com/webos21/crt/blob/main/docs/acceptance/sdk_split_acceptance.md#linux-dev-preview-candidate-rehearsal-2026-11-10)
owns build, clean compile provenance, archive/contract/source checks and the
retained failed inputs. At this local checkpoint the producer fixes were local commits. The later
public promotion below records their published source and source tag.

### Public handoff and actual package

Raw artifacts live outside both repositories at
`/home/syszone/GitLocal/crt-preview-20261110/`. `handoff-r4/release` is an
unchanged copy of the actual `crt-dev-release` output. All four declared asset
sizes/digests match; the actual archive is extracted to `handoff-r4/sdk/dev`.
The CLI receives only that public SDK, its separate portable policy, declared
LLVM binaries and copied host tools. It never reads producer-private outputs.
Fresh CLI export/output is under `cli-work-r4/d3-linux-koqzbb0q/`.

`candidate-r4/crt-cli-linux-x86_64` contains one executable and regular-file
notices/provenance, including the CLI's Apache-2.0 license and xz/liblzma records.
The immutable format-1 inventory pins every file. No SDK libraries/headers,
compiler, interpreter, permanent launcher, CA certificate or test key is packaged.
Trust mode is `explicit-at-fetch`: each fetch/acquisition selects a caller's CA
and explicit metadata pin. Test-only RSA-PSS keys/issuers qualify the implemented
explicit-key mechanism, not an official publisher. Package assembly has four
focused fixtures, including fail-closed missing-xz notices and unchanged output.

The producer's C shared SDK policy is still unqualified: `link_c` uses static
CRT libraries and `$ORIGIN` alone does not establish SDK zlib runtime search.
Its observed SDK-provider shared map remains separate from positive port-provider
C/C++ static/shared evidence. Do not advertise that SDK mode through this Preview.

### Final qualification results

| Gate | Final `.3` result |
| --- | --- |
| Fresh CLI/source-hidden/SDK-free/import/cache/unchanged-input and actual zlib cycle | **19/19** |
| Native project regression | **881/881** |
| Port provider/project regression, C/C++ static/shared | **81/81** |
| Fixed HTTPS SDK acquisition, first build/test/run, reuse and failure recovery | **73/73** |
| Bare control, host tools and pinned bootstrap resources | **30/30** |
| Real HTTPS, explicit-key signatures and failure preservation | **32/32** |
| Actual CLI archive extraction and SDK/source-free version/self-doctor | Pass; **30 files**, bytes and execute bits preserved |

The actual CLI archive is
`candidate-r4/crt-cli-0.1.0-dev-linux-x86_64-local-preview.20261110.tar.xz`
(5165244 bytes), SHA-256 `ce5aa566711858f23b8c2e169889317cb9cb00dc240ad23ec30ee877615771a3`.
Safe extraction narrows group-write permission on the executable/manifest;
content and executable permissions match, and the only executable is `crt-cli`.
The extracted package's doctor validates the pinned inventory and reports
`publisher_authentication: not_verified`, as expected without a publisher key.
Native exec traces contain no host shell/interpreter. Original/copied SDK and
bundle inventories are unchanged; the final warm CLI build executes zero commands.
The SDK-provider shared control still resolves host libz and is explicitly
unendorsed, rather than counted as positive SDK shared proof.

`qualification-r4.json` SHA-256 is `631bee1a570618a643233b1622810069e57e27b6749686a745d053ddbd59f69e`. It binds all
final result/command/input hashes, package/source pins and the identified packaging
tool patch. The actual archive and package probes use the same executable hash
as all three acquisition/environment/HTTPS drivers and the native regression.

### Reproduction and retained evidence

Use the [input qualification procedure](../guides/development.md#first-preview-input-qualification).
The base directory retains `handoff-r4.json`, `candidate-r4.json`, exact
`*-r4-invocation.json` argv, fresh source snapshots, per-command stdout/stderr,
execve traces, immutable-input inventories and `evidence/*/result.json`.
`qualification-r4.json` binds the final source, tools, assets, package and receipts.
Python/OpenSSL/bubblewrap/strace are validation-host tools; native CLI app/self-build
and control paths require no Python, shell, CMake or Ninja.

Unsuccessful inputs remain separate: the initial extra-hyphen prerelease was
rejected; the first Release core test exposed a nonvolatile ucontext resume
marker; candidate `.2` exposed the explicit-architecture C++ smoke/parser and
freestanding entry-point defects. `.2` CLI probes are preliminary evidence only.
The final `.3` source archive contains the corrected files and the exact nested
03 recipe. No previous archive, SDK, provider, store item or project pin is rewritten.
Equal build keys do not establish byte reproducibility.

### Publication boundary at local qualification

Publish the selected producer source offer and approve publisher/trust/release
metadata before public delivery. Promote SDK assets first, verify their actual
promoted bytes, then adopt the exact consumer pin and deliver the CLI. Root
`crt.lock`, local default SDK selection and committed binaries remain unchanged.
No public URL/channel, official signer or self-upgrade contract is introduced.
Every advertised host needs its final-package replay. macOS/Windows final Preview
packages, Windows/arm64 D3, Linux C shared SDK mode, Full installation, general
updates/offline resolution, D4 dependency ports and Studio remain separate.


## Public SDK promotion and consumer adoption (2026-11-10)

The producer has [published the qualified SDK](https://github.com/webos21/crt/releases/tag/v0.0.1-preview.20261110.3)
as a native Linux/x86_64 Dev prerelease, with source tag at
`0ab8f8cfbbde19e14dd5861ba176fb8881e06182`. The original `.3` manifest,
archive/content and portable-policy pins above are unchanged. The
[trust review](https://github.com/webos21/crt/blob/64dd3e6/docs/design/dev_preview_trust.md)
selects the existing caller-trusted manual pin model, explicit verified TLS/CA
and immutable-by-procedure artifacts. Publisher signatures/default channels and
server-enforced immutability are not claimed. Producer
[publication acceptance](https://github.com/webos21/crt/blob/main/docs/acceptance/sdk_split_acceptance.md#linux-dev-sdk-public-promotion-2026-11-10)
owns all six public downloads, source/tag and CA evidence.

Using exact qualified CLI `6ba046e5...1dab2`, consume the actual unauthenticated
public download directory, with local SDK/build/repository copies hidden, private
HOME/tmp, empty PATH and consumer network disabled. **12/12 checks pass**: pinned
SDK install/verify, clean producer provenance, source-only C/C++ new, first build
without configure, test/run, zero-command warm reuse, unchanged project pins,
installed SDK and all original downloads. Both generated locks select SDK
`36623957fbdd5a384cc173991e20253ff83148028ab8072298dcd434a55a2b3f`
and stored policy `90b3bb59aaac7df481becbad4e2b87473e2fefcb803123332689d7f1135e4e30`.
The correct installed-policy selector is `--sdk`; an earlier guide dry-run used
legacy `--sdk-id` and was refused for missing an explicit contract before binding.
The corrected public guide and a fresh consumer workspace pass; no product or
published artifact bytes change.

Current CLI deliberately follows no HTTPS redirects, while each public GitHub
asset uses a GitHub-storage redirect. **Direct GitHub `sdk acquire --source`
remains unsupported.** This adoption qualifies manual HTTPS download followed
by pinned local installation/installed-SDK first build. The earlier 73-case
controlled direct-HTTPS source replay is not relabeled as GitHub acquisition.
A restricted redirect strategy requires a separate CLI transport/contract review
and failure/native gates before advertising automatic public acquisition.

Raw commands/stdout/stderr and project locks are retained under
`/home/syszone/GitLocal/crt-preview-20261110/promotion-20261110/` in
`native-public-consumer-r2.json` and its private workspace. `promotion.json`
SHA-256 is `dd2ed35ce279a1d7b9117890dc1a8ab8095c2e7b425633d0b4ad35127aee055f`; it binds the public
source/assets, reviewed trust commit, CLI and consumer receipts. No root/default
SDK lock is migrated and no CLI executable release is published by this SDK
promotion. CLI delivery, other-host final packages, Linux C shared SDK mode,
Full/D4, updates and Studio remain in their owning queues.


## Restricted HTTPS redirects and GitHub acquisition (2026-11-10)

Scope: native Linux/x86_64 only (macOS/arm64 is replayed in the macOS CLI Preview release section below; Windows/x64 is not), and no
CLI executable release is published by this increment. Design and option contract:
[restricted HTTPS redirects](../reference/bootstrap_contract.md#restricted-https-redirects-experimental-cli-owned).
Default `fetch`/`sdk acquire` behavior is unchanged: redirects are refused.

| Gate | Result |
| --- | --- |
| `tools/replay_redirect_linux.py`: two loopback TLS origins serve the real promoted `.3` bytes behind redirects (allow-list, relative, 301/303/307/308, 300/304 refused, wrong authority/port, suffix look-alike, IPv6 literal, downgrade, credentials, fragment, missing/duplicate Location, loop, hop limits 3 and 5, wrong-certificate target, 404, oversize, interrupted body, digest/size mismatch, second hop leaving the list, argument and source-field refusals, full `sdk acquire` + `sdk verify` + idempotent re-acquire) | **54/54** |
| No `Authorization`/`Cookie`/`Referer` at either origin; the signed query reaches the storage origin only and appears in no CLI output | Pass |
| Native project regression with 10 added redirect-argument refusals (`native` 901, `archive` 221, `sdk-store` 114, `port-recipe` 141) | Pass |
| Real GitHub: `sdk acquire` from `webos21/crt` release `v0.0.1-preview.20261110.3` with `redirect_policy: github-release`, the host's system CA bundle (SHA-256 `a9bdea6e...5fdd`) and the pinned source; the three downloads (manifest 1683 B, policy 402958 B, archive 7697544 B) each followed one redirect to `release-assets.githubusercontent.com` and matched the pinned SHA-256/size | Pass; installed identity `36623957...2b3f` equals the promoted SDK |

The real-network run is a single observation of GitHub's current delivery host, not a promise of
its stability: a changed host fails closed as `redirect` until the reviewed list is updated. The
replay's CA is a private test CA; the system CA bundle is the caller's explicit trust input and
publisher authentication remains `not_verified`. The exercised CLI binary is a development build of
the working tree (not the earlier qualified `6ba046e5...1dab2`), so that candidate's evidence is not
carried over and no combination is labeled compatible beyond the cases above. Evidence:
`/home/syszone/redirect-replay-1/redirect-replay.json` (per-case commands and replies).

## macOS local candidate (2026-11-10)

Scope: native macOS/arm64 Dev `02-cxx` Preview candidate `v0.0.1-preview.20261110.macos.1` from clean CRT
`445f127dc2e1ef4ab538166aa2e374f25773a803` (Release; producer
[candidate record](https://github.com/webos21/crt/blob/main/docs/acceptance/sdk_split_acceptance.md#macos-dev-preview-candidate-2026-11-10)
owns the SDK/manifest/policy hashes). The CLI is built from `1e821d1b8665eb44f4d34b9582049c09a59fd3a6` plus one relocation: the private
`src/main/store_lock.h` (introduced by D3) moves to `include/crtcli/store_lock.h`, because the repository rule is that headers live under
`include/`. No behavior change; the Linux and Windows records stay at their own sources.

| Gate | Result |
| --- | --- |
| Self-build from the Preview SDK alone (Seatbelt contract replay, previous binary as bootstrap seed) | 21 sources: cold 21 executed / 0 reused, warm 0 / 21; bootstrap loop code and data identical |
| Native / archive / SDK-store / recipe regression | **922** / **221** / **114** / **141** checks |
| Imports | only `libSystem` |
| Executable | `558c13c15ffdf0394fb66e85d3a11c4f9c21c6606aae0e3f39aade353e3c7e53`, 4368360 bytes (committed as `bin/macos/crt-cli`) |
| Consumer of the local handoff, `tools/replay_preview_public_macos.py` (Seatbelt) | **30/30**: pin and asset digests, pinned install/verify/idempotence, wrong-pin refusal, C and C++ first build without configure, test, run, zero-command warm build, unchanged locks and inputs |

The producer replay driver's two guard corrections (documented `_WIN32` in a JSON fixture; the private header location) and the retained
failed attempts are recorded in the producer candidate section. Not covered at this checkpoint: a packaged CLI archive with pinned notices for macOS
(the Linux package assembly is not replayed), Developer ID signing or notarization, port/provider replays against this exact SDK, other hosts. The
executable is a local, unpromoted test aid, not a published release.

## macOS Public SDK promotion and consumer adoption (2026-11-10)

The producer has [published the qualified SDK](https://github.com/webos21/crt/releases/tag/v0.0.1-preview.20261110.macos.1) as a native macOS/arm64 Dev
prerelease with source tag at `445f127dc2e1ef4ab538166aa2e374f25773a803`; the manifest pin
`0981110c84e5d24a903dbd3965dbe92eb2fe405edfa89804e3086dfeb75983de`, SDK content `e3dc7fb1810a3f78ab8ef33882225ccb562d21f6f5977a2037ab92819326bac8`
and portable-policy digest `d5a100fd...e464` are unchanged from the local candidate above. The
[trust review](https://github.com/webos21/crt/blob/main/docs/design/dev_preview_trust.md#macosarm64-bounded-policy-adopted-2026-11-10) reuses the caller-trusted manual
pin model; no publisher signature, default channel or server-enforced immutability is claimed. Producer
[promotion acceptance](https://github.com/webos21/crt/blob/main/docs/acceptance/sdk_split_acceptance.md#macos-dev-sdk-public-promotion-2026-11-10) owns the six public downloads.

Using the committed `bin/macos/crt-cli` (`558c13c1...e53`), `tools/replay_preview_public_macos.py` consumes the actual unauthenticated public download directory under
Seatbelt (no network, scratch HOME and private temporary directory the only writable places, repositories and worktree unreadable, empty `PATH`, Xcode's clang/clang++/ld/ar).
**30/30 checks pass**: pin and asset digests, pinned install/verify and idempotent reinstall, refusal of a wrong pin with nothing published, C and C++ source-only `new`, first
build without configure through `--sdk`, test, run, zero-command warm reuse, unchanged project locks, installed SDK and downloads. Result SHA-256
`8eaee9b5b76659ff0f6fd208d2effa600856cd2c256d1e8b2f327746e7c40f13`.

As on Linux, the current CLI follows no HTTPS redirects and each GitHub asset URL redirects to GitHub storage, so **direct GitHub `sdk acquire --source` remains unsupported**; the
accepted path is manual download plus pinned local installation. No root `crt.lock`, default SDK selection or CLI release is changed; the committed macOS binary stays a
local, unpromoted test aid. Other-host final packages, a packaged macOS CLI archive, Developer ID signing, Full and D4 remain in their own queues.

## macOS final CLI package (2026-11-10)

Scope: the local Preview archive of the committed macOS/arm64 executable with its pinned notices, provenance records and an immutable format-1
inventory. This is the macOS counterpart of the Linux package qualification; it publishes nothing and creates no release, tag or default lock.

**Inputs.** Executable `bin/macos/crt-cli` (`558c13c15ffdf0394fb66e85d3a11c4f9c21c6606aae0e3f39aade353e3c7e53`), assembled by
`tools/create_bootstrap_resources.py` against the **actual public SDK bytes** of `v0.0.1-preview.20261110.macos.1`: the archive was downloaded without
authentication, verified against the manifest pin and installed by this CLI (`sdk install`), and the notices were copied from that installed SDK
(content `e3dc7fb1810a3f78ab8ef33882225ccb562d21f6f5977a2037ab92819326bac8`). The assembler fails closed on a missing component record.

**Package.** One executable and 29 regular files, 30 in all: `LICENSE.md`, the CLI's Apache-2.0 license, 18 component license files
(`crt-libc` 5, `llvm-runtimes` 8, `mbedtls` 2, `curl`, `zlib`, `xz`, `nlohmann-json` 1 each), the 7 matching dependency provenance records, and `manifest.json`
(`crt-cli.bootstrap-resources` format 1, target macOS/aarch64, 29 inventory entries with SHA-256, trust mode `explicit-at-fetch` and no CA file, the external prerequisite
`libSystem.B.dylib`). Manifest SHA-256 `9449325ea9ef5dfcd454cf705878759dfe405b71a7641baa9b66d999c198e0be`. No SDK library or header, compiler,
interpreter, launcher, CA certificate or test key is included.

**Archive.** `tools/package_cli_archive.py pack` writes `crt-cli-0.1.0-dev-macos-aarch64-local-preview.20261110.tar.xz` (1222100 bytes), SHA-256
`f1a20b50210ee4b58fd5365bdb5d003167d6e232ad341ff4dc5f75aba1aaa876`, with one top-level directory, sorted members, owner 0, mtime 0 and modes 0755 for
directories and the executable, 0644 for the rest. Packing the same package twice gives the same bytes. The tool first inspects the package against its own manifest
and refuses extra, missing or changed files, a second executable, links, special files and a missing mandatory notice or component license (4 unit tests, including
hostile archive members).

**Extraction and verification.** `package_cli_archive.py verify` extracts into a new directory with the stdlib `data` filter and re-checks the manifest: **30 files**, every
digest equal to the inventory, the only executable is `crt-cli`, no link or special file, the same manifest digest. The extracted executable is byte-identical to
`bin/macos/crt-cli`; `otool -L` lists only `/usr/lib/libSystem.B.dylib`, there is no `LC_RPATH`, and the signature is the linker's ad-hoc one
(`Identifier=crt-cli`, `Signature=adhoc`, no team identifier).

**Run from the extracted package, SDK- and source-free (Seatbelt, 9/9).** The package is copied to a fresh directory; the producer, CLI and worktree checkouts, Xcode and
`/Library/Developer` are unreadable, the network is denied, `PATH` is empty, and only the package executable may run. `--version` (`0.1.0-dev`), `--help`, bare
`doctor --scope self` and `doctor --scope self --resources <package> --resources-sha256 <pin>` pass; the last reports `publisher_authentication: not_verified`, trust
`explicit-at-fetch` and the compiled-in SDK input (`v0.0.1-preview.20261110.macos.1`, content `e3dc7fb1...bac8`, source `445f127`). Negative controls fail for their own
reasons: a wrong pin ("manifest SHA-256 mismatch"), one appended byte in a notice and an undeclared extra file (both "resource/executable inventory mismatch"); the restored
package passes again, and a foreign program and the producer tree are denied. An earlier attempt of this run used an unresolved `/tmp` path in the profile, so the executable
itself was blocked and three negative controls "passed" for the wrong reason; it is discarded and the run above is the one recorded.

Receipts are under the CRT checkout's ignored `out/preview-macos/cli-package/` (`pack.json`, `verify.json`, `self-doctor/` with `summary.json`). Not covered: publishing this
archive or any CLI release, Developer ID signing or notarization, an official publisher key (the manifest is a local inventory, not publisher authentication), port/provider
replays against this exact SDK, and other hosts.

## Windows local candidate (2026-11-10)

Scope: native Windows/x86_64 Dev `02-cxx` Preview candidate `v0.0.1-preview.20261110.windows.1` from clean CRT `b31276a6acccf0a89731f7bb0aecbdcdc9b45cda` (Release;
producer [candidate record](https://github.com/webos21/crt/blob/main/docs/acceptance/sdk_split_acceptance.md#windows-dev-preview-candidate-2026-11-10) owns the SDK/manifest/policy
hashes). The CLI is built from `9a5272d12d80a74389edbc724fd4bd602d618064` unchanged; it already contains the Windows D3 adapter and the Win64 vector-register unwind fix lives in the SDK. The Linux and
macOS records stay at their own sources.

| Gate | Result |
| --- | --- |
| Self-build from the Preview SDK alone (restricted-token contract replay, private SID, empty PATH, job-object accounting) | 21 sources: cold 21 executed / 0 reused, warm 0 / 21 |
| Native / archive / SDK-store / recipe regression | **972** / **214** / **113** / **141** checks |
| Imports | only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`; SDK-hidden help/version/doctor pass |
| Executable | `3e8c0491d2e385bbaba5c0e8bd63eeac3896f498fed762ecab4370ecf277bd90`, 44920832 bytes (committed as `bin/win-x64/crt-cli.exe`) |
| Consumer of the local handoff, `tools/replay_preview_public_windows.py` (restricted token) | **31/31**: pin and asset digests, pinned install/verify/idempotence, wrong-pin refusal, C and C++ first build without configure, test, run, zero-command warm build, unchanged locks and inputs |

The consumer driver is a Windows adaptation of the macOS one: each call runs under CRT's `crt_windows_trace` (per-run SID, empty PATH, job-object process accounting; no network or
executable denial is claimed), a native probe proves the producer and CLI trees unreadable, and the Windows SDK import-library directory is passed through `CRT_WINDOWS_SDK_LIBPATH`.
The guide's literal commands (`--os-sdk`, spaced tool paths) were also run once as a user would and pass. Not covered at this checkpoint: a packaged CLI archive with pinned notices
for Windows, Authenticode signing, Windows/arm64, port/provider replays against this exact SDK, other hosts. The executable is a local, unpromoted test aid, not a published release.

## Windows Public SDK promotion and consumer adoption (2026-11-10)

The producer has [published the qualified SDK](https://github.com/webos21/crt/releases/tag/v0.0.1-preview.20261110.windows.1) as a native Windows/x86_64 Dev prerelease with source tag at
`b31276a6acccf0a89731f7bb0aecbdcdc9b45cda`; the manifest pin `b0fca4c81d29b8eee28e0d9fb7d6e26950b094a8be4f2c091e43922366a46976`, SDK content `5bb55372ec10e3cfa24c84de981a7130f56aeb57ef0d43d5da22a65036dec7ef` and
portable-policy digest `7762f572...b726` are unchanged from the local candidate above. The
[trust review](https://github.com/webos21/crt/blob/main/docs/design/dev_preview_trust.md#windowsx86_64-bounded-policy-adopted-2026-11-10) reuses the caller-trusted manual pin model; no publisher
signature, default channel or server-enforced immutability is claimed. Producer
[promotion acceptance](https://github.com/webos21/crt/blob/main/docs/acceptance/sdk_split_acceptance.md#windows-dev-sdk-public-promotion-2026-11-10) owns the six public downloads.

Using the committed `bin/win-x64/crt-cli.exe` (`3e8c0491...bd90`), `tools/replay_preview_public_windows.py` consumes the actual unauthenticated public download directory under a restricted
token (per-run SID, producer and repositories unreadable by a native probe, private HOME and temporary directory, empty `PATH`, job-object process accounting, LLVM 22.1.8 and the Windows SDK
import libraries). **31/31 checks pass**: pin and asset digests, pinned install/verify and idempotent reinstall, refusal of a wrong pin with nothing published, C and C++ source-only `new`, first
build without configure through `--sdk`, test, run, zero-command warm reuse, unchanged project locks, installed SDK and downloads. Result SHA-256
`5784f28a815be4edb4a881fdabfff63d37885441ba802487d7398f47c6acfd20`.

As on Linux and macOS, the current CLI follows no HTTPS redirects and each GitHub asset URL redirects to GitHub storage, so **direct GitHub `sdk acquire --source` remains unsupported**; the accepted
path is manual download plus pinned local installation. No root `crt.lock`, default SDK selection or CLI release is changed; the committed Windows binary stays a local, unpromoted test aid.
A packaged Windows CLI archive with pinned notices, Authenticode signing, Windows/arm64, Full and D4 remain in their own queues.

## Windows final CLI package (2026-11-10)

Scope: the local Preview archive of the committed Windows/x86_64 executable with its pinned notices, provenance records and an immutable format-1 inventory. This is the Windows
counterpart of the Linux and macOS package qualification; it publishes nothing and creates no release, tag or default lock.

**Inputs.** Executable `bin/win-x64/crt-cli.exe` (`3e8c0491d2e385bbaba5c0e8bd63eeac3896f498fed762ecab4370ecf277bd90`), assembled by `tools/create_bootstrap_resources.py` against the
**actual public SDK bytes** of `v0.0.1-preview.20261110.windows.1`: the ZIP was downloaded without authentication, verified against the manifest pin and installed by this CLI (`sdk install`), and the
notices were copied from that installed SDK (content `5bb55372ec10e3cfa24c84de981a7130f56aeb57ef0d43d5da22a65036dec7ef`). The assembler fails closed on a missing component record.

**Package.** One executable (`crt-cli.exe`) and 28 regular files plus `manifest.json`, 30 in all: `LICENSE.md`, the CLI's Apache-2.0 license, 19 component license files (`crt-libc` 5, `llvm-runtimes` 8, `mbedtls` 2,
`curl`, `zlib`, `xz`, `nlohmann-json` 1 each), the 7 matching dependency provenance records, and `manifest.json` (`crt-cli.bootstrap-resources` format 1, target Windows/x86_64, 29 inventory entries with SHA-256,
trust mode `explicit-at-fetch` and no CA file, the external prerequisite `windows-system-runtime`: `KERNEL32.dll` and `api-ms-win-core-synch-l1-2-0.dll`, not bundled). Manifest SHA-256
`a99fee801a30c14884d108f4675a76c1643e3adaf83d83965e05a25f9344ab85`. No SDK library or header, compiler, interpreter, launcher, CA certificate, test key or second PE image is included.

**Archive.** `tools/package_cli_archive.py pack` writes `crt-cli-0.1.0-dev-windows-x86_64-local-preview.20261110.tar.xz` (5360820 bytes), SHA-256
`5c9ef4042bb15c999a44c6ae745bf3f6efa25093b509bbba7c4c3164a18b4edf`, with one top-level directory, sorted members, owner 0, mtime 0 and modes 0755 for directories and the executable, 0644 for the rest.
Packing the same package twice gives the same bytes. The archive is a `.tar.xz` like the Linux and macOS ones (Windows 10 and later extract it with the bundled `tar.exe`); no ZIP variant is made. The
tool first inspects the package against its own manifest and refuses extra, missing or changed files, a second executable, links, special files and a missing mandatory notice or component license.

**Tool change for Windows.** The earlier tool decided "the only executable" and the archive modes from the host's execute bits, which do not exist on Windows (every file reports executable). It now also
classifies files by content (PE with a valid signature at `e_lfanew`, ELF, Mach-O) and refuses any native image other than the declared executable on every host, sets the archive modes from the manifest's
executable name, and keeps the POSIX execute-bit check on POSIX hosts. Six unit tests pass on Windows, including a second PE/ELF/Mach-O image hidden in a notice with mode 0644 and a text file that merely starts with
`MZ`; the same tests were not run on Linux or macOS, where the unchanged POSIX branch is reasoned to behave as before and the earlier archives are not re-made.

**Extraction and verification.** `package_cli_archive.py verify` extracts into a new directory with the stdlib `data` filter and re-checks the manifest: **30 files**, every digest equal to the inventory, the
only native image is `crt-cli.exe`, no link or special file, the same manifest digest. The extracted executable is byte-identical to `bin/win-x64/crt-cli.exe`; its imports are only `KERNEL32.dll` and
`api-ms-win-core-synch-l1-2-0.dll`, and its certificate table is empty (not Authenticode-signed).

**Run from the extracted package, SDK- and source-free (restricted token, 12/12).** `tools/replay_cli_package_windows.py` copies the package to a fresh private directory. Each call runs under CRT's
`crt_windows_trace`: a per-run SID, an empty `PATH`, redirected stdio and job-object process accounting; a native probe proves the producer, CRT, CLI and installed-SDK trees unreadable, and no call starts any image other
than the package executable and the console host. `--version` (`0.1.0-dev`), `--help`, bare `doctor --scope self` and `doctor --scope self --resources <package> --resources-sha256 <pin>` pass; the last reports
`publisher_authentication: not_verified`, trust `explicit-at-fetch` and the compiled-in SDK input (`v0.0.1-preview.20261110.windows.1`). Negative controls fail for their own reasons: a wrong pin ("manifest SHA-256 mismatch"),
one appended byte in a notice and an undeclared extra file (both "inventory mismatch"); the restored package passes again and is byte-for-byte unchanged. Result SHA-256
`69362643032433af4963426f2833d2142e4b609b666bb04c11899870d703e733`. Unlike the macOS Seatbelt run, Windows records process images but cannot deny a foreign program or the network; that is a stated limit, not a pass.

Receipts are in the local evidence directory (`pack.json`, `verify.json`, `package-run/`). Not covered: publishing this archive or any CLI release, Authenticode signing, an official publisher key (the manifest is a local
inventory, not publisher authentication), port/provider replays against this exact SDK, Windows/arm64 and other hosts.


## Linux/x86_64 CLI Preview release (2026-11-10)

Published prerelease [`v0.1.0-preview.20261110.linux-x86_64.1`](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261110.linux-x86_64.1)
(tag target `dc796f738e96f2b75d2d9b204751742d7647be0b`; tags carry the OS-architecture). Assets: `crt-cli-0.1.0-dev-linux-x86_64-preview.20261110.1.tar.xz`
(5232844 bytes, SHA-256 `ad43afbf7e810bb7b1a587c47568ba6d17bc404e5ee05069a83d9ef56edc1383`) and `SHA256SUMS-linux-x86_64`. Public download re-verified
against the sums; the executable still reports `0.1.0-dev`.

**Inputs.** Fresh `git archive` of `dc796f7` (root `crt.lock` restored to the committed unresolved state), built by this CLI against the public SDK
`v0.0.1-preview.20261110.3` (content `36623957...2b3f`) that was itself acquired from GitHub through the new redirect policy, LLVM 21 `/usr/bin` tools.
Executable SHA-256 `a8fb76a4b257c52345d79560312e0a94f52aaa9251f67524823a0fab8476a43a` (23795192 bytes, statically linked). Assembled by
`create_bootstrap_resources.py` (manifest SHA-256 `a5c658d3...c7a`, 30 files, no CA file) and packed by `package_cli_archive.py` (deterministic: packing twice is
byte-identical; `verify` extraction passes).

**Packaged-executable run** (extracted archive, empty `PATH`, fresh HOME, real network): `--version`, self `doctor` with the manifest pin, `sdk acquire` from GitHub with
`redirect_policy: github-release` (three redirects, installed identity equals the promoted SDK), then C and C++ `new`/`build`/`test`/`run`. Not run for this exact
binary: the full native suite (901 checks were run on the equivalent development-tree build `dc796f7` source before packaging), an SDK-hidden/sandboxed receipt
and a source-port build. Work and receipts: `/home/syszone/GitLocal/crt-cli-preview-linux-20261110/`.

Not claimed: publisher authentication (digests only), macOS/Windows/aarch64 packages and redirect replays, Linux C shared-SDK mode, updates/self-upgrade.

## macOS CLI Preview release (2026-11-10)

Prerelease [`v0.1.0-preview.20261110.macos-arm64.1`](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261110.macos-arm64.1) of `webos21/crt-cli` (tags carry the
OS-architecture, as for Linux), staged as a draft, verified, then published as a non-latest prerelease. Tag target: `541b2afd53980654591a1c6e0a49ceca4057f9b7`, the clean commit the
executable was built from (it adds only the generalized redirect-replay manifest glob to the Linux Preview source `dc796f7`). Assets: `crt-cli-0.1.0-dev-macos-aarch64-preview.20261110.1.tar.xz`
(1229408 bytes, SHA-256 `2767f18da7e7d51e3e3fc1172da99581bf0508c1f6b6794817488c0c58950011`) and `SHA256SUMS-macos-aarch64`. The notes were frozen from the files named here before publication.

**Inputs.** A fresh Seatbelt contract replay builds `541b2af` from its sources with the public SDK `v0.0.1-preview.20261110.macos.1` alone (content
`e3dc7fb1810a3f78ab8ef33882225ccb562d21f6f5977a2037ab92819326bac8`, installed from the downloaded public bytes), the previously committed macOS executable being the bootstrap seed:
21 sources, cold 21 executed / 0 reused, warm 0 / 21, code and data identical between the two bootstrap stages, native **942**, archive **221**, SDK-store **114** and recipe **141**
checks, only `libSystem` imported. Executable `42727ccaef0e46551bcb9578d5eee771166e8211b8434f8121b589ab57eddfa6` (4389112 bytes). `create_bootstrap_resources.py` assembles the notices from that SDK
(manifest SHA-256 `6c7f553f512dbf170936638746d515bfd7ddb006229244b924f4786593c01e63`, 30 files, no CA file) and `package_cli_archive.py` packs them deterministically (packing twice is
byte-identical; `verify` extraction passes; the extracted executable is byte-identical to the built one; no `LC_RPATH`; ad-hoc linker signature only).

**Restricted redirects on macOS.** `tools/replay_redirect_linux.py` (now taking any host's Dev release manifest; two loopback TLS origins serve the real macOS SDK bytes behind redirects, with
OpenSSL 4.1 from Homebrew as the test PKI) passes **54/54**, the same cases as Linux: allow-list, relative and 301/303/307/308 redirects, 300/304 refused, wrong authority or port, suffix look-alike,
IPv6 literal, downgrade, credentials, fragment, missing or duplicate `Location`, loops and hop limits, wrong certificate, 404, oversize, interrupted body, digest and size mismatch, argument and
source-field refusals, and full `sdk acquire` + `sdk verify` + idempotent re-acquire. No `Authorization`, `Cookie` or `Referer` reaches either origin and the signed query appears in no output.

**Real GitHub, from the packaged executable.** The extracted archive runs under Seatbelt (the producer and CLI checkouts unreadable, empty `PATH`, fresh HOME, real network; only the package
executable, `xcrun` and Xcode may run): `--version`, self `doctor --resources` with the manifest pin, refusal of a wrong pin, then `sdk acquire` with this source file, whose values are the
promoted SDK's pins (`ca_file` is a PEM bundle of the caller's choice; here the system roots exported with `security find-certificate -a -p /System/Library/Keychains/SystemRootCertificates.keychain`,
221 KB, SHA-256 `367b835d35bc36183ddc5d1e4e85170c8a0da0b2abcb50498df791fa1f614750` on this machine; compute your own):

```json
{"schema": "crt-cli.sdk-source", "format": 0, "ca_file": "/absolute/roots.pem", "ca_sha256": "<sha256 of that file>",
 "manifest": {"url": "https://github.com/webos21/crt/releases/download/v0.0.1-preview.20261110.macos.1/release-manifest-dev-macos-aarch64.json",
              "sha256": "0981110c84e5d24a903dbd3965dbe92eb2fe405edfa89804e3086dfeb75983de", "size": 1728},
 "archive": {"name": "crt-v0.0.1-preview.20261110.macos.1-macos-aarch64-dev.tar.xz",
             "url": "https://github.com/webos21/crt/releases/download/v0.0.1-preview.20261110.macos.1/crt-v0.0.1-preview.20261110.macos.1-macos-aarch64-dev.tar.xz"},
 "contract_url": "https://github.com/webos21/crt/releases/download/v0.0.1-preview.20261110.macos.1/crt-v0.0.1-preview.20261110.macos.1-macos-aarch64-dev-toolchain.json",
 "sdk": {"version": "v0.0.1-preview.20261110.macos.1", "content_sha256": "e3dc7fb1810a3f78ab8ef33882225ccb562d21f6f5977a2037ab92819326bac8",
         "contract_sha256": "d5a100fdd0d38f940fe1c398996f013d57ccc7c4743af2b9ab4988c42b8de464"},
 "redirect_policy": "github-release", "timeout_seconds": 300}
```

All three downloads (manifest 1728 B, policy 402864 B, archive 6136776 B) each followed one redirect to `release-assets.githubusercontent.com` and matched the pinned SHA-256 and size; the installed
identity equals the promoted SDK. Then C and C++ `new`, first `build --sdk <identity>` without configure, `test`, `run` and a zero-command warm rebuild all pass: **18/18** checks in the run
(an earlier attempt of the same run, retained as `package-run-r1-xcrun-denied`, failed its eight build steps because the test profile itself denied `xcrun`; it is not counted). This is one observation of GitHub's
current delivery host, not a promise of its stability: a changed host fails closed as `redirect` until the reviewed list is updated. Publisher authentication stays `not_verified`.

Receipts are under the CRT checkout's ignored `out/cli-preview-macos/` (`replay-cli/`, `redirect-replay/`, `acquire.json`, `package/`, `package-run/`, `release/`). Not claimed: publisher authentication (digests
only), Developer ID signing or notarization, a quarantined (browser) download, Intel macOS, a source-port build from this package, updates or self-upgrade. 

**Published.** The draft carried the two assets; before publishing, their names, sizes and digests matched the pins and an authenticated re-download was byte-identical to the local files. The
prerelease was then published (`2026-11-10T04:20:48Z`, not marked latest); the tag resolves to `541b2afd53980654591a1c6e0a49ceca4057f9b7`, and the published notes are identical to the frozen file. An
**unauthenticated** public download (`curl`, HTTPS only, TLS >= 1.2, proxies disabled) followed one redirect per file to `release-assets.githubusercontent.com` and matched `SHA256SUMS-macos-aarch64`
and the local archive byte for byte. The archive extracted from those public bytes (`package_cli_archive.py verify`: 30 files, manifest `6c7f553f...c01e63`) runs under Seatbelt with `--version`, self
`doctor --resources` against the pin (`publisher_authentication: not_verified`) and a real GitHub `sdk acquire` (three redirects, installed identity `e3dc7fb1...bac8`). The committed
`bin/macos/crt-cli` is now this executable (`42727cca...a6`). Executable publication does not change root locks, defaults or STATUS.

## Windows/x64 CLI Preview release (2026-11-10)

Published prerelease [`v0.1.0-preview.20261110.windows-x86_64.1`](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261110.windows-x86_64.1), not latest, at `2026-11-10T05:12:57Z`.
Tag target and clean product/test source: `a55c0d0ea0801f0e6e8d235f20ca780f876bf970`.
It contains the common restricted redirect implementation already exercised on Linux/macOS;
no Windows product-code or producer-runtime change was required. The test-host driver adds
`.exe`, Windows HOME/SystemRoot/temp handling, hidden subprocesses, UTF-8 output and an
explicit test OpenSSL config. The earlier Windows manual-install/local-package records retain
their own binaries and limits; this section supersedes their not-yet-replayed/not-published boundary.
Product/test/manifest inputs are unchanged from Linux release `dc796f7` and macOS release
`541b2af`; `source-equivalence.json` binds those comparisons. Their native records are retained.

### Inputs, build and package

Reviewed producer `8e89ee1` and reused the public clean Release Dev SDK
`v0.0.1-preview.20261110.windows.1` from CRT `b31276a6acccf0a89731f7bb0aecbdcdc9b45cda`.
SDK content `5bb55372ec10e3cfa24c84de981a7130f56aeb57ef0d43d5da22a65036dec7ef`,
policy `7762f572b4614b3068e1ea99694ce19360e213d31c5bd01019e4ad6ce005b726`,
manifest pin `b0fca4c81d29b8eee28e0d9fb7d6e26950b094a8be4f2c091e43922366a46976` (1800 B)
and archive `c71bb91e9f2c623d44e8154c7798a446b850781bdc1cf515eb9675dc04137be1` (73323880 B)
are unchanged. LLVM 22.1.8 and Windows SDK `10.0.28000.0/um/x64` are explicit host inputs.
Native host: Windows 11 Pro x64, build 26100.
The contract replay exports a fresh bootstrap compiler policy from the public SDK
(SHA-256 `8b9eaf7efc6d8b9154400bd46911e0eb7ad2563e947190576d488c00c4b42250`);
its full copied-tree inventory is `a756b5203dc74447a3496ea0b65e036d6e235afe27963adca5fd67de64266d3e`.
These bind the replay workspace separately from the published portable policy above,
which the actual installed-SDK app consumers use. Both policies name the same `5bb55372...c7ef` SDK content.

Fresh restricted-token contract replay exports into `C:/Users/Lee/s11/w2`, builds the
21-source CLI against that SDK, forces a complete self-build and checks a zero-command warm
rebuild. All 46 product/test/manifest files are byte-equal to the clean tag source; the exported
test-host consumer driver also contains its recorded uncommitted replay-only extension.
The final executable is 45,508,096 bytes, SHA-256 `8a2e854ec33d9e78c4f8c72ed64e928f036a34af62bf31a66016c6ab982cd39b`.
Only `KERNEL32.dll` and `api-ms-win-core-synch-l1-2-0.dll` are direct imports;
the PE certificate table is empty (no Authenticode signature).

`create_bootstrap_resources.py` takes notices from the installed public SDK; the package holds
one executable, licenses/provenance and format-1 inventory: 30 files. No SDK/compiler/interpreter,
launcher, CA or key is bundled. `manifest.json` SHA-256: `7381cd58919be1f1d007cf0091c17e33236797860a8a2087b1139a7e80e6676c`.
`package_cli_archive.py` packs `crt-cli-0.1.0-dev-windows-x86_64-preview.20261110.1.tar.xz` (5,430,536 bytes), SHA-256
`4bafc8464b729049840f1f21807f505bbcc09e7d5f7bc260e63d8f971ade75c0`. Repacking is byte-identical and safe extraction verifies all 30 files.
`SHA256SUMS-windows-x86_64` is the second public asset (ASCII/LF). The committed
`bin/win-x64/crt-cli.exe` is this exact packaged executable; its predecessor is retained in ignored output.

### Results and actual GitHub download

| Gate | Result |
| --- | --- |
| Fresh native / archive / SDK-store / recipe | 992 / 214 / 113 / 141 checks passed |
| Cold / warm self-build | 21 executed, 0 reused / 0 executed, 21 reused |
| Restricted redirect replay over actual Windows SDK bytes | 54/54 |
| SDK- and source-free extracted package, negative inventory/pin controls | 12/12 |
| Packaged GitHub acquisition + C/C++ first build/test/run/cache/lock workflow | 39/39 |
| Public-byte package acquisition smoke (apps reused by exact archive/binary identity) | 23/23 |
| Bootstrap notice assembler / archive tool unit tests | 4 / 6 passed |

The aggregate's five input-gated D3 suites compile and print `skipped` without port inputs;
those exits are not source-port acceptance. Source-port execution from this exact package
is outside this transport/Preview increment.

The same 54 TLS/redirect cases as Linux/macOS include authority/port/suffix/IPv6 refusal,
HTTPS downgrade/credentials/fragment, duplicate/missing Location, loops/hop limits, wrong
certificates, HTTP status, oversize/interruption, digest/size failures, source-field refusals,
full acquisition/verify/reuse, header non-forwarding and signed-query non-disclosure.
The loopback test is a normal test-host process with an empty child PATH, not an OS sandbox.

`replay_preview_public_windows.py --sdk-source ... --package ...` runs every CLI invocation
under a per-workspace restricted token: empty PATH, private HOME/temp, native visibility
probe and job-object process accounting. Repositories, producer, prior SDK and build source
are unreadable; the package and explicitly declared LLVM/OS SDK remain available. The caller
CA is a read-only Windows ROOT export (57 certificates, 86146 B, SHA-256 `323781b9b7dba15d5772a72a8c75f5a87d73fce866c151add449762150086899`), copied
to the private workspace. It is neither an SDK default nor a bundled release trust anchor.
Each real GitHub download (manifest 1800 B, policy 440297 B, archive 73323880 B) follows one
redirect to `release-assets.githubusercontent.com`, passes its pin and installs the promised
SDK identity. Re-acquire downloads nothing; wrong HTTPS/local/package pins fail without
publication. C/C++ first build, test, run and warm builds retain exact SDK/project locks.
Source JSON, optional ROOT export and explicit LLVM/OS SDK commands are in
[the Windows user guide](../guides/windows_preview.md).
Its native Windows `tar` extraction and PowerShell ROOT-to-PEM example were also run:
30 files and matching executable/inventory pins, followed by a pinned GitHub manifest
fetch using that exported PEM (one allowed redirect).

### Publication, receipts and limits

The two assets were staged as a draft, checked against GitHub's asset sizes/digests and
authenticated re-downloads, then published as a non-latest prerelease. Final notes were
frozen before publication; the public notes match the frozen file. Unauthenticated HTTPS
downloads (Windows system trust, TLS >= 1.2, proxies disabled, one allowed GitHub-storage
redirect per asset) match both local bytes and checksum pins. Public archive extraction and
the restricted-token acquisition smoke pass on those bytes. No root/default SDK or port lock
is migrated; producer SDK bytes, API/ABI and all shared formats are unchanged.

The first contract attempt omitted `CRT_WINDOWS_SDK_LIBPATH` and stopped at provider export;
it is retained as `s11/contract`, while `contract-r2` uses the explicit directory and a new
workspace. Initial loopback PKI generation used Strawberry OpenSSL's nonexistent compiled-in
`Z:/.../openssl.cnf`; `redirect-replay` is retained and `redirect-replay-r2` uses a minimal
per-test config. No global OpenSSL setting, account ACL or system trust was changed.
The final receipts live under `C:/Users/Lee/s11`; a qualification/bound receipt copy and the
old binary are in ignored `out/windows-cli-preview-20261110/`:

- `contract-r2/inputs.json`: `63e8228c577192a4323e50e8f5b8f0db402323b731bc188874bc2a0574477ad3`.
- `contract-r2/commands.json`: `941eae5c98c728f4dd8fdde89c7c38dfdb4ee3418584dc4a3b953049368f2fa3`.
- `contract-r2/result.json`: `e2e27a0681c1df549616fd23ae3946775ca84d59e33e6a960d89f892bf65d8c5`.
- `package-result.json`: `8c517c98dfedc21ec900eddd21ccdc10f3f8754a5cf0d4a3aa2d9b1b48458eb1`.
- `redirect-replay-r2/redirect-replay.json`: `cdf345f1f3048bed4831391e58644f8d679a1604c8ff00b1ce43b0df594f85ab`.
- `package-self/result.json`: `bbea6e0619b508afc316ce1591b44e365d98164ebd90ee4ba91aae6d0441d33c`.
- `github-consumer/result.json`: `33a308f58ebd52c6af1b9baeae9ad906f9a4a5ba1e52f5a7e46791fe9061a398`.
- `public-consumer/result.json`: `a9fdef217b9ae25d8d6da549a7d9f04f88f3c64e029ade7cb4ddec7f8a34ce35`.
- `public-download.json`: `0983bac1cca1149fcbffce46fec1f60a6d6d7d822de96bc0293ab73dba29755b`.
- `publication.json`: `3e53fe1b3553a75cb2d05bbba0c534e425a870eae08475e43053bb2211cfc344`.
- `source-equivalence.json`: `84b64e44be778be9dcde22ddcb6253eed39ba4a8fb8de868f4e5b31ee96ab18b`.
- `pe-audit.json`: `6f57168d4b8082ad7cb46f5372ca6c9b7f1c8d39769b4e647d4ad2632b05afc5`.
- `user-guide-smoke.json`: `b9ab1478bb624c0488180df3574cf45c04f47ffd2d7e629561e5ec8773661c7b`.
- `release-notes.md`: `ae1ffcd5fd756dd3a7e3923f590bc42ca008ff8440efe41d696bc0bc8c186e47`.

Limits: unsigned and no publisher authentication (`not_verified`); SmartScreen/browser
reputation was not tested. Windows accounting is not network or executable denial and does
not exclude OS-internal runtime mappings. Windows/arm64, source-port builds from this exact
package, Full profiles, updates/self-upgrade and official default trust remain separate.
Linux/macOS records retain their exact sources and bytes. STATUS is refreshed here only
because the owner explicitly requested both repositories' current snapshots.

## Linux/aarch64 public SDK promotion and consumer adoption (2026-11-10)

The producer [published](https://github.com/webos21/crt/blob/main/docs/acceptance/sdk_split_acceptance.md#linuxaarch64-dev-sdk-public-promotion-2026-11-10)
`v0.0.1-preview.20261110.linux.arm64.2` (clean CRT `e6a94ab`, manifest `ba77b26e...67be3`). `tools/replay_preview_public_linux.py` is the bubblewrap counterpart of the macOS and Windows public-byte drivers
(no network, read-only root, producer and CLI source trees hidden, cleared environment, empty `PATH`, host LLVM 21.1.8 as the only toolchain). The CLI is the executable
`ae5f2b212be4351dcb8fc699785cf5c6822aa0b0dcd5183a23e94e56529ec543` (23,162,944 bytes), produced by the producer's contract replay from `5d148ba` and the SDK under test:
21 sources, cold 21 executed / 0 reused, warm 0 / 21, native 913, archive 221, SDK-store 114 and port-recipe 142 checks, 32 distinct executed paths, no `DT_NEEDED`.

It passed **30/30** twice with identical cases: on the pre-publication handoff and on the unauthenticated public download (six files, one verified redirect each, byte-identical): pinned install and verify, idempotent
reinstall, wrong-pin refusal with nothing published, C and C++ source-only `new`, first build without configure through the installed SDK, test, run, zero-command warm build, unchanged project locks, unchanged installed SDK and downloads.
Public result SHA-256 `2c2495bba6002d7e5669de6974b2a73babe9ecd7fe1428b6d625f2213ae01974`. Receipts are under the ignored `out/linux-arm64-preview-20261110/`.

Limits: glibc 2.43 only; the executable is neither committed nor published; GitHub `sdk acquire` on this host, the older loader, Full SDKs and default/production locks are not covered.

## Linux/aarch64 CLI Preview release (2026-11-11)

Published prerelease [`v0.1.0-preview.20261111.linux-arm64.1`](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261111.linux-arm64.1)
(tag target `6a008856d4a92fafbea1221b0f6c88a4559ccc29`; tags carry the OS-architecture and assets the machine name, as for macOS), staged as a draft, verified, then published as a non-latest prerelease.
Assets: `crt-cli-0.1.0-dev-linux-aarch64-preview.20261111.1.tar.xz` (5039156 bytes, SHA-256 `fa28ec8d6644da3dc32aaa8e9281c8b953b85d97498cc3cdc12f55b156014cff`) and `SHA256SUMS-linux-aarch64`.

**Inputs.** A fresh clone checked out at the clean commit `6a00885` is built by the producer's `replay_cli_contract_linux.py` (CRT `bcce264`, clean) with the actual public SDK bytes of
`v0.0.1-preview.20261110.linux.arm64.2` alone: the three assets were downloaded unauthenticated and matched the pins (manifest `ba77b26e...67be3`, archive `5c99dc59...a1c4`, contract `a3df2fbd...34d4`), and the
archive was extracted to the handoff (content `cd2ab657...0cc6`, source `e6a94ab`). Host: Ubuntu 26.04.1, glibc 2.43, LLVM 21.1.8. 21 sources, cold 21 executed / 0 reused, warm 0 / 21, native **913** checks,
execve audit 705 calls / **32** distinct paths with none unexpected, SDK inventory unchanged, help/version/doctor with SDK, source, provider and compilers hidden. The executable is `9fbef82541e237eeba2b28729b60f55b2e301c0be1f0dd14c9b31e6eb00ae1db`
(23162152 bytes, static, no `DT_NEEDED`, interpreter `/lib/ld-linux-aarch64.so.1`); the executable the suite tested, the bare executable and the packaged one are the same bytes. It differs from the earlier consumer-adoption
executable `ae5f2b21...` because that was built from `5d148ba`. `create_bootstrap_resources.py` assembled the notices from the extracted SDK (manifest SHA-256 `f7739366a3b972aa1db0278ac0edc7e6696c914902bd34b12052fea7cfb13810`, 30 files,
no CA file) and `package_cli_archive.py` packed them deterministically (packing twice is byte-identical; `verify` passes and the extracted executable is byte-identical).

**Packaged-executable run** (extracted archive, `env -i`, empty `PATH`, fresh HOME, real network, host LLVM passed by flag): `--version`, self `doctor` with the manifest pin (`resources: pass`, compiled-in SDK input equals the promoted SDK), a wrong pin
("manifest SHA-256 mismatch") and a changed notice ("inventory mismatch") refused, then real GitHub `sdk acquire` (CA bundle `/etc/ssl/certs/ca-certificates.crt`, SHA-256 `0e4c9a8e...6b71`, one redirect per file, all three digests matched,
installed identity equals the promoted SDK). C and C++ `new`, first `build --sdk <identity>` without configure, `test`, `run`, warm rebuild and project `doctor` then pass (12/12, under a parent path with spaces and Korean characters).
Project directory names must be package identifiers; a first attempt used a non-identifier name and was refused by `new`, a mistake of the run and not a CLI defect, and is not counted.

**Published.** The draft carried the two assets; an authenticated re-download was byte-identical before publication (`2026-11-10T23:52:09Z` UTC, not marked latest). An **unauthenticated** public download (HTTPS, TLS >= 1.2, proxies disabled)
followed one redirect per file and matched `SHA256SUMS-linux-aarch64` and the local archive byte for byte; the archive from those public bytes passes `verify` (30 files, same manifest), `--version`, self `doctor --resources` and `sdk acquire`.
The repository still carries no executable.

Receipts are in the local evidence directory `/home/cmjo/GitLocal/crt-cli-preview-linux-arm64-20261111/` (`replay/`, `assemble.out`, `pack.out`, `acquire.json`, `public-dl/`). Not claimed: publisher authentication (digests only),
other glibc versions or distributions, (the redirect replay and the sandboxed run were added the same day, see the next subsection) Linux C shared-SDK mode, source-port builds from this
package, updates or self-upgrade, and default/project lock adoption.

### Linux/aarch64 redirect replay and sandboxed package run (2026-11-11)

Follow-up gates for the published Linux/aarch64 Preview above, both run on the same host against the executable extracted from the **public** archive (`9fbef825...e1db`).

**Redirect replay, 54/54.** `tools/replay_redirect_linux.py` serves the real public SDK bytes (`v0.0.1-preview.20261110.linux.arm64.2`, manifest, contract and archive) behind two loopback TLS origins and passes the same
cases as Linux/x86_64 and macOS: allow-list, relative and 301/303/307/308 redirects, 300/304 refused, wrong authority or port, look-alike suffix, IPv6 literal, downgrade, credentials, fragment, missing or duplicate `Location`, loops and
hop limits, wrong certificate, 404, oversize, interrupted body, digest and size mismatch, argument and source-field refusals, and full `sdk acquire` plus idempotent re-acquire. Origin hits A 41 / B 15 / C 0.
Result file SHA-256 `df43e1ee79e9a1ea547af87e994bba39409dc109314ca2e87e0710f5b7238749`.

**Sandboxed package run, 27/27.** A producer-side orchestrator (`sandbox_run.py` in the evidence directory; it hard-codes that directory and is not a product tool) runs the extracted package under Bubblewrap with a read-only root, the producer and CLI
checkouts and every SDK, source and work tree of the evidence directory replaced by empty directories (probes prove the SDK manifest, `crt/include/fcntl.h` and `crt-cli/src` unreachable), a cleared environment, an empty `PATH`, interpreters and
build drivers masked, and strace logging every `execve`. Phase A has no network: `--version`, `--help`, self `doctor`, `doctor` with the manifest pin, a wrong pin ("manifest SHA-256 mismatch"), a changed notice and an undeclared extra file
(both "inventory mismatch", asserted by message) and the restored package. Phase B shares the network: real GitHub `sdk acquire`, idempotent re-acquire, `sdk list`, then C and C++ `new`, first `build --sdk` without configure, `test`, `run`,
warm build and project `doctor` under a parent path with spaces and Korean characters, with host LLVM 21 passed by flag. Executed images: **8 distinct paths / 56 successful calls**, namely the package executable (23), `clang` (22), `lld` (4), the four
project outputs and the probe helper; none unexpected, no shell, Python, CMake or make. `llvm-ar` never ran. Summary SHA-256 `a05fc2265bd61a2175ef541152bacb186ed3c52e63ba20eb2d6f197ad027a0a5`.
An earlier attempt of this run is discarded: `strace` was looked up by name under the empty `PATH`, the sandbox never started, and the expected-failure cases "passed" without checking why; the orchestrator now uses an absolute path and asserts the failure message.

Still not covered on this host: other glibc versions or distributions, Linux C shared-SDK mode, source-port builds from the package, updates or self-upgrade, publisher authentication, default/project lock adoption.

