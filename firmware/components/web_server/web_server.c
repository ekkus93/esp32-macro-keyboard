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
    if (httpd_resp_set_type(request, "application/json") != ESP_OK ||
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
    char response[256U];
    const int length = snprintf(response,
                                sizeof(response),
                                "{\"ok\":false,\"error\":{\"code\":\"%s\","
                                "\"message\":\"%s\"}}",
                                app_error_code_string(code),
                                message);
    if (length < 0 || (size_t)length >= sizeof(response)) {
        return httpd_resp_send_err(request,
                                   HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "response overflow");
    }
    return send_json(request, response, status);
}

static app_error_code_t read_bounded_body(httpd_req_t *request,
                                          char *buffer,
                                          size_t buffer_size,
                                          size_t maximum_length)
{
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
        if (count <= 0) {
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
    const size_t length = httpd_req_get_hdr_value_len(request, name);
    if (length == 0U || length >= buffer_size) {
        return APP_ERROR_AUTH_REQUIRED;
    }
    return httpd_req_get_hdr_value_str(request, name, buffer, buffer_size) == ESP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_AUTH_REQUIRED;
}

static app_error_code_t cookie_session_token(const char *cookie,
                                             char *token,
                                             size_t token_size)
{
    if (cookie == NULL || token == NULL || token_size < AUTH_TOKEN_HEX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const char prefix[] = SESSION_COOKIE_NAME "=";
    const size_t prefix_length = sizeof(prefix) - 1U;
    const char *cursor = cookie;
    while (*cursor != '\0') {
        while (*cursor == ' ' || *cursor == ';') {
            ++cursor;
        }
        const char *end = strchr(cursor, ';');
        const size_t length = end == NULL ? strlen(cursor) : (size_t)(end - cursor);
        if (length >= prefix_length && memcmp(cursor, prefix, prefix_length) == 0) {
            const size_t value_length = length - prefix_length;
            if (value_length != AUTH_TOKEN_HEX_BYTES - 1U || value_length >= token_size) {
                return APP_ERROR_AUTH_REQUIRED;
            }
            memcpy(token, cursor + prefix_length, value_length);
            token[value_length] = '\0';
            return APP_ERROR_NONE;
        }
        if (end == NULL) {
            break;
        }
        cursor = end + 1;
    }
    return APP_ERROR_AUTH_REQUIRED;
}

static bool origin_matches_host(const char *origin, const char *host)
{
    static const char scheme[] = "http://";
    if (origin == NULL || host == NULL || strncmp(origin, scheme, sizeof(scheme) - 1U) != 0) {
        return false;
    }
    return strcmp(origin + sizeof(scheme) - 1U, host) == 0;
}

static app_error_code_t authorize_mutation(httpd_req_t *request, char *out_session_token)
{
    char host[HTTP_HEADER_MAX_BYTES];
    char origin[HTTP_HEADER_MAX_BYTES];
    char cookie[HTTP_HEADER_MAX_BYTES];
    char csrf[AUTH_TOKEN_HEX_BYTES];

    if (get_header(request, "Host", host, sizeof(host)) != APP_ERROR_NONE ||
        get_header(request, "Origin", origin, sizeof(origin)) != APP_ERROR_NONE ||
        get_header(request, "Cookie", cookie, sizeof(cookie)) != APP_ERROR_NONE ||
        get_header(request, "X-CSRF-Token", csrf, sizeof(csrf)) != APP_ERROR_NONE ||
        !origin_matches_host(origin, host)) {
        return APP_ERROR_AUTH_REQUIRED;
    }
    const app_error_code_t cookie_result =
        cookie_session_token(cookie, out_session_token, AUTH_TOKEN_HEX_BYTES);
    if (cookie_result != APP_ERROR_NONE) {
        return cookie_result;
    }
    return auth_session_validate(out_session_token, csrf);
}

static esp_err_t status_handler(httpd_req_t *request)
{
    const wifi_ap_status_t wifi = wifi_ap_get_status();
    const macro_execution_status_t execution = macro_executor_get_status();
    char response[512U];
    const int length = snprintf(
        response,
        sizeof(response),
        "{\"ok\":true,\"data\":{\"version\":\"0.1.0\",\"idf\":\"v5.5.5\","
        "\"usbState\":\"%s\",\"wifiState\":\"%s\",\"wifiClients\":%u,"
        "\"executionState\":\"%s\"}}",
        usb_state_string(usb_keyboard_get_state()),
        wifi_state_string(wifi.state),
        (unsigned int)wifi.client_count,
        execution_state_string(execution.state));
    if (length < 0 || (size_t)length >= sizeof(response)) {
        return send_error(request,
                          "500 Internal Server Error",
                          APP_ERROR_INTERNAL,
                          "response overflow");
    }
    return send_json(request, response, "200 OK");
}

static esp_err_t limits_handler(httpd_req_t *request)
{
    char response[512U];
    const int length = snprintf(
        response,
        sizeof(response),
        "{\"ok\":true,\"data\":{\"macroNameBytes\":%u,\"macroSourceBytes\":%u,"
        "\"compiledActions\":%u,\"delayMs\":%u,\"durationMs\":%u,"
        "\"macrosPerSet\":%u,\"proceduresPerSet\":%u,\"stepsPerProcedure\":%u,"
        "\"macroSets\":%u,\"importBytes\":%u}}",
        APP_MACRO_NAME_MAX_BYTES,
        APP_MACRO_SOURCE_MAX_BYTES,
        APP_COMPILED_ACTION_MAX,
        APP_DELAY_MAX_MS,
        APP_ESTIMATED_DURATION_MAX_MS,
        APP_MACROS_PER_SET_MAX,
        APP_PROCEDURES_PER_SET_MAX,
        APP_STEPS_PER_PROCEDURE_MAX,
        APP_MACRO_SETS_MAX,
        APP_IMPORT_PACKAGE_MAX_BYTES);
    if (length < 0 || (size_t)length >= sizeof(response)) {
        return send_error(request,
                          "500 Internal Server Error",
                          APP_ERROR_INTERNAL,
                          "response overflow");
    }
    return send_json(request, response, "200 OK");
}

static esp_err_t login_handler(httpd_req_t *request)
{
    if (!server_configuration.login_enabled) {
        return send_error(request,
                          "503 Service Unavailable",
                          APP_ERROR_AUTH_REQUIRED,
                          "login is not provisioned");
    }

    uint32_t retry_after = 0U;
    const app_error_code_t allowed = auth_login_attempt_allowed(&retry_after);
    if (allowed == APP_ERROR_RATE_LIMITED) {
        char retry_value[16U];
        const int written = snprintf(retry_value, sizeof(retry_value), "%u", retry_after);
        if (written < 0 || (size_t)written >= sizeof(retry_value) ||
            httpd_resp_set_hdr(request, "Retry-After", retry_value) != ESP_OK) {
            return ESP_FAIL;
        }
        return send_error(request,
                          "429 Too Many Requests",
                          APP_ERROR_RATE_LIMITED,
                          "too many login attempts");
    }
    if (allowed != APP_ERROR_NONE) {
        return send_error(request,
                          "500 Internal Server Error",
                          allowed,
                          "login throttle unavailable");
    }

    char body[LOGIN_BODY_MAX_BYTES + 1U];
    const app_error_code_t body_result =
        read_bounded_body(request, body, sizeof(body), LOGIN_BODY_MAX_BYTES);
    if (body_result != APP_ERROR_NONE) {
        return send_error(request,
                          "400 Bad Request",
                          body_result,
                          "invalid request body");
    }

    cJSON *root = cJSON_ParseWithLength(body, strlen(body));
    if (root == NULL) {
        memset(body, 0, sizeof(body));
        return send_error(request,
                          "400 Bad Request",
                          APP_ERROR_INVALID_ARGUMENT,
                          "invalid JSON");
    }
    const cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "password");
    const bool valid_password = cJSON_IsString(password) && password->valuestring != NULL &&
                                auth_password_verify(password->valuestring,
                                                     strlen(password->valuestring),
                                                     &server_configuration.password_record);
    cJSON_Delete(root);
    memset(body, 0, sizeof(body));

    if (!valid_password) {
        const app_error_code_t failure_result = auth_login_record_failure();
        if (failure_result != APP_ERROR_NONE) {
            return send_error(request,
                              "500 Internal Server Error",
                              failure_result,
                              "could not record login failure");
        }
        return send_error(request,
                          "401 Unauthorized",
                          APP_ERROR_AUTH_FAILED,
                          "invalid credentials");
    }

    const app_error_code_t success_result = auth_login_record_success();
    if (success_result != APP_ERROR_NONE) {
        return send_error(request,
                          "500 Internal Server Error",
                          success_result,
                          "could not reset login throttle");
    }

    auth_session_view_t session = {0};
    const app_error_code_t session_result = auth_session_create(&session);
    if (session_result != APP_ERROR_NONE) {
        return send_error(request,
                          session_result == APP_ERROR_CONFLICT ? "503 Service Unavailable"
                                                               : "500 Internal Server Error",
                          session_result,
                          "could not create session");
    }

    char cookie[160U];
    const int cookie_length = snprintf(cookie,
                                       sizeof(cookie),
                                       SESSION_COOKIE_NAME "=%s; HttpOnly; SameSite=Strict; Path=/",
                                       session.session_token);
    if (cookie_length < 0 || (size_t)cookie_length >= sizeof(cookie) ||
        httpd_resp_set_hdr(request, "Set-Cookie", cookie) != ESP_OK) {
        const app_error_code_t logout_result = auth_session_logout(session.session_token);
        return send_error(request,
                          "500 Internal Server Error",
                          logout_result == APP_ERROR_NONE ? APP_ERROR_INTERNAL : logout_result,
                          "could not create login response");
    }

    char response[192U];
    const int response_length = snprintf(response,
                                         sizeof(response),
                                         "{\"ok\":true,\"data\":{\"csrfToken\":\"%s\"}}",
                                         session.csrf_token);
    if (response_length < 0 || (size_t)response_length >= sizeof(response)) {
        const app_error_code_t logout_result = auth_session_logout(session.session_token);
        return send_error(request,
                          "500 Internal Server Error",
                          logout_result == APP_ERROR_NONE ? APP_ERROR_INTERNAL : logout_result,
                          "response overflow");
    }
    return send_json(request, response, "200 OK");
}

static esp_err_t logout_handler(httpd_req_t *request)
{
    char session_token[AUTH_TOKEN_HEX_BYTES];
    const app_error_code_t authorization = authorize_mutation(request, session_token);
    if (authorization != APP_ERROR_NONE) {
        return send_error(request,
                          "401 Unauthorized",
                          authorization,
                          "authentication required");
    }
    const app_error_code_t result = auth_session_logout(session_token);
    if (result != APP_ERROR_NONE) {
        return send_error(request,
                          "500 Internal Server Error",
                          result,
                          "logout failed");
    }
    if (httpd_resp_set_hdr(request,
                           "Set-Cookie",
                           SESSION_COOKIE_NAME "=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0") !=
        ESP_OK) {
        return ESP_FAIL;
    }
    return send_json(request, "{\"ok\":true,\"data\":{}}", "200 OK");
}

static esp_err_t execution_handler(httpd_req_t *request)
{
    const macro_execution_status_t execution = macro_executor_get_status();
    char response[384U];
    const int length = snprintf(
        response,
        sizeof(response),
        "{\"ok\":true,\"data\":{\"state\":\"%s\",\"error\":\"%s\","
        "\"releaseError\":\"%s\",\"actionIndex\":%u,\"actionCount\":%u}}",
        execution_state_string(execution.state),
        app_error_code_string(execution.error),
        app_error_code_string(execution.release_error),
        (unsigned int)execution.action_index,
        (unsigned int)execution.action_count);
    if (length < 0 || (size_t)length >= sizeof(response)) {
        return send_error(request,
                          "500 Internal Server Error",
                          APP_ERROR_INTERNAL,
                          "response overflow");
    }
    return send_json(request, response, "200 OK");
}

static esp_err_t cancel_handler(httpd_req_t *request)
{
    char session_token[AUTH_TOKEN_HEX_BYTES];
    const app_error_code_t authorization = authorize_mutation(request, session_token);
    if (authorization != APP_ERROR_NONE) {
        return send_error(request,
                          "401 Unauthorized",
                          authorization,
                          "authentication required");
    }
    const app_error_code_t result = macro_executor_cancel();
    if (result != APP_ERROR_NONE) {
        return send_error(request,
                          result == APP_ERROR_NOT_FOUND ? "404 Not Found" : "409 Conflict",
                          result,
                          "no cancellable execution");
    }
    return send_json(request,
                     "{\"ok\":true,\"data\":{\"cancelRequested\":true}}",
                     "202 Accepted");
}

static bool safe_static_uri(const char *uri, char *normalized, size_t normalized_size)
{
    if (uri == NULL || normalized == NULL || normalized_size < 2U || uri[0] != '/' ||
        strncmp(uri, "/api/", 5U) == 0) {
        return false;
    }

    size_t output = 0U;
    for (size_t input = 0U; uri[input] != '\0' && uri[input] != '?'; ++input) {
        const char character = uri[input];
        const bool allowed = (character >= 'a' && character <= 'z') ||
                             (character >= 'A' && character <= 'Z') ||
                             (character >= '0' && character <= '9') || character == '/' ||
                             character == '-' || character == '_' || character == '.';
        if (!allowed || character == '\\' || character == '%' || output + 1U >= normalized_size) {
            return false;
        }
        normalized[output++] = character;
    }
    normalized[output] = '\0';
    return strstr(normalized, "..") == NULL;
}

static const char *content_type(const char *path)
{
    const char *extension = strrchr(path, '.');
    if (extension == NULL) {
        return "application/octet-stream";
    }
    if (strcmp(extension, ".html") == 0) {
        return "text/html; charset=utf-8";
    }
    if (strcmp(extension, ".js") == 0) {
        return "text/javascript; charset=utf-8";
    }
    if (strcmp(extension, ".css") == 0) {
        return "text/css; charset=utf-8";
    }
    if (strcmp(extension, ".svg") == 0) {
        return "image/svg+xml";
    }
    if (strcmp(extension, ".json") == 0) {
        return "application/json";
    }
    if (strcmp(extension, ".png") == 0) {
        return "image/png";
    }
    return "application/octet-stream";
}

static bool accepts_gzip(httpd_req_t *request)
{
    char encoding[HTTP_HEADER_MAX_BYTES];
    const size_t length = httpd_req_get_hdr_value_len(request, "Accept-Encoding");
    return length > 0U && length < sizeof(encoding) &&
           httpd_req_get_hdr_value_str(request,
                                       "Accept-Encoding",
                                       encoding,
                                       sizeof(encoding)) == ESP_OK &&
           strstr(encoding, "gzip") != NULL;
}

static esp_err_t stream_file(httpd_req_t *request, FILE *file)
{
    char buffer[STATIC_CHUNK_BYTES];
    esp_err_t result = ESP_OK;
    while (true) {
        const size_t count = fread(buffer, 1U, sizeof(buffer), file);
        if (count > 0U && httpd_resp_send_chunk(request, buffer, (ssize_t)count) != ESP_OK) {
            result = ESP_FAIL;
            break;
        }
        if (count < sizeof(buffer)) {
            if (ferror(file) != 0) {
                result = ESP_FAIL;
            }
            break;
        }
    }
    const int close_result = fclose(file);
    if (close_result != 0 && result == ESP_OK) {
        result = ESP_FAIL;
    }
    if (result == ESP_OK) {
        result = httpd_resp_send_chunk(request, NULL, 0U);
    }
    return result;
}

static esp_err_t static_handler(httpd_req_t *request)
{
    char normalized[APP_PATH_MAX_BYTES];
    if (!safe_static_uri(request->uri, normalized, sizeof(normalized))) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "invalid path");
    }
    const char *relative = strcmp(normalized, "/") == 0 ? "/index.html" : normalized;
    char path[APP_PATH_MAX_BYTES];
    const int path_length = snprintf(path, sizeof(path), STORAGE_WEB_MOUNT "%s", relative);
    if (path_length < 0 || (size_t)path_length >= sizeof(path)) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "path too long");
    }

    FILE *file = NULL;
    bool compressed = false;
    if (accepts_gzip(request)) {
        char gzip_path[APP_PATH_MAX_BYTES];
        const int gzip_length = snprintf(gzip_path, sizeof(gzip_path), "%s.gz", path);
        if (gzip_length > 0 && (size_t)gzip_length < sizeof(gzip_path)) {
            file = fopen(gzip_path, "rb");
            compressed = file != NULL;
        }
    }
    if (file == NULL) {
        file = fopen(path, "rb");
    }
    if (file == NULL) {
        return httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "not found");
    }

    if (httpd_resp_set_type(request, content_type(path)) != ESP_OK ||
        httpd_resp_set_hdr(request,
                           "Cache-Control",
                           strcmp(relative, "/index.html") == 0
                               ? "no-cache"
                               : "public, max-age=31536000, immutable") != ESP_OK ||
        (compressed && httpd_resp_set_hdr(request, "Content-Encoding", "gzip") != ESP_OK)) {
        if (fclose(file) != 0) {
            return ESP_FAIL;
        }
        return ESP_FAIL;
    }
    return stream_file(request, file);
}

