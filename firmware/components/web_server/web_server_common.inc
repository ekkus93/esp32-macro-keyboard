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
#include "web_static_path.h"
#include "wifi_ap.h"

#define HTTP_HEADER_MAX_BYTES 256U
#define LOGIN_BODY_MAX_BYTES 256U
#define STATIC_CHUNK_BYTES 1024U
#define SESSION_COOKIE_NAME "MKSESSION"

static httpd_handle_t server;
static web_server_config_t server_configuration;

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
    if (request == NULL || status == NULL || message == NULL) {
        return ESP_FAIL;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *error = cJSON_CreateObject();
    if (root == NULL || error == NULL ||
        cJSON_AddStringToObject(error, "code", app_error_code_string(code)) == NULL ||
        cJSON_AddStringToObject(error, "message", message) == NULL) {
        cJSON_Delete(error);
        cJSON_Delete(root);
        return httpd_resp_send_err(request,
                                   HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "response allocation failed");
    }
    if (cJSON_AddBoolToObject(root, "ok", false) == NULL ||
        !cJSON_AddItemToObject(root, "error", error)) {
        cJSON_Delete(error);
        cJSON_Delete(root);
        return httpd_resp_send_err(request,
                                   HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "response allocation failed");
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return httpd_resp_send_err(request,
                                   HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "response allocation failed");
    }
    const esp_err_t result = send_json(request, json, status);
    cJSON_free(json);
    return result;
}

static app_error_code_t read_bounded_body(httpd_req_t *request,
                                          char *buffer,
                                          size_t buffer_size,
                                          size_t maximum_length)
{
    if (buffer != NULL && buffer_size > 0U) {
        buffer[0] = '\0';
    }
    if (request == NULL || buffer == NULL || buffer_size == 0U ||
        request->content_len > maximum_length || request->content_len >= buffer_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    size_t received = 0U;
    while (received < request->content_len) {
        const int count =
            httpd_req_recv(request, buffer + received, request->content_len - received);
        if (count == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (count <= 0 || (size_t)count > request->content_len - received) {
            buffer[0] = '\0';
            return APP_ERROR_IO;
        }
        received += (size_t)count;
    }
    buffer[received] = '\0';
    return APP_ERROR_NONE;
}

static app_error_code_t get_header(httpd_req_t *request,
                                   const char *name,
                                   char *buffer,
                                   size_t buffer_size)
{
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

static app_error_code_t authorize_mutation(httpd_req_t *request, char *out_session_token)
{
    if (out_session_token == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_session_token[0] = '\0';
    char host[HTTP_HEADER_MAX_BYTES];
    char origin[HTTP_HEADER_MAX_BYTES];
    char cookie[HTTP_HEADER_MAX_BYTES];
    char csrf[AUTH_TOKEN_HEX_BYTES];
    if (get_header(request, "Host", host, sizeof(host)) != APP_ERROR_NONE ||
        get_header(request, "Origin", origin, sizeof(origin)) != APP_ERROR_NONE ||
        get_header(request, "Cookie", cookie, sizeof(cookie)) != APP_ERROR_NONE ||
        get_header(request, "X-CSRF-Token", csrf, sizeof(csrf)) != APP_ERROR_NONE ||
        !web_origin_matches_host(origin, host)) {
        return APP_ERROR_AUTH_REQUIRED;
    }
    const app_error_code_t cookie_result =
        web_cookie_extract_session(cookie, out_session_token, AUTH_TOKEN_HEX_BYTES);
    if (cookie_result != APP_ERROR_NONE) {
        return cookie_result;
    }
    return auth_session_validate(out_session_token, csrf);
}
