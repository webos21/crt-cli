# crt-cli Preview

**Prerelease.** Built from commit `{{COMMIT}}` (the exact tag target) by `release.yml`, against the public CRT Dev SDK pinned in
`.github/ci/pins.json` for this host. The CLI reports `crt-cli 0.1.0-dev`.

## What this Preview adds

Pinned publisher-key and signature options for `sdk acquire` (source `publisher` object) and `sdk install`/`sdk repair`
(`--publisher-key`, `--signature`, `--publisher-key-spki-sha256`); they fail closed and are off unless given. The host-tools shell relays
captured stdout in blocks so large output can complete within the configured timeout. The workflow does not exercise the publisher options on the packaged executable; the CLI
repository's replay records do (`docs/acceptance/ci_acceptance.md`).

## Asset

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| the `.tar.xz` archive attached to this release | {{ARCHIVE_BYTES}} | `{{ARCHIVE_SHA256}}` |

The archive holds one `crt-cli` executable with statically linked CRT libraries, the Apache-2.0 license, component licenses/provenance notices and a
format-1 `manifest.json` pinning every file. It contains no SDK, compiler, interpreter, CA certificate or key. Verify the archive digest
from a source you trust independently of this page.

## What was checked before publication

The `release` workflow built this commit with the pinned published Preview and public SDK, packed the archive twice with identical bytes,
and, on a separate runner, extracted it and ran the packaged executable: `--version`, self `doctor` with the manifest pin (a wrong pin is
refused), a real GitHub `sdk acquire`, then C and C++ `new`, `build`, `test`, `run` and a warm rebuild. Staging and finalization require separate approval.

## Not claimed

- **CI does not hold the signing key.** The owner signs the archive and
  checksum file offline. The finalization workflow verifies both detached
  RSA-PSS (SHA-256) signatures before publication. Verify each file with
  the [independently pinned publisher key](https://github.com/webos21/crt/blob/main/docs/security/publisher_signing.md)
  using `crt-cli fetch --publisher-key <pem> --signature <file>` or OpenSSL.
- This template does not repeat per-host test counts or toolchain versions; the workflow run that produced this release records them.
  The CLI is experimental and its JSON/command contract may change.
