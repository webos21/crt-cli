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
rebuilds and tests itself. `.sdk.json` may still point at a local handoff via
`sdk_root`. The implemented local installer can instead populate the shared
store and preserve a release's portable contract; see the
[current local workflow](../../README.md#try-the-local-workflow).

These are separate argv invocations, not a shell script. Supply actual absolute
paths and preserve the `ld.lld` driver name even when it is a symlink to `lld`.
The seed and resulting CLI both link CRT/C++/HTTP/TLS statically and must pass
import/runtime audits. The seed is never shipped. Uncommitted SDK candidates
require explicit `--allow-dirty-sdk true`; this is recorded in the local lock,
not relabeled as a clean release. Sources and library bytes must still match
the exact provider inventory and provenance.

Inspect child environments without changing the shell or project binding:

```text
/absolute/crt-cli env --profile control --json
/absolute/crt-cli env --profile app-build --project /absolute/hello --json
/absolute/crt-cli env --profile app-run --project /absolute/hello --json
```

Control works without an SDK. Project profiles validate the existing local
SDK/tool binding (including a compiler resource query), but build nothing.
Inherited values are redacted; these diagnostics are not shell export scripts.
`port-build` diagnostics and non-interactive `shell` require a separately pinned
host-tools bundle; they do not implement source recipe execution. See the
[format-0 environment contract](../reference/bootstrap_contract.md#child-environment-diagnostics-experimental-format-0)
and [independent host environment](../reference/bootstrap_contract.md#independent-host-environment-experimental-format-1).

For independent macOS host-tools/resource verification, use this repository's
`tools/replay_host_environment_macos.py` with fresh output/work paths and
identified CLI/bundle/package inputs. It uses Seatbelt to deny original
source/SDK/toolchain access and non-selected executable paths; Python, Clang
and OpenSSL are test-host prerequisites outside the product boundary. The
[macOS D1 record](../acceptance/host_environment_acceptance.md#macosarm64-independent-host-environment-replay-2026-11-03)
owns exact commands, package/trust pins and limits. CRT's HTTPS test supports
`--isolate-macos --hide <path>`; selected CA data must be outside denied roots
with the same pinned package bytes. Do not claim an execve trace or a new
SDK build from this replay.

For independent Windows D1 replay, CRT owns
`tools/replay_host_environment_windows.py` and
`tools/test_cli_fetch_https_windows.py`. Supply the selected native CLI,
independent bundle, resource package, SDK-built visibility probe and fresh
output/work paths. Repeat `--hide` (environment) or `--hidden` (HTTPS) for both
repositories and the exported SDK/source/provider worktree. The Windows adapter
uses restricted tokens plus exact job-image accounting, not executable-launch
denial. Python/OpenSSL are test-host tools only. Developer Mode and Windows
10 1903+ are prerequisites for real links and the UTF-8 image policy.

For HTTPS, select the exact package CA with `--trusted-ca-file` and the matching
disposable issuer using `--test-ca-directory`; the adapter copies/verifies CA
bytes into its private work. An OpenSSL installation with a missing default
config requires an explicit **test-host-only** `OPENSSL_CONF`. No global PATH,
trust-store, registry or ACL changes are required. Full actual argv, evidence
and limits belong to the [Windows D1 record](../acceptance/host_environment_acceptance.md#windowsx64-independent-host-environment-replay-2026-11-03).

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
not `/usr/bin` discovery launchers or an assumed installed LLD. When producing
the Dev SDK with actual Xcode compiler paths, set `SDKROOT` from
`xcrun --sdk macosx --show-sdk-path` for that producer job so its wrapper links
can find libSystem. This is a producer prerequisite; isolated consumer jobs
receive their vendor SDK path through the hashed compiler policy. See the
[latest macOS replay](../acceptance/bootstrap_acceptance.md#macos-latest-source-revalidation-2026-11-01).
The producer captures explicit vendor `-isysroot` while retaining CRT standard headers;
never add host include paths to CLI manifests.

CRT supplies the policy using `tools/export_cli_toolchain.py --sdk-root <dev>
--output <fresh-provider.json>`. That producer migration tool still uses the old
wrappers/Python/shell; do not count it as a CLI consumer dependency or a public
SDK schema. Retain the producer source, export tool hash, immutable SDK inventory
and provider digest separately. Do not overwrite an accepted candidate.

For generated projects, run `crt-cli new /absolute/hello --language cxx`, then
first build with explicit LLVM declarations and an installed SDK or pinned
`--sdk-source /absolute/source.json`. Plain build/test/run follows the resolved
pins. Ignored `.sdk.json` can supply LLVM paths and `sdk_source`; explicit
configure remains the rebinding/recovery entry point. New only generates sources.
The [source contract](../reference/bootstrap_contract.md#explicit-pinned-https-sdk-source-experimental-format-0)
records developer trust and installed reuse; general default acquisition is
separate work. Use the recorded explicit-configure baseline on hosts awaiting
the new first-build/acquisition replay.

New creates `include/greeting.h`, source and test directories. Add
`--kind static-library` or `--kind shared-library` for library templates, or
`--runtime-linkage shared` for a dynamically linked executable. Multiple
targets/dependencies are declared in `crt.json`; use `build --target <name>`.
The CLI itself still uses the single-executable policy. An SDK installed with
`sdk install` is selected with `configure --sdk <directory-or-identity-prefix>`, or automatically when it is the only one
that fits; the lock records its identity and no machine path. `--sdk-id <id>` (a copy under `$USER_HOME/.crt/sdks`) and
`--sdk-root` remain local handoffs.

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
uses a second SID that sees only its own copy of the CLI. On Windows/arm64 point `CRT_WINDOWS_SDK_LIBPATH` at `um/arm64`; the exporter's argv recorder needs only those Windows SDK
import libraries, not the Visual C++ ARM64 libraries. The driver follows the SDK target architecture. The driver has no
bootstrap-loop stage, so run that separately (see the
[Windows/arm64 record](../acceptance/bootstrap_acceptance.md#windowsarm64-native-replay-2026-10-30)).
HTTPS `fetch` cases run separately with CRT's `tools/test_cli_fetch_https.py --cli <crt-cli> --output <fresh dir>` (needs
OpenSSL and Python as build-host test tools; the CLI is run from a clean directory with an empty `PATH`).
Add `--publisher-cases` for the experimental explicit-key signature cases.
That option generates test-only RSA keys outside tracked sources; it neither
creates official publisher keys nor changes a trust store. Product signature
verification uses the selected CRT SDK's Mbed TLS directly, with no OpenSSL,
Python or shell subprocess. See the [authentication contract](../reference/bootstrap_contract.md#commands-and-results)
and the [D1 audit](../acceptance/bootstrap_acceptance.md#d1-publisher-and-host-environment-audit-2026-10-31)
for exact scope and prerequisites still missing from complete D1 acceptance.
The owner selected disposable test CA/signing identities for the current Linux
acceptance scope on 2026-11-01. Official publisher keys/URLs are not required for
these local test cases. The [fresh replay](../acceptance/bootstrap_acceptance.md#linux-test-identity-replay-2026-11-01)
records full-home isolation and leaves Windows/macOS Not run. No test CA/key is
installed as a system default or promoted to an official release trust anchor.
Keep the producer worktree under the user profile (a path that
public Users grants can read cannot be hidden this way) and prefer short work paths. With an SDK that has the
`crt-narrow-encoding-1` capability, Windows narrow strings are UTF-8 (CRT's UTF-8 manifest, Windows 10 1903 or later) and Unicode
and spaces work in paths. Earlier binaries/SDKs retain their recorded ANSI limits;
current sources require the new encoding capability. Treat a directory name containing `;` as
unsupported for programs that import a DLL beside them.

## Continuous Integration

`.github/workflows/ci.yml` runs on pull requests, pushes to `main` and manual dispatch with read-only
repository permissions and no secrets. It builds nothing from CRT: it consumes published artifacts.
The repository carries no executables (`bin/` was removed); CI bootstraps from a release.

1. **Bootstrap.** `tools/ci_bootstrap.py` downloads the host's published CRT-CLI Preview and checks its size and
   SHA-256 against `.github/ci/pins.json`. TLS is transport only; the pin authenticates the bytes.
2. **SDK.** The bootstrap CLI runs `sdk acquire` with a generated `crt-cli.sdk-source` carrying the pinned public Dev SDK
   manifest, archive, contract and content digests, the `github-release` redirect policy and the runner's system roots as
   the caller-chosen CA (its hash is recorded). Publisher authentication stays `not_verified`.
3. **Stage 1.** The bootstrap CLI builds a clean export of the checked-out sources (tracked and unignored files only).
4. **Tests.** The deterministic suites `native-test`, `archive-test`, `sdk-store-test` and `port-recipe-test` run on the
   **stage-1** executable. A bootstrap CLI's own test results are never evidence for the sources under test.
5. **Stage 2.** Stage 1 rebuilds another clean export (cold, nothing reused), then an unchanged rebuild must execute
   nothing. Byte equality of the two executables is reported, not required; both must behave identically.
6. **Audit.** Imports are checked per host (Linux: no `DT_NEEDED`/`RPATH`/`RUNPATH`; macOS: `libSystem` only, no
   `LC_RPATH`; Windows: `kernel32` and `api-ms-win-core-synch` only), then the copied executable runs
   `--version`, `--help` and `doctor --bootstrap` with no SDK store, an empty `PATH` and a fresh home.
   The native suite reads its scratch root from `TMPDIR`, which the driver therefore sets on every host.
   Its nested project fixtures also carry the parent's local OS SDK selection
   explicitly in `--os-sdk` or `.sdk.json` when the contract needs it.

The stage-2 executable, `SHA256SUMS`, `ci-summary.json`, logs and the generated source file are uploaded as an artifact.

CI uses `--native-test-jobs 2` for independent C/C++ project and variant cases.
The native C++ test collects its bounded case processes before changing HOME, environment
variables or SDK-store fixtures; every concurrent case has its own project root.
All existing cases, SDK/tool hashes and both cold self-builds remain required.
Each process has a private fixture root and an owned process group, a 600-second
case deadline, bounded result/log sizes and explicit exit/count validation.
Failure/deadline controls verify that other children are waited and a timed-out
case cannot write its delayed marker. The one-worker comparison runs cases in
the parent; environment/store mutation always stays serial. Thread-based
workers failed the Windows experiment and are not used by the retained runner.
The driver records eleven `native_phases` with check counts and durations and
rejects missing/duplicate/incomplete sections or a sum different from the final
native count. `tools/test_ci_bootstrap.py` exercises those report failure gates.

For a serial comparison, pass `--native-test-jobs 1`; the driver accepts 1-4.
It writes test-only local `.crt/native-test-settings.json` with `format: 0` and
`jobs`. A normal native-test invocation without this file uses one worker.
This is test-harness state, not a product manifest, SDK schema or public command.
The [process-worker qualification](../acceptance/ci_acceptance.md#hosted-process-worker-qualification)
records the complete counts, failed thread prototypes and measured timing limits.

The five `port-*` suites (except `port-recipe-test`) print `skipped` and exit 0 without the machine-local
`CRT_CLI_TEST_PORT_*` inputs, so they are **not** in the PR set: the driver fails if a suite prints `skipped`, and also
if `crt.json` declares a test that `pins.json` classifies neither as PR-set nor as input-gated. They belong to a
future `integration.yml` with pinned independent host tools.

Run the same driver on a native host with LLVM installed (it creates `--work`, which must be new or empty, and sets
`HOME` inside it, so the default SDK store is untouched):

```text
python3 tools/ci_bootstrap.py --work /absolute/new-dir --llvm-bin /usr/lib/llvm-21/bin
```

On macOS the Xcode tools come from `xcrun -f`; on Windows pass `--llvm-bin` and, if the newest installed Windows SDK is
not wanted, `--os-sdk`. `--tests` restricts the suites for a quick local pass; CI always runs the pinned set.
The Python driver is a build-host orchestrator like the other `tools/*.py` replays, not a CLI prerequisite.

Pins change only in a reviewed commit that cites the release record and re-checks the asset digests against the
published `SHA256SUMS`; never use a moving tag or `latest`. Linux/x86_64 uses explicit `ubuntu-26.04` and LLVM 21 from
the Ubuntu archive, matching the local rehearsal's host generation. Linux, macOS/arm64 and Windows/x64
are required checks. The Ubuntu 24.04 shared-runtime loader failure remains a
producer compatibility limit, not a qualified platform. A suite failure
no longer stops the later stages, so one run reports everything that is broken. The [TODO](../../TODO.md#github-ci) owns the
open legs and the integration/release workflows. [CI acceptance](../acceptance/ci_acceptance.md) records what has actually
been run, including the first hosted run and the Linux loader gap it found.
The [Ubuntu 26.04 hosted qualification](../acceptance/ci_acceptance.md#ubuntu-2604-hosted-linux-qualification-2026-11-10)
passes all Linux stages; its runner setting is retained. This does not qualify
the older Ubuntu 24.04 shared-runtime path.

The reviewed `src/test/data/zlib.recipe.json` fixture is checked out with LF
through `.gitattributes` on every host. Recipe verification hashes exact bytes;
line-ending conversion produces a different, unreviewed input.
The [Windows hosted qualification](../acceptance/ci_acceptance.md#windows-hosted-qualification-2026-11-10)
records a complete passing run of all three required legs, including the common
fixture regressions. Windows/macOS byte differences remain reported separately.

## Replay the Local SDK Installer (D2)

`crt-cli sdk install|list|verify|recover` is replayed per host with a driver in `tools/` that runs every CLI call in the host's
isolation: `replay_sdk_install_linux.py` (bubblewrap), `replay_sdk_install_macos.py` (Seatbelt) and `replay_sdk_install_windows.py`
(restricted token, private work directory, job-object process accounting). The input is the dev archive and manifest of a
`crt-dev-release` run (a rehearsal tag such as `v0.0.1-rehearsal.3`; nothing is uploaded), and the CLI is the self-built SDK-only
executable of that same SDK. The Windows driver needs the producer's `tools/` for `crt_windows_external`/`crt_windows_trace`:

```powershell
$env:PYTHONUTF8 = "1"
python C:/absolute/crt-cli/tools/replay_sdk_install_windows.py `
  --cli C:/absolute/bare/crt-cli.exe --release-dir C:/absolute/build/release/dev `
  --probe C:/absolute/probe.exe --crt-tools C:/absolute/crt/tools `
  --hide C:/absolute/crt --hide C:/absolute/crt-cli/src --hide C:/absolute/producer `
  --work "C:/absolute/fresh work dir" --output C:/absolute/fresh-evidence
```

`--work` and `--output` must not exist. The probe is the SDK-built executable of the contract replay; it has to be denied every
`--hide` root. On a Windows Development SDK the producer must first pass `crt-development-sdk-test`, which needs CRT with the
xz runtime recorded as `bin/liblzma-5.dll` (`1ee0379` or later). Windows offers no network namespace, so the Windows record does not claim network denial. See the
[Windows/x64 record](../acceptance/sdk_install_acceptance.md#windowsx64-isolated-replay-2026-11-04).

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

Follow [D0-D7 capability dependencies and the adopted priorities](../design/crt_cli.md#execution-priorities-adopted-2026-11-05).
D0 can start against a manually installed immutable SDK; implementing the SDK downloader is
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

Choose focused, integration and native-matrix gates using the
[change-impact policy](../design/crt_cli.md#validation-by-change-impact).
Documentation-only updates need link/consistency checks, not SDK regeneration.
For implementation, record changed modules/contracts, affected pairs and the
specific tests that justify the result. Do not repeat unchanged passing gates
without a failure or unresolved concern. Relevant parser/transaction/trust/
PAL/ABI failure and recovery checks still run before merge. Feature-bundle and
release qualification covers every advertised pair; periodic/nightly automation
is not implemented by this policy. Use acceptance for detailed receipts and
HISTORY for a short completion/link, keeping TODO concise.

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
Update STATUS only on explicit request; do not duplicate the detailed measured
support matrix in README.
Follow-on self-upgrade, project SDK/new and shipping doctor/package gates are
defined in [lifecycle acceptance](../acceptance/lifecycle_acceptance.md).
Define the interfaces in D0; self-upgrade is a later D2 transaction increment,
outside the first preview's critical path. Connect
generated-project first-build acquisition as the basic D5/D2 flow now, extend
broader workflows later in D5 and repeat real shipping checks in D6.

## Selected Common-source Technical Candidate

The [D0.7 selection](../compatibility.md#common-source-candidate-selection-2026-11-02)
binds a clean producer SDK, a separately hashed provider and the exercised CLI
inputs. Its [consumer record](../acceptance/bootstrap_acceptance.md#common-source-candidate-replay-2026-11-02)
contains the actual Linux replay and old-binary bootstrap-loop commands/receipts.
The producer owns [source/cache validation and regeneration](https://github.com/webos21/crt/blob/main/docs/acceptance/release_qualification.md#common-source-candidate-selection-2026-11-02).

Use the exact copied SDK/provider in the ignored `.sdk.json` for this local
candidate; do not turn an unpromoted candidate into a production root lock.
Source-stage regeneration uses the frozen clean producer revision and a new
version/output, never a mutable checkout or replacement of old evidence.
A common archive selects CPU-specific code/options from the SDK target;
binary SDKs remain specific to the target architecture. The ARM result in
this increment is cross compile/package/link, not native execution. Continue
D1 host-tools/environment work independently of public release qualification.

## Cross-repository Work

### Local host-tools and bootstrap data candidates

Use CRT's independent host-tools assembler/CMake entry point with a verified
native public C SDK, then supply that bundle's manifest SHA-256 explicitly to
the [implemented env/shell/doctor commands](../reference/bootstrap_contract.md#independent-host-environment-experimental-format-1).
The source SDK is a producer input; running those tools needs no SDK. Optional
`--project` adds the separately verified native SDK identity and CRT_SYSROOT;
it does not install ports or construct a compiler-wrapper bridge.

For a local bootstrap data package, producer tooling can run:

```sh
python3 tools/create_bootstrap_resources.py --binary <native-crt-cli> \
  --sdk-root <verified-dev-sdk> --output <new-package> --ca-file <explicit-ca.pem>
```

Record the emitted manifest SHA-256 independently. Diagnose with
`doctor --scope self --resources <package> --resources-sha256 <pin>`;
optional paired public-key/signature inputs authenticate that exact manifest.
Private keys are test-host inputs, never package contents. `fetch` still needs
an explicit CA file. This local assembler is not a shipped Python dependency,
installer, updater or release-publishing command. New output paths preserve
prior candidates; the Linux replay entry point and remaining native-host
checks are in [D1 acceptance](../acceptance/host_environment_acceptance.md).

### Review and authorization

Producer-owned SDK/ABI/host-tool changes belong to CRT, with a minimal
reproducer and explicit authorization for that repository. Link the producer
change and immutable artifact to the consumer result. Validate/promote the
producer first, then update the CLI's exact SDK lock. Do not replace an old
artifact or silently track CRT main to make a failed replay pass.

Commit/push, release publication, global environment changes, and destructive
cleanup require authorization covering those actions. This guide does not
grant it merely by describing the development workflow.

## Windows D3 replay

Run the producer's `tools/replay_cli_contract_windows.py` first, using the native
Windows entry point above. Keep its fresh workspace, `inputs.json`, `commands.json`
and raw logs, including a failed aggregate result when applicable.
The contract replay builds the declared tests, but environment-gated port suites
without their inputs print `skipped`; that is not source-port acceptance.

`tools/replay_port_windows.py` then supplies the pinned independent CRT host-tools
bundle and zlib archive to those suites in the same disposable workspace. Supply
the workspace's unregistered build SID (visible in `Get-Acl <work>`); do not use a
real account SID or change existing account/tool/repository ACLs. Every invocation
uses the restricted token, empty PATH, hidden-root probe and process accounting.
For a test-fixture compilation failure, retain the failed aggregate receipt. The
follow-up checks the completed bootstrap steps and unchanged product/SDK inputs,
records test-fixture synchronization, builds each requested suite and performs the
bare checks separately. It does not relabel the original aggregate as passing.

```text
python tools/replay_port_windows.py --contract-evidence C:/evidence/contract \
  --source C:/absolute/crt-cli \
  --crt-tools C:/absolute/crt/tools --host-tools C:/absolute/host-tools \
  --host-tools-sha256 <manifest-sha256> --archive C:/absolute/zlib-1.3.1.tar.gz \
  --sid <workspace-build-sid> --hide C:/absolute/crt --hide C:/absolute/crt-cli \
  --hide C:/absolute/original-sdk --output C:/evidence/fresh-d3
```

The example is one argv invocation; line-continuation syntax depends on the host
shell. The driver keeps per-suite stdout/stderr, exit codes and process images.
It returns nonzero when a D3 suite is unsupported, fails or skips. Inspect the
[Windows implementation result](../acceptance/sdk_install_acceptance.md#source-port-d3-windowsx64-implementation-and-replay-2026-11-09)
before interpreting a passing bootstrap as D3 support. Python is a replay-host
tool, not a CLI/runtime prerequisite. No network or executable-denial boundary is
claimed on Windows.

### Final Windows D3 replay inputs

Use a new contract workspace/output on each full restart; an existing work directory is
refused. A D3 follow-up may reuse the verified workspace but needs a fresh output directory.
The driver allows generated recipe executables under the test HOME's port jobs and copies
its replay-host Python runtime into the workspace for the packaged oracle. This grants no
Python dependency to the CLI. Keep the interpreter's original installation/ACLs unchanged.

## D3 common-change closure and native handoff

The 2026-11-09 closure baseline is CRT `21b9d0e` / CLI `2ae74d4` (resolve full
commits in the acceptance input record). Documentation refresh does not approve
the adapter or requalify the SDK. The user runs Linux here and macOS on a native
macOS host. Those deltas and the [bounded approval/adoption](../compatibility.md#d3-bounded-adapter-adoption-2026-11-09)
are now recorded; `crt_review` is `approved` for new items and retained pending
items must not be rewritten. Windows/x64's final native record remains the baseline
unless a changed policy/product input affects it. Windows/arm64 D3 is separate.

1. Reuse an identified public Dev SDK/provider and independent CRT host-tools
   bundle outside original checkouts. Verify recipe/test/source digests, tools,
   SDK and bundle inventories; preserve any dirty/unpromoted provenance.
2. Use a fresh CLI export/output and actual native tool paths. Run the affected
   recipe/build/store/project/command/oracle suites with input environment set;
   their skipped environment-free pass is not D3 proof. Run native project
   regression for common project changes, bare CLI/import checks and one actual
   port-command publication/reuse cycle. Inspect static/shared provider and lock
   evidence. The packaged Python driver runs only as a separately labeled oracle.
3. Keep raw argv/stdout/stderr, source/CLI/SDK/provider/host-tools/archive hashes,
   key/cache results and unchanged inventories. Do not relabel older failures,
   rebuild unrelated GFX/Web or refresh root locks/defaults/binaries.

For Linux, `tools/replay_port_linux.py` accepts the same identified source/SDK/
provider/host-tools/archive/seed and explicit LLVM inputs, plus fresh output and
work-parent paths outside hidden trees. It runs the six input-enabled D3 suites,
native project regression, source-hidden probe, SDK-free/import checks and actual
port publish/reuse/verify. `--allow-dirty-sdk` is an explicit developer-only opt-in
when the retained SDK really is dirty; it is recorded, never treated as release
qualification. Inputs/results and execve traces retain the separate oracle scope.
No environment-gated skipped suite counts as a pass.

For macOS, reuse `tools/replay_port_macos.py` on the same CLI product baseline:

```text
python3 tools/replay_port_macos.py --source /absolute/crt-cli --sdk-root /absolute/dev --contract /absolute/provider.json --contract-sha256 <sha256> --host-tools /absolute/host-tools --host-tools-sha256 <sha256> --archive /absolute/zlib-1.3.1.tar.gz --seed /absolute/native-crt-cli --output /absolute/fresh-evidence --work-parent /private/tmp --cc /absolute/clang --cxx /absolute/clang++ --linker /absolute/ld --ar /absolute/ar --hide /absolute/crt --cases port-recipe-test port-build-test port-store-test port-project-test port-command-test port-oracle-test native-test
```

Use real per-host paths/hashes, not Linux's binary/tool/provider pins. The SDK
must contain the reviewed zlib recipe/test and oracle. The seed must understand
the project's current manifest. Archiver/OS-SDK strings still require one shell
word for the recipe's Makefile. Fresh output/workspace is preferred; continuation
is allowed only through the driver's full input checks. If fixtures/product
change, retain the failed input and identify exactly what the new run covers.

After Linux results are recorded, the acceptance handoff records the exact
portable product/test snapshot and any product changes that macOS must replay.
A metadata-only review decision must leave adapter build keys unchanged; policy
changes instead need new keys/items and affected-host evidence before adoption.

Inspect the shared SDK-provider control's mapped image, not only its exit/count.
The retained Linux developer provider can resolve the C shared SDK control to
host `libz`; that historical diagnostic is not endorsed SDK shared support.
Positive port-provider mapping and static SDK-provider evidence are separate.
Record an unsupported mode/producer-policy follow-up rather than calling a host
fallback a successful SDK binding. This boundary participates in final contract
review even when the bounded D3 suite's own assertions pass.

Linux closure is now [recorded](../acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09)
with the unchanged `2ae74d4` product/test baseline. Before native macOS replay,
compare the normalized source fingerprint to
`22720f7d9e223af06e41887ec0bed9ede55568ecdd6b9b5ef3499769893bee16`.
From the CLI checkout, this host-only check uses the Linux driver's portable
inventory helper (it does not run the Linux main function):

```text
python3 -c 'from pathlib import Path; import hashlib,json; from tools.replay_port_linux import source_inputs; print(hashlib.sha256(json.dumps(source_inputs(Path.cwd()),separators=(",",":")).encode()).hexdigest())'
```

Use a fresh macOS output/workspace with the seven `--cases` listed above. Return
`inputs.json`, `result.json`, `commands.json`, source/CLI/provider/SDK/host-tools
identities, mapped-image observations and all retained failures. Do not adopt
Linux's dirty SDK/provenance or compiler policy merely to match its binary pin.
The macOS artifact/code result and supported/excluded SDK modes remain distinct
from final CRT approval and any preview/publication decision.

The [native macOS continuation is now recorded](../acceptance/sdk_install_acceptance.md#macos-d3-common-change-closure-replay-2026-11-09).
The driver records the same normalized source fingerprint as Linux, copied/original input inventories, external tool
and driver hashes, SDK/source/provider/bundle-free CLI checks and the actual `port build`/reuse/verify cycle. Its strict
native path allowlist and denial probes are distinct from the oracle-only interpreter/SDK-tool exception; it does not
provide an execve trace. Use the accepted input/result/qualification receipts, not a path or a suite count alone, at
the recorded producer/adoption decision. Do not repeat unchanged gates merely for the review-marker edit.


## First preview input qualification

The first candidate is bounded to native Linux/x86_64 Dev `02-cxx` and the
implemented CLI first-user workflow. D3 technical adoption does not promote an
SDK, prove publisher trust or turn old developer fixtures into release inputs.
Before collecting the candidate:

1. Choose a clean committed CRT producer and explicit prerelease version. Produce
   a fresh Dev candidate using the producer's existing `crt-development-sdk-test`
   and `crt-dev-release` gates, retaining actual archives, checksums, source offer,
   manifests and portable compiler contract. A local version parameter creates
   neither a public release nor a Git tag. Do not rebuild Full/Web for this gate.
2. Verify/extract that exact archive into an immutable external handoff, preserve
   SDK and provider identities separately, and declare exact LLVM binaries.
   Rebuild the CLI from a fresh export/output against those public inputs; never
   use a sibling build tree or update the repository's old lock implicitly.
3. Fix the candidate's supported workflow before final-package replay: new C/C++
   sources, pinned install/acquisition or installed-SDK first build, test/run,
   static SDK control and optional source-port zlib with pinned CRT host tools.
   Linux C shared SDK-provider mode is excluded until CRT Tranche 6.4 resolves
   compiler/runtime-search policy with new evidence. Port-provider shared proof
   does not qualify that SDK-provider mode.
4. Bind final CLI executable, licenses/notices/resources, explicit CA/trust inputs,
   OS/LLVM prerequisites and source-hidden/SDK-free execution receipts. Existing
   developer test CAs and unsigned checksums are not publisher authentication.
   Every advertised host needs final-package evidence at its own exact inputs.
5. Promote the verified producer artifact first, verify its actual promoted bytes,
   then adopt that SDK pin in the CLI and prepare CLI delivery with independent
   versions. Publication and trust decisions remain separately authorized.

The retained Linux D3 SDK has dirty development provenance and the host-tools
bundle is local-only; preserve both for reproducible technical regression. They
are not the first preview release candidate. Full delivery, D4 dependency ports,
general updates/self-upgrade and Studio do not block this bounded preparation.


The [Linux local input rehearsal](../acceptance/preview_acceptance.md#linux-local-candidate-2026-11-10)
now records a clean SDK/archive/policy and exact CLI/resource/archive combination.
Resume from those preserved inputs and receipts; never overwrite a failed or
accepted candidate. Producer source publication, official trust and actual asset
promotion remain separate from the successful local first-user rehearsal.


The SDK is now [published and exercised from public bytes](../acceptance/preview_acceptance.md#public-sdk-promotion-and-consumer-adoption-2026-11-10).
Use its manual download/pinned `sdk install` path with `--sdk` to select the
installed portable policy. GitHub asset redirects are followed only with the explicit
`--redirect-policy github-release` / source `redirect_policy` opt-in
([contract](../reference/bootstrap_contract.md#restricted-https-redirects-experimental-cli-owned));
the default stays refusal. Replay with `tools/replay_redirect_linux.py --cli <crt-cli> --release-dir <public-download> --output <new-dir>`
(Python 3, OpenSSL, loopback only) after any change to `src/main/http.cpp`, `fetch.cpp` or `sdk_acquire.cpp`.

## Windows Preview transport and package replay

`tools/replay_redirect_linux.py` also runs natively on Windows: supply the `.exe`,
the downloaded Windows release files and a fresh output. Its explicit local PKI config
avoids a vendor OpenSSL default-path dependency. Python/OpenSSL are test-host tools.

`tools/replay_preview_public_windows.py` keeps its pinned local-install mode. Add
`--sdk-source <pinned-json> --package <extracted-package> --package-manifest-sha256 <pin>`
to exercise the packaged executable's real HTTPS acquisition before the existing C/C++
workflow. It copies the caller CA into the restricted workspace; no global trust changes.
For byte-identical public archive smoke use `--acquire-only`; the result explicitly records
that app workflows were omitted. Retain the complete earlier app receipt and exact binary hash.
Set the explicit `CRT_WINDOWS_SDK_LIBPATH`; pass a fresh workspace/output, a native visibility
probe, hidden source/SDK roots and declared LLVM paths as in the existing Windows procedure.
