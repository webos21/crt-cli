#include <crtcli/http.h>
#include <curl/curl.h>
#include <mbedtls/ssl.h>
#include <mbedtls/version.h>
#include <cstring>
namespace crtcli {
bool http_tls_probe() {
    bool curl = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
    const curl_version_info_data* http = curl ? curl_version_info(CURLVERSION_NOW) : nullptr;
    bool tls = http && (http->features & CURL_VERSION_SSL) && http->ssl_version
               && std::strstr(http->ssl_version, "mbedTLS") && mbedtls_version_get_number() != 0;
    mbedtls_ssl_context context;
    mbedtls_ssl_init(&context); mbedtls_ssl_free(&context);
    if (curl) curl_global_cleanup();
    return tls;
}
}
