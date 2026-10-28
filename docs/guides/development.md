# Development and Bootstrap Guide

This guide describes the initial development process. It is not an installer
guide and does not imply that the proposed `crt` commands exist.

## Before Making Changes

1. Read [AGENTS](../../AGENTS.md), [TODO](../../TODO.md), and the
   [CLI design](../design/crt_cli.md).
2. Inspect the worktree and preserve unrelated user changes. Read the relevant
   [history](../../HISTORY.md) and [acceptance contract](../acceptance/bootstrap_acceptance.md).
3. Choose one authorized, bounded increment. Record its active scope in TODO
   before moving on; do not treat the entire roadmap as implementation approval.

The repository currently contains documentation only: no `CMakeLists.txt`,
presets, implementation, or test runner has been established. Do not advertise
`cmake --build`, CTest, or CLI commands as verified entry points yet. Add the
actual configuration/build/test commands here when those entry points exist.

## Bootstrap Planning Order

Start D0 with [product decisions](../design/crt_cli.md#10-decisions-for-the-first-implementation)
and the [initial repository/template contract](../design/crt_cli.md#initial-repository-and-template-contract),
then inventory the SDK candidate and define focused identity/diagnostic
fixtures before minimal CLI implementation. Internal module directories may
evolve; the public SDK-only project/build contract and template migrations are
the compatibility boundary, not an identical frozen source tree.

Single-executable distribution is the target. D0 records actual CRT/C++/HTTP/TLS
link/import feasibility; D1 exercises SDK-free self-diagnosis and real HTTPS.
The command prefix, precise resource/helper policy, physical SDK storage default
and acquisition timing remain review decisions. Do not silently choose them
or claim a sidecar runtime satisfies the one-file goal.

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
- External LLVM/LLD, CMake/Ninja, OS/vendor SDK, and any required
  Python/Perl/pkg-config: selected paths, versions/content identities, and roles.
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
3. Build the bounded CLI increment with only that candidate's public wrappers,
   toolchain, headers, libraries and declared resources. Record the CLI snapshot
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
release qualification. No CLI experiment has been run by writing this guide.

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

Establish a minimal CRT-native C/C++ project with the installed SDK's CMake
toolchain. Add focused tests and real entry-point documentation with that work.
Do not independently reconstruct freestanding/startup/link flags, vendor the
producer, or pull the entire UI/Web stack into the minimal bootstrap CLI.

Before source-first port acceptance, validate the actual CRT mksh/toybox/awk/
make environment. An initial adapter may call the installed versioned Python
port driver, with Python declared as a host prerequisite. Do not reimplement
recipe semantics or replace JSON recipes with a TOML DSL as an initial shortcut.

## Validation and Evidence

Use the [bootstrap acceptance](../acceptance/bootstrap_acceptance.md) for
SDK-only and runtime-closure validation. Separate fixtures, compile/link tests,
native execution, and cross compilation. A failed or skipped test is not a
passing native result.

Configuration/wiring changes need a fresh build directory or fresh CMake
configuration. Documentation-only changes need link/anchor, formatting,
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
