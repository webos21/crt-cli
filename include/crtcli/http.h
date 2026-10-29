#pragma once
namespace crtcli {
// Initializes libcurl and the Mbed TLS backend; kept free of C++ standard-library path types (see crt.json).
bool http_tls_probe();
}
