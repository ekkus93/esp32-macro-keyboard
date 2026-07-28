#include "web_api_core.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_executor.h"

#define WEB_API_PREFIX "/api/v1/"
#define WEB_API_MAX_SEGMENTS 8U

typedef struct {
    char *items[WEB_API_MAX_SEGMENTS];
    size_t count;
} path_segments_t;

static bool text_equal(const char *left, const char *right) {
    return left != NULL && right != NULL && strcmp(left, right) == 0;
}

static bool parse_uuid_segment(const char *segment, app_uuid_t *out_uuid) {
    return segment != NULL && out_uuid != NULL &&
           app_uuid_parse(segment, out_uuid) == APP_ERROR_NONE;
}

static app_error_code_t split_path(const char *uri, char *buffer, size_t buffer_size,
                                   path_segments_t *out_segments) {
    if (uri == NULL || buffer == NULL || buffer_size == 0U || out_segments == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_segments, 0, sizeof(*out_segments));
    const size_t length = strlen(uri);
    const size_t prefix_length = sizeof(WEB_API_PREFIX) - 1U;
    if (length <= prefix_length || length >= buffer_size ||
        strncmp(uri, WEB_API_PREFIX, prefix_length) != 0 || strchr(uri, '?') != NULL ||
        strchr(uri, '#') != NULL || strchr(uri, '%') != NULL || strchr(uri, '\\') != NULL ||
        strstr(uri, "//") != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(buffer, uri + prefix_length, length - prefix_length + 1U);
    char *segment = buffer;
    while (segment != NULL) {
        if (*segment == '\0' || text_equal(segment, ".") || text_equal(segment, "..") ||
            out_segments->count >= WEB_API_MAX_SEGMENTS) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        out_segments->items[out_segments->count++] = segment;
        char *separator = strchr(segment, '/');
        if (separator == NULL) {
            break;
        }
        *separator = '\0';
        segment = separator + 1;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t match_set_routes(const path_segments_t *segments,
                                         web_api_path_t *out_path) {
    if (segments->count == 1U) {
        out_path->route = WEB_API_ROUTE_SETS;
        return APP_ERROR_NONE;
    }
    if (segments->count == 2U && text_equal(segments->items[1], "import")) {
        out_path->route = WEB_API_ROUTE_SET_IMPORT;
        return APP_ERROR_NONE;
    }
    if (!parse_uuid_segment(segments->items[1], &out_path->set_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_path->has_set_id = true;
    if (segments->count == 2U) {
        out_path->route = WEB_API_ROUTE_SET;
        return APP_ERROR_NONE;
    }
    if (segments->count == 3U) {
        if (text_equal(segments->items[2], "duplicate")) {
            out_path->route = WEB_API_ROUTE_SET_DUPLICATE;
        } else if (text_equal(segments->items[2], "select")) {
            out_path->route = WEB_API_ROUTE_SET_SELECT;
        } else if (text_equal(segments->items[2], "export")) {
            out_path->route = WEB_API_ROUTE_SET_EXPORT;
        } else if (text_equal(segments->items[2], "macros")) {
            out_path->route = WEB_API_ROUTE_SET_MACROS;
        } else if (text_equal(segments->items[2], "procedures")) {
            out_path->route = WEB_API_ROUTE_SET_PROCEDURES;
        } else {
            return APP_ERROR_NOT_FOUND;
        }
        return APP_ERROR_NONE;
    }
    if (text_equal(segments->items[2], "macros")) {
        if (segments->count == 4U && text_equal(segments->items[3], "reorder")) {
            out_path->route = WEB_API_ROUTE_SET_MACROS_REORDER;
            return APP_ERROR_NONE;
        }
        if (!parse_uuid_segment(segments->items[3], &out_path->macro_id)) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        out_path->has_macro_id = true;
        if (segments->count == 4U) {
            out_path->route = WEB_API_ROUTE_SET_MACRO;
        } else if (segments->count == 5U && text_equal(segments->items[4], "validate")) {
            out_path->route = WEB_API_ROUTE_SET_MACRO_VALIDATE;
        } else if (segments->count == 5U && text_equal(segments->items[4], "duplicate")) {
            out_path->route = WEB_API_ROUTE_SET_MACRO_DUPLICATE;
        } else {
            return APP_ERROR_NOT_FOUND;
        }
        return APP_ERROR_NONE;
    }
    if (text_equal(segments->items[2], "procedures")) {
        if (segments->count == 4U && text_equal(segments->items[3], "reorder")) {
            out_path->route = WEB_API_ROUTE_SET_PROCEDURES_REORDER;
            return APP_ERROR_NONE;
        }
        if (!parse_uuid_segment(segments->items[3], &out_path->procedure_id)) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        out_path->has_procedure_id = true;
        if (segments->count == 4U) {
            out_path->route = WEB_API_ROUTE_SET_PROCEDURE;
        } else if (segments->count == 5U && text_equal(segments->items[4], "progress")) {
            out_path->route = WEB_API_ROUTE_PROCEDURE_PROGRESS;
        } else if (segments->count == 6U && text_equal(segments->items[4], "progress") &&
                   text_equal(segments->items[5], "complete")) {
            out_path->route = WEB_API_ROUTE_PROGRESS_COMPLETE;
        } else if (segments->count == 6U && text_equal(segments->items[4], "progress") &&
                   text_equal(segments->items[5], "skip")) {
            out_path->route = WEB_API_ROUTE_PROGRESS_SKIP;
        } else {
            return APP_ERROR_NOT_FOUND;
        }
        return APP_ERROR_NONE;
    }
    return APP_ERROR_NOT_FOUND;
}

static app_error_code_t match_global_routes(const path_segments_t *segments,
                                            web_api_path_t *out_path) {
    if (segments->count < 2U || !text_equal(segments->items[1], "macros")) {
        return APP_ERROR_NOT_FOUND;
    }
    if (segments->count == 2U) {
        out_path->route = WEB_API_ROUTE_GLOBAL_MACROS;
        return APP_ERROR_NONE;
    }
    if (segments->count == 3U && text_equal(segments->items[2], "reorder")) {
        out_path->route = WEB_API_ROUTE_GLOBAL_MACROS_REORDER;
        return APP_ERROR_NONE;
    }
    if (!parse_uuid_segment(segments->items[2], &out_path->macro_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_path->has_macro_id = true;
    if (segments->count == 3U) {
        out_path->route = WEB_API_ROUTE_GLOBAL_MACRO;
    } else if (segments->count == 4U && text_equal(segments->items[3], "validate")) {
        out_path->route = WEB_API_ROUTE_GLOBAL_MACRO_VALIDATE;
    } else if (segments->count == 4U && text_equal(segments->items[3], "duplicate")) {
        out_path->route = WEB_API_ROUTE_GLOBAL_MACRO_DUPLICATE;
    } else {
        return APP_ERROR_NOT_FOUND;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t match_execution_routes(const path_segments_t *segments,
                                               web_api_path_t *out_path) {
    if (segments->count == 1U) {
        out_path->route = WEB_API_ROUTE_EXECUTIONS;
        return APP_ERROR_NONE;
    }
    if (segments->count == 2U && text_equal(segments->items[1], "current")) {
        out_path->route = WEB_API_ROUTE_EXECUTION_CURRENT;
        return APP_ERROR_NONE;
    }
    if (!parse_uuid_segment(segments->items[1], &out_path->execution_id)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_path->has_execution_id = true;
    if (segments->count == 3U && text_equal(segments->items[2], "cancel")) {
        out_path->route = WEB_API_ROUTE_EXECUTION_CANCEL;
    } else if (segments->count == 3U && text_equal(segments->items[2], "confirm")) {
        out_path->route = WEB_API_ROUTE_EXECUTION_CONFIRM;
    } else {
        return APP_ERROR_NOT_FOUND;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t match_simple_routes(const path_segments_t *segments,
                                            web_api_path_t *out_path) {
    if (segments->count == 2U && text_equal(segments->items[0], "auth") &&
        text_equal(segments->items[1], "session")) {
        out_path->route = WEB_API_ROUTE_AUTH_SESSION;
    } else if (segments->count == 1U && text_equal(segments->items[0], "settings")) {
        out_path->route = WEB_API_ROUTE_SETTINGS;
    } else if (segments->count == 2U && text_equal(segments->items[0], "settings") &&
               text_equal(segments->items[1], "change-password")) {
        out_path->route = WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD;
    } else if (segments->count == 2U && text_equal(segments->items[0], "device") &&
               text_equal(segments->items[1], "restart")) {
        out_path->route = WEB_API_ROUTE_DEVICE_RESTART;
    } else if (segments->count == 2U && text_equal(segments->items[0], "device") &&
               text_equal(segments->items[1], "reset-settings")) {
        out_path->route = WEB_API_ROUTE_DEVICE_RESET_SETTINGS;
    } else if (segments->count == 2U && text_equal(segments->items[0], "device") &&
               text_equal(segments->items[1], "factory-reset")) {
        out_path->route = WEB_API_ROUTE_DEVICE_FACTORY_RESET;
    } else if (segments->count == 2U && text_equal(segments->items[0], "diagnostics") &&
               text_equal(segments->items[1], "storage")) {
        out_path->route = WEB_API_ROUTE_DIAGNOSTICS_STORAGE;
    } else if (segments->count == 3U && text_equal(segments->items[0], "diagnostics") &&
               text_equal(segments->items[1], "storage") &&
               text_equal(segments->items[2], "check")) {
        out_path->route = WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK;
    } else if (segments->count == 2U && text_equal(segments->items[0], "diagnostics") &&
               text_equal(segments->items[1], "quarantine")) {
        out_path->route = WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE;
    } else if (segments->count == 1U && text_equal(segments->items[0], "backup")) {
        out_path->route = WEB_API_ROUTE_BACKUP;
    } else if (segments->count == 1U && text_equal(segments->items[0], "restore")) {
        out_path->route = WEB_API_ROUTE_RESTORE;
    } else {
        return APP_ERROR_NOT_FOUND;
    }
    return APP_ERROR_NONE;
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
    if (media_length != sizeof("application/json") - 1U ||
        strncasecmp(content_type, "application/json", media_length) != 0) {
        return false;
    }
    if (separator == NULL) {
        return true;
    }
    ++separator;
    while (isspace((unsigned char)*separator) != 0) {
        ++separator;
    }
    return strcasecmp(separator, "charset=utf-8") == 0;
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
        const unsigned char character = (unsigned char)request_id[index];
        if (isalnum(character) == 0 && character != '-' && character != '_' && character != '.' &&
            character != ':') {
            return false;
        }
    }
    return true;
}

app_error_code_t web_api_parse_path(const char *uri, web_api_path_t *out_path) {
    if (out_path == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_path, 0, sizeof(*out_path));
    char buffer[WEB_API_URI_MAX_BYTES];
    path_segments_t segments = {0};
    app_error_code_t result = split_path(uri, buffer, sizeof(buffer), &segments);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (text_equal(segments.items[0], "sets")) {
        result = match_set_routes(&segments, out_path);
    } else if (text_equal(segments.items[0], "global")) {
        result = match_global_routes(&segments, out_path);
    } else if (text_equal(segments.items[0], "executions")) {
        result = match_execution_routes(&segments, out_path);
    } else {
        result = match_simple_routes(&segments, out_path);
    }
    if (result != APP_ERROR_NONE) {
        memset(out_path, 0, sizeof(*out_path));
    }
    return result;
}

bool web_api_route_allows_method(web_api_route_t route, web_api_method_t method) {
    switch (route) {
    case WEB_API_ROUTE_AUTH_SESSION:
    case WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE:
        return method == WEB_API_METHOD_GET;
    case WEB_API_ROUTE_SETS:
    case WEB_API_ROUTE_SET_MACROS:
    case WEB_API_ROUTE_GLOBAL_MACROS:
    case WEB_API_ROUTE_SET_PROCEDURES:
        return method == WEB_API_METHOD_GET || method == WEB_API_METHOD_POST;
    case WEB_API_ROUTE_SET:
    case WEB_API_ROUTE_SET_MACRO:
    case WEB_API_ROUTE_GLOBAL_MACRO:
    case WEB_API_ROUTE_SET_PROCEDURE:
    case WEB_API_ROUTE_PROCEDURE_PROGRESS:
    case WEB_API_ROUTE_SETTINGS:
        return method == WEB_API_METHOD_GET || method == WEB_API_METHOD_PUT ||
               method == WEB_API_METHOD_DELETE;
    case WEB_API_ROUTE_SET_EXPORT:
    case WEB_API_ROUTE_BACKUP:
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE:
        return method == WEB_API_METHOD_GET;
    case WEB_API_ROUTE_SET_IMPORT:
    case WEB_API_ROUTE_RESTORE:
    case WEB_API_ROUTE_SET_DUPLICATE:
    case WEB_API_ROUTE_SET_SELECT:
    case WEB_API_ROUTE_SET_MACRO_VALIDATE:
    case WEB_API_ROUTE_SET_MACRO_DUPLICATE:
    case WEB_API_ROUTE_SET_MACROS_REORDER:
    case WEB_API_ROUTE_GLOBAL_MACRO_VALIDATE:
    case WEB_API_ROUTE_GLOBAL_MACRO_DUPLICATE:
    case WEB_API_ROUTE_GLOBAL_MACROS_REORDER:
    case WEB_API_ROUTE_SET_PROCEDURES_REORDER:
    case WEB_API_ROUTE_PROGRESS_COMPLETE:
    case WEB_API_ROUTE_PROGRESS_SKIP:
    case WEB_API_ROUTE_EXECUTIONS:
    case WEB_API_ROUTE_EXECUTION_CANCEL:
    case WEB_API_ROUTE_EXECUTION_CONFIRM:
    case WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD:
    case WEB_API_ROUTE_DEVICE_RESTART:
    case WEB_API_ROUTE_DEVICE_RESET_SETTINGS:
    case WEB_API_ROUTE_DEVICE_FACTORY_RESET:
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK:
        return method == WEB_API_METHOD_POST;
    case WEB_API_ROUTE_EXECUTION_CURRENT:
        return method == WEB_API_METHOD_GET;
    case WEB_API_ROUTE_UNKNOWN:
    default:
        return false;
    }
}

bool web_api_route_requires_body(web_api_route_t route, web_api_method_t method) {
    if (method == WEB_API_METHOD_GET) {
        return false;
    }
    switch (route) {
    case WEB_API_ROUTE_EXECUTION_CANCEL:
    case WEB_API_ROUTE_EXECUTION_CONFIRM:
    case WEB_API_ROUTE_DEVICE_RESTART:
    case WEB_API_ROUTE_DEVICE_FACTORY_RESET:
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK:
        return false;
    default:
        return true;
    }
}

bool web_api_route_requires_session(web_api_route_t route) {
    return route != WEB_API_ROUTE_UNKNOWN;
}

bool web_api_route_requires_csrf(web_api_route_t route, web_api_method_t method) {
    (void)route;
    return method != WEB_API_METHOD_GET;
}

bool web_api_route_requires_physical_confirmation(web_api_route_t route) {
    return route == WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD ||
           route == WEB_API_ROUTE_DEVICE_RESTART || route == WEB_API_ROUTE_DEVICE_RESET_SETTINGS ||
           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET || route == WEB_API_ROUTE_RESTORE;
}

unsigned int web_api_http_status_for_error(app_error_code_t error) {
    switch (error) {
    case APP_ERROR_NONE:
        return 200U;
    case APP_ERROR_INVALID_ARGUMENT:
    case APP_ERROR_MACRO_SYNTAX:
    case APP_ERROR_MACRO_LIMIT:
        return 422U;
    case APP_ERROR_NOT_FOUND:
        return 404U;
    case APP_ERROR_CONFLICT:
    case APP_ERROR_EXECUTOR_BUSY:
        return 409U;
    case APP_ERROR_STORAGE_FULL:
        return 507U;
    case APP_ERROR_STORAGE_UNAVAILABLE:
    case APP_ERROR_STORAGE_CORRUPT:
    case APP_ERROR_USB_NOT_READY:
    case APP_ERROR_TIMEOUT:
        return 503U;
    case APP_ERROR_AUTH_REQUIRED:
    case APP_ERROR_AUTH_FAILED:
        return 401U;
    case APP_ERROR_RATE_LIMITED:
        return 429U;
    case APP_ERROR_EXECUTION_CANCELLED:
        return 409U;
    case APP_ERROR_IO:
    case APP_ERROR_INTERNAL:
    default:
        return 500U;
    }
}

unsigned int web_api_cancel_http_status(const macro_execution_status_t *status,
                                        app_error_code_t cancel_result) {
    if (cancel_result == APP_ERROR_NONE) {
        return 202U;
    }
    if (cancel_result == APP_ERROR_INTERNAL) {
        return 500U;
    }
    if (cancel_result == APP_ERROR_STORAGE_UNAVAILABLE ||
        cancel_result == APP_ERROR_USB_NOT_READY) {
        return 503U;
    }
    if (cancel_result == APP_ERROR_NOT_FOUND && status != NULL) {
        switch (status->state) {
        case EXECUTION_COMPLETED:
        case EXECUTION_CANCELLED:
        case EXECUTION_FAILED:
        case EXECUTION_TIMED_OUT:
            return 409U;
        case EXECUTION_IDLE:
        case EXECUTION_RUNNING:
        default:
            return 404U;
        }
    }
    return 409U;
}
