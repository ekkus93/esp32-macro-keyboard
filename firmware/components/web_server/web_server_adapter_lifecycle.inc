void web_adapter_lifecycle_reset(web_adapter_lifecycle_t *lifecycle)
{
    if (lifecycle != NULL) {
        memset(lifecycle, 0, sizeof(*lifecycle));
    }
}

app_error_code_t web_adapter_lifecycle_start(web_adapter_lifecycle_t *lifecycle,
                                              const web_adapter_lifecycle_ops_t *ops,
                                              size_t route_count)
{
    if (lifecycle == NULL || !valid_lifecycle_ops(ops) || route_count == 0U ||
        lifecycle->handle != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    void *handle = NULL;
    if (ops->start(ops->context, &handle) != 0 || handle == NULL) {
        return APP_ERROR_INTERNAL;
    }
    lifecycle->handle = handle;
    lifecycle->cleanup_error = APP_ERROR_NONE;
    for (size_t index = 0U; index < route_count; ++index) {
        if (ops->register_route(ops->context, handle, index) != 0) {
            if (ops->stop(ops->context, handle) == 0) {
                web_adapter_lifecycle_reset(lifecycle);
                return APP_ERROR_INTERNAL;
            }
            lifecycle->registered_routes = index;
            lifecycle->cleanup_error = APP_ERROR_IO;
            return APP_ERROR_IO;
        }
        lifecycle->registered_routes = index + 1U;
    }
    return APP_ERROR_NONE;
}

app_error_code_t web_adapter_lifecycle_stop(web_adapter_lifecycle_t *lifecycle,
                                             const web_adapter_lifecycle_ops_t *ops)
{
    if (lifecycle == NULL || !valid_lifecycle_ops(ops)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (lifecycle->handle == NULL) {
        return APP_ERROR_NONE;
    }
    if (ops->stop(ops->context, lifecycle->handle) != 0) {
        lifecycle->cleanup_error = APP_ERROR_IO;
        return APP_ERROR_IO;
    }
    web_adapter_lifecycle_reset(lifecycle);
    return APP_ERROR_NONE;
}
