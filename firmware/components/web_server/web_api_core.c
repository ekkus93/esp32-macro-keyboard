#include "web_api_core.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "web_http_status.h"

static bool token_equals_ci(const char *start, size_t length, const char *expected) {
    if (start == NULL || expected == NULL || strlen(expected) != length) {
        return false;
    }
    for (size_t index = 0U; index < length; ++index) {
        if (tolower((unsigned char)start[index]) != tolower((unsigned char)expected[index])) {
            return false;
        }
    }
    return true;
}

bool web_api_content_type_is_json(const char *content_type) {
    if (content_type == NULL) {
        return false;
    }
    while (isspace((unsigned char)*content_type) != 0) {
        ++content_type;
    }
    const char *separator = strchr(content_type, ';');
    const size_t media_length =
        separator == NULL ? strlen(content_type) : (size_t)(separator - content_type);
    size_t trimmed_length = media_length;
    while (trimmed_length > 0U && isspace((unsigned char)content_type[trimmed_length - 1U]) != 0) {
        --trimmed_length;
    }
    if (!token_equals_ci(content_type, trimmed_length, "application/json")) {
        return false;
    }
    if (separator == NULL) {
        return true;
    }
    ++separator;
    while (isspace((unsigned char)*separator) != 0) {
        ++separator;
    }
    return token_equals_ci(separator, strlen(separator), "charset=utf-8");
}

bool web_api_request_id_is_valid(const char *request_id) {
    if (request_id == NULL) {
        return false;
    }
    const size_t length = strlen(request_id);
    if (length == 0U || length > WEB_API_REQUEST_ID_MAX_BYTES) {
        return false;
    }
    for (size_t index = 0U; index < length; ++index) {
        const unsigned char value = (unsigned char)request_id[index];
        if (isalnum(value) == 0 && value != '-' && value != '_' && value != '.' && value != ':') {
            return false;
        }
    }
    return true;
}

typedef struct {
    const char *path;
    web_api_route_t route;
} route_entry_t;

app_error_code_t web_api_parse_path(const char *uri, web_api_path_t *out_path) {
    if (out_path != NULL) {
        memset(out_path, 0, sizeof(*out_path));
    }
    if (uri == NULL || out_path == NULL || uri[0] != '/' || strchr(uri, '?') != NULL ||
        strchr(uri, '#') != NULL || strchr(uri, '%') != NULL || strstr(uri, "//") != NULL ||
        strstr(uri, "..") != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const route_entry_t routes[] = {
        {"/api/v1/auth/session", WEB_API_ROUTE_AUTH_SESSION},
        {"/api/v1/settings", WEB_API_ROUTE_SETTINGS},
        {"/api/v1/settings/change-password", WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD},
        {"/api/v1/device/restart", WEB_API_ROUTE_DEVICE_RESTART},
        {"/api/v1/device/reset-settings", WEB_API_ROUTE_DEVICE_RESET_SETTINGS},
        {"/api/v1/device/factory-reset", WEB_API_ROUTE_DEVICE_FACTORY_RESET},
        {"/api/v1/diagnostics", WEB_API_ROUTE_DIAGNOSTICS_FULL},
    };
    for (size_t index = 0U; index < sizeof(routes) / sizeof(routes[0]); ++index) {
        if (strcmp(uri, routes[index].path) == 0) {
            out_path->route = routes[index].route;
            return APP_ERROR_NONE;
        }
    }
    return APP_ERROR_NOT_FOUND;
}

bool web_api_route_allows_method(web_api_route_t route, web_api_method_t method) {
    switch (route) {
    case WEB_API_ROUTE_AUTH_SESSION:
    case WEB_API_ROUTE_DIAGNOSTICS_FULL:
        return method == WEB_API_METHOD_GET;
    case WEB_API_ROUTE_SETTINGS:
        return method == WEB_API_METHOD_GET || method == WEB_API_METHOD_PUT;
    case WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD:
    case WEB_API_ROUTE_DEVICE_RESTART:
    case WEB_API_ROUTE_DEVICE_RESET_SETTINGS:
    case WEB_API_ROUTE_DEVICE_FACTORY_RESET:
        return method == WEB_API_METHOD_POST;
    case WEB_API_ROUTE_UNKNOWN:
    default:
        return false;
    }
}

bool web_api_route_requires_body(web_api_route_t route, web_api_method_t method) {
    if (method == WEB_API_METHOD_GET || route == WEB_API_ROUTE_DEVICE_RESTART) {
        return false;
    }
    return route == WEB_API_ROUTE_SETTINGS || route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||
           route == WEB_API_ROUTE_DEVICE_RESET_SETTINGS ||
           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET;
}

bool web_api_route_requires_session(web_api_route_t route) {
    return route != WEB_API_ROUTE_UNKNOWN;
}

bool web_api_physical_confirmation_required(web_api_route_t route, bool confirmation_enabled) {
    if (!confirmation_enabled) {
        return false;
    }
    return route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||
           route == WEB_API_ROUTE_DEVICE_RESTART || route == WEB_API_ROUTE_DEVICE_RESET_SETTINGS ||
           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET;
}

bool web_api_route_requires_physical_confirmation(web_api_route_t route) {
    return web_api_physical_confirmation_required(route, true);
}

bool web_api_route_requires_worker(web_api_route_t route) {
    (void)route;
    return false;
}

unsigned int web_api_http_status_for_error(app_error_code_t error) {
    switch (error) {
    case APP_ERROR_NONE:
        return WEB_HTTP_STATUS_OK;
    case APP_ERROR_INVALID_ARGUMENT:
    case APP_ERROR_MACRO_SYNTAX:
    case APP_ERROR_MACRO_LIMIT:
        return WEB_HTTP_STATUS_UNPROCESSABLE_ENTITY;
    case APP_ERROR_NOT_FOUND:
        return WEB_HTTP_STATUS_NOT_FOUND;
    case APP_ERROR_CONFLICT:
    case APP_ERROR_EXECUTOR_BUSY:
    case APP_ERROR_EXECUTION_CANCELLED:
        return WEB_HTTP_STATUS_CONFLICT;
    case APP_ERROR_STORAGE_FULL:
        return WEB_HTTP_STATUS_INSUFFICIENT_STORAGE;
    case APP_ERROR_STORAGE_UNAVAILABLE:
    case APP_ERROR_STORAGE_CORRUPT:
    case APP_ERROR_USB_NOT_READY:
    case APP_ERROR_TIMEOUT:
        return WEB_HTTP_STATUS_SERVICE_UNAVAILABLE;
    case APP_ERROR_AUTH_REQUIRED:
    case APP_ERROR_AUTH_FAILED:
        return WEB_HTTP_STATUS_UNAUTHORIZED;
    case APP_ERROR_RATE_LIMITED:
        return WEB_HTTP_STATUS_TOO_MANY_REQUESTS;
    case APP_ERROR_IO:
    case APP_ERROR_INTERNAL:
    default:
        return WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR;
    }
}
