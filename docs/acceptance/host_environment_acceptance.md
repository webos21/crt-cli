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

## Remaining qualification

This closes the feasible Linux D1 implementation/validation increment. Replay
the **new** artifact/adapter on native Windows/x64 and macOS/arm64, including
initial private DLL/dylib resolution/collision, real aliases/Windows links,
Unicode paths, scoped doctor, cwd, normal/timeout cleanup, packaged resources
and HTTPS/signature negatives. Existing binaries on those hosts retain their
earlier source identities. Other architectures likewise need their own replay.

Complete Tranche 2/D1 remains open. The current shell is non-interactive;
terminal/interactive cancellation is unimplemented. Source-port recipe/compiler
bridges and cross-target builds belong to D3. D2 owns installation/selection and
self-upgrade recovery; official release metadata, expiry/key rotation/revocation
and production release qualification remain separate. Neither successful local
signatures nor the producer Python tests adopt a production pin or public release.
