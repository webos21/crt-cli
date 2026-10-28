# Native CLI Dependencies

Unmodified header-only dependencies are pinned by full commit, byte length and
SHA-256 in each `import.json`: nlohmann/json (MIT) and PicoSHA2 (MIT). Their
licenses are retained alongside the source. Builds consume these checked-in
bytes and do not download dependencies. No CRT or upstream runtime source is
vendored here; the target runtime comes exclusively from the selected SDK.
Release packaging must retain these licenses together with SDK/CRT dependency
notices; this source import does not constitute release qualification.
