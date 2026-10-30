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
CLI build/test path. The first seed must now also use CRT: PAL queries and
JSON/SHA-2 come from the SDK, not host libraries or CLI-vendored headers. The
old host-C++ seed command is obsolete. With an already produced native seed:

```text
/absolute/native-seed configure --project /absolute/crt-cli --sdk-root /absolute/handoff/dev --sdk-contract /absolute/handoff/native-toolchain-v1.json --contract-sha256 <exact-provider-sha256> --cc /absolute/clang --cxx /absolute/clang++ --linker /absolute/ld.lld --ar /absolute/llvm-ar
/absolute/native-seed build --project /absolute/crt-cli
/absolute/crt-cli/out/native/crt-cli test --project /absolute/crt-cli --json
/absolute/crt-cli/out/native/crt-cli doctor --bootstrap --json
```

To avoid repeating those flags, keep them in an ignored `/absolute/crt-cli/.sdk.json`
([format](../reference/bootstrap_contract.md#local-sdkjson)). A CLI that already
supports it then needs only `crt-cli configure --project /absolute/crt-cli`; the
seed that predates it still needs the explicit flags once. The goal of the
bootstrap loop is: a changed SDK plus an already built `crt-cli` produce a new
`crt-cli`, even when the change follows a CRT contract change, and the result
rebuilds and tests itself. Until the SDK installer exists (D2), `.sdk.json`
points at a local handoff via `sdk_root`; `$HOME/.crt/sdks` is not populated.

These are separate argv invocations, not a shell script. Supply actual absolute
paths and preserve the `ld.lld` driver name even when it is a symlink to `lld`.
The seed and resulting CLI both link CRT/C++/HTTP/TLS statically and must pass
import/runtime audits. The seed is never shipped. Uncommitted SDK candidates
require explicit `--allow-dirty-sdk true`; this is recorded in the local lock,
not relabeled as a clean release. Sources and library bytes must still match
the exact provider inventory and provenance.

For native macOS, CRT supplies a producer-side acceptance driver that builds
the initial CRT-native seed from provider argv, then runs native self-build,
tests and bare-binary checks under Seatbelt:

```sh
python3 /absolute/crt/tools/replay_cli_contract_macos.py \
  --cli-source /absolute/crt-cli --sdk-root /absolute/handoff/dev \
  --output /absolute/evidence \
  --clang /absolute/XcodeToolchain/usr/bin/clang \
  --clangxx /absolute/XcodeToolchain/usr/bin/clang++ \
  --linker /absolute/XcodeToolchain/usr/bin/ld \
  --ar /absolute/XcodeToolchain/usr/bin/ar
```

That Python process is a build-host test orchestrator outside the product
sandbox, not a CLI prerequisite. All CLI/generated build commands remain
native argv. Select actual Xcode tools (`xcrun -f <tool>` before isolation),
not `/usr/bin` discovery launchers or an assumed installed LLD. The producer
captures explicit vendor `-isysroot` while retaining CRT standard headers;
never add host include paths to CLI manifests.

CRT supplies the policy using `tools/export_cli_toolchain.py --sdk-root <dev>
--output <fresh-provider.json>`. That producer migration tool still uses the old
wrappers/Python/shell; do not count it as a CLI consumer dependency or a public
SDK schema. Retain the producer source, export tool hash, immutable SDK inventory
and provider digest separately. Do not overwrite an accepted candidate.

For generated projects, run `crt-cli new /absolute/hello --language cxx`, select
the same local SDK explicitly with configure, then run test/build/run with
`--project /absolute/hello`. New only generates sources. Missing SDK selection
currently fails clearly; automatic acquisition remains planned.

New creates `include/greeting.h`, source and test directories. Add
`--kind static-library` or `--kind shared-library` for library templates, or
`--runtime-linkage shared` for a dynamically linked executable. Multiple
targets/dependencies are declared in `crt.json`; use `build --target <name>`.
The CLI itself still uses the single-executable policy. Installed SDK selection
can use `configure --sdk-id <id>` under `$USER_HOME/.crt/sdks`; an explicit
`--sdk-root` remains available for a local handoff, not an installer.

Fresh isolated Linux replay must export sources and SDK outside producer trees,
hide producer/original inputs, make Python/shell/CMake/Ninja executables unavailable,
trace actual execve calls and run native self-build/tests. Bare-binary checks then
hide the relocated SDK/source/output as well. Keep logs, exact argv, source/export
hashes, ELF imports, mapped images and unchanged inventories. Linux evidence does
not grant macOS/Windows acceptance. The previous CMake receipts remain historical.

The Linux counterpart uses native Bubblewrap, strace and readelf:

```sh
python3 /absolute/crt/tools/replay_cli_contract_linux.py \
  --cli-source /absolute/crt-cli --sdk-root /absolute/handoff/dev \
  --output /absolute/fresh-evidence \
  --clang /usr/bin/clang --clangxx /usr/bin/clang++ \
  --linker /usr/bin/ld.lld --ar /usr/bin/llvm-ar-21
```

Choose installed LLVM paths and a fresh evidence directory. Repeat `--hide`
for additional producer checkouts; the producer/CLI source roots are hidden
by default. The producer-side Python orchestrator is outside the tested
consumer namespace. The
[current Linux record](../acceptance/bootstrap_acceptance.md#linux-revalidation-after-windows-changes-2026-10-30)
identifies the required PAL process-group fix and explicit dirty-SDK acceptance;
an older SDK that rejects SETPGROUP cannot run the new common CLI runner.

On macOS, use a native Seatbelt file/exec policy instead of Linux bubblewrap.
The accepted replay denies producer-tree reads, permits only selected native
tools/CLI/generated executables, and probes actual failures of shell, Python,
CMake, Ninja, make and SDK-wrapper execution. The bare replay additionally denies
the relocated SDK, project, provider and compiler/linker execution. Emit literal
UTF-8 policy paths: JSON `\u` escapes do not represent Unicode SBPL paths.
This is OS-enforced denial, not merely a filtered PATH or renamed directory.
The [macOS evidence](../acceptance/bootstrap_acceptance.md#native-json-builder-on-macosarm64-2026-10-29)
records policies, failed attempts and actual commands; it does not claim a
macOS execve trace or Linux mount-namespace semantics.

On native Windows, CRT supplies the counterpart driver. It needs the Windows SDK import-library directory in
`CRT_WINDOWS_SDK_LIBPATH` and real LLVM tools (`clang`, `clang++`, `ld.lld`, `llvm-ar`):

```powershell
python C:/absolute/crt/tools/replay_cli_contract_windows.py `
  --cli-source C:/absolute/crt-cli --sdk-root C:/absolute/handoff/dev `
  --hidden C:/absolute/producer-worktree --output C:/absolute/evidence `
  --clang "C:/Program Files/LLVM/bin/clang.exe" --clangxx "C:/Program Files/LLVM/bin/clang++.exe" `
  --linker "C:/Program Files/LLVM/bin/ld.lld.exe" --ar "C:/Program Files/LLVM/bin/llvm-ar.exe"
```

It runs every consumer process under a restricted token (producer trees unreadable through the per-run SID, `PATH` empty,
stdio redirected) and records the executables each run starts through a job object: accounting, not denial. The bare phase
uses a second SID that sees only its own copy of the CLI. Keep the producer worktree under the user profile (a path that
public Users grants can read cannot be hidden this way) and use short, ASCII work paths: Windows narrow paths are ANSI
bytes and the imported libc++ rejects non-ASCII narrow/wide conversion today. Treat a directory name containing `;` as
unsupported for programs that import a DLL beside them.

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
