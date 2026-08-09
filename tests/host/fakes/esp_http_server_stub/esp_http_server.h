#ifndef ESP_HTTP_SERVER_H
#define ESP_HTTP_SERVER_H

/* Host-test stand-in for ESP-IDF's esp_http_server.h.
 *
 * The real header pulls in freertos/FreeRTOS.h, http_parser.h, sdkconfig.h,
 * and esp_event.h -- none of which can be compiled or linked on the host.
 * web_server_blob.c (and the small set of shared adapter helpers it calls
 * into) were the first first-party code to include esp_http_server.h and be
 * compiled for a host test target; this header supplies just the subset of
 * types/macros/prototypes those translation units reference, now shared by
 * every *_route_tests target listed in tests/host/CMakeLists.txt (see each
 * target's include-directory list -- this directory is always listed ahead
 * of any other include path so this stub, not the real ESP-IDF header,
 * resolves).
 *
 * httpd_req_t.aux is unused by the real handlers here beyond being an opaque
 * pointer; tests point it at a fake_httpd_request_t (fake_httpd.h) that the
 * fake_httpd.c implementations below use to record request/response state.
 *
 * httpd_start()/httpd_stop()/httpd_register_uri_handler()/
 * httpd_uri_match_wildcard() are declared here (matching the real ESP-IDF
 * prototypes exactly) but implemented only by fakes/fake_httpd_router.c, and
 * linked only into web_server_lifecycle_tests -- the one target that compiles
 * web_server_lifecycle.c, the only first-party file that calls them. No
 * other *_route_tests target references these four symbols, so the
 * declarations are harmless additions for every other target linking this
 * header (see fake_httpd_router.h for why this is a separate fake from
 * fake_httpd.c's per-request body/response double). */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef int esp_err_t;

#define ESP_OK 0
#define ESP_FAIL (-1)

/* Only the four methods web_server_api.c's method_from_request() maps
 * (HTTP_GET/POST/PUT/DELETE) -- the real llhttp-derived httpd_method_t has
 * dozens more, none of which any first-party handler under host test reads.
 * Values are internal to this stub (nothing compares them against the real
 * ESP-IDF enum), so they need not match http_parser.h's numbering. */
typedef enum {
    HTTP_GET = 0,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
} httpd_method_t;

typedef struct httpd_req {
    size_t content_len;
    char uri[256];
    httpd_method_t method;
    void *aux;
} httpd_req_t;

#define HTTPD_RESP_USE_STRLEN (-1)
#define HTTPD_SOCK_ERR_TIMEOUT (-3)

typedef enum {
    HTTPD_500_INTERNAL_SERVER_ERROR = 0,
} httpd_err_code_t;

int httpd_req_recv(httpd_req_t *request, char *buffer, size_t buffer_length);
size_t httpd_req_get_hdr_value_len(httpd_req_t *request, const char *field);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *request, const char *field, char *value,
                                      size_t value_size);
esp_err_t httpd_resp_set_hdr(httpd_req_t *request, const char *field, const char *value);
esp_err_t httpd_resp_set_status(httpd_req_t *request, const char *status);
esp_err_t httpd_resp_set_type(httpd_req_t *request, const char *type);
esp_err_t httpd_resp_send(httpd_req_t *request, const char *buffer, ssize_t buffer_length);
esp_err_t httpd_resp_send_chunk(httpd_req_t *request, const char *buffer, ssize_t buffer_length);
esp_err_t httpd_resp_send_err(httpd_req_t *request, httpd_err_code_t error, const char *message);

/* Route registration/dispatch subset -- only web_server_lifecycle.c uses these.
 * Types/macro/prototypes match the real ESP-IDF esp_http_server.h exactly
 * (fields and behavior ported from $IDF_PATH/components/esp_http_server for
 * fidelity, the same rationale documented in fakes/esp_idf_misc_stub's
 * ESP-IDF-derived stand-ins); implementation is fakes/fake_httpd_router.c. */
typedef void *httpd_handle_t;

typedef bool (*httpd_uri_match_func_t)(const char *reference_uri, const char *uri_to_match,
                                       size_t match_upto);

typedef struct {
    uint16_t max_uri_handlers;
    size_t stack_size;
    httpd_uri_match_func_t uri_match_fn;
} httpd_config_t;

#define HTTPD_DEFAULT_CONFIG()                                                                     \
    { .max_uri_handlers = 8, .stack_size = 4096, .uri_match_fn = NULL, }

typedef struct httpd_uri {
    const char *uri;
    httpd_method_t method;
    esp_err_t (*handler)(httpd_req_t *request);
    void *user_ctx;
} httpd_uri_t;

esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config);
esp_err_t httpd_stop(httpd_handle_t handle);
esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri_handler);
bool httpd_uri_match_wildcard(const char *uri_template, const char *uri_to_match,
                              size_t match_upto);

#endif
