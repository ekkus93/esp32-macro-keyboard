static bool request_accepts_gzip(httpd_req_t *request)
{
    char encoding[HTTP_HEADER_MAX_BYTES];
    if (request == NULL) {
        return false;
    }
    const size_t length = httpd_req_get_hdr_value_len(request, "Accept-Encoding");
    return length > 0U && length < sizeof(encoding) &&
           httpd_req_get_hdr_value_str(request,
                                       "Accept-Encoding",
                                       encoding,
                                       sizeof(encoding)) == ESP_OK &&
           web_accept_encoding_gzip(encoding);
}

static void *open_static_adapter(void *context, const char *path)
{
    (void)context;
    return fopen(path, "rb");
}

static int read_static_adapter(void *context,
                               void *handle,
                               uint8_t *buffer,
                               size_t capacity,
                               size_t *out_count,
                               bool *out_eof)
{
    (void)context;
    FILE *file = handle;
    const size_t count = fread(buffer, 1U, capacity, file);
    *out_count = count;
    *out_eof = count < capacity;
    return *out_eof && ferror(file) != 0 ? -1 : 0;
}

static int send_static_chunk_adapter(void *context,
                                     const uint8_t *buffer,
                                     size_t length)
{
    httpd_req_t *request = context;
    return httpd_resp_send_chunk(request,
                                 (const char *)buffer,
                                 (ssize_t)length) == ESP_OK
               ? 0
               : -1;
}

static int close_static_adapter(void *context, void *handle)
{
    (void)context;
    return fclose(handle);
}

static esp_err_t static_handler(httpd_req_t *request)
{
    if (request == NULL) {
        return ESP_FAIL;
    }
    web_adapter_static_file_t file = {0};
    const app_error_code_t open_result = web_adapter_open_static_file(
        request->uri,
        request_accepts_gzip(request),
        STORAGE_WEB_MOUNT,
        open_static_adapter,
        NULL,
        &file);
    if (open_result == APP_ERROR_INVALID_ARGUMENT) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "invalid path");
    }
    if (open_result == APP_ERROR_NOT_FOUND) {
        return httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "not found");
    }
    if (open_result != APP_ERROR_NONE) {
        return httpd_resp_send_err(request,
                                   HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "static file unavailable");
    }
    if (httpd_resp_set_type(request, file.content_type) != ESP_OK ||
        httpd_resp_set_hdr(request, "Cache-Control", file.cache_control) != ESP_OK ||
        (file.compressed &&
         httpd_resp_set_hdr(request, "Content-Encoding", "gzip") != ESP_OK)) {
        if (fclose(file.handle) != 0) {
            return ESP_FAIL;
        }
        return ESP_FAIL;
    }
    const web_adapter_stream_ops_t stream_ops = {
        .context = request,
        .read = read_static_adapter,
        .send_chunk = send_static_chunk_adapter,
        .close = close_static_adapter,
    };
    return web_adapter_stream_file(file.handle, &stream_ops) == APP_ERROR_NONE ? ESP_OK
                                                                               : ESP_FAIL;
}
