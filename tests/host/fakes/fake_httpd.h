#ifndef FAKE_HTTPD_H
#define FAKE_HTTPD_H

#include <stdbool.h>
#include <stddef.h>

#include "esp_http_server.h"

/* Fake backend behind httpd_req_t.aux -- lets host tests drive the real
 * blob_*_handler() functions in firmware/components/web_server/web_server_blob.c
 * (and the web_server_common.c helpers they call) with a scriptable request
 * and an inspectable response, in lieu of a real esp_http_server / lwIP
 * socket. See esp_http_server_stub/esp_http_server.h for why this exists. */

#define FAKE_HTTPD_BODY_CAPACITY (256U * 1024U)
#define FAKE_HTTPD_RESPONSE_CAPACITY (256U * 1024U)
#define FAKE_HTTPD_HEADER_CAPACITY 16U
#define FAKE_HTTPD_HEADER_NAME_MAX 64U
#define FAKE_HTTPD_HEADER_VALUE_MAX 256U
#define FAKE_HTTPD_STATUS_MAX 32U
#define FAKE_HTTPD_TYPE_MAX 64U

typedef struct {
    char name[FAKE_HTTPD_HEADER_NAME_MAX];
    char value[FAKE_HTTPD_HEADER_VALUE_MAX];
} fake_httpd_header_t;

typedef struct {
    /* Request side, set up by the test before invoking a handler. */
    char body[FAKE_HTTPD_BODY_CAPACITY];
    size_t body_length;
    size_t body_cursor;
    size_t receive_chunk; /* 0 = deliver as much as requested in one call */
    /* When non-zero, the next this-many httpd_req_recv() calls each return
     * HTTPD_SOCK_ERR_TIMEOUT (decrementing this counter) before delivery
     * resumes; 5 consecutive timeouts exhausts web_adapter_stream_bounded_body's
     * retry budget (WEB_ADAPTER_TIMEOUT_RETRIES_MAX = 4) and surfaces
     * APP_ERROR_TIMEOUT without ever requiring a real body. */
    size_t forced_timeout_count;
    fake_httpd_header_t request_headers[FAKE_HTTPD_HEADER_CAPACITY];
    size_t request_header_count;

    /* Response side, populated by the handler under test via the fake
     * httpd_resp_* functions; inspected by the test afterward. */
    char response_status[FAKE_HTTPD_STATUS_MAX];
    char response_type[FAKE_HTTPD_TYPE_MAX];
    fake_httpd_header_t response_headers[FAKE_HTTPD_HEADER_CAPACITY];
    size_t response_header_count;
    char response_body[FAKE_HTTPD_RESPONSE_CAPACITY];
    size_t response_body_length;
    bool responded;        /* httpd_resp_send() or a terminating send_chunk(len=0) occurred */
    bool chunk_terminated; /* httpd_resp_send_chunk() was called with length 0 */
    bool send_err_called;
    httpd_err_code_t last_send_err_code;
} fake_httpd_request_t;

void fake_httpd_reset(fake_httpd_request_t *fake);
/* Wires request->aux to `fake` and request->content_len/uri to the given values. */
void fake_httpd_bind(httpd_req_t *request, fake_httpd_request_t *fake, const char *uri,
                     size_t content_len);
void fake_httpd_set_body(fake_httpd_request_t *fake, const void *body, size_t length,
                         size_t receive_chunk);
void fake_httpd_add_request_header(fake_httpd_request_t *fake, const char *name, const char *value);
const char *fake_httpd_response_header(const fake_httpd_request_t *fake, const char *name);

#endif
