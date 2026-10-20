# Agent Development Guidelines

## Mission

Build `crt-cli`, a CRT-native development-environment manager for Linux,
Windows, and macOS. Its user-facing executable is `crt`.

The CLI makes CRT application development reproducible without requiring users
to assemble startup objects, freestanding flags, sysroots, runtime link order,
or worker/DLL search paths manually. It manages SDKs, host tools, source-built
ports, project creation, build/run/test/package workflows, and diagnostics.

The intended product is written in CRT-native C/C++ with a C++17 baseline.
Rust/Cargo and Homebrew are references for user experience and package models,
not requirements to implement the CLI in Rust/Go or use the host C/C++ runtime.
CRT Studio is a later, separate GUI consumer of the CLI's public contract.

## Design References and Starting Work

Start with [README](README.md), the [documentation index](docs/README.md),
[local CLI design](docs/design/crt_cli.md), [development guide](docs/guides/development.md),
and [bootstrap acceptance](docs/acceptance/bootstrap_acceptance.md).
[TODO](TODO.md) owns In Progress/Planned work; [HISTORY](HISTORY.md) owns
dated completed work. This repository owns ongoing CLI design; the pinned CRT
references below record the integration baseline and producer-owned boundaries,
not a second independently maintained CLI design mirror.

The initial design baseline is CRT revision
`d21430ddeb59aea76cf6abd58c15a69f55cfda52`:

