/* Shared fixture for the blob HTTP-adapter suites below: a 64-lowercase-hex
 * session token accepted by the fake auth double, request/response helpers
 * built on fake_httpd.h, and test doubles for the two functions
 * web_server_blob.c's dependency chain needs that cannot be linked for a host
 * build (see fake_storage_blob.h's header comment for the storage_blob
 * functions and storage_partition_capacity; auth_session_validate's real implementation
 * lives in auth.c, which -- like every other NVS/FreeRTOS-backed "outer"
 * component file in this codebase -- is not compiled for host tests). */

#define BLOB_TEST_VALID_SESSION_TOKEN                                                              \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"

static app_error_code_t g_blob_test_auth_result;

app_error_code_t auth_session_validate(const char *session_token) {
    (void)session_token;
    return g_blob_test_auth_result;
}

static const char *blob_test_status_text(unsigned int status) {
    switch (status) {
    case WEB_HTTP_STATUS_OK:
        return "200 OK";
    case WEB_HTTP_STATUS_CREATED:
        return "201 Created";
    case WEB_HTTP_STATUS_NO_CONTENT:
        return "204 No Content";
    case WEB_HTTP_STATUS_BAD_REQUEST:
        return "400 Bad Request";
    case WEB_HTTP_STATUS_UNAUTHORIZED:
        return "401 Unauthorized";
    case WEB_HTTP_STATUS_NOT_FOUND:
        return "404 Not Found";
    case WEB_HTTP_STATUS_PAYLOAD_TOO_LARGE:
        return "413 Payload Too Large";
    case WEB_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE:
        return "415 Unsupported Media Type";
    case WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR:
        return "500 Internal Server Error";
    case WEB_HTTP_STATUS_SERVICE_UNAVAILABLE:
        return "503 Service Unavailable";
    case WEB_HTTP_STATUS_INSUFFICIENT_STORAGE:
        return "507 Insufficient Storage";
    default:
        return "500 Internal Server Error";
    }
}

/* A close mirror of web_server_api.c's real web_api_send_status_error(): that
 * function lives in web_server_api.c, which also pulls in device_controls,
 * esp_system (esp_restart), and the full API dispatch pipeline -- all
 * out of scope for a blob-focused target. send_blob_error() (web_server_blob.c)
 * always sets X-Request-ID itself before calling this, so unlike the
 * production version this double does not need a request_id parameter. */
esp_err_t web_api_send_status_error(httpd_req_t *request, unsigned int status,
                                    app_error_code_t code, const char *message) {
    web_api_response_t response = {0};
    const web_api_error_spec_t spec = {
        .status = status,
        .code = code,
        .message = message,
        .field = NULL,
        .has_parser_location = false,
        .byte_offset = 0U,
        .line = 0U,
        .column = 0U,
    };
    if (web_api_response_error(&response, &spec) != APP_ERROR_NONE) {
        return ESP_FAIL;
    }
    esp_err_t result = ESP_FAIL;
    if (httpd_resp_set_status(request, blob_test_status_text(status)) == ESP_OK &&
        httpd_resp_set_hdr(request, "Cache-Control", "no-store") == ESP_OK) {
        if (response.body == NULL || response.body_length == 0U) {
            result = httpd_resp_send(request, NULL, 0);
        } else if (httpd_resp_set_type(request, "application/json") == ESP_OK) {
            result = httpd_resp_send(request, response.body, (ssize_t)response.body_length);
        }
    }
    web_api_response_free(&response);
    return result;
}

static void blob_test_reset(fake_httpd_request_t *fake) {
    fake_httpd_reset(fake);
    fake_storage_blob_reset();
    g_blob_test_auth_result = APP_ERROR_NONE;
}

static void blob_test_authenticate(fake_httpd_request_t *fake) {
    fake_httpd_add_request_header(fake, "Cookie", "MKSESSION=" BLOB_TEST_VALID_SESSION_TOKEN);
}

static void blob_test_set_request_id(fake_httpd_request_t *fake, const char *request_id) {
    fake_httpd_add_request_header(fake, "X-Request-ID", request_id);
}

static void blob_test_expect_response_id(const fake_httpd_request_t *fake, const char *request_id) {
    const char *header = fake_httpd_response_header(fake, "X-Request-ID");
    TEST_CHECK(header != NULL);
    TEST_CHECK_EQ_STRING(request_id, header);
}
