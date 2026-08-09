#ifndef ESP_HTTP_SERVER_H
#define ESP_HTTP_SERVER_H

/* Host-test stand-in for ESP-IDF's esp_http_server.h.
 *
 * The real header pulls in freertos/FreeRTOS.h, http_parser.h, sdkconfig.h,
 * and esp_event.h -- none of which can be compiled or linked on the host.
 * web_server_blob.c (and the small set of shared adapter helpers it calls
 * into) are the only first-party code that includes esp_http_server.h and is
 * compiled for a host test target; this header supplies just the subset of
 * types/macros/prototypes those translation units reference. It is resolved
 * only for the web_server_blob_tests target (see tests/host/CMakeLists.txt),
 * which lists this directory ahead of any other include path.
 *
 * httpd_req_t.aux is unused by the real handlers here beyond being an opaque
 * pointer; tests point it at a fake_httpd_request_t (fake_httpd.h) that the
 * fake_httpd.c implementations below use to record request/response state.
 */

#include <stddef.h>
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

#endif
