# Contract Compatibility and Adoption

This document owns the cross-repository contract adoption record, not SDK ABI
policy or a duplicate test log. Producer evidence belongs to CRT acceptance;
CLI behavior to [bootstrap acceptance](acceptance/bootstrap_acceptance.md);
Studio GUI/editor/integration evidence to Studio acceptance. Architecture and
work order remain in [the CLI design](design/crt_cli.md) and [TODO](../TODO.md).

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

## Contract Inventory

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

## Adoption and Exercised Combinations

There is **no approved runtime combination yet**. CLI D0/D1 and Studio S0/S1
remain **Not run**; see their owning acceptance ledgers. No SDK/host-tool
candidate, CLI executable, or Studio executable was selected or run during this
documentation review. Design consistency is not runtime compatibility.

Add a combination record only with these fields and links to original evidence:

- Review date, scope and producer/CLI/Studio revisions, including dirty-source
  identity if applicable; separate original design provenance from adoption.
- SDK archive size/digest, source/build/content identity, stage, target and
  options/capabilities; build host, host-tools and external-toolchain identities.
- Adopted development/host-tool/recipe/CLI schema versions, or explicit
  draft/unavailable status; implemented command/capability subset consumed.
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
