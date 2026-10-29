# CRT-CLI: Development Environment and Source-first Ports

Adopted design baseline: 2026-10-20. This repository owns ongoing CLI design.
The initial text was adopted from the [cross-repository CRT design](https://github.com/webos21/crt/blob/d21430ddeb59aea76cf6abd58c15a69f55cfda52/docs/design/crt_cli.md)
at CRT revision `d21430ddeb59aea76cf6abd58c15a69f55cfda52`.
That pinned document records the integration baseline; do not maintain two
independently edited mirrors. Record subsequent CLI decisions here and coordinate
SDK-boundary changes with the producer. The
[current native-provider review](../compatibility.md#committed-source-checkpoint-2026-10-29)
records the later producer contract and matching CLI implementation commits.

The full workflow, stable project/lock schemas and Host Tools Bundle remain
implementation targets. The bounded native bootstrap is described in the
[implemented contract](../reference/bootstrap_contract.md). The [SDK Development Contract](https://github.com/webos21/crt/blob/d21430ddeb59aea76cf6abd58c15a69f55cfda52/docs/design/sdk_development_contract.md)
owns producer boundaries; [CRT Studio](https://github.com/webos21/crt/blob/d21430ddeb59aea76cf6abd58c15a69f55cfda52/docs/design/crt_studio.md)
owns the future GUI design. Current work belongs in [TODO](../../TODO.md),
completed work in [HISTORY](../../HISTORY.md), and measured bootstrap evidence
in [bootstrap acceptance](../acceptance/bootstrap_acceptance.md).

## 1. Adopted Direction and Superseded Proposals

CRT-CLI is an independent development tool built against CRT libc/PAL and its
C++ runtime. The intended application workflow is shown below; command syntax
is a v1 draft.

```sh
crt-cli sdk install --profile cxx
crt-cli doctor
crt-cli new hello --language cxx
cd hello
crt-cli add zlib
crt-cli build
crt-cli run
```

The Rust reference concerns the **toolchain-selection, project-declaration,
and lock-file experience**. It does not mean implementing the CLI in Rust or
converting C/C++ libraries into Rust crates. Additional ports use
Homebrew-style upstream fetch/recipe/local builds, with stricter SDK, ABI,
and variant identities for C/C++.

The final direction supersedes these earlier proposals:

- Complete the IDE/Desktop UI first -> deliver the CLI, installation, and port engine first.
- Include the CLI in Studio -> separate `crt-cli`, with Studio as a later consumer.
- Require a host-native Go/Rust CLI -> CRT-native CLI plus bootstrap runtime closure.
- Mix independently versioned SDK stages like crates -> one validated SDK combination, separate port dependencies.
- Start with a binary-port registry -> source-first, local cache, then a trusted binary cache.

The initial scope excludes a general-purpose C/C++ package manager, a public
registry server, automatic execution of arbitrary recipes, a complete LLVM
port, and an IDE plugin marketplace.

## 2. Product and Internal Boundaries

```text
CLI / CI / AI / external IDE / future Studio
                 |
        crt-cli commands + versioned JSON contract
                 |
  SDK resolver / environment resolver / project runner
                 |
  port resolver -> fetch/verify -> recipe executor -> immutable store
                 |
       installed SDK wrapper/toolchain/runtime contract
```

Separate the SDK/port installation control plane from port-build and app-run
environments. The CLI delegates technical policy to the SDK and supplies common
process execution, cancellation, and result collection. Do not reimplement
compile/link flags, startup order, or the Windows exception ABI in individual
commands. Logs must expose actual tools, argv, and environment selections.

Product sources target CRT-native C/C++. The owner selected a native JSON
manifest-driven builder on 2026-10-28: no CMake, Python or shell dependency in
CLI control/self-build or generated app build/test/run. Driver logic moves inside
the CLI; SDK-owned declarative policy retains ABI/startup/link ownership.
Upstream configure/make ports may still need explicitly selected CRT shell/tools;
the native recipe engine must preserve reviewed producer semantics.

### Initial Repository and Template Contract

The owner selected the following initial structure. Modules may evolve; the CLI
and generated applications share the native project/build contract.

```text
crt-cli/
  crt.json                   experimental JSON project declaration
  crt.lock                   exact project inputs; no promoted root pin yet
  src/main/main.cpp          command dispatch
  src/main/{core,process,toolchain,project,bootstrap}.cpp
  src/test/native_test.cpp   C/C++ tests, built through the same backend
  include/crtcli/             project-owned module headers
  third_party/                dependency-ownership note; JSON/SHA come from SDK
  tools/bootstrap_host.cpp   temporary stage-zero native seed; never shipped
  docs/
  .crt/                      ignored machine-local project binding
  out/                       ignored native build output/receipts
```

`new` creates `src/main/main.cpp`, a separate source/header pair, and a C/C++
test in `src/test/` (or `.c` equivalents with `--language c`). Source lists,
include directories and test targets are explicit JSON fields, not shell commands.
The initial schema is format 0; see the [implemented contract](../reference/bootstrap_contract.md).
A temporary hosted C++ seed uses the same native backend to build the first
CRT-native CLI. Subsequent self-build/tests use `crt-cli`, with no seed/runtime
launcher exemption in the distributed product. LLVM remains an external tool.
Template migrations and regressions preserve existing projects; CLI upgrades do
not rewrite sources or project pins implicitly.

### Single-Executable Distribution Goal

The intended CLI distribution is one native executable per supported host,
with non-OS CRT/C++/HTTP/TLS dependencies linked into it. This replaces the
earlier minimal-dynamic-bundle default as the product goal. Linux feasibility
is recorded; macOS/Windows native proof remains pending. OS/PAL dependencies remain permitted.
LLVM, project SDKs, CRT host tools and port-build interpreters are separate
development assets, not required merely to start the CLI control commands.

D0 checks linking/imports and minimal execution; D1 verifies SDK-free execution,
diagnostics and the actual HTTPS stack. A sidecar CRT runtime cannot silently
count as a successful one-file result. Record a limitation and review the
contract if a native target cannot meet it. The selected data/helper rules are
below; trust provisioning, notices and exact update activation still need evidence.

## 3. CRT_HOME and Immutable Installations

Proposed default user data locations follow. System-wide/administrator
installation is not the initial default.

| Host | Default data root |
| --- | --- |
| Windows | `$USER_HOME/.crt` (`USERPROFILE` through CRT PAL) |
| Linux | `$USER_HOME/.crt` (`HOME` through CRT PAL) |
| macOS | `$USER_HOME/.crt` (`HOME` through CRT PAL) |

The owner fixed SDK installations at `$USER_HOME/.crt/sdks` on every host on
2026-10-29, superseding the earlier per-OS root proposal. Overrides for other
CLI/cache data remain future design. A space-free home path is not required.

```text
CRT_HOME/
  bin/                         optional stable executable entry/reference
  cli/<cli-version>-<digest>/   CLI executable + agreed resource/metadata policy
  host/<host-tuple>/<tools-id>/ manifest + existing rootfs-relative layout
  toolchains/<toolchain-id>/    external LLVM/OS SDK paths, versions, fingerprints
  sdks/<sdk-id>/
                               manifest.json, include/, lib/, bin/, system/,
                               libexec/, tools/, share/, crt-toolchain.cmake
  ports/store/<build-key>/      include/, lib/, bin/, manifest, notices, evidence
  registry/<revision>/         verified, pinned recipe/index snapshot
  sources/<source-digest>/      verified original archive / pristine source
  builds/<job-id>/              per-job source copy, build/install staging, rootfs
  downloads/                   incomplete downloads and verified cache
  cache/                       regenerable data
  logs/                        bounded job logs and diagnostics
  state/                       active pointers, install transactions, references
```

This is a logical target layout, not an instruction to rearrange existing SDK
internals. In particular, do not assume `crt-toolchain.cmake` lives in a new
`cmake/` directory.
The SDK subtree is the selected shared immutable storage model. Its installer
and stable on-disk schema remain D2 work, not an implementation assumed here.

SDKs, host tools, CLI versions, and ports have independent installation
identities and lifetimes. Updates install a new directory and atomically change
a selection pointer. Never overwrite an in-use executable, runtime DLL, or SDK.
Removing an SDK must not remove the CLI itself. Initial bootstrap must also
work through OS archive extraction and direct CLI execution from a version
directory. Stable selection must not introduce a permanent auxiliary executable.
The selected policy allows bounded temporary
update helpers and previous-version retention are allowed.

Installation proceeds through TLS/origin verification, digest checks, safe
temporary extraction, inventory/ABI validation, and atomic publication on the
same filesystem. Reject traversal, absolute/drive paths, escaping symlink
targets, case collisions, Windows reserved paths, and extraction-size abuse.
Preserve safe, real internal SDK symlinks. Fail clearly when Windows Developer
Mode is missing; do not change it automatically.

Serialize concurrent installations with per-identity locks and recover
interrupted transactions. Failures preserve previous installations and project
locks. Cache cleanup/GC respects usage, lock references, and active jobs; it
must not arbitrarily delete installed SDKs or project sources.

### Project SDK Selection and Storage

Each project declares its SDK requirements and locks the selected version,
profile, target, actual capabilities and artifact/content identities. Different
projects may use different SDKs concurrently. A build target consumes one
validated runtime/ABI combination; additional ports must be built for that
exact SDK/target/toolchain. Never mix arbitrary libc/C++/upper-stage versions.

SDK content profiles are independent of Debug/Release build settings. Initially
select existing verified stage archives or the implemented Dev profile; a new
profile name is not a new producer stage/schema. Arbitrary delta-component SDK
assembly remains deferred until ownership/ABI/license/inventory rules exist.

The owner selected shared immutable SDK payloads with project-specific exact
references on 2026-10-28. A project binding must never follow a moving global
default. Handoff/export/isolated-test copies remain available; a general
project-local installer is not selected or implemented.

Shared payloads require validated immutability, reference/alias and GC behavior.
Example `.crt` paths are not a stable wire contract. Backend changes must not
change SDK identity or silently migrate locks. Keep project manifests/locks
in version control; generated SDK/view/port/build data is recreatable and excluded.

Cargo-style manifest/lock workflows, project toolchain selection and shared
caches are useful models; they do not require Cargo's physical layout or a
second compiler-management implementation. CRT SDKs do not bundle LLVM.

## 4. Host, Target, and Execution Environments

Always track `build_host`, `target`, `host_tools_identity`, and `sdk_identity`
separately. Record both tuples even for native development and explicitly
reject unverified cross combinations. Do not execute a target generator on the
host or report runtime success based only on cross compilation.

| Profile | Execution | Boundary |
| --- | --- | --- |
| `control` | SDK/registry fetch, selection, checks | CLI-private runtime; no accidental selection of another SDK's DLLs through PATH |
| `port-build` | configure/make and approved generators | CRT mksh/toybox/awk/make, pinned SDK wrappers and port dependencies |
| `app-build` | Native manifest -> LLVM argv | SDK toolchain and project dependency view; no automatic host header/library discovery |
| `app-run` | Applications/workers | Application runtime closure only; no build tools or unrestricted package-store PATH |

Do not modify system PATH, registry settings, or user shell configuration by
default. Adding `CRT_HOME/bin` is optional; do not put mksh/make/awk on the
global PATH. Construct child environments and exact executable paths per job.
Do not silently substitute Git Bash/MSYS/native tools for a target port's
CRT shell/make.

Existing `CRT_SYSROOT` identifies the **target SDK**; `CRT_ROOTFS` identifies
the **process filesystem namespace**. Initially, prepare an execution view
that preserves read-only bundle/SDK files and provides separate writable
scratch space. Resolve current port-driver coupling between `CRT_ROOTFS` and
the SDK root through adapters/contract tests before separating host tools.
Two environment variables alone do not prove cross-host separation works.

Distinguish the Windows shell's guest PATH, `/system/bin:/bin:/usr/bin`, from
native LLVM paths. Delegate approved native-tool invocation to the
existing bridge and preserve paths containing spaces or Korean characters in
actual argv. Pin exact environment construction to an SDK contract version.

The environment/project scopes of `doctor` must diagnose host/target identity,
Developer Mode and symlinks, LLVM/LLD/OS SDK and explicitly declared
port-build generators,
tool runtime DLLs, recipe applet requirements, and actual CRT compile/link/run
probes. It must not automatically install OS SDKs/compilers or change security
settings.

Restricted PATH/include/library roots plus output audits are **not an OS
sandbox**. Recipes execute code. Initially use official reviewed revisions;
make external-registry opt-in and execution risks explicit. Do not claim that
`--offline` plus PATH restrictions prevent malicious scripts from accessing
the network.

## 5. Source-first Port Recipe Engine

### Reuse Existing Assets

Initial official recipe sources are `crt`'s `porting/recipes/*.json`,
`porting/tests/`, `porting/shims/`, and installed
`tools/crt-port-build.py`. Do not bulk-rewrite the current JSON
`schema_version: 1` into TOML. New registry metadata points to exact CRT
revisions, recipe/test/shim digests, and engine requirements. Consider a TOML
recipe DSL only after demonstrating a need, with version negotiation and
conversion tests.

Do not interpret tools such as make in existing `dependencies` as target
libraries. The registry adapter must distinguish `host tools`,
`host generators`, `target ports`, and `required SDK stage/capabilities`.
CRT, as recipe owner, reviews these technical meanings; the CLI must not
guess them from names.

Start with the existing zlib recipe, then `sqlite-amalgamation` and libpng.
Providing the user-facing name `sqlite3` requires an explicit registry alias.
OpenAL Soft and SDL3 are future porting candidates, not packages assumed to
be fully validated already.

### Execution and Installation

```text
resolve exact SDK/recipe/dependencies
  -> fetch pinned source -> verify -> pristine source copy
  -> SDK-owned configure/build -> tests + binary/ABI audit
  -> immutable port-store publish -> project dependency view + lock commit
```

- Do not rebuild SDK binaries. Diagnose missing upper stages/tools explicitly
  and request separate SDK installation/selection.
- Do not arbitrarily patch upstream sources. Investigate CRT/PAL deficiencies
  against the Bionic baseline first. Unavoidable exceptions require prior
  review, rationale, removal conditions, and exact patch hashes/provenance.
  Recipes must not become hidden workarounds.
- Resolve target headers/libraries only from the SDK and selected store
  closure. Allowed OS/framework/driver boundaries must match SDK declarations.
- Build-host generators may be built separately as native tools. Do not ship
  those executables or their host runtimes in target ports/application bundles.
- Attempt native static/shared variants and include link/run smokes.
  Record unsupported variants with reasons and host/target-specific status;
  omissions are not successful acceptance.
- Installation into private build staging is allowed, but do not publish a
  store item as accepted until tests/audits pass. Keep failed outputs/logs
  distinct from accepted cache entries.

A build key hashes the normalized inputs below.

```text
K = H(source digest, registry/recipe/test/shim revision,
      SDK content identity, host/target, external toolchain identity,
      host-tools/generator identities, language/runtime ABI policy,
      linkage/build type/features, relevant declared environment,
      transitive dependency build keys)
```

Even for one upstream version, different SDKs, PIC settings, C++ exception/TLS
policies, backends, static/shared linkage, or Debug/Release configurations
produce separate variants. Identical build keys alone do not guarantee
byte-for-byte reproducibility. Installation manifests record input identities,
file/alias inventories, runtime closures, license/SBOM data, tool versions,
test results, and unsupported scope. Validate inventory/compatibility on local
cache reuse. A trusted binary cache is a later feature: verify the same key and
provenance, and retain an explicit source-build option.

Do not overwrite an SDK's public dependency when a port with the same name is
requested. Resolve whether to use the SDK-provided instance or explicit
isolation, and report conflicts. Do not promote WebKit-private libraries to
public application ports. v1 does not allow conflicting package ABIs within
the same link/runtime closure.

## 6. Project Manifest, Lock, and Build/Run/Package

`crt.json` expresses user intent; `crt.lock` records exact resolution. The
current format-0 C/C++ declaration is documented in the
[implemented contract](../reference/bootstrap_contract.md). Dependency/registry,
locked/offline and package semantics below remain planned extensions; their
presence in this design does not make them current commands.

Locks record schema, normalized host/target, SDK archive/content digests and
source commit, registry revision, every port's source/recipe/variant/build key,
host-tools identities, and external-toolchain identities. Resolve
`native`/channels/latest into concrete identities. Keep local executable
absolute paths in machine state, but verify content/version identities against
the lock. Distinguish per-OS resolutions.

`--locked` forbids re-resolution and manifest/lock changes. Downloads/builds
of exactly locked inputs remain possible; `--offline` separately forbids
network use. Missing offline archives/registry/store entries cause failure.
Only explicit update operations change the lock. SDK updates do not
automatically migrate existing project locks.

Project views expose only selected store entries without modifying SDKs.
Native dependency metadata must express transitive links, static archive ordering
and runtime files. The compiler module executes SDK-owned argument templates
directly, including startup objects and Windows exception ABI policy. Existing
SDK wrappers/CMake entry points remain producer/legacy consumers; native projects
do not require them. Do not add host `/usr/lib` as a fallback.

`new`, `add`, `remove`, and `update` show their change scope and preserve
original manifests/locks on failure. Remove changes project declarations;
it does not immediately delete shared store entries. Do not overwrite user
sources or existing projects.

`new` creates the manifest, basic sources and public build entry point from a
versioned template. Its initial shape is reviewed in D0; full project commands
and self-build/template parity are validated in D5. Source-module layout may
evolve while the public build contract remains compatible.

SDK acquisition occurs on first configure/build; `new` creates sources without
downloading an SDK (owner decision, 2026-10-28). Explicit installation may
prepopulate the store. Define network/change scope, failure boundaries and
diagnostics before implementing those commands.
An SDK requirement does not imply a project-local payload or a download on
every invocation. Honor exact locked inputs and offline behavior; preserve
existing sources/locks and never silently use a different globally selected SDK.

Run selects an explicit program/argv/cwd and runtime closure. Web projects
preserve private Host/Web/Network/GPU worker and resource layouts. Package
assembles the application, required runtimes, ports, notices, and source
obligations. Acceptance requires execution on a target machine without the
development SDK/build/store. Declare OS driver/framework exceptions in manifests.

## 7. CLI and Automation Contract

Representative v1 command groups follow. Explicitly reject commands that the
initial implementation does not support; do not provide successful placeholders.

| Command | Responsibility |
| --- | --- |
| `sdk install/list/use/update` | Immutable SDK/host-tool selection and installation |
| `upgrade` | Update the CLI itself through verified staging, health checks, activation and recovery |
| `doctor` | Explicit self, environment and project diagnostic scopes |
| `new`, `configure`, `build`, `run`, `test`, `package` | Project lifecycle |
| `search`, `info`, `add`, `install`, `remove`, `update`, `rebuild`, `tree` | Source-first ports and project dependencies |
| `env`, `shell` | Display a computed profile or run a shell in that environment |
| `cache clean` | Reference-aware cache cleanup |

The executable prefix is `crt-cli` (owner decision, 2026-10-28). Most command
groups remain planned; only the bounded bootstrap subset in the development
guide is implemented. There is no implicit `crt` alias.

Use `new` as the primary project-creation command. The earlier `create` name
may be a compatibility alias, not a separate feature. `install <port>` prepares
the store; `add <port>` includes project declaration/locking and source build.

All commands provide help/version, non-interactive operation, and clear exit
status. With `--json`, stdout contains one versioned final JSON object;
human/child logs go to stderr or log artifacts. Route application stdout from
run through a separate stream/file so it cannot corrupt the JSON envelope.
Long-running JSONL job events use a separate opt-in contract. Structure
command/target/plan, status/code, artifact/log references, diagnostics,
suggested actions, and cancellation results. Never record passwords, tokens,
or sensitive environment values.

Stable error codes distinguish at least SDK/host mismatch, unsupported schemas,
missing host tools, symlink prerequisites, source-digest mismatch, dependency
conflicts, and configure/build/test/audit failures. Freeze numeric exit-code
mappings with the v1 schema. Distinguish actual application exit/signal status
from wrapper errors.

Execute argv arrays rather than interpolating user paths/names into shell
strings. Jobs have timeouts, cancellation, and explicit process ownership;
avoid global name-based kills or hidden infinite retries. Windows automation
uses hidden/non-interactive children by default; open explicit shells/terminals
only at user request. If unfinished Windows process-group/terminal PAL support
is required, address it through a narrow reproducer and a separate implementation
gate.

### Doctor Scopes

- Self: executable/version/architecture, available trusted installation
  metadata, integrity and bootstrap resource/runtime readiness, without requiring
  a project SDK or compiler. Missing integrity evidence is unavailable/unknown,
  not a fabricated successful integrity result.
- Environment: host/target, required tools/rootfs, LLVM/OS SDKs, aliases and
  actual compile/link/run probes where the selected scope requires them.
- Project: manifest/lock compatibility, exact SDK/profile/capabilities and port
  identities, selected paths and readiness for the requested build/run action.

The initial implemented scope is `doctor --bootstrap`, described in the
[bootstrap contract](../reference/bootstrap_contract.md). Full self/environment/project
scopes remain later work. Diagnose by default without
installing tools, changing security settings or rewriting locks. A self-check
on an SDK-free machine must not fail merely because an unrelated project
compiler is absent. Candidate self-checks participate in upgrade validation;
fixtures alone do not establish executable integrity or native health.

### CLI Self-Upgrade

Define the contract in D0 and implement/test it with D2 installation
transactions. `upgrade` changes the CLI, while SDK/port/project updates remain
explicit operations with separate locks. The planned transaction is:

1. Resolve an intended CLI release and host identity using the agreed metadata
   trust/authentication policy. A checksum alone is not publisher authentication.
2. Download to private staging; verify metadata, size/digest, target and package
   rules before executing the candidate. Retain the previous usable version.
3. Run bounded candidate version/self-doctor checks in the agreed SDK-free
   environment, including initial OS loader/resource behavior.
4. Activate through a validated OS-specific operation. Specify running-file
   handling, restart and any launcher/temporary-helper role under the agreed
   one-file definition; never assume overwriting an in-use executable works.
5. On failure/interruption, retain or restore the prior usable selection and
   record the failed identity. Serialize concurrent upgrades and preserve
   active jobs and their referenced versions.
6. Preserve all project SDK/port locks, templates and sources. Cleanup is
   reference-aware and bounded; old-version retention/GC is explicit.

The [lifecycle acceptance contract](../acceptance/lifecycle_acceptance.md)
owns these future gates. D6 repeats release/package/recovery checks for the
actual shipping artifact. No upgrade command or rollback mechanism exists yet.

## 8. Cross-repository Operations

`crt` owns SDK/Host Tools production and official recipe contracts;
`crt-cli` owns the resolver/UX and execution engine. Do not hardcode registry
content into CLI sources; start from a verified CRT revision. Split out
`crt-ports` only when port scale warrants it, not as an immediate fourth
repository requirement.

Linked issues/PRs record both revisions, contract changes, immutable
artifact/SDK locks, and host/target results. Merge/promote the producer first,
then update consumer locks; do not assume atomic cross-repository merges.
Isolate experimental APIs through opt-in/versioning and prevent runtime
dependencies on CLI implementation.

Initial development may use an unreleased, locally produced Dev SDK. One
authorized session may work on the adjacent producer/consumer repositories,
with an immutable copied/extracted SDK handoff outside producer source/build
trees. CLI inputs use that public SDK alone; producer access is for SDK work
and diagnosis. Follow the [local candidate loop](../guides/development.md#local-dev-sdk-candidate-loop)
and record experiments separately from promoted lock adoption. Public SDK and
CLI releases need not be simultaneous; promotion precedes final consumer-pin
adoption and publication.

Use the [compatibility/adoption record](../compatibility.md) to separate original
design provenance, adopted SDK/host-tool contracts, CLI wire capabilities, and
combinations actually exercised by Studio. Apply its shared-change review
links; no proposed v1 label is evidence of an implemented or tested schema.

Fast CI covers parsers/resolvers/fixtures/SDK contracts; integration consumes
selected existing artifacts; full replay verifies native hosts, paths, and
failure recovery at milestones. Do not duplicate a full WebKit build for
every documentation change. External PRs receive no publishing tokens;
only a separate trusted promotion process publishes artifacts. CI success
and actual GUI/native-device acceptance are distinct.

## 9. Joint Development Sequence

This sequence consolidates the initial IDE-first J0-J6, CLI-only C0-C5, and
binary-first B/P proposals. Windows host-tools H0-H6 priorities are reflected
in D0-D4. This table defines future work order and acceptance goals, not a
completion checklist. Each repository's TODO/acceptance owns actual status.

| Stage | `crt` producer work | `crt-cli` consumer work | Acceptance goal |
| --- | --- | --- | --- |
| D0 Contracts/inventory | SDK inputs/startup/link/import contracts and fixtures | Product decisions, evolving initial layout, minimal CLI and one-file feasibility; lock/JSON drafts | SDK-only compile/link/run and import proof; no private build inputs; layout/decision review |
| D1 Windows environment/bootstrap | Separate host tools/rootfs and SDK technical validation | Single-executable SDK-free bootstrap, trust inputs, self-doctor and host environments | Native one-file execution/HTTPS; actual CRT tools without global PATH changes |
| D2 SDK management/self-upgrade | Immutable SDK/profile artifacts and contract metadata | Project SDK storage/selection and verified CLI self-upgrade transactions | Three-host installs, candidate health/activation/recovery and unchanged project pins |
| D3 First source port | Existing zlib recipe/tests and packaged driver contract | Fetch/verify/build/test/store adapter | Windows CRT mksh/make static/shared zlib link/run; SDK/source roots unchanged |
| D4 Dependencies/cache/lock | Review SQLite/libpng and host-generator/target-dependency roles | Resolver, variants/build keys, local cache, project views | SQLite and libpng->zlib; rerun/recovery/ABI-mixing rejection; three-native-OS replay |
| D5 Application workflow | Extend native compiler/runtime/dependency contracts | new/add/configure/build/run, layer-specific templates | External C/C++ and supported GFX/Media/UI/Web apps run; Web worker/ABI checks |
| D6 Deployment/automation stability | Runtime closure and notices/source contracts | test/package, locked/offline/cancel, versioned JSON | App runs without SDK/store/build roots; CI/AI controls workflow through commands; repeated child-process acceptance |
| D7 Studio integration | Consumer-driven generic UI/input/PAL gaps | CLI compatibility and debug-adapter contracts | A thin [Studio](https://github.com/webos21/crt/blob/d21430ddeb59aea76cf6abd58c15a69f55cfda52/docs/design/crt_studio.md) frontend uses the same workflow |

D0 snapshots may start with manually installed existing SDKs. Unfinished D1/D2
work does not block runtime development or recipe-schema fixtures. Conversely,
D3 acceptance must not substitute Git Bash for an actual CRT host environment.

D7 formalizes approved Studio combinations; it does not gate all Studio S0
work. SDK-only UI PoCs and labeled protocol fixtures can proceed early. Live
project build/run experiments require actual D5 capabilities, and robust
job/cancellation/package integration needs the applicable D6 contracts. JSON
drafts start in D0; D6 stabilizes them. Gate each feature on real command/schema
support and evidence, not on a tranche number or an unfinished placeholder.

Use small increments within each stage: contract, implementation, bounded test,
native replay, then documentation. Record host failures, unsupported
static/shared variants, and cross-compile-only results separately. Do not
assume every upstream port already works on all three OSes.

## 10. Decisions for the First Implementation

Owner decisions recorded on 2026-10-28:

| Surface | Selected policy |
| --- | --- |
| Executable | `crt-cli`, matching the repository; no implicit `crt` alias |
| Distribution | One shipped executable; declared OS dependencies, cache/certificate data, previous versions and temporary update helpers allowed; no permanent auxiliary launcher |
| SDK storage | Shared immutable payloads with project-specific exact references |
| Acquisition | First configure/build acquires a missing SDK; `new` only creates sources |

These are product policies, not claims that the installer/updater exists.
The following mechanisms still require concrete contracts and tests:

- General SDK development-contract location/schema, public imports and runtime query.
- TLS trust provisioning, metadata authentication, notices and exact update-helper activation.
- Host-tools/rootfs adapter, registry/recipe trust, final project/lock schemas.
- Per-OS activation/recovery, usage references and GC.

The initial module layout may evolve. The implemented bootstrap input/result
contracts and host-side experimental format-0 fixtures are documented separately;
they are not SDK Build Contract v1 or a promoted project lock. Later D1-D7
commands and public releases still require their own implementation and evidence.
