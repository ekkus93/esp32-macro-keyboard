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
    if (request == NULL ||
        !web_static_uri_normalize(request->uri, normalized, sizeof(normalized))) {
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
    if (request_accepts_gzip(request)) {
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

    if (httpd_resp_set_type(request, web_content_type(path)) != ESP_OK ||
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

