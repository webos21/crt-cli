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

## Windows/x64 private-DLL host-tools replay (2026-11-20)

**Pass for the bounded local native fixture, 27/27.** The previously published static Windows bundle cannot exercise this gate.
Clean producer `1e489d6c90ad5695ebf8caf3aee63b7d5b8738e1` assembles a separately identified test bundle containing actual,
unmodified imported AWK linked to CRT `libc.dll` and `libm.dll`; mksh/toybox/make retain their original static bytes.
The [producer acceptance](https://github.com/webos21/crt/blob/e8724b85587342ac67ff6361ef08217eaec7ca69/docs/acceptance/host_tools_acceptance.md#windowsx64-private-dll-closure-replay-2026-11-20)
owns exact SDK/source/compiler/runtime identities, replay commands and raw evidence. Host: native Windows 11/x64 `10.0.26100`,
LLVM 22.1.8, Windows SDK `10.0.28000.0`.

Actual consumer: unchanged published `v0.1.0-preview.20261117.windows-x86_64.1` executable,
SHA-256 `feb9779109892cf73235307617398e8dfe70bdadcc86d3dff22ba04642c84df1`.
CLI source baseline `a1db91a91421a1fa738b1be3f47ae7d71ed8621e` receives documentation only; no fresh source build or current-source matrix pass
is inferred. Original public Dev SDK `v0.0.1-preview.20261110.windows.1`, source `b31276a6acccf0a89731f7bb0aecbdcdc9b45cda`,
content `5bb55372ec10e3cfa24c84de981a7130f56aeb57ef0d43d5da22a65036dec7ef` is unchanged. The fixture compiles C sources with its declared
shared C compile policy and links those objects with the declared shared **C++** link policy; pure-C shared link policy remains separate.

| Local fixture / receipt | SHA-256 |
| --- | --- |
| Host-tools manifest pin | `6e1e70f96cc71e2e5c3f179dbc2031d60ab9949e23765c4c64e430a0dd7568e7` |
| Payload | `c61c5b5b15db9d544c46fd2dbff787d5d3338ad4b518ea49edf172689d335af8` |
| Accepted `replay-r2/result.json` | `c03a5af4f5f1e526f0bb1e3aff23bd769409e0a7971b392da59ce0bccf093ba2` |
| Actual mapped-module snapshots | `0e73b7b2dee19aa46a065a806452f657e5c04338d57d63e729387cd9a2ec30fc` |

The producer fixes the initial-load gap by placing byte-identical private DLLs beside each copied executable alias, covered by the existing
format-1 inventory. It verifies the actual PE import graph, native DLL flags/machine, OS shadows and adjacent closure before publication.
No new CLI command, schema, permanent launcher or late DLL-search hook is needed. The CLI already pins and validates the whole inventory.

- Two independently relocated space/Hangul copies retain the same pin; actual AWK/math output is `42` and `9`.
  Canonical AWK starts with empty PATH. Physical `bin/awk.exe` and `usr/bin/awk` aliases and CLI shell jobs survive hostile cwd/PATH.
- Eight Toolhelp snapshots record both CRT DLLs beside the actual running AWK image (16 private mappings total), with hashes equal to the
  pinned canonical closure. The selected bundle is read-only to the consumer. All 18 restricted calls record only declared images (45 processes).
- A working test-only poisoned libm writes a marker and exits 97 in separate cwd and PATH positive controls when the adjacent copy is removed.
  With the verified adjacent copies restored, the aliases and CLI job succeed without loading poison. The trap uses only declared OS imports
  and is never an accepted runtime dependency or a host C/C++ runtime fallback.
- Missing alias DLLs are refused before shell-job creation; changed DLL bytes and a missing canonical DLL fail CLI validation.
  Direct canonical execution with libm absent fails at initial load (`0xC0000135`) while original SDK DLL access is denied.
  Restoring bytes/modes makes the same manifest pin run again; both relocated payloads remain unchanged afterward.
- The original SDK and all imported AWK source bytes remain unchanged. Fresh assembly reproduces the entire public static payload
  `dc106db47ed8bcbb56bdd6da0415dce527653fcb42b0c54bdc1402dca74c35fe`.

Isolation is a restricted token, selected original-input denial and owned job-object process accounting. Native probes deny access to the original
SDK manifest/DLLs, policy, CRT build/source entry points, producer AWK, original CLI and CLI manifest. This does not deny arbitrary execution/network.
Python/LLVM/Toolhelp and the hostile DLL builder are producer/test-host inputs, not bundle-consumer prerequisites. Only disposable fixture ACLs change;
no global PATH, registry, security setting, SDK or STATUS changes. The test suppresses expected missing-DLL dialogs per process and restores that mode.

Evidence: `C:/Users/Lee/s15/replay-r2/` with isolated work `C:/Users/Lee/s15/r2/`, retained in CRT ignored
`out/host-private-windows-20261120/`. Dirty preliminary `replay-r1` also passes 27/27 and remains identified separately.
Focused producer tests: 51 cases (50 pass, one Windows-inapplicable POSIX permission skip), including nine new synthetic PE policy cases;
the actual runtime proof is the separate native replay above. Reproduction uses the committed producer driver and fresh output/work directories.

This closes the specific CRT/CLI Windows/x64 private-DLL initial-load/collision test gate and the linked
[CLI TODO](../../TODO.md#windowsx64-private-dll-host-tools). It does not publish an all-shared host-tools release, change public static assets/signatures,
SDK/CI/default/root/project pins, qualify Windows/arm64 private DLLs or fix its D1 output timeout. Existing historical static-bundle limitations retain
their original scope; the [compatibility ledger](../compatibility.md#windowsx64-private-dll-local-host-tools-qualification-2026-11-20) records this later adoption.

## Windows/x64 published host-tools consumer replay (2026-11-19)

Consumer acceptance of the public [Windows/x86_64 host-tools Preview](https://github.com/webos21/crt/releases/tag/v0.1.0-hosttools.20261119.windows-x86_64.1)
(archive `bd65a2b8d1a5b7865110233bb47a1d13ea56724ad76593333cc349bb0980a210`, 784724 bytes, signed; manifest pin `cd6cb75e91b8f9202cda82876de9898e19fb4aeb01be981ebcfde9eabb7b7e00`, content
`dc106db47ed8bcbb56bdd6da0415dce527653fcb42b0c54bdc1402dca74c35fe`). The producer record, signature verification (openssl 9/9; `crt-cli fetch --publisher-key --signature` 8/8, `signature_verified`) and the native 18/18 tool replay are in
[CRT host-tools acceptance](https://github.com/webos21/crt/blob/main/docs/acceptance/host_tools_acceptance.md#windowsx86_64-host-tools-public-signature-and-native-replay-2026-11-19). The release notes said the CLI's fail-closed
controls and port suites had not been run on this exact artifact; this record runs them. Host: Windows 11 Pro 10.0.26100 x64, LLVM 22.1.8, Windows SDK `10.0.28000.0`; CRT `84eec172`, `crt-cli` `c79d57c`.

**1. D1 fail-closed controls with the published CLI, 32/32.** The executable of the public `crt-cli` Preview 20261117 (`feb97791...84df1`) and the unchanged public bundle run `replay_host_environment_windows.py`
(restricted token per run, empty `PATH`, job-object process accounting, native probe that the CRT/CLI/producer trees are unreadable): self and environment doctor and report, the immutable payload refuses writes, the tools workflow,
normal and timeout cleanup, child exit status, the stdout limit, job-directory reuse refusal, **wrong pin, a link in an immutable parent, a modified tool, an unexpected payload file, missing `make` with no host fallback and a wrong execution
host are all refused**, and the resource-package controls (pin, signature, invalid signature, wrong key, missing signature, wrong host, unknown schema, tampered notice, missing notice, tampered binary, tampered CA, restored package). The
first attempt stopped on a harness expectation that the resources package carry a CA file (the published CLI package trusts `explicit-at-fetch` and has none); a package assembled with the Windows root-store CA
(`create_bootstrap_resources.py --ca-file`, 57 roots, SHA-256 `cc82d2bb87a60eebc7b8dd12b22efdc9eebd13ac44da2098b1f1945e4bd2562b`) passes 32/32. The first attempt is retained; neither the bundle nor the CLI changed.

**2. The five port suites and the real `port build` on this exact artifact, 7/7.** A CLI built from the current sources by the public Dev SDK `v0.0.1-preview.20261110.windows.1` alone passes the producer contract replay: **21** sources, cold 21 executed / 0 reused,
warm 0 / 21, native **992**, archive **214**, SDK-store **113** and recipe **142** checks, SDK-hidden help/version/doctor, only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll` imported, 727 recorded processes; executable
`b8d588b8799bf5fc164929e1958fe180e821099ce807832591917954de94edb7` (45795328 bytes). `tools/replay_port_windows.py` then supplies the public bundle (copied into the restricted-token workspace, inventories compared before and after) and the pinned zlib source
(`9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23`): `port-recipe-test` **142**, `port-build-test` **36**, `port-store-test` **97**, `port-project-test` **83**, `port-command-test` **33**, `port-oracle-test` **21** checks, and the actual
`port build zlib` (static and shared recipe tests pass) all pass with no skipped suite, no undeclared process image in any call and an unchanged SDK and bundle; the bare SDK-hidden checks (visibility, help, version, doctor, imports) pass.
This exercises the `promotion: preview` bundle label that earlier replays could not (the CLI accepts it since `4a8d48c`).

Limits: Windows records process images but denies neither executables nor the network, so isolation is the restricted token, the empty environment and the relocated, inventory-checked copy. The bundle ships no private DLL, so
the private-DLL search-path gate stays open. A job rootfs with real links still needs Developer Mode (the CLI diagnoses it). The CI pin, `integration.yml` Windows leg, Windows/arm64 and macOS consumer replays, and any STATUS change are separate; no default pin or
lock was changed. Evidence (outside both repositories): `C:\Users\Lee\s12\` (`d1-public-r2/`, `contract/`, `d3b/`, the retained first attempts `d1-public/` and `d3/`).

## Windows/arm64 published host-tools consumer replay (2026-11-19)

Native Windows/ARM64 (`10.0.26300`), CRT verifier `0d9d573b93a7c9edb00af18f2d1351d9e774b6a8`, CLI source
`2460919048c5f95211721c8d4ea4454098ffd54f`, LLVM/Clang/LLD 22.1.8. Input is the public signed
[host-tools Preview](https://github.com/webos21/crt/releases/tag/v0.1.0-hosttools.20261119.windows-aarch64.1), archive
`c74ad50ea5f5008d515e1aac3581fa5d73d7acba276ae990a9d3df84599706ee`, manifest
`6f7225ac4ffafe4c5651bf025b47d7b58df0939c520dbf40b7a029aa2892b6f1`. Producer verification, public-download receipts,
OpenSSL checks, PE audit and the 18/18 native producer replay are owned by
[CRT acceptance](https://github.com/webos21/crt/blob/main/docs/acceptance/host_tools_acceptance.md#windowsaarch64-host-tools-public-signature-and-native-replay-2026-11-19).

### Published CLI signature and D1: 31/32, qualification remains open

The public [ARM64 CLI Preview](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261118.windows-arm64.1)
archive SHA-256 is `c5ff710532c0ff9191ae373b48a8f8fe5ceb04bea70647989f9146e255e46e11`, signature
`b72eb649d476673d784a6f4f84c090f73da9df505372cfc9005d19fa2a2da5e6`, executable
`132c8b5ae15f66ed9dd82fcb5a425160e4d682c008ab218353b7eed30f6d3db8`. OpenSSL verifies the CLI archive with the pinned
publisher key. This binary passes four real public-URL `fetch --publisher-key --signature --redirect-policy github-release` cases:
the ARM64 host-tools signature reports `signature_verified`; the x64 archive's signature and a bit-changed signature fail with
`publisher_signature_invalid`; a wrong digest fails with `digest_mismatch`. Refusals publish no output.

`replay_host_environment_windows.py` completes **32 cases, 31 passed, overall failed** on that public CLI and bundle. Selection,
immutable-write denial, shell/awk/make workflow, normal/timeout descendant cleanup, child exit, reused job, wrong pin,
linked immutable parent, modified tool, extra payload, absent make with no host fallback, and a foreign execution host behave as expected.
Resource pin/signature, invalid signature/key, missing signature/notice, wrong host/schema and changed notice/binary/CA checks pass.
The local resource package was assembled with `create_bootstrap_resources.py --ca-file` from this CLI and the public Dev SDK;
resource manifest `afc4c96bead65cd85345fe80ddefd06bf8ac9037a49e7ba2ca8bbf72105606e9`, Windows-store CA
`3cb88031f066192761b84355b7a921d18592c1874146aab63ea355ca8fffd0a8`. Resource signatures use disposable test keys,
separate from publisher verification. Only new fixture directories receive private/read-only ACLs.

**Failed case: `stdout-limit`.** `dd if=/dev/zero bs=65536 count=160` exceeds the 90-second outer harness deadline. A focused
restricted-token run with an outer budget of 900 seconds and the same inner 60-second limit finishes after **91.172 seconds**:
exit **124**, inner `timed_out: true`, `output_truncated: true`, captured **8388608** bytes (SHA-256
`2daeb1f36095b44b318410b3f4e8b5d989dcc7bb023d1426c492dab0a3053e74`). Thus the size cap is observed, but the required
successful completion is not. Direct bundle `dd` and `dd.exe` each return 65536 bytes successfully; a small CLI shell job also
captures exactly 65536 bytes. The CLI deliberately relays captured stdout to stderr, explaining the NUL-filled stderr log.
No runtime fix or relaxed accepted timeout is claimed. CRT's harness now records an outer timeout as a failed case and continues
to collect the remaining results; it also replaces x64-only host constants with the actual native host and a different-architecture rejection.

### SDK-only contract reuse and D3: 7/7

The initial new contract replay at `C:\w\s\contract` stops at `build-visibility`, exit 10: its original source/SDK copies under
`C:\w\s` inherit `BUILTIN\Users` read access. The boundary probe correctly refuses to continue. Existing ACLs were not changed.
The separately retained successful replay at `C:\Users\cmjo\w\contract` uses profile-private original inputs. Its complete source
inventory (`aa3c462ed6e0e1b90c6c052a10254901528783f60ec1f93525f8acdc9f6ee679`) and SDK inventory
(`7e4cb9cab5b60b2de11e8df805eead2928193237090f4a89353c50d7382c22b9`) equal the failed run's inputs. Every current `crt.json`,
`include/` and `src/` file matches the retained export after CRLF normalization; SDK inventory and built executable are rechecked
before reuse (`contract-reuse.json`). This is reuse of an identical successful replay, not a claim that the failed workspace passed.

That replay uses only public Dev SDK `v0.0.1-preview.20261118.windows.arm64.1` (CRT `e7c763094d96ccba1b524077b24b04b876a91d5b`,
content `3b251637845621ba28d6ba1d9c2f6a5e097f6b328f57054f7e7cb5c5eb52621f`). It builds 21 sources, cold 21 executed / 0 reused,
warm 0 / 21, and passes native **1005**, archive **214**, SDK-store **113**, recipe **142** checks. Five environment-gated port suites
are explicitly skipped in that aggregate; they are exercised separately below. SDK-hidden visibility/help/version/doctor pass; the binary
imports only `kernel32.dll` and `api-ms-win-core-synch-l1-2-0.dll`. Binary SHA-256:
`b4a5c9505b3ba1706a938a1d136ce722c8bd9db3553ee05ae1e10c8b79d83532` (45767680 bytes); 596 process records, cold/warm cache receipts retained.

Reproduction entry points (producer/test-host Python, never product prerequisites): `tools/replay_port_windows.py --contract-evidence
<retained-contract> --source <cli> --crt-tools <crt-tools> --host-tools <public-extraction> --host-tools-sha256 <manifest-pin>
--archive <zlib-1.3.1.tar.gz> --output <fresh-evidence> --sid <workspace-build-SID> --hide <original-input> ...`.
The original workspace build SID is read from its ACL; no account or global security setting is created.
With this exact public bundle and zlib source `9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23`, D3 passes **7/7**:
`port-recipe-test` **142**, `port-build-test` **36**, `port-store-test` **97**, `port-project-test` **83**, `port-command-test` **33**, `port-oracle-test` **21**, plus the actual `port build zlib`. Static/shared recipe tests pass, there are no skipped D3 suites or undeclared process images,
and SDK/bundle inventories remain unchanged. The five final bare visibility/help/version/doctor/import checks pass.

Evidence is retained in CRT's ignored `out/host-tools-public-replay-20261119-winarm64/`: `d1/` (first aborted attempt), `d1-r2/`
(completed 31/32), `stdout-limit-focused/`, `dd-probe.json`, `cli-fetch-signature.json`, `contract-reuse.json`, `d3/` and replay scripts.
The original failed and successful contract directories named above are preserved. Windows isolation is restricted-token denial of selected
original inputs, an empty environment, relocated inputs and job-object process accounting, not executable/network denial. Real job-rootfs links
require Developer Mode. No private DLL is shipped; that qualification stays open. D1's large-output case, hosted integration/pin adoption and
official/default trust remain open; no SDK bytes, public release asset, project lock, CI pin, executable in `bin/` or STATUS changed.
