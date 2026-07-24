#include "web_server.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "esp_http_server.h"
#include "macro_executor.h"
#include "macro_limits.h"
#include "storage.h"
#include "usb_keyboard.h"
#include "web_content.h"
#include "web_cookie.h"
#include "web_origin.h"
#include "web_server_adapter.h"
#include "web_static_path.h"
#include "wifi_ap.h"

#define HTTP_HEADER_MAX_BYTES 256U
#define LOGIN_BODY_MAX_BYTES 256U
#define STATIC_CHUNK_BYTES WEB_ADAPTER_STATIC_CHUNK_BYTES
#define SESSION_COOKIE_NAME "MKSESSION"

static web_server_config_t server_configuration;
static web_adapter_lifecycle_t server_lifecycle;

static const char *usb_state_string(usb_keyboard_state_t state)
{
    switch (state) {
    case USB_KEYBOARD_UNINITIALIZED:
        return "uninitialized";
    case USB_KEYBOARD_DISCONNECTED:
        return "disconnected";
    case USB_KEYBOARD_ENUMERATING:
        return "enumerating";
    case USB_KEYBOARD_READY:
        return "ready";
    case USB_KEYBOARD_SUSPENDED:
        return "suspended";
    case USB_KEYBOARD_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

static const char *wifi_state_string(wifi_ap_state_t state)
{
    switch (state) {
    case WIFI_AP_STOPPED:
        return "stopped";
    case WIFI_AP_STARTING:
        return "starting";
    case WIFI_AP_READY:
        return "ready";
    case WIFI_AP_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

static const char *execution_state_string(execution_state_t state)
{
    switch (state) {
    case EXECUTION_IDLE:
        return "idle";
    case EXECUTION_RUNNING:
        return "running";
    case EXECUTION_COMPLETED:
        return "completed";
    case EXECUTION_CANCELLED:
        return "cancelled";
    case EXECUTION_FAILED:
        return "failed";
    default:
        return "unknown";
    }
}

static esp_err_t send_json(httpd_req_t *request, const char *json, const char *status)
{
    if (request == NULL || json == NULL || status == NULL ||
        httpd_resp_set_type(request, "application/json") != ESP_OK ||
        httpd_resp_set_status(request, status) != ESP_OK ||
        httpd_resp_set_hdr(request, "Cache-Control", "no-store") != ESP_OK) {
        return ESP_FAIL;
    }
    return httpd_resp_send(request, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t send_error(httpd_req_t *request,
                            const char *status,
                            app_error_code_t code,
                            const char *message)
{
    if (request == NULL) {
        return ESP_FAIL;
    }
    char response[512U];
    if (status == NULL ||
        web_adapter_build_error_json(code, message, response, sizeof(response)) !=
            APP_ERROR_NONE) {
        return httpd_resp_send_err(request,
                                   HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "response encoding failed");
    }
    return send_json(request, response, status);
}

static int receive_body_adapter(void *context, char *buffer, size_t capacity)
{
    httpd_req_t *request = context;
    const int count = httpd_req_recv(request, buffer, capacity);
    return count == HTTPD_SOCK_ERR_TIMEOUT ? WEB_ADAPTER_RECEIVE_TIMEOUT : count;
}

static app_error_code_t read_bounded_body(httpd_req_t *request,
                                           char *buffer,
                                           size_t buffer_size,
                                           size_t maximum_length)
{
    if (request == NULL) {
        if (buffer != NULL && buffer_size > 0U) {
            buffer[0] = '\0';
        }
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return web_adapter_read_bounded_body(request->content_len,
                                         buffer,
                                         buffer_size,
                                         maximum_length,
                                         receive_body_adapter,
                                         request);
}

static app_error_code_t get_header_adapter(void *context,
                                            const char *name,
                                            char *buffer,
                                            size_t buffer_size)
{
    httpd_req_t *request = context;
    if (buffer != NULL && buffer_size > 0U) {
        buffer[0] = '\0';
    }
    if (request == NULL || name == NULL || buffer == NULL || buffer_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const size_t length = httpd_req_get_hdr_value_len(request, name);
    if (length == 0U || length >= buffer_size) {
        return APP_ERROR_AUTH_REQUIRED;
    }
    return httpd_req_get_hdr_value_str(request, name, buffer, buffer_size) == ESP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_AUTH_REQUIRED;
}

static app_error_code_t validate_session_adapter(void *context,
                                                  const char *session_token,
                                                  const char *csrf_token)
{
    (void)context;
    return auth_session_validate(session_token, csrf_token);
}

static app_error_code_t authorize_mutation(httpd_req_t *request, char *out_session_token)
{
    return web_adapter_authorize_mutation(get_header_adapter,
                                          validate_session_adapter,
                                          request,
                                          out_session_token,
                                          AUTH_TOKEN_HEX_BYTES);
}
