# Dev SDK / CLI Preview Acceptance

This record owns bounded local Preview input and final-artifact qualification.
The [lifecycle contract](lifecycle_acceptance.md) keeps general shipping/update
requirements; [compatibility](../compatibility.md) owns contract adoption.

## Linux local candidate (2026-11-10)

Scope: native Linux/x86_64 Dev `02-cxx`, one CLI executable with pinned notice
resources, caller-supplied CA data, external LLVM and optional independent CRT
host tools. This is an unpublished local rehearsal. The selected SDK version is
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
retained failed inputs. Both producer fixes are local commits until source
publication; their clean status is not a claim of remote availability.

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

### Remaining publication gates

Publish the selected producer source offer and approve publisher/trust/release
metadata before public delivery. Promote SDK assets first, verify their actual
promoted bytes, then adopt the exact consumer pin and deliver the CLI. Root
`crt.lock`, local default SDK selection and committed binaries remain unchanged.
No public URL/channel, official signer or self-upgrade contract is introduced.
Every advertised host needs its final-package replay. macOS/Windows final Preview
packages, Windows/arm64 D3, Linux C shared SDK mode, Full installation, general
updates/offline resolution, D4 dependency ports and Studio remain separate.
