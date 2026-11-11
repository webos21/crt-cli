# Request to CRT: publish the independent host-tools bundle

**From** `webos21/crt-cli` **to** `webos21/crt` (producer). **Date** 2026-11-11. This is a request, not a change: no CRT file was edited
for it, and nothing here claims the bundle is published or promoted.

## Why

The CLI's port suites (`port-build|store|oracle|command|project-test`) run the reviewed zlib recipe through the CRT-native shell, `make`,
`awk` and toybox applets. Those tools come from the independent host-tools bundle
([`crt.experimental.host-tools`, format 1](https://github.com/webos21/crt/blob/main/docs/design/host_tools_contract.md)). The bundle has no
public release: its manifest says `promotion: local-only`, and `webos21/crt` publishes only Dev SDK Previews. As a result
[`integration.yml`](../.github/workflows/integration.yml) cannot pin the bundle, and its `pins` and `ports` jobs fail on purpose
(hosted run `42957820373`). A skipped suite would look like a pass, so the pin gate stays red until the inputs exist.

The CLI cannot fix this itself: the bundle format, assembler and promotion are CRT-owned, and the CLI must not reference a producer build
tree or vendor CRT source.

## What the CLI needs, per host

For each host listed below, one immutable public download that the CLI's `HostTools::load` accepts, plus the values the CI pin records
(`.github/ci/integration-pins.json`, field `host_tools`):

| Pin field | Meaning |
| --- | --- |
| `url` | HTTPS URL of the bundle archive (a GitHub release asset is fine; the CLI's `github-release` redirect policy handles its redirect) |
| `name` | archive file name |
| `size` | archive size in bytes |
| `sha256` | archive SHA-256 |
| `manifest_sha256` | SHA-256 of the extracted bundle's `manifest.json` (the value the CLI takes as `--host-tools-sha256`) |
| `version` | the bundle's own version string (for example `v0.1.0-hosttools.<date>.<n>`) |

Please also publish, next to the archive, a checksum file and a short release record naming the producer commit, the source SDK's manifest
hash and content identity, and whether the assembler tree was clean. The CLI will quote these in its acceptance record and will not
upgrade a pin to a bundle whose provenance says dirty.

## Hosts

| Host | Needed first | Existing evidence in `docs/acceptance/host_tools_acceptance.md` |
| --- | --- | --- |
| Linux/x86_64 | yes (hosted CI `ubuntu-26.04`) | local candidate 2026-11-02, `v0.1.0-hosttools.20261102.4`, manifest `59bdf8f8...2652` |
| Linux/aarch64 | yes (hosted CI `ubuntu-26.04-arm`) | **none**: no aarch64 bundle has been assembled or replayed |
| macOS/arm64 | later | independent replay 2026-11-03, `v0.1.0-hosttools.20261103.macos.1` |
| Windows/x64 | later | independent replay 2026-11-03, `v0.1.0-hosttools.20261103.windows.5` (no private runtime DLLs in the selected closure) |

The first two rows are what the integration workflow pins today. Linux/aarch64 is the larger ask: the assembler's host detection accepts
`aarch64`, but the record contains no native aarch64 run.

## Questions CRT needs to answer

1. **Input SDK.** `tools/create_host_tools.py` takes a verified public `01-c` SDK (`--sdk-root`). Can the bundle be assembled from the
   published **Dev SDK Preview** (stage `02-cxx`, which already ships `system/bin` applets, `bin/awk` and `bin/make`), or must CRT publish an
   `01-c` SDK as well? If a different input is needed, which one, and should it be a published asset?
2. **Promotion.** What replaces `promotion: local-only` for a published bundle, and what is the review that grants it? The CLI will treat the
   published, digest-pinned bytes as the trust anchor (no publisher signature), exactly as for the Dev SDK Preview.
3. **Version scheme and release tag** for the bundle, and whether it is released from `webos21/crt` or another repository.
4. **Linux/aarch64.** Is the assembler expected to work there unchanged? If CRT finds a producer-side gap, the CLI would like a minimal
   reproducer and a linked fix, as with the earlier `posix_spawn` and `fcntl.h` fixes.
5. **Make source notice.** The assembler takes an optional pristine make cache whose `COPYING` must match the pinned notice. Which source
   does the published bundle use, and is it reproducible in CI?
6. **Cadence.** Is the bundle versioned independently of the SDK, or rebuilt with each SDK Preview? The CLI pins both and will not move one
   without the other being checked.

## What the CLI will do once it is published

1. Fill `host_tools` for the host in `.github/ci/integration-pins.json` in a reviewed pull request, quoting `tools/ci_integration.py pins`
   and the release record.
2. Adapt `tools/replay_port_linux.py` to a hosted runner so that the `ports` job runs the five suites (today it fails explicitly because it
   is not wired).
3. Add a `schedule:` trigger to `integration.yml` in the same commit, because a daily run that is red for a known reason is not useful.
4. Record the first green hosted run in [CI acceptance](acceptance/ci_acceptance.md).

## Not requested

- No change to the SDK, the native policy, the CLI's contract or any lock.
- No public release of the CLI or promotion of the bundle to a default; the CLI keeps pinning explicit digests.
- No macOS or Windows bundle publication before Linux, unless CRT prefers to publish all four at once.
