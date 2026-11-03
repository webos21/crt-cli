# D2 Local SDK Installer Acceptance

Contract: [CLI design section 3](../design/crt_cli.md) (immutable store, transactional install) and the CRT
[install metadata](https://github.com/webos21/crt/blob/main/docs/design/sdk_development_contract.md#install-metadata-format-1-crt-31)
(format 1). Scope of this record: local archives only; no network acquisition, updates, project-lock migration or CLI
self-upgrade.

## Linux/x86_64 isolated replay (2026-11-03)

`tools/replay_sdk_install_linux.py` runs every CLI call in bubblewrap: user/ipc/uts/net namespaces (no network), read-only
root, source and producer trees hidden, cleared environment with an empty PATH, private `/tmp`, one writable directory
(store + sentinel tree). The PID namespace is shared on purpose: lock/transaction owners are PIDs, and a fresh PID
namespace per run would recycle small PIDs. Inputs: the dev archive and `release-manifest-dev-linux-x86_64.json` of a
`crt-dev-release` **rehearsal** (tag `v0.0.1-rehearsal.1`, uncommitted code in a scratch clone; not a release candidate),
CLI = the seed dev-loop binary (not a self-built, SDK-only CLI). Result: **48/48 checks**, evidence retained outside the
repositories (`result.json`, work tree).

| Area | Checks |
| --- | --- |
| Install from pinned manifest | identity equals the promised one; one SDK directory; `.staging`/`.locks` empty; `list`, `verify`; reinstall idempotent; host recomputation of the identity equals the manifest's |
| Immutability | flipped byte in an installed file: `verify` fails, reinstall refuses ("immutable"); restoring the byte passes |
| Tampered inputs | flipped archive byte, extra trailing byte, wrong pinned manifest digest: refused, nothing published |
| Hostile archives (digest supplied) | traversal, absolute path, absolute/escaping symlink, write-through symlink, hard link, device node, case collision, not an SDK: refused; no SDK, no staging, no locks; nothing outside the store; sentinel tree unchanged |
| Interruption | SIGKILL during extraction: nothing published, leftovers present, `recover` removes them, next install and `verify` pass |
| Concurrency | two parallel installs with `--lock-wait`: both succeed (one new, one already installed), one SDK, bookkeeping empty |
| Read-only store | refused |
| Zip | a zip of the same SDK installs with the identical identity; `verify` passes |

The first run exposed a harness error (killing only bwrap's process group left the CLI running, so `recover` correctly
kept its live transaction); the replay now kills the CLI by its command line. The harness failing there is also evidence
that `recover` does not remove a live owner's transaction.

## Not verified

macOS and Windows replays (including Windows lock-owner liveness and the `lzma` import-library name); a release
candidate from a committed tree; Full-stage archives; SDK-only self-built CLI as the replay binary; disk-full and
power-loss behavior; very large archives beyond the 186 MB dev SDK.
