#include "web_server_internal.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "api_contracts_v2.h"
#include "app_error.h"
#include "setup_contract_v2.h"
#include "web_server.h"
#include "web_server_adapter.h"

#define WEB_MAX_URI_HANDLERS 28U
#define WEB_HTTPD_TASK_STACK_BYTES 24576U

static const httpd_uri_t normal_routes[] = {
    /* /api/v1/setup is deliberately absent here (scripts/check-setup-route-
     * isolation.sh enforces that): once provisioned, GET/POST /api/v1/setup
     * fall through to the generic wildcard routes below and are answered by
     * api_handler through the WEB_API_ROUTE_SETUP case in
     * web_api_handle_administration(), which returns 404 / 409 per SPEC 13.4
     * without requiring a session. */
    {.uri = "/api/v1/status", .method = HTTP_GET, .handler = status_handler},
    {.uri = "/api/v1/limits", .method = HTTP_GET, .handler = limits_handler},
    {.uri = "/api/v1/auth/login", .method = HTTP_POST, .handler = login_handler},
    {.uri = "/api/v1/auth/logout", .method = HTTP_POST, .handler = logout_handler},
    {.uri = "/api/v1/blob", .method = HTTP_GET, .handler = blob_list_handler},
    {.uri = "/api/v1/blob", .method = HTTP_POST, .handler = blob_create_handler},
    {.uri = "/api/v1/blob/*", .method = HTTP_GET, .handler = blob_load_handler},
    {.uri = "/api/v1/blob/*", .method = HTTP_DELETE, .handler = blob_delete_handler},
    {.uri = "/api/v1/send", .method = HTTP_POST, .handler = send_create_handler},
    {.uri = "/api/v1/send", .method = HTTP_GET, .handler = send_get_handler},
    {.uri = "/api/v1/send", .method = HTTP_DELETE, .handler = send_cancel_handler},
    {.uri = "/api/v1/*", .method = HTTP_GET, .handler = api_handler},
    {.uri = "/api/v1/*", .method = HTTP_POST, .handler = api_handler},
    {.uri = "/api/v1/*", .method = HTTP_PUT, .handler = api_handler},
    {.uri = "/api/v1/*", .method = HTTP_DELETE, .handler = api_handler},
    {.uri = "/*", .method = HTTP_GET, .handler = static_handler},
};

static const httpd_uri_t setup_routes[] = {
    {.uri = "/api/v1/setup", .method = HTTP_GET, .handler = setup_state_handler},
    {.uri = "/api/v1/setup", .method = HTTP_POST, .handler = setup_submit_handler},
    {.uri = "/*", .method = HTTP_GET, .handler = static_handler},
};

typedef struct {
    const httpd_uri_t *routes;
    size_t count;
} route_table_t;

static route_table_t active_route_table(void) {
    if (server_configuration.mode == WEB_SERVER_MODE_SETUP) {
        return (route_table_t){
            .routes = setup_routes,
            .count = sizeof(setup_routes) / sizeof(setup_routes[0]),
        };
    }
    return (route_table_t){
        .routes = normal_routes,
        .count = sizeof(normal_routes) / sizeof(normal_routes[0]),
    };
}

static bool setup_code_valid(const char *code) {
    if (code == NULL || strlen(code) != APP_V2_SETUP_CODE_DIGITS) {
        return false;
    }
    for (size_t index = 0U; index < APP_V2_SETUP_CODE_DIGITS; ++index) {
        if (code[index] < '0' || code[index] > '9') {
            return false;
        }
    }
    return true;
}

static void clear_setup_configuration_secret(void) {
    memset(server_configuration.setup_code, 0, sizeof(server_configuration.setup_code));
}

/* server_configuration.setup_code is only the transport for the plaintext
 * code from app_core; it is wiped immediately below. The persistent,
 * one-time-consumable copy that setup_submit_handler() validates against for
 * the rest of setup mode lives in `setup_session`. */
