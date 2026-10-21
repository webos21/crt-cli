# HISTORY: CRT-CLI Completed Work

Reverse-chronological, dated record of completed work. Current and planned
steps belong to [TODO](TODO.md). Designs, procedures, and measured acceptance
keep their own detailed records under [docs](docs/README.md).

Dates identify when the work was completed or documented. Add a span when work
crosses days; include commit/reproduction references when available without
inventing commits or test results. Older months may later be summarized under
`docs/history/` with links to their original Git records.

## 2026-10-21

### Connect producer contracts, bootstrap acceptance and Studio adoption

Reviewed CRT `a23a173`, CLI `a76d8ad` and Studio `d3793e3` against the supplied
cross-repository assessment. Added [Contract Compatibility and Adoption](docs/compatibility.md)
with exact review revisions, independent existing manifest/recipe formats,
draft/unassigned contract versions and evidence requirements. The original
design provenance remains pinned; no runtime combination is approved.

Added D1-07 SDK-free HTTPS metadata/artifact verification, shipped-runtime and
trust rejection cases; D2 still owns SDK installation transactions. Clarified
that JSON drafts begin in D0, D6 stabilizes automation, and formal D7 approval
does not block SDK-only Studio S0 or real earlier capability experiments.
Updated TODO, index, design and agent rules to use the adoption record and
producer-owned shared-change procedure. Automated checks remain planned.
This is documentation work only: CLI sources/build targets and native
bootstrap acceptance are still absent, with runtime ledgers left Not run.
Documentation validation checks local links/anchors, English text, fences,
conflict markers and whitespace; external URLs were not network-validated.

## 2026-10-20

### Establish the documentation layout and initial work queue

Created the user-facing README, documentation index, local CLI design,
development guide, and D0/D1 bootstrap acceptance contract. The local design
adopts the CRT cross-repository baseline at
`d21430ddeb59aea76cf6abd58c15a69f55cfda52`, records its origin, and becomes the
owner of subsequent CLI-specific design decisions. SDK and Studio contracts
remain linked to their producer-owned documents instead of being copied here.

Initialized TODO with D0 consumer contracts/bootstrap definition as the current
focus and D1-D7 as ordered plans. The README distinguishes intended command
examples from available features; the guide does not invent CMake presets or
test commands, and acceptance contains no fabricated runtime pass results.
Documentation checks cover local links/anchors, English text, code fences,
and whitespace. No CLI implementation, SDK build, runtime replay, commit,
publication, or release approval is claimed by this documentation work.

### Define repository agent guidelines

Created [AGENTS.md](AGENTS.md) from the pinned CRT CLI design. It establishes
CRT-native C/C++ and SDK-only consumption, Windows CRT host tools/rootfs,
source-first ports, immutable identities/installations, structured command and
process contracts, staged verification, and English documentation ownership.
The initial repository has no product build or executable yet; the guidelines
explicitly distinguish proposed contracts from implemented functionality.