- [CRT-CLI design and D0-D7 sequence](https://github.com/webos21/crt/blob/d21430ddeb59aea76cf6abd58c15a69f55cfda52/docs/design/crt_cli.md).
- [SDK Development Contract](https://github.com/webos21/crt/blob/d21430ddeb59aea76cf6abd58c15a69f55cfda52/docs/design/sdk_development_contract.md).
- [CRT Studio boundary](https://github.com/webos21/crt/blob/d21430ddeb59aea76cf6abd58c15a69f55cfda52/docs/design/crt_studio.md).

Read applicable instructions and inspect git status before editing. Read
README, TODO, HISTORY, and relevant design/acceptance documents when present.
An adjacent CRT checkout may supply design context; it is not a build input.
Track later adopted contract changes explicitly rather than silently following
CRT main. Never infer that proposed commands, schemas, bundles, or public
releases already exist.

During initial setup, do not invent build targets, presets, test commands, or
completed milestones. Establish and document actual entry points as the
implementation grows. Creating this file does not authorize implementation of
the entire roadmap, creation of other repositories, or publication.

## Repository and Ownership Boundaries

- CRT owns libc/PAL, imported libc++/libc++abi/libunwind, startup, wrappers,
  toolchains, SDK production, CRT host tools, and official recipe policy.
- This repository owns CLI UX, resolution, installation transactions,
  environment construction, port execution/store management, and projects.
- Studio owns editor/workbench/debugging UX. Do not implement an IDE here.
- Depend on public, installed SDK artifacts with exact identities. Do not
  vendor CRT source, include private headers, reference sibling build trees,
  or add `add_subdirectory(../crt)` as a shortcut.
- CRT runtime/SDK production must not require CLI sources. CLI bootstrap uses
  an already produced SDK; optional product assembly is a separate step.
- A CRT/PAL failure calls for a minimal reproducer and a linked producer-side
  fix. Do not silently modify a sibling repository or upstream source without
  authorization covering that change.

## Language, Build, and Runtime Contract

Use CMake, Ninja by default, and CTest for project-owned tests. Product code
must build against CRT, not host glibc/libSystem/MSVCRT or host C++ libraries
as substitute standard runtimes. SDK-declared OS/PAL boundaries remain allowed.
Do not add a required Go/Rust product implementation without an explicit
architecture decision.

Delegate compiler flags, startup selection, sysroot, compiler-rt, link order,
and static/shared runtime policy to the selected SDK's wrappers and
`crt-toolchain.cmake`. Do not copy `crt0.c` into projects or independently
hardcode the SDK's low-level flags in command handlers. Preserve Windows CRT
DWARF/Itanium/libunwind ABI policy; do not mix MSVC/GNU C++ runtimes.

Clang/LLD, CMake/Ninja, OS/vendor SDKs, and declared generators may be external
native host tools. Record and validate their identities. They are distinct
from CRT target libraries and are not implicitly bundled into SDKs or apps.
If an initial adapter invokes the installed SDK's Python port driver, declare
the Python requirement and pin that driver contract. Do not hide the
dependency or immediately duplicate its recipe semantics.

Build the CLI against a pinned SDK and distribute its minimal runtime closure
with it. The bootstrap CLI must run on a machine without a development SDK.
Do not require the complete UI/Web stack merely to install SDKs or fetch ports.
Validate CRT-built HTTP/TLS dependencies and trust configuration explicitly.
Windows imported DLLs must resolve at initial process load, not through an
assumed global PATH or an unverified `runtime/` directory.

## SDK Identity and Host Environments

Keep `build_host`, `target`, `host_tools_identity`, `sdk_identity`, and external
toolchain identity separate, even for native builds. Do not execute target
generators on the build host or label a cross compile as runtime acceptance.
Reject unsupported combinations explicitly.

Reuse the existing cumulative stage IDs: `01-c`, `02-cxx`, `03-gfx-simple`,
`04-gfx-media`, `05-ui`, and `06-web`. Proposed profiles select these stages;
they do not allow arbitrary mixtures of libc/C++/upper-runtime versions.
SDK selection and additional port dependency resolution are different tasks.
Verify actual capabilities instead of inferring them from a profile name.

Maintain independent, versioned areas for CLI runtimes, host tools, SDKs,
toolchain records, registry snapshots, source caches, jobs, and port stores.
Follow the design's `CRT_HOME` layout without rearranging SDK internals.
Support user-selected locations and real paths containing spaces and Unicode.

- `control`: CLI-private runtime for selection, fetch, and diagnostics.
- `port-build`: CRT mksh/toybox/awk/make, selected SDK wrappers, approved host
  generators, and exact port dependencies.
- `app-build`: SDK toolchain and selected project dependency view.
- `app-run`: application runtime closure, resources, and required workers only.

Do not modify global PATH, shell configuration, registry, or security settings
by default. Construct environments and executable paths per child job.
`CRT_SYSROOT` is the target SDK; `CRT_ROOTFS` is the process filesystem
namespace. Preserve `/system/bin`, `/bin`, `/usr/bin`, applet aliases, and
native-tool bridges. Resolve existing driver/rootfs coupling before claiming
independent host-tool/cross-target operation.

On Windows, use the actual CRT-built mksh, toybox, separate awk, and make for
port acceptance. Git Bash/MSYS/Cygwin are not silent replacements. Native host
generators may use separately declared tools; this does not change the target
port shell contract. Enumerate supported applets rather than assuming toybox
provides every Unix command or make.

Windows build/source-stage hosts require Developer Mode and real symlinks.
Diagnose the missing prerequisite; never enable it automatically or substitute
MSYS marker links, shortcuts, or deep copies. PATH filtering and output audits
are not an OS sandbox.

## Source-first Ports and Project Locks

Start with the reviewed CRT `porting/recipes/*.json`, tests/shims, and installed
port-driver entry point. Preserve the existing JSON recipe schema initially;
do not replace it with a new TOML DSL. Registry metadata must pin recipe,
test/shim, engine, and source identities. Classify host tools, host generators,
target ports, and SDK capabilities explicitly, not by guessing package names.

Use this sequence: resolve -> fetch -> verify -> pristine source copy ->
SDK-owned configure/build -> tests and ABI audit -> immutable store publish ->
project dependency view and lock update. Do not rebuild SDKs as an implicit
side effect of adding a port. Missing SDK capabilities require an explicit
installation/selection decision.

Do not patch upstream sources to hide CRT/PAL gaps. Follow CRT's Bionic-first
investigation policy. Any unavoidable exception needs prior review, rationale,
removal conditions, and exact patch hashes/provenance. Do not introduce host
headers/libraries as fallback dependencies or invent configure capabilities.

Port build keys include source/recipe/test/shim revisions, SDK content,
host/target, toolchain and host-tool/generator identities, ABI policy,
linkage/build type/features, relevant environment, and transitive dependency
keys. Separate build variants and do not claim byte reproducibility solely
because keys match. Source-first comes before trusted binary-cache support.

Never install ports into an immutable SDK. Detect SDK-provided dependency
collisions and conflicting package ABIs. WebKit-private Skia/OpenSSL/FFmpeg
are not public application port packages. Start with zlib, then
`sqlite-amalgamation` and libpng; OpenAL Soft/SDL3 are later candidates, not
assumed accepted ports.

`crt.toml` describes project intent; `crt.lock` records exact resolved inputs.
Their schemas are design work until implemented and tested. Pin concrete SDK,
registry, source, recipe, variant, dependency, and tool identities; a version,
channel, branch, or `latest` alone is not sufficient. `--locked` forbids
re-resolution/lock changes; `--offline` separately forbids network use.
Missing cached inputs must fail clearly. SDK update must not silently migrate
existing project locks.

## Installation, Trust, and Recovery

Verify trusted metadata, TLS, archive size/digest, and provenance as applicable.
A checksum alone is not publisher authentication. Recipes execute code; use
reviewed registry revisions, make third-party trust opt-in explicit, and do
not claim sandboxing or signed metadata that has not been implemented.

Extract into staging and reject traversal, absolute/drive paths, escaping
symlinks, case collisions, reserved Windows paths, and extraction-size abuse.
Validate inventories, aliases, ABI/runtime closure, notices, and tests before
publishing. Failed candidates are not accepted cache entries.

Use per-identity locks and atomic publication. Install updates side by side;
never overwrite in-use executables, runtimes, SDKs, or store items. Preserve
previous selections and project manifests/locks on failure. Removing a project
dependency does not immediately delete shared store content. Cleanup respects
references, active jobs, and an explicitly bounded target.

## CLI and Process Contracts

Use `new` as the primary project-creation name; `create` may be an alias.
Do not return success from unimplemented placeholder commands. Support clear
help/version, non-interactive operation, and stable error/exit semantics as
each command is implemented.

With `--json`, stdout contains one versioned final JSON object. Human logs and
child output use stderr or explicit log/stream artifacts; application output
must not corrupt JSON. A separate opt-in contract handles JSONL job events.
Distinguish CLI failures from child exit/signal status. Never log credentials,
private keys, tokens, or sensitive environment values.

Execute argv arrays; do not interpolate user paths into shell strings. Bound
jobs with timeouts/cancellation and track owned process trees. Do not use
global name-based kills, hidden infinite retries, or visible recurring console
loops. Windows automation launches hidden/non-interactive children; open an
interactive shell/terminal only when requested. Treat missing PAL cancellation
or terminal support as a separate, reproduced producer-side gap.

## Development and Verification

Follow the design's D0-D7 order: establish SDK/host-tool contracts and a minimal
CRT-native CLI first, then the Windows environment/bootstrap, safe SDK
management, first source port, dependency/cache/lock handling, application
workflows, deployment/automation, and finally Studio integration. Do not expand
into IDE work or a registry service while these foundations remain unproven.

For each requested increment:

1. Record a small, explicit scope and actual prerequisites.
2. Define/version the contract and add focused fixtures/regressions.
3. Implement against installed SDKs without producer-private inputs.
4. Run applicable tests, document evidence/limits, then proceed to the next step.

Use fresh CMake configuration for wiring changes; stale caches are not proof.
Document real build/test commands once they exist. Parser/resolver/transaction
fixtures belong in fast tests; actual artifact consumers and native replay are
separate gates. Documentation-only changes do not require a full WebKit build.

Attempt both static/shared port variants and link/run smokes, recording
unsupported variants and reasons. Replay applicable installation/environment
and workflow gates on native Windows/x64, Linux/x86_64, and macOS/arm64.
Record other architectures and cross-compile-only results honestly; do not
substitute WSL for native device/GPU/media acceptance. Test interrupted and
tampered downloads, ABI conflicts, concurrent installs, rollback, cache reuse,
spaces/Unicode paths, DLL collisions, and owned-process cleanup.

Validate packaged apps/CLI without development SDK/build/store dependencies.
Inspect actual ELF/Mach-O/PE dependencies and mapped runtime images where
applicable. Preserve Web worker/resource layout and declared OS prerequisites.
Compile success, CI green, skipped tests, and development snapshots are not
interchangeable with runtime acceptance or public-release authorization.

## Documentation and Change Discipline

Write new code comments and documentation in English. Use [docs/README.md](docs/README.md)
for the documentation ownership rules and index. Keep design/rationale in
`docs/design/`, procedures in guides, contracts/evidence in acceptance, and
current ordered work in TODO. Once initialized, HISTORY records completed work
and TODO retains only concise active/planned steps. Update STATUS only when
explicitly requested. Do not invent missing files or completed implementation
to make the initial repository look populated.

Link CRT-owned ABI/SDK evidence instead of duplicating it. Record the exact
producer/consumer revisions, artifacts, host/target, commands, outcomes, and
limitations needed to resume work on another machine. Keep build/download/cache
outputs outside tracked source; bound tool output and retain full logs as
artifacts without secrets.

Preserve user changes. Do not change a neighboring repository, create/push
commits, publish artifacts, or alter release/security settings without task
authorization. Cross-repository changes use linked small increments:
producer validation/merge and immutable SDK promotion first, consumer lock
update second. Retain failed identities and previous known-good locks rather
than replacing artifacts or masking failures with retries.
