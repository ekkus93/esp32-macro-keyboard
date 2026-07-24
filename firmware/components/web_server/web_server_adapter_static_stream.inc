app_error_code_t web_adapter_open_static_file(const char *uri,
                                               bool accept_gzip,
                                               const char *mount,
                                               web_adapter_open_fn open_file,
                                               void *context,
                                               web_adapter_static_file_t *out_file)
{
    if (out_file != NULL) {
        memset(out_file, 0, sizeof(*out_file));
    }
    if (uri == NULL || mount == NULL || open_file == NULL || out_file == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char normalized[APP_PATH_MAX_BYTES];
    if (!web_static_uri_normalize(uri, normalized, sizeof(normalized))) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const char *relative = strcmp(normalized, "/") == 0 ? "/index.html" : normalized;
    const int base_length = snprintf(out_file->path,
                                     sizeof(out_file->path),
                                     "%s%s",
                                     mount,
                                     relative);
    if (base_length < 0 || (size_t)base_length >= sizeof(out_file->path)) {
        out_file->path[0] = '\0';
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char base_path[sizeof(out_file->path)];
    memcpy(base_path, out_file->path, (size_t)base_length + 1U);
    if (accept_gzip) {
        const int gzip_length = snprintf(out_file->path,
                                         sizeof(out_file->path),
                                         "%s.gz",
                                         base_path);
        if (gzip_length > 0 && (size_t)gzip_length < sizeof(out_file->path)) {
            out_file->handle = open_file(context, out_file->path);
            out_file->compressed = out_file->handle != NULL;
        }
    }
    if (out_file->handle == NULL) {
        memcpy(out_file->path, base_path, (size_t)base_length + 1U);
        out_file->handle = open_file(context, out_file->path);
        out_file->compressed = false;
    }
    if (out_file->handle == NULL) {
        out_file->path[0] = '\0';
        return APP_ERROR_NOT_FOUND;
    }
    out_file->content_type = web_content_type(base_path);
    out_file->cache_control = strcmp(relative, "/index.html") == 0
                                  ? "no-cache"
                                  : "public, max-age=31536000, immutable";
    return APP_ERROR_NONE;
}

app_error_code_t web_adapter_stream_file(void *handle,
                                         const web_adapter_stream_ops_t *ops)
{
    if (handle == NULL || ops == NULL || ops->read == NULL ||
        ops->send_chunk == NULL || ops->close == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    uint8_t buffer[WEB_ADAPTER_STATIC_CHUNK_BYTES];
    app_error_code_t result = APP_ERROR_NONE;
    bool eof = false;
    while (!eof) {
        size_t count = 0U;
        if (ops->read(ops->context, handle, buffer, sizeof(buffer), &count, &eof) != 0 ||
            count > sizeof(buffer) || (count == 0U && !eof)) {
            result = APP_ERROR_IO;
            break;
        }
        if (count > 0U && ops->send_chunk(ops->context, buffer, count) != 0) {
            result = APP_ERROR_IO;
            break;
        }
    }
    if (ops->close(ops->context, handle) != 0 && result == APP_ERROR_NONE) {
        result = APP_ERROR_IO;
    }
    if (result == APP_ERROR_NONE && ops->send_chunk(ops->context, NULL, 0U) != 0) {
        result = APP_ERROR_IO;
    }
    return result;
}
