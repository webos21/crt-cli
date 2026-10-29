# Native CLI Dependencies

JSON and SHA-2 are Development SDK dependencies, not CLI-owned source imports.
The CLI uses installed `nlohmann/json.hpp` and Mbed TLS SHA-256; the SDK also
provides cJSON for embedded C consumers and Mbed TLS SHA-224/384/512.
Pins, source hashes, notices and static/shared port evidence belong to CRT's
recipes and installed SDK manifest. No dependency is downloaded during a CLI
build, and no sibling CRT source/build tree is a consumer include path.
CLI packaging must retain notices for dependencies actually compiled/linked
into the executable; SDK ownership does not remove redistribution obligations.
