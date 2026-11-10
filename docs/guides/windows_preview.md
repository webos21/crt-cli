# Windows/x64 CLI Preview

The Windows/x64 Preview packages one CRT-native `crt-cli.exe`, its component licenses,
provenance and a pinned inventory. The Dev SDK is downloaded separately; LLVM and the
Windows SDK are external build prerequisites. The executable is unsigned and publisher
identity is not authenticated. Verify the archive against a digest obtained through a
source you trust independently. Exact asset, executable and manifest pins are in
[acceptance](../acceptance/preview_acceptance.md#windowsx64-cli-preview-release-2026-11-10).

## Extract and diagnose

Download the archive and `SHA256SUMS-windows-x86_64` from the
[Windows/x64 prerelease](https://github.com/webos21/crt-cli/releases/tag/v0.1.0-preview.20261110.windows-x86_64.1).
Use a fresh directory. Windows' `tar.exe` can extract the `.tar.xz` archive:

```powershell
Get-FileHash .\crt-cli-0.1.0-dev-windows-x86_64-preview.20261110.1.tar.xz -Algorithm SHA256
tar -xJf .\crt-cli-0.1.0-dev-windows-x86_64-preview.20261110.1.tar.xz
Set-Location .\crt-cli-windows-x86_64
.\crt-cli.exe --version
.\crt-cli.exe doctor --scope self --resources . --resources-sha256 7381cd58919be1f1d007cf0091c17e33236797860a8a2087b1139a7e80e6676c
```

The executable reports `0.1.0-dev`. Windows 10 1903 or later is required for the
UTF-8 path policy. The acceptance machine uses LLVM 22.1.8 and Windows SDK
10.0.28000.0; no MSYS, WSL, Python or shell is required for CLI/app workflows.

## Acquire the pinned SDK

Choose a PEM CA bundle and compute its SHA-256. You may export the Windows ROOT
certificates to a caller-owned PEM; this reads certificates without changing trust:

```powershell
$pemCerts = Get-ChildItem Cert:\LocalMachine\Root | ForEach-Object {
    "-----BEGIN CERTIFICATE-----`n" + [Convert]::ToBase64String($_.RawData, [Base64FormattingOptions]::InsertLineBreaks) + "`n-----END CERTIFICATE-----"
}
[IO.File]::WriteAllText((Join-Path (Get-Location) 'roots.pem'), ($pemCerts -join "`n"), [Text.Encoding]::ASCII)
(Get-FileHash .\roots.pem -Algorithm SHA256).Hash.ToLowerInvariant()
```

Save the following as `sdk-source.json`, replacing the CA path/hash with your own.
The remaining pins identify the existing public Windows Dev SDK Preview:

```json
{
  "schema": "crt-cli.sdk-source",
  "format": 0,
  "ca_file": "C:/absolute/roots.pem",
  "ca_sha256": "<SHA-256 of your chosen PEM bundle>",
  "manifest": {
    "url": "https://github.com/webos21/crt/releases/download/v0.0.1-preview.20261110.windows.1/release-manifest-dev-windows-x86_64.json",
    "sha256": "b0fca4c81d29b8eee28e0d9fb7d6e26950b094a8be4f2c091e43922366a46976",
    "size": 1800
  },
  "archive": {
    "name": "crt-v0.0.1-preview.20261110.windows.1-windows-x86_64-dev.zip",
    "url": "https://github.com/webos21/crt/releases/download/v0.0.1-preview.20261110.windows.1/crt-v0.0.1-preview.20261110.windows.1-windows-x86_64-dev.zip"
  },
  "contract_url": "https://github.com/webos21/crt/releases/download/v0.0.1-preview.20261110.windows.1/crt-v0.0.1-preview.20261110.windows.1-windows-x86_64-dev-toolchain.json",
  "sdk": {
    "version": "v0.0.1-preview.20261110.windows.1",
    "content_sha256": "5bb55372ec10e3cfa24c84de981a7130f56aeb57ef0d43d5da22a65036dec7ef",
    "contract_sha256": "7762f572b4614b3068e1ea99694ce19360e213d31c5bd01019e4ad6ce005b726"
  },
  "redirect_policy": "github-release",
  "timeout_seconds": 300
}
```

```powershell
.\crt-cli.exe sdk acquire --source .\sdk-source.json
.\crt-cli.exe sdk list
```

The opt-in `github-release` policy checks every HTTPS redirect against the bounded
GitHub storage list. The default refuses redirects. Manifest, policy and archive pins
are checked before immutable installation; a matching installed identity is reused.
This verifies integrity against your explicit inputs, not publisher signatures or
an official default-download channel. See the [source/redirect contract](../reference/bootstrap_contract.md#restricted-https-redirects-experimental-cli-owned).

## Build C or C++

Substitute your installed LLVM and Windows SDK paths. The `--os-sdk` directory holds
the Windows `um/x64` import libraries; CRT supplies the C/C++ headers and runtime.

```powershell
.\crt-cli.exe new hello-cxx --language cxx
.\crt-cli.exe build --project hello-cxx --sdk 5bb55372ec10e3cfa24c84de981a7130f56aeb57ef0d43d5da22a65036dec7ef --cc "C:/Program Files/LLVM/bin/clang.exe" --cxx "C:/Program Files/LLVM/bin/clang++.exe" --linker "C:/Program Files/LLVM/bin/ld.lld.exe" --ar "C:/Program Files/LLVM/bin/llvm-ar.exe" --os-sdk "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.28000.0/um/x64"
.\crt-cli.exe test --project hello-cxx
.\crt-cli.exe run --project hello-cxx
.\crt-cli.exe build --project hello-cxx
```

For C, use a new project name with `--language c`. `new` creates sources without
acquiring an SDK; first build selects the installed identity and writes the project
lock. Later builds reuse those pins and tools. Windows/arm64, Full profiles,
source-port builds from this exact package, SDK updates and self-upgrade are outside
this Preview's tested scope.