app_error_code_t web_server_start(const web_server_config_t *configuration)
{
    if (configuration == NULL || server != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    server_configuration = *configuration;

    httpd_config_t http_configuration = HTTPD_DEFAULT_CONFIG();
    http_configuration.max_uri_handlers = 24U;
    http_configuration.stack_size = 8192U;
    http_configuration.uri_match_fn = httpd_uri_match_wildcard;
    if (httpd_start(&server, &http_configuration) != ESP_OK) {
        server = NULL;
        memset(&server_configuration, 0, sizeof(server_configuration));
        return APP_ERROR_INTERNAL;
    }

    const httpd_uri_t routes[] = {
        {.uri = "/api/v1/status", .method = HTTP_GET, .handler = status_handler},
        {.uri = "/api/v1/limits", .method = HTTP_GET, .handler = limits_handler},
        {.uri = "/api/v1/auth/login", .method = HTTP_POST, .handler = login_handler},
        {.uri = "/api/v1/auth/logout", .method = HTTP_POST, .handler = logout_handler},
        {.uri = "/api/v1/executions/current", .method = HTTP_GET, .handler = execution_handler},
        {.uri = "/api/v1/executions/current/cancel", .method = HTTP_POST, .handler = cancel_handler},
        {.uri = "/*", .method = HTTP_GET, .handler = static_handler},
    };
    for (size_t index = 0U; index < (sizeof(routes) / sizeof(routes[0])); ++index) {
        if (httpd_register_uri_handler(server, &routes[index]) != ESP_OK) {
            const esp_err_t stop_result = httpd_stop(server);
            server = NULL;
            memset(&server_configuration, 0, sizeof(server_configuration));
            return stop_result == ESP_OK ? APP_ERROR_INTERNAL : APP_ERROR_IO;
        }
    }
    return APP_ERROR_NONE;
}

app_error_code_t web_server_stop(void)
{
    if (server == NULL) {
        return APP_ERROR_NONE;
    }
    const esp_err_t result = httpd_stop(server);
    if (result == ESP_OK) {
        server = NULL;
        memset(&server_configuration, 0, sizeof(server_configuration));
        return APP_ERROR_NONE;
    }
    return APP_ERROR_INTERNAL;
}
