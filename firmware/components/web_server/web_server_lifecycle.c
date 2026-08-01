#include "web_server_internal.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "web_server.h"
#include "web_server_adapter.h"

#define WEB_MAX_URI_HANDLERS 28U

/* httpd task stack. 8192 (the ESP-IDF-ish default this used to carry) overflows
 * on real hardware for any request reaching a storage write path: those nest
 * several multi-kilobyte frames (storage_set_index_t ~2 KB, storage_uuid_order_t
 * ~8 KB, transaction manifests, APP_PATH_MAX_BYTES buffers, cJSON scratch).
 * scripts/check-stack-usage.sh enforces per-frame limits; this is the budget
 * those frames are measured against. */
#define WEB_HTTPD_TASK_STACK_BYTES 24576U

static const httpd_uri_t normal_routes[] = {
    {.uri = "/api/v1/status", .method = HTTP_GET, .handler = status_handler},
    {.uri = "/api/v1/limits", .method = HTTP_GET, .handler = limits_handler},
    {.uri = "/api/v1/auth/login", .method = HTTP_POST, .handler = login_handler},
    {.uri = "/api/v1/auth/logout", .method = HTTP_POST, .handler = logout_handler},
    {.uri = "/api/v1/*", .method = HTTP_GET, .handler = api_handler},
    {.uri = "/api/v1/*", .method = HTTP_POST, .handler = api_handler},
    {.uri = "/api/v1/*", .method = HTTP_PUT, .handler = api_handler},
    {.uri = "/api/v1/*", .method = HTTP_DELETE, .handler = api_handler},
    {.uri = "/*", .method = HTTP_GET, .handler = static_handler},
};

static const httpd_uri_t setup_routes[] = {
    {.uri = "/api/v1/setup-state", .method = HTTP_GET, .handler = setup_state_handler},
    {.uri = "/api/v1/setup/credentials", .method = HTTP_POST, .handler = setup_credentials_handler},
    {.uri = "/api/v1/setup/complete", .method = HTTP_POST, .handler = setup_complete_handler},
    {.uri = "/api/v1/setup/restart", .method = HTTP_POST, .handler = setup_restart_handler},
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

static void clear_setup_configuration_secrets(void) {
    memset(server_configuration.setup_device_id, 0, sizeof(server_configuration.setup_device_id));
    memset(server_configuration.setup_ap_ssid, 0, sizeof(server_configuration.setup_ap_ssid));
    memset(server_configuration.setup_code, 0, sizeof(server_configuration.setup_code));
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
        return !configuration->login_enabled;
    default:
        return false;
    }
}

app_error_code_t web_server_start(const web_server_config_t *configuration) {
    if (!server_configuration_valid(configuration) || server_lifecycle.handle != NULL ||
        web_server_setup_owns_resources()) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    server_configuration = *configuration;
    app_error_code_t result = APP_ERROR_NONE;
    if (server_configuration.mode == WEB_SERVER_MODE_SETUP) {
        result = web_server_setup_init(&server_configuration);
        clear_setup_configuration_secrets();
    }
    if (result == APP_ERROR_NONE) {
        const web_adapter_lifecycle_ops_t operations = server_lifecycle_ops();
        const route_table_t table = active_route_table();
        result = web_adapter_lifecycle_start(&server_lifecycle, &operations, table.count);
    }
    if (result != APP_ERROR_NONE && server_lifecycle.handle == NULL) {
        app_error_code_t setup_cleanup = APP_ERROR_NONE;
        if (server_configuration.mode == WEB_SERVER_MODE_SETUP &&
            web_server_setup_owns_resources()) {
            setup_cleanup = web_server_setup_deinit();
        }
        if (setup_cleanup == APP_ERROR_NONE) {
            memset(&server_configuration, 0, sizeof(server_configuration));
        }
    }
    return result;
}

app_error_code_t web_server_stop(void) {
    const web_adapter_lifecycle_ops_t operations = server_lifecycle_ops();
    app_error_code_t result = web_adapter_lifecycle_stop(&server_lifecycle, &operations);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (server_configuration.mode == WEB_SERVER_MODE_SETUP) {
        result = web_server_setup_deinit();
    }
    if (result == APP_ERROR_NONE) {
        memset(&server_configuration, 0, sizeof(server_configuration));
    }
    return result;
}

bool web_server_owns_resources(void) {
    return web_adapter_lifecycle_owns_resources(&server_lifecycle) ||
           web_server_setup_owns_resources();
}
