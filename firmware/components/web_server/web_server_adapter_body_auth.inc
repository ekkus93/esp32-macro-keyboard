app_error_code_t web_adapter_read_bounded_body(size_t content_length,
                                                char *buffer,
                                                size_t buffer_size,
                                                size_t maximum_length,
                                                web_adapter_receive_fn receive,
                                                void *context)
{
    if (buffer != NULL && buffer_size > 0U) {
        buffer[0] = '\0';
    }
    if (buffer == NULL || buffer_size == 0U || receive == NULL ||
        content_length > maximum_length || content_length >= buffer_size) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    size_t received = 0U;
    size_t consecutive_timeouts = 0U;
    while (received < content_length) {
        const int count = receive(context, buffer + received, content_length - received);
        if (count == WEB_ADAPTER_RECEIVE_TIMEOUT) {
            ++consecutive_timeouts;
            if (consecutive_timeouts > WEB_ADAPTER_TIMEOUT_RETRIES_MAX) {
                buffer[0] = '\0';
                return APP_ERROR_TIMEOUT;
            }
            continue;
        }
        consecutive_timeouts = 0U;
        if (count <= 0 || (size_t)count > content_length - received) {
            buffer[0] = '\0';
            return APP_ERROR_IO;
        }
        received += (size_t)count;
    }
    buffer[received] = '\0';
    return APP_ERROR_NONE;
}

app_error_code_t web_adapter_authorize_mutation(web_adapter_get_header_fn get_header,
                                                 web_adapter_validate_session_fn validate,
                                                 void *context,
                                                 char *out_session_token,
                                                 size_t token_size)
{
    if (out_session_token != NULL && token_size > 0U) {
        out_session_token[0] = '\0';
    }
    if (get_header == NULL || validate == NULL || out_session_token == NULL ||
        token_size < AUTH_TOKEN_HEX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char host[256U];
    char origin[256U];
    char cookie[256U];
    char csrf[AUTH_TOKEN_HEX_BYTES];
    if (get_header(context, "Host", host, sizeof(host)) != APP_ERROR_NONE ||
        get_header(context, "Origin", origin, sizeof(origin)) != APP_ERROR_NONE ||
        !web_origin_matches_host(origin, host) ||
        get_header(context, "Cookie", cookie, sizeof(cookie)) != APP_ERROR_NONE ||
        get_header(context, "X-CSRF-Token", csrf, sizeof(csrf)) != APP_ERROR_NONE) {
        return APP_ERROR_AUTH_REQUIRED;
    }
    const app_error_code_t cookie_result =
        web_cookie_extract_session(cookie, out_session_token, token_size);
    if (cookie_result != APP_ERROR_NONE) {
        out_session_token[0] = '\0';
        return cookie_result;
    }
    const app_error_code_t validation = validate(context, out_session_token, csrf);
    if (validation != APP_ERROR_NONE) {
        out_session_token[0] = '\0';
    }
    return validation;
}
