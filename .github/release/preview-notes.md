# crt-cli Preview

**Prerelease.** Built from commit `{{COMMIT}}` (the exact tag target) by `release.yml`, against the public CRT Dev SDK pinned in
`.github/ci/pins.json` for this host. The CLI reports `crt-cli 0.1.0-dev`.

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

- **No publisher authentication**: the archive and manifest are not signed. Trust comes from digests you pin yourself (`publisher_authentication: not_verified`).
- This template does not repeat per-host test counts or toolchain versions; the workflow run that produced this release records them.
  The CLI is experimental and its JSON/command contract may change.
