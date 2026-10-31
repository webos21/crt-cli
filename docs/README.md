# Documentation

Start with the [project introduction](../README.md),
[CLI design](design/crt_cli.md), and
[development guide](guides/development.md).
Current work is in [TODO](../TODO.md); completed work is in
[HISTORY](../HISTORY.md). [AGENTS.md](../AGENTS.md) defines development rules.

## Architecture and Policy

- [CRT-CLI: Development Environment and Source-first Ports](design/crt_cli.md):
  product boundaries, installation layout, environment/recipe/project contracts,
  and D0-D7 dependency order. This repository owns subsequent CLI design.
- [Contract Compatibility and Adoption](compatibility.md): independent existing
  formats vs proposed contracts, exact baseline/adoption records and shared
  review links. Local Linux bootstrap evidence exists; no promoted production
  SDK/CLI/Studio combination is approved yet.

The initial producer-owned design baseline remains pinned below. The later
[committed native-provider review](compatibility.md#committed-source-checkpoint-2026-10-29)
records the exporter/contract and CLI implementation revisions independently
from the SDK build baseline:

- [SDK Development Contract](https://github.com/webos21/crt/blob/d21430ddeb59aea76cf6abd58c15a69f55cfda52/docs/design/sdk_development_contract.md).
- [CRT Studio boundary](https://github.com/webos21/crt/blob/d21430ddeb59aea76cf6abd58c15a69f55cfda52/docs/design/crt_studio.md).

## Development and Usage Guides

- [Development and Bootstrap Guide](guides/development.md): how to approach the
  initial implementation, [test local unreleased Dev SDK candidates](guides/development.md#local-dev-sdk-candidate-loop),
  record SDK/tool inputs, and document real commands.
  [Linux bootstrap build/test entry points](guides/development.md#build-and-test-the-bootstrap)
  now exist; the full installer/project workflow remains planned.

## Acceptance Contracts and Evidence

- [D0/D1 Bootstrap Acceptance](acceptance/bootstrap_acceptance.md): SDK-only
  CLI consumption, initial layout/decisions, single-executable feasibility,
  SDK-free self-doctor/HTTPS and Windows host-environment gates, and
  the evidence format. Planned criteria are not passing results.
  The [D1 audit](acceptance/bootstrap_acceptance.md#d1-publisher-and-host-environment-audit-2026-10-31)
  separates measured Linux/explicit-key cases from official trust and native-host gaps.
- [CLI Lifecycle Acceptance](acceptance/lifecycle_acceptance.md): future
  self-upgrade/activation/recovery, per-project SDK/new, scoped doctor and
  shipping-artifact gates. Complete lifecycle gates remain Not run; partial
  project-generation/self-build evidence is linked to native D0 acceptance.
- [Implemented Bootstrap Contract](reference/bootstrap_contract.md): SDK input
  and command/result formats, experimental project fixtures and actual limits.

## Document Ownership

| Document | Owns | Does not own |
| --- | --- | --- |
| Root README | User-facing purpose, maturity, navigation | Detailed roadmap, per-host measurements |
| AGENTS | Contributor/agent rules and boundaries | Running work status |
| TODO | Concise In Progress and Planned steps | Completed investigation logs |
| HISTORY | Dated completed changes and why they were made | Future work queue |
| `docs/design/` | Architecture, contracts, decisions, dependency order | Duplicate completion checklist |
| `docs/guides/` | Actual setup/build/use/reproduction procedures | Claims of unimplemented commands |
| `docs/acceptance/` | Acceptance requirements, measured evidence, limitations | General user introduction |
| `docs/compatibility.md` | Contract adoption and links to exercised combinations | Duplicate SDK/CLI/Studio measurement logs |

Write documentation in English. Update the owning document and link to it
rather than restating the same status in multiple places. Each completed
increment moves its detailed outcome to HISTORY and applicable acceptance;
remove it from TODO, except brief checked prerequisites needed for an active
sequence. Keep unsuccessful attempts separate from accepted evidence.

Do not routinely create/update STATUS; it is an explicitly requested snapshot,
not another work queue. Add reference, release, or monthly-history directories
only when actual content needs them. A later history archive must preserve
decisions, reproduction entry points, limitations, and original Git references.

The local CLI design is adopted from a pinned CRT cross-repository baseline,
not a live mirror. Coordinate future shared SDK contract changes explicitly.
Keep CRT ABI/runtime measurements in CRT acceptance and link their original
evidence. Keep CLI installation/port/project results here; later Studio UX
results belong to Studio.
