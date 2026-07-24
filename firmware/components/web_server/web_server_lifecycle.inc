static const httpd_uri_t server_routes[] = {
    {.uri = "/api/v1/status", .method = HTTP_GET, .handler = status_handler},
    {.uri = "/api/v1/limits", .method = HTTP_GET, .handler = limits_handler},
    {.uri = "/api/v1/auth/login", .method = HTTP_POST, .handler = login_handler},
    {.uri = "/api/v1/auth/logout", .method = HTTP_POST, .handler = logout_handler},
    {.uri = "/api/v1/executions/current", .method = HTTP_GET, .handler = execution_handler},
    {.uri = "/api/v1/executions/current/cancel",
     .method = HTTP_POST,
     .handler = cancel_handler},
    {.uri = "/*", .method = HTTP_GET, .handler = static_handler},
};

static int start_server_adapter(void *context, void **out_handle)
{
    (void)context;
    httpd_config_t configuration = HTTPD_DEFAULT_CONFIG();
    configuration.max_uri_handlers = 24U;
    configuration.stack_size = 8192U;
    configuration.uri_match_fn = httpd_uri_match_wildcard;
    httpd_handle_t handle = NULL;
    if (httpd_start(&handle, &configuration) != ESP_OK) {
        return -1;
    }
    *out_handle = handle;
    return 0;
}

static int register_route_adapter(void *context, void *handle, size_t route_index)
{
    (void)context;
    if (route_index >= sizeof(server_routes) / sizeof(server_routes[0])) {
        return -1;
    }
    return httpd_register_uri_handler((httpd_handle_t)handle,
                                      &server_routes[route_index]) == ESP_OK
               ? 0
               : -1;
}

static int stop_server_adapter(void *context, void *handle)
{
    (void)context;
    return httpd_stop((httpd_handle_t)handle) == ESP_OK ? 0 : -1;
}

static web_adapter_lifecycle_ops_t server_lifecycle_ops(void)
{
    return (web_adapter_lifecycle_ops_t){
        .context = NULL,
        .start = start_server_adapter,
        .register_route = register_route_adapter,
        .stop = stop_server_adapter,
    };
}

app_error_code_t web_server_start(const web_server_config_t *configuration)
{
    if (configuration == NULL || server_lifecycle.handle != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    server_configuration = *configuration;
    const web_adapter_lifecycle_ops_t ops = server_lifecycle_ops();
    const app_error_code_t result = web_adapter_lifecycle_start(
        &server_lifecycle,
        &ops,
        sizeof(server_routes) / sizeof(server_routes[0]));
    if (result != APP_ERROR_NONE && server_lifecycle.handle == NULL) {
        memset(&server_configuration, 0, sizeof(server_configuration));
    }
    return result;
}

app_error_code_t web_server_stop(void)
{
    const web_adapter_lifecycle_ops_t ops = server_lifecycle_ops();
    const app_error_code_t result = web_adapter_lifecycle_stop(&server_lifecycle, &ops);
    if (result == APP_ERROR_NONE) {
        memset(&server_configuration, 0, sizeof(server_configuration));
    }
    return result;
}
