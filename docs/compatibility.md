# Contract Compatibility and Adoption

This document owns the cross-repository contract adoption record, not SDK ABI
policy or a duplicate test log. Producer evidence belongs to CRT acceptance;
CLI behavior to [bootstrap acceptance](acceptance/bootstrap_acceptance.md);
Studio GUI/editor/integration evidence to Studio acceptance. Architecture and
work order remain in [the CLI design](design/crt_cli.md) and [TODO](../TODO.md).

## D3 Common-change Replay on macOS/arm64 (2026-11-09)

Reviewed CRT `2fdce21d687936c313a3a5990d7acdc3ab824c54` / CLI `938518c4b3bb2c887e921af602082b46ac9be5b4`.
The product/test snapshot is exactly Linux's `22720f7d...bee16` baseline from `2ae74d4`. The
[native macOS continuation](acceptance/sdk_install_acceptance.md#macos-d3-common-change-closure-replay-2026-11-09) passes
29/29 gates against the unchanged clean rehearsal.7 SDK (`02e67f3e...c999b`) and provider (`912094d0...e189`), not Linux's
dirty developer inputs. Source-hidden self-build, SDK-free checks, six D3 suites, 921 native checks, actual store use,
key/cache/inventory checks and observed port/SDK provider maps qualify the exact new executable `929f7291...114175`.

No product/recipe/SDK or shared-format change is adopted here. The unchanged macOS key and new host-only replay
receipts are technical evidence for the pending review, not final approval, binary publication or consumer-lock migration.
Windows/x64 keeps its exact unchanged baseline; Windows/arm64 D3 and Linux's unendorsed C shared SDK mode remain separate.

## D3 closure review and affected-host handoff (2026-11-09)

At the owner's request, review CRT `21b9d0e` / CLI `2ae74d4` without
relabeling the earlier three-host technical records. The zlib recipe/test
pins match the producer and the compiled adapter. Review topics are host/target
dependency roles, supported field subset, generated-Makefile workaround,
OS layout/environment/make arguments, binary audit, provider/lock boundaries
and the documented packaged-oracle differences. SDK/recipe/runtime formats
and bytes are not changed by documentation cleanup.

`crt_review` remains pending. `adapter_json` excludes review metadata and
`port_key_inputs` includes the declared executor environment/make policy;
recording approval alone must not invalidate an item. Actual policy changes
need new keys and affected-host evidence. The [Linux affected-delta replay](acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09)
passes at its retained dirty developer inputs; the [matching native macOS delta also passes](acceptance/sdk_install_acceptance.md#macos-d3-common-change-closure-replay-2026-11-09)
on its own identified clean SDK. The historical Linux SDK C shared control's host libz mapping is
explicitly unendorsed and remains a producer-policy review item. Existing final
Windows/x64 evidence is reused only for unchanged inputs. Windows/arm64 D3,
Full installation and public release/promotion remain separate gates.

The [development handoff](guides/development.md#d3-common-change-closure-and-native-handoff)
and producer acceptance own exact replay inputs/results. This paragraph is a
review plan, not final producer approval or a fresh three-host run.

## D3 Windows/x64 Adapter Implementation (2026-11-09)

Reviewed producer `ff6b9e2` and CLI `3db09d7` plus the Windows delta. The unchanged
clean rehearsal.10 SDK from CRT `05b5678` and native policy
`5b1ad5ba0465268a8485035f92066b19e0df9942458c212804cd7121366d29d1`
are exercised with independently pinned CRT host tools. [Native evidence](acceptance/sdk_install_acceptance.md#source-port-d3-windowsx64-implementation-and-replay-2026-11-09)
binds final source/binary/receipt hashes and passes executor, PE/store, provider/lock and oracle cases.
Windows layout and import-library key inputs are CLI adapter policy; recipe schema 1,
native policy 1, host-tools 1 and project lock 1 are unchanged. No producer artifact
promotion or consumer root-lock migration occurs. The Windows/x64 binary is refreshed.
Formal table approval, common-change replay on other hosts and Windows/arm64 remain open.
This local result supersedes the missing-capability boundary below for its tested inputs only.

## D3 Replay Boundary on Windows/x64 (2026-11-09)

Reviewed CRT `a62d5e2d3a17ed0984e659552413a6d044330f6c` and CLI
`4936df5e2abae90e84cd3a52d452b7a9bb612329`. The producer delta since the clean
rehearsal.10 SDK's `05b56788501b02f71a8b8084cc4041effa1475a9` is documentation only;
that public SDK is reused with policy `5b1ad5ba0465268a8485035f92066b19e0df9942458c212804cd7121366d29d1`.
The [Windows record](acceptance/sdk_install_acceptance.md#source-port-d3-replay-windowsx64-2026-11-09)
binds the identical macOS product inventory, corrected Windows-only test fixtures, actual CLI image,
independent CRT host tools, common regression passes and retained input-enabled D3 failures.

No SDK, native policy 1, recipe schema 1, host-tools 1, project lock 1 or product code changes.
The producer Windows replay guards now admit JSON recipe data and the private store-lock helper;
they do not add a runtime capability. The Windows executor, adapter layout and PE port audit remain
unsupported: this records a negative D3 result, not a compatible Windows source-port combination.
Formal table approval, promotion, root locks/defaults, committed binaries and STATUS remain unchanged.

## D3 Source-port Adapter on macOS/arm64 (2026-11-09)

CRT `2f4103d97880d5a51c474ef7b65fe810186c49de` produced a clean local Dev SDK `v0.0.1-rehearsal.7`, content
`02e67f3ef8e8eef48f4497c1f5f5ad938e2fdc98ff143fd6cdf849b46fec999b`, with portable provider
`912094d0016e8d1a8a6c116e1b257867cfc2f284b148de8d5b419f4d6c97e189`.
CLI `1427da0` plus the macOS delta is qualified by the
[native D3 record](acceptance/sdk_install_acceptance.md#source-port-d3-replay-macosarm64-2026-11-09), including exact source/executable and
independent host-tools pins, actual static/shared consumption and the reference-driver differences.

The CLI-owned experimental adapter/store policy now includes Mach-O layouts/audit and macOS OS-SDK metadata in the
key. Linux key serialization and recipe schema 1, native compiler policy 1, host-tools format 1 and project lock 1 are
unchanged. This is local native macOS adoption, not approval of arbitrary future dylibs or a new release. Formal CRT
adapter-table approval remains pending; no producer promotion, consumer root lock/default change, Windows D3 acceptance
or current-delta Linux replay is implied. The earlier Linux records keep their exact tested inputs.

## Windows/x64 Concurrent-install Fix (2026-11-06)

Reviewed CRT `05b5678` (libunwind recipe patch; no SDK format, asset schema or CLI source change) and CLI `ea9b8cd`. The
[record](acceptance/sdk_install_acceptance.md#windowsx64-concurrent-install-sigsegv-fix-2026-11-06) replays the installer, selection, first-build and HTTPS drivers with a CLI self-built against
SDK `v0.0.1-rehearsal.10` (content `b984ec30b4b2...`): 101/101 x3, 68/68, 86/86, 74/74. The previously open Windows/x64 concurrent-install crash is closed by the
root-cause fix; Windows/arm64, a published asset, official trust and promotion remain untouched, and the earlier rehearsal.8 records keep their own scope.

## Windows/arm64 Selection, First Build and HTTPS Replay (2026-11-06)

Reviewed CRT `a00c6be` (host-tools and documentation only since `0631131`) and CLI `160a4a6`. The
[Windows/arm64 record](acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsarm64-2026-11-06) replays the Linux, macOS and Windows/x64 receipt, selection, first-build and pinned-HTTPS
increments against the **existing** rehearsal.7 SDK of CRT `0631131` (content `c4b352012b426c60e2d4d666febf565e17239f8f534714e7b67d89430f051233`, manifest without top-level `format`). No format, producer asset or SDK
byte changes. Installer/receipt driver 101/101, selection 68/68, first build 86/86 and HTTPS 74/74 pass on this native combination; the proven combination is that SDK with the CLI executable `d855f092...0582`. The
Windows/x64 concurrent-install crash is neither fixed nor closed (15 of 15 repeated concurrent installs pass here, as supporting evidence only), and Full-stage metadata, official trust, promotion and STATUS remain untouched.

## Windows/x64 Selection, First Build and HTTPS Replay (2026-11-06)

Reviewed CRT `4db4c85` (host-tools and documentation only since `436b4f2`) and CLI `ea9b8cd`. The
[Windows/x64 record](acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-windowsx64-2026-11-06) replays the macOS/Linux selection,
first-build and pinned-HTTPS increments against the **existing** rehearsal.8 SDK of CRT `436b4f2` (content
`e80fa143094eae0338716e0d1419bda07ea455ceadc21e0309c7e88622487159`, manifest without top-level `format`). No format,
producer asset or SDK byte changes. Selection 68/68, first build 86/86, HTTPS 74/74 and the installer/receipt driver 101/101 pass on this
native combination; the proven combination is that SDK with the CLI executable `01bfa8c7...1e37`. A DLL planted in the caller's working
directory shadows the shared runtime on Windows; that is documented, not defended. The concurrent-install crash is neither fixed nor closed
(one passing run), and Windows/arm64 receipt/selection/first-build/acquisition, official trust, promotion and STATUS remain untouched.

## macOS Private-dylib Host-tools Replay (2026-11-06)

Reviewed CRT `82466be` plus the retained assembler/runtime-alias delta and CLI
`1dc3c03` plus the preceding macOS workflow correction. The executable remains
`94ddda0e...9ea8d`; no further consumer product change is needed. The
[native consumer evidence](acceptance/host_environment_acceptance.md#macosarm64-private-dylib-host-tools-replay-2026-11-06)
and [producer fixture](https://github.com/webos21/crt/blob/main/docs/acceptance/host_tools_acceptance.md#macosarm64-private-dylib-closure-replay-2026-11-06)
bind a real shared-AWK closure to manifest
`1e71a84593d220cbe09d62932c7d446073a5dc8f5bc7313128142aaa9c2301e8`.

Existing host-tools format 1 covers the three private dylibs and six internal
runtime links needed by copied executable aliases. Relative private load names
and the declared OS boundary are producer requirements; CLI's existing pinned
inventory and clean child environment enforce the consumer boundary. This is
local macOS/arm64 fixture adoption, not promotion of the modified SDK view,
every future dynamic bundle or Windows private-DLL acceptance. Static bundle
payload is unchanged. No SDK/default/lock/trust/binary/STATUS adoption occurs.

## macOS Selection, First Build and HTTPS Replay (2026-11-06)

Reviewed CRT `82466be582e57c8770a6816f1c6b9ed0bf819f76` and the CLI
increments `8788b59`, `4f55a52`, `1dc3c03`. The
[macOS/arm64 replay](acceptance/sdk_install_acceptance.md#selection-first-build-and-pinned-https-replay-macosarm64-2026-11-06)
uses clean CRT `784389b` Dev, content
`72d1b37593abb2f5dd9d58a8338f44191b0b2023d6cbf2621cf7cd688ce80b4c`,
and its unchanged original rehearsal.6 archive/contract/manifest. It owns exact
before/after source and CLI identities, affected native cases and retained failures.

The local consumer correction accepts the actual release envelope, which has
no required top-level `format`; **archive `install.format` is 1** and remains
validated. This aligns with existing producer assets without changing source,
receipt, portable-provider, SDK or project-lock formats. No runtime/PAL or SDK
bytes change. Selection, first-build and pinned acquisition are exercised on
this exact native developer combination, not a promoted/default pin.
Earlier Linux records retain their fixture/binary scope; the correction needs
original-producer-asset replay there and on Windows. Windows concurrency/DLL,
broader profiles, official trust and production promotion remain open.
No committed binary, default, root lock, release or STATUS is refreshed.

## Explicit Pinned HTTPS Source Adoption (2026-11-06)

Reviewed CRT `6e064f2cc2ff84b5507d7be8000668c730dfe73a` and CLI
`4f55a52bd75660b4c8d2543da58da6337efd45de`. CLI owns the implemented
`crt-cli.sdk-source`, format 0, acquisition transaction and project integration.
CRT still owns release/install metadata format 1, portable compiler policy,
SDK content/provenance and technical qualification. Existing assets/fields
supply the metadata/archive/contract handoff; no producer schema changes.

The source file explicitly pins HTTPS URLs, CA bytes, manifest size/digest,
SDK content/version and portable-contract digest. Cold acquisition applies the
existing fetch TLS/size/digest checks and local release installer. Verified
installed-identity reuse does not fetch/reverify remote metadata; it compares
caller-trusted SDK/contract pins with the installed receipt and actual payload.
The source is optional local input, never a moving default or publisher key.
Project-lock/build-result formats are unchanged; source input hashes and
transport evidence live in acquisition output rather than changing existing
SDK/tool selection keys.

[Linux consumer evidence](acceptance/sdk_install_acceptance.md#pinned-https-sdk-acquisition-linuxx86_64-2026-11-06)
uses the retained dirty developer SDK/release fixture and disposable local TLS
trust. This qualifies that exact native technical combination, not an official
public release, default trust policy or another-host support claim. Producer
promotion precedes future production adoption; Windows concurrency/DLL and
macOS/Windows first-build/acquisition replay remain open.

## Installed SDK First-build Binding (2026-11-06)

Reviewed producer `c9e6aa9ecb73c85c73ba32761b2715d0b09fbe71` and CLI base
`8788b59a77841537033915b34bbbf3c9a013b906`. CLI owns first-build input
selection and success-only project binding; the SDK still owns all compiler,
startup and link policy. Existing manifest/provider/receipt/project-lock and
build-result formats are retained. A bound or incomplete project requires
explicit configure for selection changes; no automatic lock migration occurs.

The [Linux first-build evidence](acceptance/sdk_install_acceptance.md#first-build-without-configure-linuxx86_64-2026-11-06)
uses the retained explicitly dirty Dev fixture with unchanged provider/SDK pins,
a fresh CLI and isolated new/build/test/run consumers. It is local technical
adoption, not production trust/promotion or another-host qualification. Pure C
shared policy in that retained provider emits static inputs; actual C++ shared
and C static results are recorded separately. CRT tracks the policy audit.
Remote acquisition and native macOS/Windows first-build replay remain open;
existing Windows concurrency/DLL gates are unchanged.

## Installed SDK Full-identity Selection Review (2026-11-05)

Reviewed CRT `d856a009358dcf6391c89ee0af7390b54db2419b` and CLI base
`483e76d846808aa3391b329908c4b5883c815c66`. The consumer matcher now checks
all supplied content-reference digits, including those beyond the store's
12-digit directory suffix. This corrects existing selection semantics without
changing receipt/provider/install-metadata/project-lock formats or requiring
new producer artifacts. CRT owns SDK identity/portable policy; CLI owns exact
selection and lock preservation.

The [Linux collision slice](acceptance/sdk_install_acceptance.md#installed-sdk-selection-and-search-path-collisions-linuxx86_64-2026-11-05)
uses the same explicitly dirty, unpromoted receipt developer fixture, with a
fresh CLI and bounded SDK-only/consumer checks. It records exact source/binary/
SDK/tool identities and synthetic resolver-fixture scope. No new combination
is claimed compatible on macOS/Windows until affected-host replay. Windows
concurrency/receipt/DLL gates retain their existing open scope. No production
pin, default, lock, committed binary, trust root or public artifact is changed.

## Installation Receipt Windows/x64 Scoped Replay (2026-11-05)

Reviewed CRT `dc62e8db883ad0218b32c442915d7d76fe7f5b5e` and CLI
`51f1dc092444b529cbddb01785727f8dbe9ff0b8`. The
[bounded consumer evidence](acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-windowsx64-2026-11-05)
uses clean CRT `436b4f2` Dev, original rehearsal.8 release inputs and installed
identity `e80fa143094eae0338716e0d1419bda07ea455ceadc21e0309c7e88622487159`.
Its product/test snapshot matches receipt implementation `502992e` and the
Linux/macOS increment. Receipt format 1, pin enforcement, explicit repair and
real Windows symlink refusal pass on this combination. Producer/native-policy/
install-metadata/project-lock formats and SDK bytes remain unchanged.

This is **not full Windows adoption/acceptance**: the first concurrent installer
crashes natively; a passing continuation does not erase it. Exact inputs,
partial/passing results and the reporting-only checker fix live in acceptance.
Native crash investigation and Windows/arm64 receipt replay remain open. No
production/default pin, committed binary, lock, trust or STATUS is refreshed.

## Installation Receipt macOS Replay Adoption (2026-11-05)

Reviewed producer handoff CRT `c3daf0888882a70883b713b38728c8e54d8a8f9d`
and CLI implementation `502992e76e58f271114dc4abf97930cd5979d17d`.
The [macOS/arm64 consumer replay](acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-macosarm64-2026-11-05)
uses the retained clean `784389b` Development SDK, content
`72d1b37593abb2f5dd9d58a8338f44191b0b2023d6cbf2621cf7cd688ce80b4c`,
and its original `v0.0.1-rehearsal.6` archive/portable contract/manifest.
It owns the exact tested binary and input hashes, SDK-only build, store tests
and isolated receipt/repair/project evidence. The product/test source snapshot
is identical to the Linux increment below; the SDK/runtime combination differs.

This adopts CLI-owned receipt format 1 and explicit repair on this recorded
native combination, without changing producer/native-provider/install-metadata/
project-lock formats or SDK bytes. It is not a current-HEAD runtime rebuild,
a five-pair receipt qualification, artifact promotion or a default SDK/CLI pin.
Native Windows receipt replay remains open. Committed binaries, root locks,
default selection, trust roots and STATUS are unchanged.

## Installation Receipt Local Adoption (2026-11-05)

Reviewed the producer planning baseline `6f2892d` and CLI `4aa9a81`.
The [producer handoff](https://github.com/webos21/crt/blob/main/docs/design/sdk_development_contract.md#installation-time-contract-pin-handoff-crt-31)
reuses existing install metadata and referenced portable-contract assets.
CLI owns the implemented local `crt-cli.sdk-install-receipt`, format 1,
required-presence/digest enforcement and explicit `sdk repair` lifecycle.
SDK/native-provider/install-metadata/project-lock formats and SDK identities
are unchanged; `receipt_stored` is an additive install/repair result field.

The uncommitted product/test source snapshot is
`e380f275fffe299fcb6ab1fde1bea641e44ace9ef55c230554f51ed148f08694`;
tested binary `68f0535d03ec40d1ea719c8f9f214a1a884c7afeec30eff10bc0bc135cb553a7`.
[Consumer acceptance](acceptance/sdk_install_acceptance.md#installation-receipts-and-explicit-repair-linuxx86_64-2026-11-05)
records the exact copied dirty-local SDK/provider and labeled archive/manifest
fixture, native/store checks and isolated transaction/project proof. This is a
bounded Linux developer combination, not a promoted SDK or a new common native
matrix. Earlier clean Stage 2 runs retain their original artifact identities.

New clients refuse missing/untrusted legacy receipts for store-based adoption;
explicit repair requires the original archive and independently pinned release
manifest, never a hash inferred from current contract bytes. Known pins cannot
change or be downgraded; receipt-marked no-contract installs can gain the first
verified contract. Handoff roots and explicitly pinned external policy remain
explicit developer choices. Older clients do not enforce this store policy and
are not qualified receipt readers. Unknown/malformed receipts fail closed.

The new metadata replacement/recovery delta has the subsequent
[macOS replay](#installation-receipt-macos-replay-adoption-2026-11-05);
native Windows replay remains open. The root SDK defaults, committed bin
snapshots, project locks, trust roots and production adoption are unchanged. No release/promotion or
STATUS refresh is performed by this implementation increment.

## First User Workflow Planning Review (2026-11-05)

The subsequent [receipt implementation](#installation-receipt-local-adoption-2026-11-05)
has its own scoped execution evidence; this planning review does not stand in
for that proof.

Reviewed producer CRT `433a7d65b34ee25d71eb9d7c96976c817f5936cd` and consumer
CLI `3978cedbefa2abda7c4cd0c11ca1649912d7943a` before this owner-approved
documentation update. CRT owns SDK/portable compiler/recipe policy and artifact
promotion; CLI owns store receipts, tool selection, first-build binding and
acquisition. The [producer policy](https://github.com/webos21/crt/blob/main/docs/design/sdk_development_contract.md#change-impact-and-first-user-workflow-adopted-2026-11-05)
and [consumer execution/gates](design/crt_cli.md#execution-priorities-adopted-2026-11-05)
retain D0-D7 IDs while removing general updates/self-upgrade and Full metadata
from the first-flow dependency chain. D3 can proceed against fixed contracts.

Local install and Stage 2 project/contract selection already have five native
records, including [Windows/x64](acceptance/sdk_install_acceptance.md#toolchain-contract-and-project-sdk-selection-replay-stage-2-windowsx64-2026-11-05).
Their original SDK/CLI revisions differ and remain unchanged. The latest
[stored-contract verify increment](acceptance/sdk_install_acceptance.md#sdk-verify-covers-the-stored-contract-linuxx86_64-2026-11-05)
has Linux developer/unit evidence only; no common-latest-HEAD qualification or
binary refresh is inferred. Native dev is the supported consumer profile;
broader app profiles and Full installation are separate increments.

The next contract work is preserving installation-time contract digest and
required-presence state, with explicit recovery for existing stores. Its wire
format has not been assigned by this plan. First-build configure and explicitly
pinned remote acquisition are likewise planned, not implemented commands or
new schema capabilities. Dev supplies zlib, so D3 must prove actual isolated
source-build/store consumption and define collisions with the supplied library.

No SDK install metadata, native-provider, project/lock or recipe schema, product
source, artifact pin or runtime combination is changed here. Explicit technical
handoffs remain allowed; approved official trust and actual producer promotion
precede default public acquisition and production consumer adoption. No release,
nightly automation, test execution or publication is claimed. Root README/STATUS
were explicitly synchronized; detailed evidence remains in acceptance.

## Technical Bootstrap Gates and Common Source Contract Review (2026-11-02)

Reviewed producer documentation/tool commit
[`1537a9183419f238d03d6307aa991fc3e0959a0e`](https://github.com/webos21/crt/commit/1537a9183419f238d03d6307aa991fc3e0959a0e)
against consumer baseline `8fca58af136cfc50efed196529e23cf9b6572629`.
The owner closes work-queue D0.5/D0.6 at the five existing native combinations,
matching producer 1.5/1.6. The
[consumer technical review](acceptance/bootstrap_acceptance.md#technical-bootstrap-gate-review-2026-11-02)
links the original per-host identities and scoped execution evidence. This is
not a new runtime combination, a common latest-revision matrix, complete D0/D1
acceptance, or selection of a production SDK/provider/CLI pin.

Accepted the producer's
[common source-stage naming contract](https://github.com/webos21/crt/blob/1537a9183419f238d03d6307aa991fc3e0959a0e/docs/design/sdk_development_contract.md#common-source-stage-assets-adopted-2026-11-02):
one `crt-<version>-<os>-<stage>-source.tar.xz`, with the same selected bytes and
recipe size/SHA-256 across target architectures. Binary SDK archives remain
architecture-specific. This is policy adoption, not an implemented resolver
change or a new source-asset lock. Old candidate names/hashes remain historical
identities; they are not overwritten or silently replaced.

The [producer comparison](https://github.com/webos21/crt/blob/1537a9183419f238d03d6307aa991fc3e0959a0e/docs/acceptance/release_qualification.md#source-asset-comparison-checkpoint-2026-11-02)
records different supplied baselines and preserved stale libc++abi patch state;
it does not establish the cause of the missing original collision pair.
Canonical-source/cache repair, common asset/recipe regeneration, affected
validation and final candidate selection remained CRT 1.7 / CLI D0.7 work at
that review; the later [candidate selection](#common-source-candidate-selection-2026-11-02)
closes those items with new affected-scope evidence.
Regenerated inputs do not inherit the old artifacts' runtime acceptance.
Promote producer artifacts before adopting production consumer pins. This
review changes no executable, SDK payload, schema, provider format or lock,
and grants no public-release approval. Original immutable baselines and all
previous exercised-combination records remain unchanged.

## Independent Host-tools Windows Replay (2026-11-03)

Reviewed CRT `9d8be12e27802b87e0bca61fa562a281b1c7319b` plus the local producer
fixes, committed as [`b6bf52be64047e718dc0f2870472a5ffece913f9`](https://github.com/webos21/crt/commit/b6bf52be64047e718dc0f2870472a5ffece913f9),
against CLI `0b40a7087e567e8b7bfae2aa880d9f4f2e1abdb3` plus Python fixture/doc
edits. Product input hash `2aa32f07ea920ee4405970f194ccdf1183608f3ba8675ca28e60e80920930155` is unchanged from the
Linux/macOS increment. SDK/provider/host-tools/resources formats remain their
existing experimental versions; no new command/schema or production lock.

**Local native Windows/x64 replay passes.** Selected SDK is Development/windows/
x86_64 `development`, producer/base/compile `9d8be12` **dirty=true**, manifest
`31bcc5d0b2874bda33c17198f0c391c588ad9be32b9a59bbd57cf6f6a194b421`, content `8c3e90fe4fcaeda0e00e9748d6e90ec671af251b469d56cbc188be963a6a7057` and provider
`fdb73e4c11389b1085d90e0ba94a96c0f6ac0d9c0a1616f201f7e01c68e9b38a`. Independent tools `513eab805ed0753b67d00939fd8b00e03a577f6f14cbe74b2ad4e3862691b1cf`,
resources `d0f25131acba0fdaead34606aeaf6abefe556fe513009299f3bd9419c2f58602` and CLI `a9d922ef71fb042a8b2ed7419db7da3d5b078700937b661f14dd63d4c1bb223c` bind
this exact combination, not the intermediate failed candidates or a release.

The [producer record](https://github.com/webos21/crt/blob/b6bf52be64047e718dc0f2870472a5ffece913f9/docs/acceptance/host_tools_acceptance.md#windowsx64-independent-bundle-replay-2026-11-03) owns C/C++/tool assembly, runtime repairs,
licenses and build checks. The [Windows consumer record](acceptance/host_environment_acceptance.md#windowsx64-independent-host-environment-replay-2026-11-03)
owns isolated native/bootstrap/imports, independent tools/resources, selected
SDK/project and package-CA HTTPS results. Original artifact provenance remains
dirty; the implementation commit is not substituted for the build identity.

Together with the original Linux/macOS records, this closes the bounded local
three-primary-host D1 increment. It is not production promotion, a new ARM
replay, interactive terminal/source-port acceptance, installer support or
official publisher trust. D2 is next in the queue. Preserve prior combinations,
design baselines, failed receipts, committed binaries/defaults and locks.

## Independent Host-tools macOS Replay (2026-11-03)

Reviewed committed producer
[`518953f4d9ecc48c1d082644414e0a409a5a49ad`](https://github.com/webos21/crt/commit/518953f4d9ecc48c1d082644414e0a409a5a49ad)
and consumer `095afbcde4b0501267520381b24ba8114315d89a`. The previously adopted
experimental host-tools/resource/native-provider format 1 and project formats
are unchanged. New edits are replay tooling/documentation, not an SDK ABI,
consumer command/schema or product implementation change.

**Local macOS/arm64 technical replay passes.** The build SDK remains the verified
clean `8db69f4` Development/macOS/aarch64 `v0.4.0-cli.20261101`, content
`a438a4a08bc0bec98d53ad95d3cbc196525d655428086088508a3df9ea0f2f16`, provider
`489ebea23e8531cfdd57cd3ba526a163f18c42616e9a856961af49ace673a21d`.
Clean `518953f` assembles new independently pinned host tools from that SDK's
C-only predecessor; this is not a fresh SDK build at the assembler revision.
Selected tool manifest
`0a81db3eacb89b54c106d8f57bfd5a0e6bb57a24977e124f583e7c81751f1160`, resource
manifest `e3ea7042fef2258120c1e4bb0a3766c36e6c733196ecb9e6ffd0943c97dfe689` and CLI
`3aec79f15ec1b86d8abb23a9406549d7057c8b9fdbc6a0308b095d18f7d8b898` bind this
combination, not another host's candidate or a newly promoted lock.

The [consumer record](acceptance/host_environment_acceptance.md#macosarm64-independent-host-environment-replay-2026-11-03)
owns product input reconciliation, isolated self-build/807 native checks,
previous-binary bootstrap, 31 tools/resource cases, explicit SDK selection,
32 packaged-CA HTTPS/signature cases and their limits. The producer's
[macOS record](https://github.com/webos21/crt/blob/9d8be12e27802b87e0bca61fa562a281b1c7319b/docs/acceptance/host_tools_acceptance.md#macosarm64-independent-bundle-replay-2026-11-03)
owns assembly/native binding/license evidence. No private dylib collision
scenario is claimed for the actual static-tool/libSystem-only closure.

Producer replay tooling and evidence are committed as
[`9d8be12e27802b87e0bca61fa562a281b1c7319b`](https://github.com/webos21/crt/commit/9d8be12e27802b87e0bca61fa562a281b1c7319b).
That commit records the existing execution; the tested SDK, assembler and
bundle identities above are unchanged, not relabeled as builds of this commit.

This extends the bounded Linux adoption to an identified macOS combination,
not complete D1, production promotion, installer/source-port support or release
trust. Windows/x64 must replay the new increment. Historical combinations and
failed receipts remain intact; committed binaries/defaults, original immutable
design baselines, production locks and STATUS are unchanged.

## Review Baseline, Not an Artifact Lock

The 2026-10-21 documentation review inspected these local pre-change revisions:

| Repository | Inspected revision |
| --- | --- |
| CRT | `a23a1732f982e7bd60ecc01a6704a9d40cd3adc9` |
| CRT-CLI | `a76d8ad9e8bcd69923f24e2d303197461e99ddf3` |
| CRT Studio | `d3793e3d22f423fe589551b0822a1fc3b6e9fb78` |

Both consumer repositories contain documentation only at those revisions.
Their original adopted design baseline remains CRT
`d21430ddeb59aea76cf6abd58c15a69f55cfda52`. Neither that design revision nor
this review snapshot identifies a downloaded/validated SDK or a runnable
CLI/Studio combination. Record later contract adoption explicitly; never
silently follow `main` as an artifact identity.

## Local Candidate Workflow Review (2026-10-28)

Reviewed CRT `3c7f43747872de099eb627c51a0376befb40506b` (plus its existing
uncommitted numbered TODO plan) and CLI
`5fcdc995d231fe2f9dbab4292000a81e0b4fce93` before this working-tree documentation
update. The original design/review revisions above remain unchanged. This
review defines a development workflow; these revisions are not SDK artifact
locks, a schema implementation or an exercised runtime combination.

CRT owns local Dev SDK production, public interfaces and technical verification;
CLI owns the independent consumer and this candidate/adoption record. The
[producer assembly evidence](https://github.com/webos21/crt/blob/3c7f43747872de099eb627c51a0376befb40506b/docs/acceptance/release_qualification.md)
and [SDK split evidence](https://github.com/webos21/crt/blob/3c7f43747872de099eb627c51a0376befb40506b/docs/acceptance/sdk_split_acceptance.md)
do not grant CLI D0/D1 acceptance. The producer's implemented Dev packaging
profile is distinct from the proposed general SDK development contract.

### Local Candidates and Promotion

A candidate can be built/verified locally without a public SDK release. One
authorized session may work on both repositories; follow the
[handoff procedure](guides/development.md#local-dev-sdk-candidate-loop).
The CLI consumes a copied/extracted SDK outside producer trees. Access to CRT
source/build results for diagnosis does not make them consumer build inputs.

Record an actual local experiment with the combination fields below and links
to [D0 case evidence](acceptance/bootstrap_acceptance.md#local-candidate-boundary),
explicitly identifying its local/unpromoted state. Development results may be
Pass/Fail/Not run for a bounded case; they do not silently enter the approved
combination or promoted-lock set. Capture changed artifacts under new identities
and preserve failed records. No candidate or CLI execution is selected by this
documentation update; the runtime ledger remains Not run.

SDK/CLI versions and publication times are independent. Before final consumer
lock adoption/public CLI delivery, validate/promote the producer, replay against
the actual promoted SDK and adopt that exact artifact identity. Local candidate
success does not validate a different merge/release artifact, nor authorize
publication. No new lock/schema version or local-install command is introduced.

### Product Planning Clarification (2026-10-28)

This checkpoint precedes the owner choices and native experiment below;
the [later record](#owner-decisions-and-linux-bootstrap-2026-10-28) supersedes
its pending-choice/runtime status without changing the original review baseline.

The same working-tree review adds early initial-layout decisions, a
single-executable distribution goal, CLI self-upgrade, scoped doctor and
project-specific SDK version/profile selection. The prior dynamic-bundle
default is replaced as a product goal, not invalidated as historical producer
evidence. Source/module directories may evolve while the public project/build
contract remains the same as generated projects.

| Planned surface | Owner and review boundary |
| --- | --- |
| CLI executable/resources | CLI; CRT supplies link/runtime contracts; native one-file and SDK-free HTTPS proof required |
| CLI upgrade/doctor | CLI; explicit self/environment/project scopes and verified candidate activation/recovery; no project-pin mutation |
| Project SDK binding/new | CLI; exact SDK/profile/target/capability/content identities, with producer artifact/ABI rules retained |
| Physical storage/acquisition | CLI decision pending: project-local versus shared immutable payload; new/install/first-build timing not selected |

The executable prefix and detailed data/launcher/helper policy also remain
[open decisions](design/crt_cli.md#10-decisions-for-the-first-implementation).
No new schema version, backend support or implemented command is assigned by
this plan. [Bootstrap](acceptance/bootstrap_acceptance.md) and
[lifecycle](acceptance/lifecycle_acceptance.md) criteria are future gates; all
runtime/adoption results remain Not run. Original pinned baselines above are
unchanged and this update selects no actual SDK/CLI artifact combination.

## Owner Decisions and Linux Bootstrap (2026-10-28)

The owner selected `crt-cli`, shared immutable SDK payloads with exact project
references, first-configure/build acquisition (source-only new), and one shipped
executable allowing OS dependencies/data/certificates/cache/previous versions/
temporary helpers but no permanent launcher. Storage/acquisition/updater are
still planned mechanisms, not capabilities inferred from this decision.

This is the retained CMake checkpoint, superseded for active development by
the native JSON builder record below.

Local experiment `linux-d0-20261028` is **Pass for native Linux D0 only** and
**not promoted**. Original evidence and artifact identities are owned by
[Linux bootstrap acceptance](acceptance/bootstrap_acceptance.md#linuxx86_64-d0-bootstrap-2026-10-28).
It consumes clean producer SDK `3741ea515f129483efa631c6d2663d27de9ebc62`, Dev
profile at the rehearsal version, with exact archive/content hashes in that
record. CLI execution uses the exported dirty source based on
`cfd27bb5c21f95c056cf4567216c03cd32f7d44f`, snapshot
`b07b2ed65ac4918a6a3ea56747a8ebbe7ef44d74975cc71cb92bace8883a9e7c`.
No clean-source compile or matching public-release version is inferred.

Implemented subset: help/version/doctor --bootstrap, CLI-owned bootstrap SDK
input/result format 1, and host-side experimental manifest/lock format 0.
These are not general SDK development-contract v1, native project resolution
or adopted stable lock formats. `crt.lock` remains unresolved; CMake consumes
an explicitly fingerprinted local SDK handoff. Inspector support is currently
native, known-clean Dev input only, not arbitrary Full/dirty/legacy profiles.

Two CTest suites, actual argument/contract/configure rejection cases,
source/SDK-hidden replay and bare-image import/mapping evidence pass. The
compiled input and program bytes are identified in the original receipts.
Executable authentication and actual CLI HTTPS are not verified; D1-07,
macOS/Windows replay, installer/new/updater and formal promotion/adoption remain
open. Older source-asset filename variants remain a producer promotion gate;
this experiment does not substitute a different asset under a recipe.

## Native Manifest/Driver Experiment (2026-10-28)

**Pass, native Linux/x86_64 only; unpromoted.** The owner superseded the initial
CMake/Python prototype with `crt.json`, internal compiler-driver modules and
C/C++ `src/main`/`src/test`. The native app/control/self-build path invokes LLVM
argv directly and has no Python/shell/CMake/Ninja prerequisite. SDK production
keeps its own existing tools; optional upstream configure/make ports are separate.

The earlier clean SDK exposed the getdents64 type defect. Producer fix
`64b447ba40ef339a65ca009adeda80530a83689c` is used by a fresh clean local Dev SDK
`v0.4.0-cli.20261028`, with separately hashed experimental native provider format 0.
CLI base is `cfd27bb` plus source export
`5c5416312a73e0d7b2f8eec7bd94ab00d4db5e971cea1919a4ac83f6cd9ffe66`.
Exact SDK/archive/provider/binary hashes, owner fix, commands, failed attempts,
429 native assertions, SDK/source-hidden replay and import/maps audit are in
[native bootstrap acceptance](acceptance/bootstrap_acceptance.md#native-json-builder-on-linuxx86_64-2026-10-28).
The hosted seed and native self-build yield identical bytes in this experiment.

Bounded new/configure/build/test/run now exist with explicit local inputs.
Format-0 JSON locks distinguish unresolved from local-experiment; no stable or
promoted root pin is adopted. Automatic acquisition/store installation, self-upgrade,
CLI HTTPS, complete host-tool runtime attestation and macOS/Windows replay remain
open. These are not SDK Build Contract v1 or general D5/D6 completion claims.

## Committed Source Checkpoint (2026-10-29)

The source commits for the native Linux experiment are now explicit:

| Role | Commit / meaning |
| --- | --- |
| CRT runtime and SDK build baseline | `64b447ba40ef339a65ca009adeda80530a83689c`; unchanged clean local Dev SDK |
| CRT exporter/contract documentation | `17de3510c37cdb07e3c5c98583b22ebe4e1463ec`; includes the runtime fix and experimental exporter |
| CLI implementation | `1270695ee8e1ab53b661c4540af859cb2463d840`; native modules, manifest/lock, tests and pinned dependency headers |

The [producer contract](https://github.com/webos21/crt/blob/17de3510c37cdb07e3c5c98583b22ebe4e1463ec/docs/design/sdk_development_contract.md#experimental-native-compiler-provider-2026-10-28)
and [exporter](https://github.com/webos21/crt/blob/17de3510c37cdb07e3c5c98583b22ebe4e1463ec/tools/export_cli_toolchain.py)
are reviewed at that immutable source revision. This supplements the original
`d21430d` design baseline; it does not replace historical provenance.
All 19 non-Markdown inputs in the original receipt match the CLI implementation
commit. The exporter bytes match its recorded SHA-256. Execution still belongs
to the 2026-10-28 exported source/candidate; no later clean-build replay, new SDK
identity or acceptance of a changed artifact is inferred from committing files.

Documentation cleanup follows the implementation commit. It preserves original
receipts, failed attempts and the unresolved repository lock. No new code was
needed in this consolidation; version/bootstrap doctor was checked again using
the byte-identical review executable. Complete macOS/Windows/lifecycle gates
remain open, and repository pushes do not promote or publish SDK/CLI releases.

## macOS Native Candidate Review (2026-10-29)

**Local D0 Pass, unpromoted.** Reviewed clean CRT SDK build baseline
`17de3510c37cdb07e3c5c98583b22ebe4e1463ec`, CLI
`0f14ed3bee6b6512a6969b6118dcbd82c1c564b7` plus its exact exported macOS patch,
and unchanged Studio `27548e1c6d29b5a63d7eb6adce8aaf47b2f5d807`.
The [native macOS record](acceptance/bootstrap_acceptance.md#native-json-builder-on-macosarm64-2026-10-29)
owns SDK/archive/provider/exporter/source and bare-binary hashes, 474 assertions,
actual OS isolation, imports/maps, failed attempts and limits. The older Linux
experiment and accepted input identities remain unchanged.

CRT owns the macOS provider's explicit vendor SDK and Bionic runtime/startup
flags; CLI owns native host/format inspection, POSIX process execution and the
local project binding. Provider format 0 adds a macOS target and host-bound
`external_os_sdk` field; existing SDK manifest/profile format 1 is unchanged.
The SDK's copied bytes are not modified. Any later provider or SDK requires a
new exact identity/pin; no promoted root lock is adopted here. The unresolved
repository request is now `*`, while resolved projects still pin the selected
version/content/tool identities and reject version mismatch.

The bootstrap result remains experimental format 1, with additive allocation-
ownership evidence; this is not general SDK Build Contract v1 or stable D6
automation. SDK-free help/version/doctor is exercised, not complete D1 HTTPS,
installer, package/updater or Studio integration. Windows replay and clean
producer promotion/consumer adoption remain open. No public release or
cross-repository merge/lock update is authorized by this local candidate.

## Contract Inventory

This is the original pinned review inventory. The implemented bootstrap-only
formats and current local result are identified above; they do not assign
versions/support to the full proposed command/job/project contracts below.

Versions below are independent. A proposed first version is not an implemented
schema, compatibility guarantee, or public release.

| Surface | Owner / consumer | Observed format or proposal | Required validation |
| --- | --- | --- | --- |
| SDK `manifest.json` | CRT / CLI, Studio | Existing `format: 1`; not a development-contract version | Exact SDK identity, stage/target/options, inventory and consumer fixture |
| Isolated-stage recipe | CRT / stage builder | Existing `schema_version: 2`; not a port recipe | Predecessor/stage/OS and source/build recipe checks |
| Port recipe and executor | CRT technical policy; CLI adapter / CLI | Existing JSON `schema_version: 1`; adapter/registry version not assigned | Pinned installed driver/recipe/tests/shims and actual zlib static/shared build/run |
| SDK development contract | CRT / CLI, Studio | Proposed v1 sidecar/extension; schema/path not implemented | Schema/negative fixtures plus SDK-only consumer compile/link/run and runtime audit |
| Independent Host Tools Bundle | CRT / CLI | Proposed; independent contract version not assigned | Native tool/alias/rootfs/DLL execution; reject incompatible target tools on the build host |
| CLI command/final JSON/job events | CLI / Studio, automation | Draft; implemented wire versions/capabilities not assigned | Versioned fixtures, actual commands, errors/cancellation and separate output streams |
| Public UI capabilities for Studio | CRT / Studio | Existing public APIs; no separate versioned "Studio UI Profile v1" | Exact SDK capabilities and native GUI/input/IME evidence in Studio |

Source observations are pinned to the reviewed CRT revision:
[manifest producer](https://github.com/webos21/crt/blob/a23a1732f982e7bd60ecc01a6704a9d40cd3adc9/tools/create_dist.py),
[stage schema](https://github.com/webos21/crt/blob/a23a1732f982e7bd60ecc01a6704a9d40cd3adc9/distribution/stage_recipe.schema.json),
[zlib recipe](https://github.com/webos21/crt/blob/a23a1732f982e7bd60ecc01a6704a9d40cd3adc9/porting/recipes/zlib.json).
Existing CRT acceptance does not automatically satisfy these new consumer gates.
When a candidate is selected, record its actual formats rather than assuming
that later artifacts retain these versions.

## Windows Native Candidate Review (2026-10-29)

**Local D0 Pass, unpromoted.** Clean CRT SDK build baseline `d054a665612fc6b0008e5de5bdc40eb85688a4c8` (detached worktree,
`v0.4.0-cli.20261029`, `crt-c-test` 166/166), CLI `265105a51e4a17767419c237e33a73f3936d6a53` plus the exact local Windows
delta (source fingerprint in the
[Windows record](acceptance/bootstrap_acceptance.md#native-json-builder-on-windowsx64-2026-10-29)), Studio unchanged. Provider
format 0 gains a `windows/x86_64` target with `.exe`/`.dll` artifacts and `external_os_sdk` set to the Windows SDK import-library
directory; SDK manifest/profile format 1 is unchanged and the SDK inventory order is now defined component-wise and
case-sensitively. CRT owns the Windows wrappers, startup and the provider; the CLI owns `posix_spawn` execution, the
native-child hint for external tools, per-child runtime `PATH` and the `sdk_consumer_option_sources` project key
(experimental format 0 addition). The changed CLI sources are **not** replayed on Linux/macOS; those records remain valid only
for their original sources. Open producer items (Unicode paths, per-library consumer options, `fread` cost, process groups)
are listed in the Windows record. No promoted combination is adopted.

## Linux/aarch64 Native Replay (2026-10-30)

**Pass, local/unpromoted Linux/aarch64**, CLI `50bda2e96ecb29d4100ac3496053fb02b384ca2d`.
First replay of this host pair. The Dev SDK was built natively from CRT
[`641b034`](https://github.com/webos21/crt/commit/641b0346a08914c80c1a51fae443795e3418c06d)
plus an uncommitted CRT working tree: the toolchain exporter accepts Linux/aarch64
and `include/fcntl.h` carries the arm64 `O_NOFOLLOW`/`O_DIRECTORY` values (the
x86_64 values disabled `O_NOFOLLOW`). The SDK is explicitly dirty and used with
`--allow-dirty-sdk`; patch, test-file, SDK, provider and binary identities are in
[Linux/aarch64 replay](acceptance/bootstrap_acceptance.md#linuxaarch64-native-replay-2026-10-30).
No CLI code changed. The CRT header change is only replayed here: Linux/x86_64,
macOS and Windows must replay it before any combination is approved, and the CRT
changes need producer review and a clean promoted SDK. The root lock stays unresolved.

## Windows/arm64 Native Replay (2026-10-30)

**Pass, local/unpromoted Windows/arm64**, CLI `ec187d5944160b9eaf8f3a9c124ce1fbdeec7aa6`.
First replay of this host pair. The Dev SDK was built natively from a clean CRT
[`67f9516`](https://github.com/webos21/crt/commit/67f951626b9e7164d93fd6510cc25290ff7744d3)
(version `v0.4.0-cli.20261030`, not a tag). That commit follows the Linux/aarch64 `fcntl.h` change and adds the
Windows/arm64 target to the toolchain exporter and replay driver; the replay itself ran with CRT
[`9a31667`](https://github.com/webos21/crt/commit/9a316671ebd372df44cb0fd20b7138d5231a1cd3), which also makes the
exporter's argv recorder freestanding. The provider and SDK profile formats are unchanged. No CLI code
changed. SDK, provider, source and binary identities are in
[Windows/arm64 replay](acceptance/bootstrap_acceptance.md#windowsarm64-native-replay-2026-10-30). Windows/x64 has not
replayed the CRT changes, so no Windows combination is approved; the CRT commits need producer review and a promoted
SDK. The root lock stays unresolved.

## Windows/arm64 Fetch, Unicode and Release-asset SDK (2026-10-31)

**Partial pass, local/unpromoted Windows/arm64.** CLI `f786f85` plus the uncommitted `fetch` command
(experimental; contract in [bootstrap_contract](reference/bootstrap_contract.md#commands-and-results)) replays against a
Development SDK **extracted from a locally collected ZIP** of clean CRT
[`1c3d120`](https://github.com/webos21/crt/commit/1c3d120422451c98e65bd6344177945d3f4f01f5), version `v0.4.0-rehearsal.20261030`
(no tag, nothing uploaded). The ZIP's two symlinks are file copies, so its inventory differs from the producer directory's in
those entries; installer design must define it. The CRT test driver is
[`182d901`](https://github.com/webos21/crt/commit/182d90128908ac499f726e86b8c8701129785c3b). Because no CRT release exists for
this target, the promoted-SDK criterion is not met and no pin is adopted. Windows narrow-path handling is an open CRT/CLI
decision; `ws2_32.dll` is loaded by name from the PAL. Details:
[acceptance](acceptance/bootstrap_acceptance.md#windowsarm64-https-fetch-unicode-paths-and-release-asset-sdk-2026-10-31).

## Windows Narrow-String Model (2026-10-31)

**Implemented locally, replayed on Windows/arm64 only; unpromoted.** CRT
[`85546d9`](https://github.com/webos21/crt/commit/85546d9bd45a7466e75a30905e3db0113a7763c6) embeds a UTF-8 active-code-page
manifest in Windows executables through the SDK link policy (a new SDK file, `share/crt/windows/utf8.manifest`) and adds
`crt_get_narrow_encoding()` and the `crt-narrow-encoding-1` capability, which the exporter now requires. The provider carries
the link option, so the CLI hard-codes nothing, but CLI sources from this increment call the new query and need an SDK that has
it: **older SDKs and the other hosts' recorded replays do not combine with these sources** until their SDKs are rebuilt. A
combination also requires Windows 10 1903 or later for non-ASCII paths. See the CRT
[design](https://github.com/webos21/crt/blob/main/docs/design/windows_narrow_strings.md) and the
[implementation record](acceptance/bootstrap_acceptance.md#windows-utf-8-narrow-strings-implemented-2026-10-31). No pin is adopted.

## Linux Latest-source Revalidation (2026-10-31)

**Pass, local/unpromoted Linux/x86_64.** CLI
`36c6f65012a4166788b10d3ba225d67c67c428d5` plus the scope-only TODO edit replays
against a fresh clean CRT `f826397399eec66153451a18ba4fd0f7a1b5cbc0` Dev SDK
(`v0.4.0-cli.20261031`, a local parameter, no tag). The producer/base/compile
identities agree; `crt-narrow-encoding-1` is present, so the new PAL calls now
have native Linux evidence. No implementation fix was required. The exact
export, SDK content/inventory, compiler provider, runtime audit and 684-check
result belong to [Linux acceptance](acceptance/bootstrap_acceptance.md#linux-latest-source-revalidation-2026-10-31).

All 17 controlled HTTPS transport/integrity cases additionally pass with SDK,
producer/CLI source and provider unavailable. This is not publisher
authentication or complete D1-07. The new binary/local SDK config replace the
previous development selections with preserved backups. The tracked root lock
stays unresolved; no producer promotion or production pin is adopted. The
other native hosts retain their original snapshots and need current-source
replay independently.

## Experimental Publisher Verification and D1 Audit (2026-10-31)

CLI base `36c6f65` plus the separately identified signature/test delta consumes
the same clean CRT `f826397` Dev candidate. It adds optional explicit-key
RSA-PSS/SHA-256 verification using installed Mbed TLS, with no new SDK schema
or producer rebuild. Public-key ownership, official key distribution/rotation,
release-metadata binding and replay protection are unassigned; test keys are
not production pins. The actual native replay, signature negatives, real
repository HTTPS and host-tools findings are in
[D1 acceptance](acceptance/bootstrap_acceptance.md#d1-publisher-and-host-environment-audit-2026-10-31).
Complete D1 remains open, including the independent host-tools/CLI environment
contract and current-source Windows/macOS results. No official signing key,
release publication or production combination is established by this experiment.

## Owner-selected Test Identities on Linux (2026-11-01)

The owner selected disposable test certificates/keys for the current Linux
authentication scope, with Windows/macOS Not run. CLI `bd5359e` and producer
test driver `fa5dc30` pass all 32 controlled TLS/integrity/signature cases with
fresh identities and the original home tree hidden. This needs no official
key/URL and does not adopt a production trust anchor, SDK pin or new schema.
See [the exact replay](acceptance/bootstrap_acceptance.md#linux-test-identity-replay-2026-11-01).
The existing binary/SDK identities and earlier native replay remain unchanged;
full Linux D1 environment/resource work is still open.

## Windows/arm64 Latest-source Revalidation (2026-11-02)

**Pass, local/unpromoted Windows/arm64.** CLI `f36e858972c3342292a6bb09dc5fc18cb55eb8ef` plus one test edit replays against a clean CRT
[`d12e210`](https://github.com/webos21/crt/commit/d12e21013c1da6eed0d9238100df93c8febdba1c) Development SDK (`v0.4.0-cli.20261102`, not a tag). The earlier clean
`ee0b5e3` SDK does **not** combine with these sources: its Windows PAL leaves the `CRT_*` bootstrap variables in a CRT child's environment, which the CLI's
child-environment contract rejects. `d12e210` is the first CRT revision known to combine on this pair; Windows/x64 later combined with it too (see the next section). The OS adds
`PROCESSOR_ARCHITECTURE` to ARM64 processes and the CLI tolerates only that name. No pin or promoted combination is adopted. Details:
[acceptance](acceptance/bootstrap_acceptance.md#windowsarm64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02).

## Linux Merged-source Revalidation (2026-10-30)

**Pass, local/unpromoted Linux/x86_64**, CLI `ddaf288322e3292fbba53bc2bc4946c701ab5241`.
The Windows-shared process/hash/output/HTTP/cache changes now have fresh Linux
evidence. CRT baseline `a46e4307e2350c3c9de0826bd467bd32cae609c4` needed a
non-Windows `POSIX_SPAWN_SETPGROUP` implementation; the failed clean candidate
and corrected dirty source snapshot are separately identified in
[Linux revalidation](acceptance/bootstrap_acceptance.md#linux-revalidation-after-windows-changes-2026-10-30).
No CLI code changed; the original 15 code/config inputs match the executed export.

The corrected Dev SDK is `development`, explicitly dirty/unpromoted, with
producer patch/content/provider identities and native runtime audits in that
record. The root lock stays unresolved. The existing committed Linux executable
also bootstraps current sources against the new SDK through `.sdk.json`, then
rebuilds itself through the native backend; the full native suite belongs to
the separate passing replay. This does not adopt a production SDK
lock or validate a future clean/release artifact. The changed sources/common
PAL path still need native macOS replay; Windows' prior record remains at its
own source/SDK identities.

The producer fix is committed as
[`74ea992a4ef702c853435cb6319e329647db7cee`](https://github.com/webos21/crt/commit/74ea992a4ef702c853435cb6319e329647db7cee).
The exercised SDK remains the recorded `a46e430` plus dirty patch candidate;
committing/pushing the fix does not relabel it as a clean SDK build or adopt a
production lock. CLI source inputs remain byte-identical to `ddaf288`; the
consumer commit records documentation and the verified Linux test executable.

## Windows/x64 Latest-source Revalidation (2026-11-02)

**Pass, local/unpromoted Windows/x64.** CLI `1499a7b5675105a7db5301b17d406832e2962a0d` replays against a clean CRT
[`b4c5b5a`](https://github.com/webos21/crt/commit/b4c5b5a63196c48ea21a35bad8e73423488dd046) Development SDK (`v0.4.0-cli.20261102`, not a tag), which contains
the `d12e210` PAL fix. No CLI source change was needed: Windows/x64 does not add `PROCESSOR_ARCHITECTURE` (measured with Python), so the arm64 test tolerance is
unused there. The earlier x64 SDK (`fa5dc30` plus dirty delta) predates the child-environment check and is not the combination to cite for these sources.
No pin or promoted combination is adopted. Details: [acceptance](acceptance/bootstrap_acceptance.md#windowsx64-latest-source-revalidation-and-spawn-environment-fix-2026-11-02).

## D2 Local SDK Installer on Windows/x64 (2026-11-04)

**Pass, local/unpromoted Windows/x64.** CLI `2df5610` plus the Windows edits (archive streams opened by `path.string()`, POSIX mode checks skipped in the
archive test) installs a rehearsal Development SDK zip (`v0.0.1-rehearsal.3`, install metadata format 1) built from clean CRT
[`1ee0379`](https://github.com/webos21/crt/commit/1ee037923ef6a3460c6f99c6e426d294fe5b7025). CRT `a48000c` alone does **not** combine with these
sources on Windows: its Development SDK build fails `verify_dist` (the xz DLL is recorded as `lib/lzma.dll`) and the shared consumer does not link
liblzma; `1ee0379` is the first CRT revision known to combine on this pair. Install metadata format 1 and the SDK profile are unchanged. No pin or promoted
combination is adopted. Details: [acceptance](acceptance/sdk_install_acceptance.md#windowsx64-isolated-replay-2026-11-04).

## Adoption and Exercised Combinations

There is **no promoted production combination yet**. Local bootstrap replays
exist for all five native pairs at their recorded source/SDK identities;
the latest fetch/encoding sources pass at the recorded Windows/arm64,
Linux/x86_64, Windows/x64 and [macOS/arm64](acceptance/bootstrap_acceptance.md#macos-latest-source-revalidation-2026-11-01) snapshots.
Linux/aarch64 passes the latest sources in the [2026-11-02 replay](acceptance/bootstrap_acceptance.md#linuxaarch64-latest-source-revalidation-2026-11-02).
The earlier [Windows/x64 replay](acceptance/bootstrap_acceptance.md#windowsx64-latest-source-revalidation-2026-11-01)
builds and passes UTF-8/HTTPS checks, but fails owned-descendant timeout cleanup;
that failed combination is retained. The subsequent
[owned-spawn fix](acceptance/bootstrap_acceptance.md#windowsx64-owned-descendant-timeout-fix-2026-11-01) passes
the full native suite with a new local SDK.
Complete D1 and Studio S0/S1 remain open. Use the owning acceptance ledgers for scope, not a general
claim that the SDK/CLI/Studio product combination is compatible.

Add a combination record only with these fields and links to original evidence:

- Review date, scope and producer/CLI/Studio revisions, including dirty-source
  identity if applicable; separate original design provenance from adoption.
- SDK archive size/digest, source/build/content identity, stage, target and
  options/capabilities; build host, host-tools and external-toolchain identities.
- Adopted development/host-tool/recipe/CLI schema versions, or explicit
  draft/unavailable status; implemented command/capability subset consumed.
- Actual CLI distribution/resource/helper policy and project SDK storage mode;
  keep SDK content profile, build mode and physical backend distinct.
- Original case IDs/evidence for fixtures, consumer build/link, native runtime,
  SDK-free HTTPS bootstrap, and Studio live integration as applicable.
- Outcome per scope/host: **Pass**, **Fail**, **Not run**, **Skipped** with
  reason, or **Compile/package only**. Fixture results never stand in for live
  command/GUI evidence; omitted capabilities are not implicitly approved.
- Owner review, limits, rejected/previous combinations, migration/rejection
  policy and exact lock updates. D7 formal approval requires the applicable
  CLI and Studio integration evidence, not merely a version match.

Early SDK-only UI PoCs and protocol fixtures do not wait for D7. Live project
build/run requires implemented workflow commands; JSON drafts begin in D0 and
applicable job/cancellation/package behavior is stabilized in D6. Studio owns
its detailed feature prerequisites in `docs/design/crt_studio.md`; tranche
numbers or fixture success alone are not capability detection.

## Shared-change Review

The producer owns the common review procedure in
`crt: docs/design/sdk_development_contract.md`, section 8 (Cross-repository
Contract Review), in the [CRT repository](https://github.com/webos21/crt).
Use it for owner/baseline changes, version/capability/error/lifetime changes,
proposed vs implemented commands, TODO-to-acceptance links, and producer-first
artifact promotion followed by consumer lock adoption. Preserve the original
design baseline and previous working combinations; do not rewrite them.

This ledger records adoption through linked repository changes, not atomic
three-repository merges. Review source references at immutable revisions before
claiming a new baseline. An unimplemented interface remains a proposal even if
all three documents describe it consistently.

The checklist is manual now. Lightweight automated reference/owner/version/
capability and TODO-to-acceptance checks are planned, not an existing CI job.
They must not require identical Markdown across owners or rebuild WebKit for
every documentation edit. Remote link availability was not a runtime test.

## macOS Merged-source Revalidation (2026-10-30)

**Pass, local/unpromoted macOS/arm64**, CLI `754ca2992e7b6b2ffa339d965f6085f05f505297`.
The Windows-shared process, hash, output, HTTP, cache and `.sdk.json` changes now
have macOS evidence against a Development SDK built from CRT
[`74ea992`](https://github.com/webos21/crt/commit/74ea992a4ef702c853435cb6319e329647db7cee)
(dirty only by the macOS replay driver; used with explicit dirty-SDK acceptance).
The common `POSIX_SPAWN_SETPGROUP` fix works on macOS. The previously committed
macOS executable bootstraps the current sources, and the result rebuilds them
through `.sdk.json`; the two builds match in code and data segments but not in
`LC_UUID`/`__LINKEDIT`, which ld64 varies per build. Details and hashes:
[macOS revalidation](acceptance/bootstrap_acceptance.md#macos-revalidation-after-windows-and-linux-changes-2026-10-30).
No production lock is adopted and the root lock stays unresolved.

## Windows/x64 Latest-source Review (2026-11-01)

**Partial pass; full native regression fails.** CLI
`e66abf2e96a19c6c8c5b3ad59ffcbec48f666b1c` plus a scope-only TODO edit consumes
a fresh clean CRT `fa5dc305efa3cf3c807786550dd727023cad8bd3` Development SDK,
local version `v0.4.0-cli.20261101`. It provides the implemented
`crt-narrow-encoding-1` capability and native-provider format 1; no new schema
is introduced. Exact SDK/provider/tool/source hashes, producer tests and raw
receipts belong to [Windows acceptance](acceptance/bootstrap_acceptance.md#windowsx64-latest-source-revalidation-2026-11-01).

Build/self-build, Unicode app workflows, SDK-hidden runtime/import checks and
32 controlled HTTPS/signature cases pass. The aborting native suite proves a
descendant survives timeout: Windows PAL process-group kill is `ENOSYS`.
Later suite cases are Not run. The producer owns the required process fix;
test-host Python/OpenSSL limitations are recorded separately. Earlier passing
snapshots remain historical and do not qualify this combination. Committed
host binaries and the unresolved root lock are retained; no production pin,
official trust anchor, release or complete D0/D1 approval is adopted.


## Windows/x64 Owned-spawn Fix Review (2026-11-01)

**Pass, local/unpromoted Windows/x64.** CLI implementation base `e66abf2`
consumes CRT `fa5dc30` plus the identified uncommitted PAL/test delta via a new
`development` Dev SDK. Public Bionic signatures and native-provider format 1
are unchanged. Job-backed owned spawn cancellation fixes the preceding failure;
general groups/catchable signals remain unsupported. Exact source, SDK/tool,
provider and binary hashes and producer/consumer receipts are in
[acceptance](acceptance/bootstrap_acceptance.md#windowsx64-owned-descendant-timeout-fix-2026-11-01).

The full 788-check native suite, SDK-hidden runtime/import
audits and 32 transport/signature cases pass. The original failed combination
is retained. The Windows/x64 binary/local defaults are refreshed with backups;
no production lock or promoted SDK is adopted. Windows/arm64 replay, broader
D1 environment/resource gates and official release trust remain open.

### Committed Producer Reconciliation (2026-11-01)

Producer source/contract/evidence commit
[`8db69f404dcfa70b5aa0569cae51a962fd0615d2`](https://github.com/webos21/crt/commit/8db69f404dcfa70b5aa0569cae51a962fd0615d2)
contains the exact three code/test inputs of the passing Windows/x64 candidate.
It was committed after testing; the SDK's `fa5dc30` plus dirty-delta build
identity, provider, binary and failed snapshots are not relabeled. No clean
SDK rebuild, producer promotion or production-lock adoption is claimed.

## macOS Latest-source Review (2026-11-01)

**Pass, local/unpromoted macOS/arm64.** Unchanged CLI product sources at
`a9d43c661f73be2081300695c0337e9e57b60c18` consume a fresh clean CRT
`8db69f404dcfa70b5aa0569cae51a962fd0615d2` Dev SDK, local version
`v0.4.0-cli.20261101`, with `crt-narrow-encoding-1`. The separately hashed
native compiler provider remains format 1; no shared schema or ownership
change is introduced. CLI bootstrap/project formats remain experimental 0.

SDK content `a438a4a08bc0bec98d53ad95d3cbc196525d655428086088508a3df9ea0f2f16`, provider
`489ebea23e8531cfdd57cd3ba526a163f18c42616e9a856961af49ace673a21d` and binary
`72577b9b7dd1e602635ccdabef0fe728e66441c7bd9b07607ae8994ff2de949e` bind the measured combination. Native tests, the
old-binary self-bootstrap loop, SDK-hidden doctor/imports, 32 controlled
HTTPS/signature cases and pinned-repository HTTPS pass. Full product-input
and runtime details belong to
[macOS acceptance](acceptance/bootstrap_acceptance.md#macos-latest-source-revalidation-2026-11-01).

The local binary and ignored config are refreshed; clean provenance is
explicitly exercised without dirty opt-in. Root lock remains unresolved and
no production artifact is adopted. Previous identities are retained. The
SDK-derived make space-SHELL failure remains a host-environment limit;
independent host-tools, shipping trust/resources, general D1 and release
qualification are not approved by this bounded replay. Original immutable
design baselines and earlier host records remain unchanged.

The producer evidence is committed as
[`c4e19f3`](https://github.com/webos21/crt/commit/c4e19f3e47f6fd37f3d0d03c027bec5cfe251677).
That documentation commit records the existing clean `8db69f4` build; the SDK
provenance, content and provider identities above are unchanged. This consumer
commit records the reviewed evidence and verified macOS executable.

## Experimental Child-environment Diagnostics (2026-11-02)

The consumer adds format-0 `env` diagnostics for control, app-build and app-run
using the same environment builder as process execution. This is a CLI-owned
command/result addition; it changes no producer compiler-policy schema, SDK
inventory or pinned design baseline. Project profiles reuse the existing exact
SDK/tool binding, with build host and target recorded separately. No independent
host-tools identity, rootfs layout or port-build adapter is inferred from SDK
contents. `port-build` is rejected until a producer-owned contract is adopted.

Linux validation retains the clean CRT `f826397` handoff/provider from the
preceding Linux record. Source export, new binary, isolation boundaries and
native outcomes belong to the
[environment acceptance record](acceptance/bootstrap_acceptance.md#linux-child-environment-diagnostics-2026-11-02).
Earlier Windows/macOS passes apply to their recorded sources; this command
increment needs its own native replay there. Official trust, SDK promotion and
complete D1 acceptance remain separate.


## Linux/aarch64 Latest-source Review (2026-11-02)

**Pass, local/unpromoted Linux/aarch64.** CLI `9e0110ff2bc9f7030ab66c726bac2286e5526cc2`
consumes a fresh clean CRT `c4e19f3e47f6fd37f3d0d03c027bec5cfe251677` Development
SDK at local version `v0.4.0-cli.20261102`, with `crt-narrow-encoding-1`.
Native-policy format 1, existing profile formats and original immutable design
baselines are unchanged. No producer/consumer code change was needed.

Exact SDK/content/provider/source/binary identities, 766 native checks,
previous-binary bootstrap fixed point, bare import/mapping and SDK-free
32-case HTTPS/signature evidence belong to
[acceptance](acceptance/bootstrap_acceptance.md#linuxaarch64-latest-source-revalidation-2026-11-02).
The clean SDK is selected without dirty opt-in in the bootstrap loop. Local
binary/defaults are refreshed with previous bytes retained. This closes this
pair's latest fetch/encoding/environment replay, without promoting an SDK,
adopting a production lock or approving complete Tranche 1/D1. Windows/arm64
process-fix replay, coherent exact assets and broader D1/resource gates remain
open. Producer records remain owned by CRT SDK split acceptance.

## Common-source Candidate Selection (2026-11-02)

**Selected local technical combination; CRT 1.7 / CLI D0.7 complete.** Producer
[acceptance](https://github.com/webos21/crt/blob/8616fd64b13fa41925c1fce2b08f4937b7e8dcf1/docs/acceptance/release_qualification.md#common-source-candidate-selection-2026-11-02)
selects canonical common Linux 02/03 archives from clean CRT
`49879bf82e2742baeb1878e492d42c7d561b6f7a`, version
`v0.4.0-sourcefix.20261102.5`, after rejecting polluted caches and regenerating
new candidates. It owns upstream/recipe/asset identities, ordinary C++/Dev
consumer evidence, exact nested-recipe validation and aarch64 cross-build scope.
Producer closure documentation is reviewed at `8616fd6`; it does not relabel
the SDK built from `49879bf`. Same version/OS/stage means the same source archive and recipe hash on either
CPU; startup, loader and compiled code still follow the SDK's target identity.

Consumer code baseline is `7cab48e03957b603e010738429a9226129af37e4` (unchanged
product inputs). The selected external Linux/x86_64 Development SDK content is
`cf2eaf9912ff66594dc0c9d403fddfd25cae620f91d46e2ab01fc7a9aa53f658`, native
provider `60ea71f7e55defec6a4faaeb0862e0e01a42dc35bbc39a5a5c43ffe9b15f5185`;
Clang/LLD/LLVM ar 21.1.8. The selected bare binary is
`6457b1a60038860d02452e5216df8fbcee9297e5e011b26050f4aee203c6efc8`.
SDK/base/compile provenance is clean at the producer commit above. Existing
SDK/profile, experimental native-provider format 1 and project/lock format 0
are unchanged; this is no new general SDK contract or installer/resolver API.

The original [consumer replay](acceptance/bootstrap_acceptance.md#common-source-candidate-replay-2026-11-02)
records exact inventory/result hashes, 754 native checks, cold/warm cache,
producer-tree-hidden SDK consumption, ELF/PAL imports and the previous-binary
bootstrap fixed point. `bin/linux-x64/crt-cli` and ignored local defaults now
select that handoff, with backups. Product-input reconciliation distinguishes
later documentation/binary edits from executed source exports.

This selects a bounded **technical candidate**, not a promoted production
combination. Historical five-pair native results keep their own identities;
the new ARM result is cross compile/package/link only, and no new macOS/Windows
SDK execution is claimed. D1 HTTPS/signature, independent host-tools/port-build,
shipping resources/trust and later installation/upgrade gates retain their own
scope. The root lock stays unresolved, old source variants are not overwritten,
and no tag, upload or public release is performed. Producer promotion must
precede any future production consumer pin. STATUS is untouched.

## Independent Host-tools and D1 Linux Adoption (2026-11-02)

Reviewed CRT `8616fd64b13fa41925c1fce2b08f4937b7e8dcf1` and CLI
`adc03b04ed124c71f0726a895da2cc2cde98b78d` before this working-tree increment.
CRT owns the implemented local-only `crt.experimental.host-tools`, format 1,
[contract](https://github.com/webos21/crt/blob/main/docs/design/host_tools_contract.md)
and [producer evidence](https://github.com/webos21/crt/blob/main/docs/acceptance/host_tools_acceptance.md).
CLI adopts exact caller-selected manifest pins and owns `port-build`, bounded
script execution, scoped doctor and `crt-cli.bootstrap-resources`, format 1.
The latter is an executable/notices/trust inventory, not a new SDK format,
release-metadata schema or publisher key policy. Unknown format/host, altered
payload/aliases and missing execute permission fail before a job; no host
fallback or implicit SDK upgrade is introduced.

The Linux technical combination uses the same clean `49879bf` Dev SDK/provider
and LLVM identities as the common-source selection above. The source snapshot
`2aa32f07ea920ee4405970f194ccdf1183608f3ba8675ca28e60e80920930155` is
uncommitted, and produces CLI
`9b82bd300379b45be4cb36becfbef56bf41b1ff5cc3d2cb510b9e60447b10284`.
Independent C-only-derived host-tools `v0.1.0-hosttools.20261102.4` use manifest
`59bdf8f83ce7817175010953abe652fe6f52852f8aff9a5528f2ce8b64812652` and payload
`3b75fb9bbab8c9e9a1c7a542f5ced227f1c66bb92ed875e87d78f86e1aba11bf`.
The resource manifest is
`654e31b1ca2e7a37072d59fcf9ecbc8b997ff346dde3ca6930617acc2645cf05`.
Its pinned CA and explicit resource/fetch signing identities are disposable
tests; they are not official publisher credentials.

[D1 Linux evidence](acceptance/host_environment_acceptance.md) records 767 native
checks, cold/warm build, 31 isolated host-tools/resource cases and 32 isolated
HTTPS/signature cases using the selected package CA. Actual imports, argv,
source lists, old failures and artifact hashes remain there. CLI diagnostics
separate execution host, tools and optional native project SDK; they do not
implement a cross-target recipe/compiler adapter. `bin/linux-x64/crt-cli` is
refreshed with a backup. Other hosts' committed test binaries remain at their
recorded earlier source snapshots and do not inherit these new features.

This is Linux local adoption only. Native Windows/x64 and macOS/arm64 replay
of the new tool/resource/scoped-doctor/process boundary remains open, including
private initial-load resolution. Root locks and SDKs stay unchanged; no product
or host-tools artifact is promoted/published. Producer promotion remains required
before any future production consumer lock. Interactive terminals, source-port
compiler bridges, installer/upgrade transactions and official release trust
retain their separate owners/gates. STATUS is untouched.
