# GitHub CI Acceptance

This record owns what the CI driver and workflow have actually been run against. The design and the
procedure are in the [development guide](../guides/development.md#continuous-integration); open work is in
[TODO](../../TODO.md#github-ci). A passing local rehearsal is not a GitHub-hosted run.

## Linux/x86_64 local rehearsal of the driver (2026-11-10)

Scope: `tools/ci_bootstrap.py` on one native host with the real public downloads, no workflow engine.

| Item | Exact identity |
| --- | --- |
| Source under test | CLI `0cae8c8bc1fee02a31baf230aba703460fe1afa3` plus this change's uncommitted CI files and the `bin/` removal (product, test and manifest sources are unchanged) |
| Bootstrap CLI | Published `v0.1.0-preview.20261110.linux-x86_64.1`, archive SHA-256 `ad43afbf...1383` (matches the release `SHA256SUMS`), executable `a8fb76a4...a43a` |
| SDK | Public `v0.0.1-preview.20261110.3`, acquired through `github-release` redirects; installed content `36623957...2b3f` equals the pin |
| Host tools | Ubuntu 26.04.1, Ubuntu clang/LLVM 21.1.8 at `/usr/lib/llvm-21/bin`, GNU readelf 2.46, Python 3.14.4 |
| Command | `python3 tools/ci_bootstrap.py --work <new-dir> --llvm-bin /usr/lib/llvm-21/bin` (private `HOME` inside `--work`) |

| Gate | Result |
| --- | --- |
| Pin check, version, `sdk acquire` of the pinned SDK from GitHub | Pass (3.8 s) |
| Stage 1 (bootstrap CLI, explicit `configure` to the installed SDK + its stored contract) | 21 executed / 0 reused (84 s) |
| Suites on the stage-1 executable: native / archive / sdk-store / port-recipe | 901 / 221 / 114 / 141 checks pass, none skipped (same counts as the published Preview record) |
| Stage 2 (stage-1 copy, first build with `--sdk`, rebuilt at stage 1's path) | 21 executed / 0 reused; unchanged rebuild 0 executed |
| Stage 1 vs stage 2 bytes | Identical, SHA-256 `7f622894...ca61` |
| Import audit | No `DT_NEEDED`, `RPATH` or `RUNPATH`; interpreter `/lib64/ld-linux-x86-64.so.2` only |
| Executable alone (no SDK store, empty `PATH`, fresh home): `--version`, `--help`, `doctor --bootstrap` | Pass for both stages, identical checks |
| Negative controls: wrong bootstrap digest; test unclassified in `pins.json`; test in both lists | Each fails with its own message, exit 1 |

Findings fixed during the rehearsal (the first two attempts failed, both in the driver, not the product):

- `native-test` reads `sdk_root` and `sdk_contract` from the project's `.crt/local.json` because it drives explicit
  `--sdk-root`/`--sdk-contract` flows. A first-build store binding deliberately keeps no machine path, so stage 1 is bound by
  `configure` against the store directory and the contract stored beside it.
- `__FILE__` and `-g` debug info embed the absolute source path, so stages built in different directories never match
  byte for byte. Stage 2 is therefore rebuilt at stage 1's path; the earlier "different bytes" result was this, not drift.
- The bare-run check reused one directory for both stages; it is now per stage.

## Not run

- No GitHub Actions run exists yet: the runner images, the pinned LLVM 21 apt install (the key block was verified against
  `apt.llvm.org`'s `InRelease` signature and a sudo-free copy of the commands, not on a runner), Python 3.12 on the runner,
  `actions/*` at their pinned commits and artifact upload are unexercised.
- The driver's macOS/arm64 and Windows/x64 code paths (system-root export, `xcrun`, `otool`, `llvm-readobj`, Windows SDK
  discovery, LLVM 22.1.8 install) are written from the published Preview records but have not been executed.
  Both legs run with `continue-on-error` until they pass.
- Input-gated port suites, redirect/transport replays, Linux/aarch64 and Windows/arm64 are not covered.
- Publisher authentication stays `not_verified`: pins authenticate the bytes, the runner's system roots only protect transport.
