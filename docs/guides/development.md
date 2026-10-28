# Development and Bootstrap Guide

This guide describes the initial development process. It is not an installer
guide. The implemented subset and the intended full workflow are distinct.

## Before Making Changes

1. Read [AGENTS](../../AGENTS.md), [TODO](../../TODO.md), and the
   [CLI design](../design/crt_cli.md).
2. Inspect the worktree and preserve unrelated user changes. Read the relevant
   [history](../../HISTORY.md) and [acceptance contract](../acceptance/bootstrap_acceptance.md).
3. Choose one authorized, bounded increment. Record its active scope in TODO
   before moving on; do not treat the entire roadmap as implementation approval.

The native JSON bootstrap replaces the initial CMake prototype. The
[implemented contract](../reference/bootstrap_contract.md) owns formats and scope;
[acceptance](../acceptance/bootstrap_acceptance.md) owns per-host outcomes.

## Build and Test the Bootstrap

Use native LLVM and a produced Dev SDK plus its separately hashed experimental
compiler policy. No CMake, Python, shell or network dependency is needed by the
CLI build/test path. A temporary hosted C++ seed builds the first CRT-native CLI:

```text
clang++ -std=c++17 -O1 -fuse-ld=lld --ld-path=/absolute/ld.lld tools/bootstrap_host.cpp src/main/core.cpp src/main/process.cpp src/main/toolchain.cpp src/main/project.cpp -o /absolute/native-seed
/absolute/native-seed configure --project /absolute/crt-cli --sdk-root /absolute/handoff/dev --sdk-contract /absolute/handoff/native-toolchain-v0.json --contract-sha256 <exact-provider-sha256> --cc /absolute/clang --cxx /absolute/clang++ --linker /absolute/ld.lld
/absolute/native-seed build --project /absolute/crt-cli
/absolute/crt-cli/out/native/crt-cli test --project /absolute/crt-cli --json
/absolute/crt-cli/out/native/crt-cli doctor --bootstrap --json
```

These are separate argv invocations, not a shell script. Supply actual absolute
paths and preserve the `ld.lld` driver name even when it is a symlink to `lld`.
The seed is a disposable build-host tool using host C++; the resulting CLI links
CRT/C++/HTTP/TLS statically and must pass its own import/runtime audit. The seed
is never shipped and subsequent self-build/test uses the CRT-native CLI.

CRT supplies the policy using `tools/export_cli_toolchain.py --sdk-root <dev>
--output <fresh-provider.json>`. That producer migration tool still uses the old
wrappers/Python/shell; do not count it as a CLI consumer dependency or a public
SDK schema. Retain the producer source, export tool hash, immutable SDK inventory
and provider digest separately. Do not overwrite an accepted candidate.

For generated projects, run `crt-cli new /absolute/hello --language cxx`, select
the same local SDK explicitly with configure, then run test/build/run with
`--project /absolute/hello`. New only generates sources. Missing SDK selection
currently fails clearly; automatic acquisition remains planned.

Fresh isolated Linux replay must export sources and SDK outside producer trees,
hide producer/original inputs, make Python/shell/CMake/Ninja executables unavailable,
trace actual execve calls and run native self-build/tests. Bare-binary checks then
hide the relocated SDK/source/output as well. Keep logs, exact argv, source/export
hashes, ELF imports, mapped images and unchanged inventories. Linux evidence does
not grant macOS/Windows acceptance. The previous CMake receipts remain historical.

## Bootstrap Planning Order

