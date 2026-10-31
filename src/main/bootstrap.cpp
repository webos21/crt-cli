#include "bootstrap_config.h"
#include <crtcli/core.h>
#include <crt/platform.h>
#include <crt/dl.h>
#include <crtcli/http.h>
#include <zlib.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <set>
#include <string>
#include <vector>
#include <cstdint>
#include <dlfcn.h>
#include <new>

namespace {
crtcli::Json runtime_maps() {
    std::set<std::string> images;
    int status = crt_dl_iterate_images([](const crt_dl_image* image, size_t, void* data) {
        try { static_cast<std::set<std::string>*>(data)->insert(image->path); return 0; }
        catch (...) { return 1; }
    }, &images);
    return {{"status", status == 0 && !images.empty() ? "collected" : "unavailable"}, {"images", images}};
}

} // namespace
namespace crtcli {
Reply bootstrap_doctor() {
    bool cxx = false;
    try { throw std::runtime_error("CRT bootstrap"); }
    catch (const std::exception& error) { cxx = std::strcmp(error.what(), "CRT bootstrap") == 0; }
    Dl_info image{}, allocation{}, deallocation{};
    auto allocate = static_cast<void* (*)(size_t)>(&::operator new);
    auto deallocate = static_cast<void (*)(void*)>(&::operator delete);
    bool local_cxx_allocation = dladdr(reinterpret_cast<const void*>(&bootstrap_doctor), &image) &&
        dladdr(reinterpret_cast<const void*>(allocate), &allocation) &&
        dladdr(reinterpret_cast<const void*>(deallocate), &deallocation) &&
        image.dli_fbase == allocation.dli_fbase && image.dli_fbase == deallocation.dli_fbase;
    const unsigned char input[] = "crt-cli-bootstrap";
    unsigned char compressed[128] = {}, restored[128] = {};
    uLongf compressed_size = sizeof(compressed), restored_size = sizeof(restored);
    bool zlib = compress2(compressed, &compressed_size, input, sizeof(input), Z_DEFAULT_COMPRESSION) == Z_OK;
    zlib = zlib && uncompress(restored, &restored_size, compressed, compressed_size) == Z_OK;
    zlib = zlib && restored_size == sizeof(input) && std::memcmp(restored, input, sizeof(input)) == 0;
    bool tls = http_tls_probe();
    bool narrow = std::strcmp(crt_get_narrow_encoding(), "utf-8") == 0;
    bool ok = cxx && local_cxx_allocation && zlib && tls && narrow;
    Json data{{"scope", "bootstrap"}, {"target", {{"os", CRT_CLI_TARGET_OS}, {"arch", CRT_CLI_TARGET_ARCH}}},
              {"sdk_build_input", {{"version", CRT_CLI_SDK_VERSION}, {"source_commit", CRT_CLI_SDK_COMMIT}, {"content_sha256", CRT_CLI_SDK_CONTENT}}},
              {"checks", {{"cxx_runtime", cxx ? "pass" : "fail"}, {"cxx_allocation_owner", local_cxx_allocation ? "pass" : "fail"}, {"zlib", zlib ? "pass" : "fail"}, {"http_tls_linkage", tls ? "pass" : "fail"}, {"narrow_encoding", narrow ? "pass" : "fail"}, {"executable_integrity", "not_verified"}, {"https_network", "not_run"}}},
              {"allocation_images", {{"executable", image.dli_fname ? image.dli_fname : ""},
                  {"new", allocation.dli_fname ? allocation.dli_fname : ""}, {"delete", deallocation.dli_fname ? deallocation.dli_fname : ""}}},
              {"narrow_encoding", crt_get_narrow_encoding()},
              {"sdk_store", sdk_store_root().string()},
              {"runtime_maps", runtime_maps()}};
    return {ok ? 0 : 1, "doctor", data, ok ? "bootstrap C++/zlib/HTTP/TLS pass; integrity not verified; HTTPS not run" : "bootstrap checks failed"};
}
}
