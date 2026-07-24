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
    for (size_t index = 0U; index < sizeof(routes) / sizeof(routes[0]); ++index) {
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