Start D0 with [product decisions](../design/crt_cli.md#10-decisions-for-the-first-implementation)
and the [initial repository/template contract](../design/crt_cli.md#initial-repository-and-template-contract),
then inventory the SDK candidate and define focused identity/diagnostic
fixtures before minimal CLI implementation. Internal module directories may
evolve; the public SDK-only project/build contract and template migrations are
the compatibility boundary, not an identical frozen source tree.

Single-executable distribution is the target. D0 records actual CRT/C++/HTTP/TLS
link/import feasibility; D1 exercises SDK-free self-diagnosis and real HTTPS.
The owner selected crt-cli, shared immutable SDKs with project references,
first-configure/build acquisition and no permanent launcher on 2026-10-28.
Cache/certificate data, previous versions and temporary update helpers are
allowed; their exact trust/activation mechanisms still require validation.
A sidecar CRT runtime does not satisfy the one-file goal.

Project SDK version/profile/target/content selection is independent of storage
and CLI upgrades. Record the actual backend and build mode separately from
SDK content profiles. Directory examples below are handoff examples, not a
decision on generated-project SDK installation or an implemented CLI layout.

## D0 Input Inventory

The first implementation must identify real inputs, not placeholders:

- Installed SDK stage, target OS/architecture, producer revision/build ID,
  archive/content digests, manifest, and actual public wrapper/toolchain paths.
- Execution host identity and the CRT shell/tools/rootfs available for it,
  including aliases, DLL/runtime dependencies, and declared generators.
- External LLVM/LLD and OS/vendor SDK; optional port-build generators:
  selected paths, versions/content identities, and roles.
- Missing SDK consumer contracts or host/target separation that require a
  producer-side change, rather than a local wrapper workaround.

Record candidate inputs and observed behavior in the owning acceptance evidence
before calling them validated. The CRT design revision is a documentation
baseline, not the identity of a downloaded SDK. Do not copy headers/libraries
from an adjacent CRT source/build tree or assume a package has been released.

## Local Dev SDK Candidate Loop

A public CRT Dev SDK release is not required to begin CLI development. Use
one authorized session for adjacent `crt` and `crt-cli` repositories, keeping
their changes, commands and evidence scoped to the owning repository. CRT
source/build results are available for producer work and diagnosis; CLI
configure/link/run inputs remain a produced SDK's public interface.

Use an explicit handoff, for example:

```text
workspace/
  crt/                         producer source and its own build outputs
  crt-cli/                     independent CLI source
  handoff/<candidate-id>/dev/   copied or extracted Dev SDK
  cli-build/<candidate-id>/     fresh external CLI build and test outputs
```

These are example directories, not a new installation layout or implemented
CLI command. The candidate must be outside the producer source/build tree so
those trees can be made unavailable during consumer validation.

1. Identify both repository commits and any dirty source snapshots, the build
   host/target and external tools. Use the actual producer's provenance; a newer
   documentation commit does not relabel an older compiled SDK. Identify dirty
   source changes by an exact patch/export digest, not only a dirty boolean.
2. Produce/verify the Dev SDK through CRT-owned tools. Copy or extract its
   complete payload into a fresh handoff, preserving real internal symlinks.
   Record archive size/hash when packaged, a content inventory/digest, actual
   profile/manifest formats and the producer verification receipt. A directory
   path, version string or `main` reference alone is not a candidate identity.
3. Build the bounded CLI increment with only that candidate's public compiler policy,
   headers, libraries and declared resources. Record the CLI snapshot
   and exact commands when implementation supplies real entry points. Do not
   link loose CRT build objects, include private headers, or add the producer
   source with CMake. Initial manual handoff does not require a D2 installer.
4. Relocate the candidate and validate with CRT source/build trees unavailable.
   Record D0 native execution/ABI and input-inventory checks separately from
   compile-only results. SDK-free CLI execution later uses its own minimal
   single-executable contract under D1, rather than this development handoff.
5. Diagnose failures in the owning repository. An authorized producer fix is
   followed by a new SDK identity, fresh CLI configuration and replay. Preserve
   failed candidates and previous results; do not modify an accepted handoff
   in place or satisfy a missing SDK file through a source-tree fallback.

Record results under [D0 acceptance](../acceptance/bootstrap_acceptance.md#local-candidate-boundary)
and link the exact experiment in [compatibility](../compatibility.md#local-candidates-and-promotion).
Local candidate evidence may establish a bounded development result without
public release or promotion. It is not an adopted production lock or automatic
release qualification. Actual local experiment results belong to acceptance; reading the guide
alone does not grant a pass.

For coordinated publication, merge/validate the producer and promote an exact
retrievable SDK artifact, then replay the CLI against that actual artifact,
adopt its pin and publish the CLI. SDK and CLI versions/publication times are
independent. A successful pre-merge/local candidate does not validate a changed
merged artifact. Public release actions remain separately authorized.

## Implementation Sequence

Follow [D0-D7](../design/crt_cli.md#9-joint-development-sequence). D0 can start
against a manually installed immutable SDK; implementing the SDK downloader is
not required just to build the first CLI. Define the consumer/identity/result
contracts and minimal runtime closure before growing command implementations.

Build the CRT-native C/C++ CLI through the native manifest/compiler policy. Add
focused tests and real entry-point documentation with that work.
Do not independently reconstruct freestanding/startup/link flags, vendor the
producer, or pull the entire UI/Web stack into the minimal bootstrap CLI.

Before source-first port acceptance, validate the actual CRT mksh/toybox/awk/
make environment. The future native executor must preserve the reviewed JSON
recipe semantics and regression results without a Python prerequisite; optional
upstream shell/make requirements must remain explicit.

## Validation and Evidence

Use the [bootstrap acceptance](../acceptance/bootstrap_acceptance.md) for
SDK-only and runtime-closure validation. Separate fixtures, compile/link tests,
native execution, and cross compilation. A failed or skipped test is not a
passing native result.

Configuration/wiring changes need a fresh native source export and build output.
Documentation-only changes need link/anchor, formatting,
fence, and consistency checks, not a full CRT/WebKit build. Keep generated
build/download/store data outside tracked source and use bounded logs.

Use actual paths containing spaces and Unicode. Windows build/source-stage
hosts require Developer Mode; report missing prerequisites without changing
the machine configuration. Run automated child processes hidden and bounded,
track ownership, and clean up only the processes created for the job.

Record exact commands, inputs, host/target, results, skips, and limitations in
acceptance. Put completed changes and causes in HISTORY; keep TODO concise.
Do not update STATUS or duplicate the measured support matrix in README.
Follow-on self-upgrade, project SDK/new and shipping doctor/package gates are
defined in [lifecycle acceptance](../acceptance/lifecycle_acceptance.md).
Define the interfaces in D0, implement self-upgrade with D2 transactions, connect
generated-project SDK acquisition in D5 and repeat real shipping checks in D6.

## Cross-repository Work

Producer-owned SDK/ABI/host-tool changes belong to CRT, with a minimal
reproducer and explicit authorization for that repository. Link the producer
change and immutable artifact to the consumer result. Validate/promote the
producer first, then update the CLI's exact SDK lock. Do not replace an old
artifact or silently track CRT main to make a failed replay pass.

Commit/push, release publication, global environment changes, and destructive
cleanup require authorization covering those actions. This guide does not
grant it merely by describing the development workflow.
