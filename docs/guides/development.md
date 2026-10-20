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

## Cross-repository Work

Producer-owned SDK/ABI/host-tool changes belong to CRT, with a minimal
reproducer and explicit authorization for that repository. Link the producer
change and immutable artifact to the consumer result. Validate/promote the
producer first, then update the CLI's exact SDK lock. Do not replace an old
artifact or silently track CRT main to make a failed replay pass.

Commit/push, release publication, global environment changes, and destructive
cleanup require authorization covering those actions. This guide does not
grant it merely by describing the development workflow.
