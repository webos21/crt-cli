# D1 Host Environment and Bootstrap Resources

This is the native evidence for the new independent host-tools adapter,
non-interactive shell, scoped doctor and bootstrap data package. The
[implemented contract](../reference/bootstrap_contract.md#independent-host-environment-experimental-format-1)
owns command/JSON semantics; [compatibility](../compatibility.md#independent-host-tools-and-d1-linux-adoption-2026-11-02)
owns adoption; [TODO](../../TODO.md) retains the native replay gaps.
Earlier SDK-shell/bootstrap records remain in
[bootstrap acceptance](bootstrap_acceptance.md), at their original identities.

## Linux/x86_64 local candidate (2026-11-02)

Reviewed CLI `adc03b04ed124c71f0726a895da2cc2cde98b78d` and CRT
`8616fd64b13fa41925c1fce2b08f4937b7e8dcf1` before the uncommitted increment.
The consumer uses public installed SDK/provider inputs and the independently
assembled tools. CRT source, private headers and build trees are not CLI inputs.
CRT owns host-tools assembly/format/prerequisites; CLI owns environment/process
construction, diagnosis, resource packaging and this native proof.

| Selected input | Identity |
| --- | --- |
| CLI product/test source snapshot | `2aa32f07ea920ee4405970f194ccdf1183608f3ba8675ca28e60e80920930155` |
| CLI executable | SHA-256 `9b82bd300379b45be4cb36becfbef56bf41b1ff5cc3d2cb510b9e60447b10284`, 13,889,400 bytes |
| Build SDK | Clean CRT `49879bf82e2742baeb1878e492d42c7d561b6f7a`, `v0.4.0-sourcefix.20261102.5`, Development profile |
| SDK content / provider | `cf2eaf9912ff66594dc0c9d403fddfd25cae620f91d46e2ab01fc7a9aa53f658` / `60ea71f7e55defec6a4faaeb0862e0e01a42dc35bbc39a5a5c43ffe9b15f5185` |
| Host-tools | `v0.1.0-hosttools.20261102.4`, manifest `59bdf8f83ce7817175010953abe652fe6f52852f8aff9a5528f2ce8b64812652` |
| Host-tools payload | `3b75fb9bbab8c9e9a1c7a542f5ced227f1c66bb92ed875e87d78f86e1aba11bf` |
| Bootstrap resource manifest | `654e31b1ca2e7a37072d59fcf9ecbc8b997ff346dde3ca6930617acc2645cf05` |
| Explicit disposable test CA | `d1e7e109c961d8197e53ad4903c9f980816f7ccd573a5fd9cc5341a17c8b2e21` |
| Resource test public-key file | `0e378b907d1cd8938f132b2502bf56c627fe41fdc6dbf15de88c64ac3a4819b1` |

The source hash is SHA-256 of the compact UTF-8 JSON list of sorted
`[relative-path, file-SHA-256]` entries copied from `src`, `include`, `crt.json`
and the original unresolved `crt.lock`. `final-r2/selected.json` retains that
complete input list and exact argv. It is a working-tree snapshot, not a clean
CLI commit. The separate SDK/tool identities retain the
[Tranche 1 selection](bootstrap_acceptance.md#common-source-candidate-replay-2026-11-02).
Pending tracked patches, new files and their hashes are retained in each
repository's `out/tranche2-20261102/review-snapshot/`.
Dirty SDK opt-in is **disabled** in the selected final build. No production lock,
publisher policy, release or artifact promotion is adopted.

Host-tools come from public native `01-c`, not the Development/C++ SDK. Their
manifest records real PT_INTERP and an explicit OS loader prerequisite; it was
assembled after the audit correction. Exact assembler identities, source SDK,
notices, make license pin and producer tests are
[CRT-owned](https://github.com/webos21/crt/blob/main/docs/acceptance/host_tools_acceptance.md).

## Build and native regression

Original receipts live under `out/tranche2-20261102/` in this repository.
`replay-r3` and `replay-r4` run CRT's
`tools/replay_cli_contract_linux.py` with the current CLI source export,
`crt/out/tranche17-20261102/dev5`, `/usr/bin/clang`, `/usr/bin/clang++`,
`/usr/bin/ld.lld` and `/usr/bin/llvm-ar-21`. The original CRT/CLI checkouts and
frozen worktree are hidden; Python, shell, make, CMake and Ninja are masked.
Both pass cold **13/0**, warm **0/13**, 767 native checks, bare visibility and
ELF/execve audits, with unchanged source/relocated SDK inventories.

After adding stricter resource schema/target checks, the selected final source
is copied to `crt-cli-replay/crt-cli-contract-6q_chy_c/source-final`. The previous
passing CRT-native CLI configures it without dirty opt-in, builds it cold and
runs the resulting binary's warm build and full native suite under the same
source-hidden boundary. `final-r2` records **13/0 cold**, **0/13 warm** and
**767 passing checks**, including default self doctor, read-only project doctor,
child cwd with spaces/Korean text, normal-completion descendant cleanup,
timeout cleanup and bounded stdout capture. The failed preliminary invocation
`final/configure.stdout` is retained: `--allow-dirty-sdk false` is invalid;
omitting the opt-in is the supported way to require clean SDK provenance.

`final-r2/native-audit-r4.json`, `cli-readelf-program.txt` and the selected package's
self-doctor record agree: no DT_NEEDED/RPATH/RUNPATH, OS interpreter
`/lib64/ld-linux-x86-64.so.2`, mapped images only the CLI, that loader and vdso.
CRT/C++/curl/Mbed TLS/zlib are statically linked; no host libc/C++ runtime or
permanent launcher is loaded. Project-specific port-build diagnostics and a
real selected-SDK shell job separately verify host/target identity separation,
correct `CRT_SYSROOT` and unchanged `crt.lock`/`.crt/local.json`.

## Independent tools and resource negatives

Reproduction entry point, using fresh evidence/job paths:

```sh
python3 tools/replay_host_environment_linux.py \
  --cli <selected-native-crt-cli> --host-tools <independent-host-tools> \
  --resources <bootstrap-resources> --output <fresh-evidence-directory> \
  --work-parent <outside-the-repositories> \
  --hide <crt-checkout> --hide <cli-checkout> --hide <sdk-directory> \
  --hide <provider-file> --hide <source-export> --hide <llvm-directory>
```

`host-environment-r3/commands.json` retains the full actual hidden-path list and
argv. The working set is copied to
`crt-cli-replay/d1-native-icpbz2c5/`, including `tools space 한글` and
`bootstrap space 한글`. Bubblewrap denies network, hides the original
repositories, selected source/SDK/provider copies and LLVM, masks host
shells/awk/make/Python/build drivers, clears the environment and mounts the
selected tools/package read-only. The caller-owned working set and job scratch
are writable.
Environment/rootfs filtering inside the product is not that OS sandbox.

**31 cases pass**, including the actual mksh pipeline, substitution, heredoc,
awk result, toybox copy/compare/remove, GNU make recipe and recursive make.
Physical tool, script, cwd and job paths contain spaces/Korean text. Exact PATH
string checks and absent CPATH/credential/CRT_SYSROOT sentinels prove the private
environment; hostile ambient MAKEFLAGS/CRT_ROOTFS/PATH do not reach the job.
The read-only environment report creates no job. Shell preserves exit 7,
returns 124 on timeout and caps stdout at 8 MiB. Both normal and timed-out jobs
start a background child, then prevent its delayed marker write. Work reuse,
linked parents inside immutable tools, wrong pins/hosts, changed/missing files,
extra payload and removed alias execute permission fail without fallback.

The resource package contains one CRT-native executable, notices/provenance for
CRT libc, LLVM runtimes, curl, Mbed TLS, zlib and nlohmann-json, LICENSE and the
explicit test CA. Its manifest declares the source SDK's OS prerequisite set.
It contains no SDK headers/libraries, compiler, Python/shell script, helper,
private test key or permanent launcher. Exact manifest pin and detached
RSA-PSS/SHA-256 test signature pass; wrong key/signature, missing paired input,
unknown schema/target, changed/missing notices, changed executable and changed
CA all fail. Three producer-only packaging fixtures additionally prove complete
data inventories, immutable outputs, escaped-link rejection and no partial
publication. The selected executable file match is not memory attestation or
an official publisher claim.

The execve audit uses separate per-process logs, full paths and decoded
strace octal escapes for Unicode; all 15 distinct executed paths resolve to the
selected CLI/package or bundle. No host tool executes. Earlier
`host-environment-r1` is retained as failed evidence: a non-ASCII grep pattern
hits the existing regex import's documented multibyte limitation
(`crt/libc/src/regex/netbsd-compat.h`), and tracing every syscall makes the
unbuffered stdout stress exceed its timeout. The corrected test verifies the
Unicode path by exact shell comparison, exercises ASCII grep and uses
`strace --seccomp-bpf -ff` to trace only execve. It does not claim Unicode regex
support or change runtime/upstream sources. r2 passes before the r4 OS-loader
manifest correction; r3 is the final declared-prerequisite combination.

## SDK-free HTTPS with selected trust data

`https-final-r2/invocation.json` records CRT's producer-only
`tools/test_cli_fetch_https.py --publisher-cases --isolate-linux`, with the
selected package executable and `--trusted-ca-file` pointing to that package's
`share/trust/ca.pem`. `--test-ca-directory` supplies its matching disposable
issuer only to the test host. The CLI never reads the CA private key.

All **32 cases pass**: real HTTPS metadata/artifact, signed metadata/artifact,
wrong/expired/host-mismatched/malformed/missing CA, tampered metadata/artifact
size/digest, interruption, timeout, 404, redirect, plain HTTP, existing output,
and wrong/malformed/weak/private/missing publisher keys, corrupted/short/large
signatures, wrong padding/salt and unpaired authentication options. Failures
publish nothing, leave no staging files and preserve the earlier output.

This consumer has empty PATH, source/SDK/provider/LLVM and host-tools paths
hidden, host interpreters/build drivers masked. Only the test server's network
namespace is shared for controlled loopback HTTPS. Execve logs contain only
the selected copied CLI. CA bytes exactly match the package pin above; detached
fetch signatures use a separately retained disposable key, not an official CRT
publisher. Plain doctor still reports network `not_run`; these are explicit
network tests. Earlier `https-final` passes the same binary/CA before adding
the package's OS-prerequisite declaration and is retained separately.

## macOS/arm64 independent host-environment replay (2026-11-03)

**Pass for this new local macOS combination**, reviewed at producer
`518953f4d9ecc48c1d082644414e0a409a5a49ad` and consumer
`095afbcde4b0501267520381b24ba8114315d89a`. The same 21 product/test inputs
as the Linux increment produce the new native executable; their compact sorted
`[path, SHA-256]` list (original unresolved `crt.lock`, `crt.json`, `src`,
`include`) hashes to `2aa32f07ea920ee4405970f194ccdf1183608f3ba8675ca28e60e80920930155`.
The full exported checkout's separate inventory is
`b0b5f47361a3e7b7e8d124cdef39c800aefb20838bc2657ad8c0ca20f55152ab`.
New changes are test-host replay tools and documentation, not product code.
Execution host: native arm64, macOS 27.2 build `26B123`.

| Selected input / output | Identity |
| --- | --- |
| Public build SDK | Retained clean CRT `8db69f404dcfa70b5aa0569cae51a962fd0615d2`, `v0.4.0-cli.20261101`, Development/macOS/aarch64 |
| Dev manifest SHA-256 | `c0cd45bed2623b89f8e57ff95cc38bf47fea04998aecca6363e2eab41a51c764` |
| SDK content / provider SHA-256 | `a438a4a08bc0bec98d53ad95d3cbc196525d655428086088508a3df9ea0f2f16` / `489ebea23e8531cfdd57cd3ba526a163f18c42616e9a856961af49ace673a21d` |
| New CLI | `3aec79f15ec1b86d8abb23a9406549d7057c8b9fdbc6a0308b095d18f7d8b898`, 3,675,736 bytes |
| Independent host tools | `v0.1.0-hosttools.20261103.macos.1`, manifest `0a81db3eacb89b54c106d8f57bfd5a0e6bb57a24977e124f583e7c81751f1160` |
| Host-tools payload | `6e6984bb1edd573132ad2bbff01ea8828485be4a7fb239c029f686f53b58608e` |
| Resource manifest | `e3ea7042fef2258120c1e4bb0a3766c36e6c733196ecb9e6ffd0943c97dfe689` |
| Explicit disposable CA | `ab6d34458652dfc88b7760453a41698e03587357cf6199d39e8489379f85aa3f` |

This is a new CLI/tools replay against a retained verified SDK, **not** a fresh
SDK build at `518953f`, replay of Linux's `49879bf` bytes, canonical macOS source
asset regeneration or production adoption. The producer's
[macOS record](https://github.com/webos21/crt/blob/9d8be12e27802b87e0bca61fa562a281b1c7319b/docs/acceptance/host_tools_acceptance.md#macosarm64-independent-bundle-replay-2026-11-03)
owns C-only tool extraction, licenses, actual native imports and CMake cold/cache
assembly. The selected tool manifest records clean `518953f`. Formats remain
experimental host-tools/resource/native-provider 1 and existing project formats.

### Native self-build and runtime boundary

CRT's `tools/replay_cli_contract_macos.py` exports the SDK/source outside the
repositories, denies original producer/CLI reads, denies network and permits
only the selected native toolchain/CLI/generated test images. Its probes prove
host shell/Python/make/CMake/Ninja/wrapper execution is denied. Bare probes
additionally prove relocated SDK/source/provider reads and compiler execution
are denied. Clang/Clang++/ld/ar come from the explicit Xcode toolchain; exact
argv, tool hashes and vendor SDK path are in `cli-replay-r1/`.

Fresh self-build passes **13/0 cold**, **0/13 warm** and **807 native checks**,
including project doctor, cwd/Unicode paths, child environments, normal and
timeout descendant cleanup, C/C++ static/shared targets and generated tests.
The SDK inventories remain unchanged. The legacy replay driver passes its
dirty-SDK opt-in flag, but both producer/compile provenance are clean; the
separate `clean-selection.json` also verifies a fresh project can select this
same SDK **without** the opt-in. No dirty input is relabeled clean.

The previous committed macOS CLI (`72577b9b7dd1e602635ccdabef0fe728e66441c7bd9b07607ae8994ff2de949e`)
bootstraps stage1/stage2 with identical code/data. Bytes differ only in LC_UUID
(16 bytes) and __LINKEDIT (267 bytes); this is **not** whole-file byte equality.
The selected ordinary self-build executable above, not either loop artifact,
is the package/host-environment/HTTPS consumer.

Mach-O imports contain only `/usr/lib/libSystem.B.dylib`, no LC_RPATH; the CLI
owns malloc/new/delete and the CRT-owned binding audit has no foreign provider.
PAL runtime maps also enumerate Apple's transitive shared-cache images,
including `/usr/lib/libc++.1.dylib`; their presence is retained, not suppressed
or confused with CLI standard-library symbol bindings. The four host tools
likewise statically own CRT and have no private dylib dependencies. Thus this
proves the selected closure, **not** a private-dylib collision/load scenario.

### Independent tools, resources and selected project

New producer-only `tools/replay_host_environment_macos.py` runs the corresponding
**31 cases**, all passing in `host-environment-r2`. Bundle/package/script/cwd/job
paths contain spaces and Korean text. Seatbelt denies the original repositories,
the entire relocated source/SDK/provider worktree and Xcode toolchain; only the
copied CLI, native bundle roles and a dedicated boundary probe may execute.
Tools and package are read-only to consumers, while the test orchestrator mutates
its disposable copies for negative cases. Actual read/exec denial is probed;
no Linux execve trace or Windows accounting mechanism is claimed.

Pipeline, command substitution, heredoc, awk arithmetic, toybox copy/compare/remove,
make and recursive make pass. Ambient PATH/CPATH/secret/MAKEFLAGS/CRT_ROOTFS and
DYLD path sentinels do not reach shell jobs. Physical cwd is verified; virtual
SHELL/MAKEFLAGS avoid make's space-containing physical-shell problem without an
upstream patch or launcher. Normal and timed-out shell jobs stop the delayed
background marker, exit 7 is preserved and stdout is capped at 8 MiB.

Wrong host/pin, changed/missing/extra tools, removed alias execute permission,
immutable linked-parent job paths and job reuse fail without host fallback.
Pinned resources, explicit RSA-PSS signatures and their wrong-key/signature,
schema/target, missing input/notice, changed executable/notice/CA negatives pass.
The package contains one executable plus notices/trust data, no SDK/compiler,
interpreter, private signing key or permanent launcher. Orchestrator-only private
test keys are not packaged. Six portable resource/policy fixtures also pass.
An interrupted replay receipt cannot report final success.

`selected-project/result.json` separately permits the **explicit** relocated
SDK and resource-query compiler while still denying original repositories and
host shell/build-driver execution. Project doctor, port-build environment and
actual shell validate CRT_SYSROOT plus the unchanged `crt.lock`/`.crt/local.json`.
That phase is not labeled SDK-free; the 31-case tools phase needs no SDK.

### Packaged trust, failure preservation and reproduction

`https-r2` passes all **32** D1-07 HTTPS/integrity/RSA-PSS cases using the exact
selected package CA, copied outside denied inputs without changing its bytes.
The consumer has empty PATH, SDK/source/provider/Xcode/tools hidden and a Seatbelt
allowlist containing **only its copied CLI**. The Python/OpenSSL loopback server
and disposable issuer/signing material are test-host inputs, not dependencies
or official publisher trust. Consumer networking remains enabled for the
controlled endpoints; this is not a network-namespace claim.

The initial `https-r1` never executed the CLI: tempfile returned `/var/...`, but
Seatbelt matches canonical `/private/var/...`, so execvp returned 71. The test
driver now resolves the work directory before building the policy; failed logs
are retained rather than described as TLS/product failures. The selected CA
must also be accessible as explicit package data, not via the hidden original
producer directory. No CRT/PAL or upstream fix was needed.

Receipts are retained in **CRT** `out/tranche2-macos-20261103/`, particularly
`cli-replay-r1/{inputs,commands,result,bootstrap-loop}.json`, `selected-inputs-final.json`,
`product-inputs.json`, `host-tools-bindings.json`, `host-environment-r2/`,
`selected-project/`, `clean-selection.json`, `https-r{1,2}/` and producer logs.
Work copies are outside both repositories in the recorded temporary paths.
The first successful tools run r1 remains retained; r2 uses the final driver
with explicit completion reporting. All receipts identify the actually used
SDK/provider/package/tool bytes, not later documentation edits.

Reproduce with new output/work paths:

```sh
python3 tools/replay_host_environment_macos.py \
  --cli <new-native-cli> --host-tools <selected-independent-tools> \
  --resources <selected-bootstrap-package> --output <fresh-evidence> \
  --work-parent /private/tmp \
  --hide <crt-checkout> --hide <cli-checkout> --hide <sdk-source-provider-worktree> \
  --hide <xcode-toolchain-usr>
```

Then use CRT's `tools/test_cli_fetch_https.py --publisher-cases --isolate-macos`,
the selected `--trusted-ca-file`, matching `--test-ca-directory` on the test host
and the same hidden input roots. Exact invocations are in the retained receipts.
No committed binary/local SDK defaults, production pin, release or STATUS changes
were made. Windows/x64 replay of this new increment still gates Tranche 2/D1.

## Windows/x64 independent host-environment replay (2026-11-03)

**Pass for the recorded local Windows combination.** Reviewed CRT
`9d8be12e27802b87e0bca61fa562a281b1c7319b` with the producer patches subsequently
committed as `b6bf52be64047e718dc0f2870472a5ffece913f9`, and CLI
`0b40a7087e567e8b7bfae2aa880d9f4f2e1abdb3` with two portable Python fixture
corrections. CLI product code is unchanged: its 21 pre-configure product inputs
hash to `2aa32f07ea920ee4405970f194ccdf1183608f3ba8675ca28e60e80920930155`, matching the Linux/macOS increment.
Use `inputs.json` for the original unresolved `crt.lock`; configure creates a
different, resolved lock in the disposable export. No committed lock/binary is
changed. Windows/x64 is native, not WSL or an ARM compile-only result.

| Selected input/output | SHA-256 / identity |
| --- | --- |
| Development SDK manifest | `31bcc5d0b2874bda33c17198f0c391c588ad9be32b9a59bbd57cf6f6a194b421` |
| SDK content | `8c3e90fe4fcaeda0e00e9748d6e90ec671af251b469d56cbc188be963a6a7057` |
| Native compiler policy | `fdb73e4c11389b1085d90e0ba94a96c0f6ac0d9c0a1616f201f7e01c68e9b38a` |
| New CLI | `a9d922ef71fb042a8b2ed7419db7da3d5b078700937b661f14dd63d4c1bb223c`, 24,328,192 bytes |
| Independent host tools | `v0.1.0-hosttools.20261103.windows.5`, `513eab805ed0753b67d00939fd8b00e03a577f6f14cbe74b2ad4e3862691b1cf` |
| Host-tools payload | `69a725081e93134cdd2b16d7a9a6a08f14d5a4d061d883fc430f7609a87f8b16` |
| Resource manifest | `d0f25131acba0fdaead34606aeaf6abefe556fe513009299f3bd9419c2f58602` |
| Explicit disposable package CA | `68d4d79773112ce918f0449b479667dca38eaf527b808e1fdff4b3728c56ccb3` |

The final public SDK is CRT `out/tranche2-windows-20261103/dev-rootfs-fix`,
version `development`, producer/base/compile `9d8be12` with **dirty=true**.
That provenance is preserved, not relabeled as a clean build of the eventual
commit. New C/host tools and fresh Development ports include the Windows PAL
corrections; unchanged imported C++ artifacts retain their pin/source identity
and pass static/shared smokes against the new base. The
[producer record](https://github.com/webos21/crt/blob/b6bf52be64047e718dc0f2870472a5ffece913f9/docs/acceptance/host_tools_acceptance.md#windowsx64-independent-bundle-replay-2026-11-03) owns source/cache repair, runtime/shell fixes,
UTF-8 manifest installation, licenses, direct/CMake bundle identity and limits.
Formats remain experimental host-tools/resources/native-provider 1 and existing
project formats. These are local candidates, not production adoption.

### Native build, isolation and independent tools

CRT's existing Windows replay driver exports SDK/source outside both repositories,
runs each consumer under a private restricted token, probes producer-tree read
denial, clears PATH and records job-process images. Fresh native self-build
passes **13 executed/0 reused**, then **0 executed/13 reused**, and **857
native checks**. It covers Unicode/punctuation paths, native C/C++ static/shared
targets, generated tests and process cleanup. The original and relocated SDK
inventories remain unchanged. Clang/Clang++/LLD/llvm-ar and Windows SDK
10.0.28000.0/um/x64 are declared build-host tools; no host shell/Python/CMake/Ninja
or SDK shell wrapper executes in the native app build path.

Bare help/version/self-doctor additionally deny the relocated source, SDK and
provider through a second SID. Direct PE imports are only `kernel32.dll` and
`api-ms-win-core-synch-l1-2-0.dll`; allocation ownership and UTF-8 doctor checks
pass. Runtime maps include Windows' transitive OS images, including msvcrt;
that is not a direct host-libc/C++ dependency or a hidden private runtime bundle.
The previous committed Windows CLI `a04ff851fb1156a161f01f308c368648ba1b091a001977e449fb86009820237a` also builds
stage 1, which builds stage 2 with all 13 work items cold. The two files are
**not byte-identical**; successful bootstrap is not a reproducible-binary claim.
The ordinary self-build CLI above is selected for packaging and D1 tests.

New CRT producer-side `tools/replay_host_environment_windows.py` passes **32
cases** in `host-environment-r4` with separately selected r5 tools. Tool/package,
script/cwd/job names contain spaces and Hangul. Real rootfs directory symlinks
are checked; Developer Mode was already enabled and no setting was changed.
Private ACLs apply only to newly created fixture copies. Tools/resources are
read-only to the consumer; orchestrator mutations test rejection in disposable
copies. Process records require exact selected image paths plus OS conhost,
including rejection of missing/unknown images. This is **Windows accounting,
not per-executable launch denial or a hostile-code sandbox**. Public LLVM/OS
files are not claimed unreadable; no compiler executes in this SDK-free phase.

Pipeline/substitution/heredoc, awk arithmetic, toybox copy/compare/remove, make
and recursive make pass. The child PATH is exactly its declared single selected
tool directory; original PATH/CPATH/credential/MAKEFLAGS/rootfs sentinels do not
become job policy. Physical cwd and virtual SHELL are verified. Normal and
timeout cleanup both start a background child and prevent its delayed marker;
child exit 7, timeout 124 and the 8 MiB stdout limit are preserved. Wrong pins,
hosts, missing/changed/extra tools, linked immutable job parents and reused jobs
fail without fallback. A POSIX execute-bit mutation is not claimed on Windows.

Pinned resources and explicit RSA-PSS test signatures pass, along with wrong
key/signature, unpaired input, schema/host, executable/notice/CA tamper and
missing-notice rejections. Restored package/tool inventories pass again. The
package contains one executable, declared notices/provenance and trust data;
no SDK/compiler, interpreter, private signing key or permanent launcher.
File integrity checks are not memory attestation or official publisher trust.

`selected-project-r4` separately allows an explicit copied SDK/provider and
compiler resource query while denying original producer/source trees. Project
doctor, port-build env and actual shell verify CRT_SYSROOT and unchanged
`crt.lock`/`.crt/local.json`. This is **not** the SDK-free tools phase.

### Selected trust, retained failures and reproduction

`https-r2` passes all **32 D1-07 cases** under a new restricted SID with empty
PATH and source/SDK/provider hidden. The adapter copies the exact selected
package CA outside the denied producer tree, records its SHA-256 and requires
it match the test issuer certificate. Every fetch process is the copied CLI
or OS conhost, with full process records retained. Tests cover real HTTPS and
RSA-PSS positives, certificate/metadata/size/digest/signature failures,
interruption, timeout, HTTP/redirect rejection and preservation of prior output
with no published staging leftovers. Python/OpenSSL and disposable issuers/keys
belong to the loopback test host; this establishes explicit test-identity trust,
not official release-key/expiry/revocation policy.

Initial SDK `d054a66` is correctly rejected for missing narrow-encoding capability.
An intermediate new CLI passes 857 checks/32 HTTPS cases, but its old PAL
overrides a selected child rootfs; those receipts remain historical. Failed D1
r1-r3 reveal the Windows PATH fixture assumption, async status255, directory
search/TMPDIR and bootstrap-env defects; the producer owns minimal reproductions
and repairs. No hostile sentinels or guards were removed to obtain the final
pass. The initial selected-project fixture used a Unicode/space basename where
`new` requires a package identifier; retries retain Unicode in the parent and
use `hello` as the basename. The API is unchanged. Strawberry OpenSSL's compiled
default config path is absent; the test host supplies only a local explicit
`OPENSSL_CONF`, never a CLI dependency or global trust-store change.

Receipts stay in CRT `out/tranche2-windows-20261103/`: `final-result.json`,
`cli-replay-r3/`, `host-environment-r4/`, `https-r2/`, `bootstrap-loop-r2/`,
`selected-project-r4/`, `bootstrap-r2/`, `candidate-final-status.json` and producer
records. The full copied work paths/argv and source/tool fingerprints are in
`inputs.json`, `commands.json` and the respective results. Earlier failed or
different identities are retained, not overwritten. Six Python resource/policy
fixtures pass on Windows after native suffix/PurePosixPath corrections; CRT's
existing CI tooling selection passes 174/178 with four conditional skips.

Use CRT's `replay_cli_contract_windows.py`, then its new
`replay_host_environment_windows.py --cli <new-cli> --host-tools <pinned-bundle>
--resources <pinned-package> --probe <sdk-built-visibility-probe> --output <fresh>
--hide <producer> --hide <cli-source> --hide <build-work>` with fresh work paths.
Run `test_cli_fetch_https_windows.py` separately with `--publisher-cases`,
`--trusted-ca-file <package-ca>` and `--test-ca-directory <matching-test-issuer>`.
All are producer/test-host tools, not product prerequisites. No committed binary,
SDK default, production lock, release, original design baseline or STATUS changes.

## Remaining qualification

The recorded Linux/x86_64, macOS/arm64 and Windows/x64 combinations close the
bounded non-interactive local Tranche 2/D1 increment. Their actual artifacts,
source revisions and limits remain separate; this is not a common latest SDK
or new ARM runtime replay. Interactive terminal/job control remains outside
this increment. D2 owns installation/selection/self-upgrade; D3 owns source
recipes/compiler bridges and cross-target work. Official release metadata/key
lifecycle and production artifact promotion remain separate from test signatures.

## macOS/arm64 private-dylib host-tools replay (2026-11-06)

**Pass, bounded local fixture.** The earlier macOS bundle was entirely static;
it did not qualify this scenario. CRT baseline `82466be` plus its identified
producer/test delta now builds actual, unmodified AWK import sources with clean
CRT `784389b` Dev's shared C policy, and assembles a separate test bundle. CLI
baseline `1dc3c03` plus the preceding macOS workflow fix yields the same tested
static executable `94ddda0ee179b3e47bf4dbfc91502415ef33209dfa9945706f7cb1116fd9ea8d`.
No further CLI product implementation or SDK runtime change is needed here.

The [producer acceptance](https://github.com/webos21/crt/blob/main/docs/acceptance/host_tools_acceptance.md#macosarm64-private-dylib-closure-replay-2026-11-06)
owns exact source/build/manifest/payload/dyld identities and evidence. The
fixture is **not** an official all-shared host-tools release: mksh/toybox/make
retain their original bytes; actual AWK links CRT `libc`, `libm`, `libdl` dylibs.
Copied aliases retain their bytes and use six pinned internal links to the
canonical private closure. The ordinary static bundle's complete payload is
unchanged; manifest version 1 and CLI inventory semantics are unchanged.

Native replay **27/27** proves two space/Hangul relocations, real dyld loading
from the bundle, correct AWK/math output through CLI jobs and a physical alias,
and unchanged original SDK/bundle data after fixture restoration. The caller
pins the manifest; missing/modified dylibs are refused before job creation.
Removing copied-alias runtime links produces an actual loader failure, and
restoring them makes that alias run again. The closure passes the existing
CRT binding/allocator audit with its declared libSystem PAL boundary.

A working host-only poisoned `libdl.dylib` writes a marker and exits 97 in the
unfiltered direct-launch positive control. With the same hostile cwd/DYLD paths
given to the CLI, shell jobs use the verified bundle and never load the poison.
The trap is not an accepted runtime. **Apple's sandbox-exec strips DYLD keys**:
the test-only native helper sets them after that protected boundary, then execs
the selected image. Thus the test does not mistake OS filtering for CLI
filtering. No permanent launcher, global environment or security/trust setting
is introduced. Direct manual execution under arbitrary DYLD overrides is not
claimed safe; existing PAL absolute-host-path passthrough is unchanged.

Source/SDK/producer/compiler paths and network are denied by Seatbelt; only
the selected CLI, native bundle executables and test helper may execute. The
selected bundle is read-only. The SDK, compiler, Python, signing and Mach-O
inspection tools are test-host/producer inputs, never prerequisites to consume
the bundle. Original `/usr/bin/awk` fallback is explicitly denied.

Exact invocation and evidence remain in CRT ignored
`out/host-private-macos-20261106/`, with accepted `replay-r3/result.json`
SHA-256 `d068c05a558688f64ff82ec19ae46b693a642ba91bbba6e49e9935f637d5fad3`.
The first attempt (18/25) and 25/25 intermediate result are retained separately.
The same native DYLD bridge strengthens earlier selection/first-build tests:
**66/66 and 86/86**, with five policy-generation checks and four repeated
consumer binding audits. Their exact records live in
[SDK installation acceptance](sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-macosarm64-2026-11-06).

This closes the missing macOS/arm64 private-closure test slice only. Windows
private-DLL/arm64, other future dynamic bundles, production trust/promotion and
default/binary/root-lock adoption remain separate. STATUS is unchanged.