static app_error_code_t initialize_setup_session(void) {
    const app_v2_string_view_t code_view = {
        .data = server_configuration.setup_code,
        .length = strlen(server_configuration.setup_code),
    };
    return app_v2_setup_session_init(&setup_session, code_view) == APP_V2_SETUP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_INVALID_ARGUMENT;
}

static int start_server_adapter(void *context, void **out_handle) {
    (void)context;
    httpd_config_t configuration = HTTPD_DEFAULT_CONFIG();
    configuration.max_uri_handlers = WEB_MAX_URI_HANDLERS;
    configuration.stack_size = WEB_HTTPD_TASK_STACK_BYTES;
    configuration.uri_match_fn = httpd_uri_match_wildcard;
    httpd_handle_t handle = NULL;
    if (httpd_start(&handle, &configuration) != ESP_OK) {
        return -1;
    }
    if (web_server_async_start() != APP_ERROR_NONE) {
        (void)httpd_stop(handle);
        return -1;
    }
    *out_handle = handle;
    return 0;
}

static int register_route_adapter(void *context, void *handle, size_t route_index) {
    (void)context;
    const route_table_t table = active_route_table();
    if (route_index >= table.count) {
        return -1;
    }
    return httpd_register_uri_handler((httpd_handle_t)handle, &table.routes[route_index]) == ESP_OK
               ? 0
               : -1;
}

static int stop_server_adapter(void *context, void *handle) {
    (void)context;
    if (web_server_async_stop() != APP_ERROR_NONE) {
        return -1;
    }
    return httpd_stop((httpd_handle_t)handle) == ESP_OK ? 0 : -1;
}

static web_adapter_lifecycle_ops_t server_lifecycle_ops(void) {
    return (web_adapter_lifecycle_ops_t){
        .context = NULL,
        .start = start_server_adapter,
        .register_route = register_route_adapter,
        .stop = stop_server_adapter,
    };
}

static bool server_configuration_valid(const web_server_config_t *configuration) {
    if (configuration == NULL) {
        return false;
    }
    switch (configuration->mode) {
    case WEB_SERVER_MODE_NORMAL:
        return configuration->login_enabled;
    case WEB_SERVER_MODE_SETUP:
        return !configuration->login_enabled && configuration->setup_device_name[0] != '\0' &&
               setup_code_valid(configuration->setup_code);
    default:
        return false;
    }
}

app_error_code_t web_server_start(const web_server_config_t *configuration) {
    if (!server_configuration_valid(configuration) || server_lifecycle.handle != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    server_configuration = *configuration;
    memset(&setup_session, 0, sizeof(setup_session));
    if (server_configuration.mode == WEB_SERVER_MODE_SETUP &&
        initialize_setup_session() != APP_ERROR_NONE) {
        clear_setup_configuration_secret();
        memset(&server_configuration, 0, sizeof(server_configuration));
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const web_adapter_lifecycle_ops_t operations = server_lifecycle_ops();
    const route_table_t table = active_route_table();
    const app_error_code_t result =
        web_adapter_lifecycle_start(&server_lifecycle, &operations, table.count);
    if (server_configuration.mode == WEB_SERVER_MODE_SETUP) {
        clear_setup_configuration_secret();
    }
    if (result != APP_ERROR_NONE && server_lifecycle.handle == NULL) {
        memset(&server_configuration, 0, sizeof(server_configuration));
        memset(&setup_session, 0, sizeof(setup_session));
    }
    return result;
}

app_error_code_t web_server_stop(void) {
    const web_adapter_lifecycle_ops_t operations = server_lifecycle_ops();
    const app_error_code_t result = web_adapter_lifecycle_stop(&server_lifecycle, &operations);
    if (result == APP_ERROR_NONE) {
        memset(&server_configuration, 0, sizeof(server_configuration));
        memset(&setup_session, 0, sizeof(setup_session));
    }
    return result;
}

bool web_server_owns_resources(void) {
    return web_adapter_lifecycle_owns_resources(&server_lifecycle);
}
