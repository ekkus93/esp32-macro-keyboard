#include "fake_http_backend.h"

#include <stdlib.h>
#include <string.h>

void fake_http_backend_reset(fake_http_backend_t *http)
{
    if (http == NULL) {
        abort();
    }
    memset(http, 0, sizeof(*http));
    fake_call_log_reset(&http->calls);
}

void fake_http_backend_set_body(fake_http_backend_t *http,
                                const char *body,
                                size_t receive_chunk)
{
    if (http == NULL || body == NULL || strlen(body) >= sizeof(http->body)) {
        abort();
    }
    memcpy(http->body, body, strlen(body) + 1U);
    http->body_length = strlen(body);
    http->body_cursor = 0U;
    http->receive_chunk = receive_chunk;
}

void fake_http_backend_add_header(fake_http_backend_t *http,
                                  const char *name,
                                  const char *value)
{
    if (http == NULL || name == NULL || value == NULL ||
        http->header_count >= FAKE_HTTP_HEADER_CAPACITY ||
        strlen(name) >= FAKE_HTTP_HEADER_NAME_MAX ||
        strlen(value) >= FAKE_HTTP_HEADER_VALUE_MAX) {
        abort();
    }
    fake_http_header_t *header = &http->headers[http->header_count++];
    memcpy(header->name, name, strlen(name) + 1U);
    memcpy(header->value, value, strlen(value) + 1U);
}

const char *fake_http_backend_get_header(fake_http_backend_t *http, const char *name)
{
    if (http == NULL || name == NULL) {
        abort();
    }
    if (fake_call_log_record(&http->calls, "http_get_header", strlen(name), 0U)) {
        return NULL;
    }
    for (size_t index = 0U; index < http->header_count; ++index) {
        if (strcmp(http->headers[index].name, name) == 0) {
            return http->headers[index].value;
        }
    }
    return NULL;
}

int fake_http_backend_receive(fake_http_backend_t *http, char *output, size_t capacity)
{
    if (http == NULL || output == NULL || capacity == 0U) {
        abort();
    }
    if (fake_call_log_record(&http->calls, "http_receive", capacity, http->body_cursor)) {
        return -1;
    }
    if (http->body_cursor >= http->body_length) {
        return 0;
    }
    size_t count = http->body_length - http->body_cursor;
    if (http->receive_chunk != 0U && count > http->receive_chunk) {
        count = http->receive_chunk;
    }
    if (count > capacity) {
        count = capacity;
    }
    memcpy(output, http->body + http->body_cursor, count);
    http->body_cursor += count;
    return (int)count;
}

int fake_http_backend_send(fake_http_backend_t *http,
                           int status,
                           const char *data,
                           size_t length)
{
    if (http == NULL || (data == NULL && length != 0U) ||
        length > sizeof(http->response) - http->response_length) {
        abort();
    }
    if (fake_call_log_record(&http->calls,
                             "http_send",
                             (uint64_t)(uint32_t)status,
                             length)) {
        return -1;
    }
    if (length > 0U) {
        memcpy(http->response + http->response_length, data, length);
        http->response_length += length;
    }
    http->status = status;
    return 0;
}
