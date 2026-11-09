# Project Status

Evidence-based SDK/CLI snapshot refreshed **2026-11-09** at the owner's explicit
request. Reviewed CLI `2ae74d4d55bd1d7210c96ae596a48b2a934931bf` and CRT
`21b9d0e6067ed8253d424912132b9be24e459049`. This initial documentation refresh imports
existing acceptance records; the later [Linux delta](docs/acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09)
passes on the same product baseline, while the owner's macOS delta stays open.
[TODO](TODO.md) owns current work, [HISTORY](HISTORY.md) completed work and
[acceptance](docs/acceptance/sdk_install_acceptance.md) exact combinations.

## Implemented and exercised scope

| Surface | Evidence boundary |
| --- | --- |
| Native CLI/project builder | CRT-native C++17, crt.json -> direct LLVM argv; C/C++ source-only new, first build/configure/build/test/run; external LLVM/OS SDKs required |
| SDK install/receipts/selection | Immutable native Dev store, exact contracts, explicit repair/recovery and format-1 project pins; five-pair records retain their original baselines |
| First-build/HTTPS acquisition | Explicit pinned SDK source, CA/size/digest checks, installed-identity reuse and lock-preserving failure; Linux/macOS/Windows records are [per-host](docs/acceptance/sdk_install_acceptance.md) |
| Independent host tools | Pinned CRT shell/make/applets for port workflows; control/app paths need no host Python, shell or CMake; [host environment](docs/acceptance/host_environment_acceptance.md) owns loader/OS limits |
| D3 zlib source port | Native execution, separate store, ELF/Mach-O/PE audit, provider choice, port locks and packaged oracle have Linux/x86_64, macOS/arm64 and Windows/x64 records; formal approval/latest common-delta closure remain open |

Windows/x64's earlier concurrent-install crash was root-caused/fixed in the
producer libunwind recipe and replayed with the fixed SDK; its retained failure
is not a current unresolved gate. Windows/arm64 D2 is distinct from unqualified
D3. Current committed binaries retain their [dated identities](bin/README.md).
None of these records is a universal latest-source support matrix or release.

## Limits and ordered follow-up

- CLI SDK installation/selection currently supports native `dev` at `02-cxx`;
  general profiles and Full `06-web` installation are separate work.
- D3 adapter `crt_review` stays pending until final producer review and the
  owner's macOS replay close; Linux common-change replay passes. Existing Windows/x64 evidence is reused only for
  unchanged policy/product inputs; Windows/arm64 D3 is separate.
- The retained Linux SDK's C shared SDK-provider control resolves host libz;
  this is [unendorsed diagnostic evidence](docs/acceptance/sdk_install_acceptance.md#linux-d3-common-change-replay-and-macos-handoff-2026-11-09),
  not SDK shared qualification. Producer 6.4 retains policy/runtime-search review.
- Initial preview targets Dev SDK + CLI and the actual user/package/trust path.
  Existing Full assembly/approval records remain valid for their own inputs;
  Full simultaneous delivery does not block this CLI preview.
- D4 requires a new SQLite amalgamation adapter plus libpng -> zlib dependency,
  variant/key/lock/cache handling; existing configure-only D3 is not that resolver.
- Official default trust/URLs, producer promotion before production CLI pin
  adoption, final release artifact qualification and publication remain open.
  Developer test CA, rehearsal assets and root unresolved locks are not promotion.
- General updates/self-upgrade, GC, wider profiles/cross targets and Studio retain
  their later queues. No new runtime/SDK build or default pin is adopted here.

Native evidence and limits belong to [compatibility](docs/compatibility.md) and
acceptance. Future STATUS refreshes remain explicit owner requests.
