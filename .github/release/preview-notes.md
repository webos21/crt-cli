# crt-cli Preview

**Prerelease.** Built from commit `{{COMMIT}}` (the exact tag target) by `release.yml`, against the public CRT Dev SDK pinned in
`.github/ci/pins.json` for this host. The CLI reports `crt-cli 0.1.0-dev`.

## What this Preview adds

Pinned publisher-key and signature options for `sdk acquire` (source `publisher` object) and `sdk install`/`sdk repair`
(`--publisher-key`, `--signature`, `--publisher-key-spki-sha256`); they fail closed and are off unless given. The CLI's `fetch` redirect policy and
host-tools support of the earlier Previews are unchanged. The workflow does not exercise the new options on the packaged executable; the CLI
repository's replay records do (`docs/acceptance/ci_acceptance.md`).

## Asset

| File | Bytes | SHA-256 |
| --- | ---: | --- |
| the `.tar.xz` archive attached to this release | {{ARCHIVE_BYTES}} | `{{ARCHIVE_SHA256}}` |

The archive holds one statically linked executable `crt-cli`, the Apache-2.0 license, component licenses/provenance notices and a
format-1 `manifest.json` pinning every file. It contains no SDK, compiler, interpreter, CA certificate or key. Verify the archive digest
from a source you trust independently of this page.

## What was checked before publication

The `release` workflow built this commit with the pinned published Preview and public SDK, packed the archive twice with identical bytes,
and, on a separate runner, extracted it and ran the packaged executable: `--version`, self `doctor` with the manifest pin (a wrong pin is
refused), a real GitHub `sdk acquire`, then C and C++ `new`, `build`, `test`, `run` and a warm rebuild. A reviewer approved the publication.

## Not claimed

- **Publisher authentication is separate from this build.** The workflow publishes the archive first and holds no signing key. If a
  `<archive>.sig` asset is attached to this release it is a detached RSA-PSS (SHA-256) signature by the publisher key whose fingerprint
  (SHA-256 of the DER SubjectPublicKeyInfo) is `59218576956e493476bc36f7385d8e01a03ff9b92946a531b013ad3937d43fa2`; check that fingerprint against
  copies you trust independently (see `docs/security/publisher_signing.md` and its signed notice in the CRT repository) and verify with
  `crt-cli fetch --publisher-key <pem> --signature <file>` or `openssl`. Without such an asset the archive is **not signed**: trust comes from
  digests you pin yourself (`publisher_authentication: not_verified`). Signing is not part of what the workflow checked.
- This template does not repeat per-host test counts or toolchain versions; the workflow run that produced this release records them.
  The CLI is experimental and its JSON/command contract may change.
