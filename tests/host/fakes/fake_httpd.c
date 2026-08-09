#include "fake_httpd.h"

#include <stdlib.h>
#include <string.h>

static fake_httpd_request_t *of(httpd_req_t *request) {
    if (request == NULL || request->aux == NULL) {
        abort();
    }
    return request->aux;
}

void fake_httpd_reset(fake_httpd_request_t *fake) {
    if (fake == NULL) {
        abort();
    }
    memset(fake, 0, sizeof(*fake));
}

void fake_httpd_bind(httpd_req_t *request, fake_httpd_request_t *fake, const char *uri,
                     size_t content_len) {
    if (request == NULL || fake == NULL || uri == NULL || strlen(uri) >= sizeof(request->uri)) {
        abort();
    }
    memset(request, 0, sizeof(*request));
    request->aux = fake;
    request->content_len = content_len;
    memcpy(request->uri, uri, strlen(uri) + 1U);
}

void fake_httpd_set_body(fake_httpd_request_t *fake, const void *body, size_t length,
                         size_t receive_chunk) {
    if (fake == NULL || (body == NULL && length != 0U) || length > sizeof(fake->body)) {
        abort();
    }
    if (length > 0U) {
        memcpy(fake->body, body, length);
    }
    fake->body_length = length;
    fake->body_cursor = 0U;
    fake->receive_chunk = receive_chunk;
}

void fake_httpd_add_request_header(fake_httpd_request_t *fake, const char *name,
                                   const char *value) {
    if (fake == NULL || name == NULL || value == NULL ||
        fake->request_header_count >= FAKE_HTTPD_HEADER_CAPACITY ||
        strlen(name) >= FAKE_HTTPD_HEADER_NAME_MAX ||
        strlen(value) >= FAKE_HTTPD_HEADER_VALUE_MAX) {
        abort();
    }
    fake_httpd_header_t *header = &fake->request_headers[fake->request_header_count++];
    memcpy(header->name, name, strlen(name) + 1U);
    memcpy(header->value, value, strlen(value) + 1U);
}

const char *fake_httpd_response_header(const fake_httpd_request_t *fake, const char *name) {
    if (fake == NULL || name == NULL) {
        abort();
    }
    for (size_t index = 0U; index < fake->response_header_count; ++index) {
        if (strcmp(fake->response_headers[index].name, name) == 0) {
            return fake->response_headers[index].value;
        }
    }
    return NULL;
}

int httpd_req_recv(httpd_req_t *request, char *buffer, size_t buffer_length) {
    fake_httpd_request_t *fake = of(request);
    if (buffer == NULL || buffer_length == 0U) {
        abort();
    }
    if (fake->forced_timeout_count > 0U) {
        --fake->forced_timeout_count;
        return HTTPD_SOCK_ERR_TIMEOUT;
    }
    if (fake->body_cursor >= fake->body_length) {
        return 0;
    }
    size_t count = fake->body_length - fake->body_cursor;
    if (fake->receive_chunk != 0U && count > fake->receive_chunk) {
        count = fake->receive_chunk;
    }
    if (count > buffer_length) {
        count = buffer_length;
    }
    memcpy(buffer, fake->body + fake->body_cursor, count);
    fake->body_cursor += count;
    return (int)count;
}

size_t httpd_req_get_hdr_value_len(httpd_req_t *request, const char *field) {
    fake_httpd_request_t *fake = of(request);
    if (field == NULL) {
        abort();
    }
    for (size_t index = 0U; index < fake->request_header_count; ++index) {
        if (strcmp(fake->request_headers[index].name, field) == 0) {
            return strlen(fake->request_headers[index].value);
        }
    }
    return 0U;
}

esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *request, const char *field, char *value,
                                      size_t value_size) {
    fake_httpd_request_t *fake = of(request);
    if (field == NULL || value == NULL || value_size == 0U) {
        abort();
    }
    for (size_t index = 0U; index < fake->request_header_count; ++index) {
        if (strcmp(fake->request_headers[index].name, field) == 0) {
            const size_t length = strlen(fake->request_headers[index].value);
            if (length >= value_size) {
                return ESP_FAIL;
            }
            memcpy(value, fake->request_headers[index].value, length + 1U);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t httpd_resp_set_hdr(httpd_req_t *request, const char *field, const char *value) {
    fake_httpd_request_t *fake = of(request);
    if (field == NULL || value == NULL) {
        abort();
    }
    for (size_t index = 0U; index < fake->response_header_count; ++index) {
        if (strcmp(fake->response_headers[index].name, field) == 0) {
            if (strlen(value) >= FAKE_HTTPD_HEADER_VALUE_MAX) {
                abort();
            }
            memcpy(fake->response_headers[index].value, value, strlen(value) + 1U);
            return ESP_OK;
        }
    }
    if (fake->response_header_count >= FAKE_HTTPD_HEADER_CAPACITY ||
        strlen(field) >= FAKE_HTTPD_HEADER_NAME_MAX ||
        strlen(value) >= FAKE_HTTPD_HEADER_VALUE_MAX) {
        abort();
    }
    fake_httpd_header_t *header = &fake->response_headers[fake->response_header_count++];
    memcpy(header->name, field, strlen(field) + 1U);
    memcpy(header->value, value, strlen(value) + 1U);
    return ESP_OK;
}

esp_err_t httpd_resp_set_status(httpd_req_t *request, const char *status) {
    fake_httpd_request_t *fake = of(request);
    if (status == NULL || strlen(status) >= sizeof(fake->response_status)) {
        abort();
    }
    memcpy(fake->response_status, status, strlen(status) + 1U);
    return ESP_OK;
}

esp_err_t httpd_resp_set_type(httpd_req_t *request, const char *type) {
    fake_httpd_request_t *fake = of(request);
    if (type == NULL || strlen(type) >= sizeof(fake->response_type)) {
        abort();
    }
    memcpy(fake->response_type, type, strlen(type) + 1U);
    return ESP_OK;
}

esp_err_t httpd_resp_send(httpd_req_t *request, const char *buffer, ssize_t buffer_length) {
    fake_httpd_request_t *fake = of(request);
    const size_t length = buffer_length == HTTPD_RESP_USE_STRLEN
                              ? (buffer == NULL ? 0U : strlen(buffer))
                              : (size_t)buffer_length;
    if (length > 0U && buffer == NULL) {
        abort();
    }
    if (length > sizeof(fake->response_body) - fake->response_body_length) {
        abort();
    }
    if (length > 0U) {
        memcpy(fake->response_body + fake->response_body_length, buffer, length);
        fake->response_body_length += length;
    }
    fake->responded = true;
    return ESP_OK;
}

esp_err_t httpd_resp_send_chunk(httpd_req_t *request, const char *buffer, ssize_t buffer_length) {
    fake_httpd_request_t *fake = of(request);
    if (buffer_length == 0) {
        fake->chunk_terminated = true;
        fake->responded = true;
        return ESP_OK;
    }
    const size_t length = (size_t)buffer_length;
    if (buffer == NULL || length > sizeof(fake->response_body) - fake->response_body_length) {
        abort();
    }
    memcpy(fake->response_body + fake->response_body_length, buffer, length);
    fake->response_body_length += length;
    fake->responded = true;
    return ESP_OK;
}

esp_err_t httpd_resp_send_err(httpd_req_t *request, httpd_err_code_t error, const char *message) {
    fake_httpd_request_t *fake = of(request);
    (void)message;
    fake->send_err_called = true;
    fake->last_send_err_code = error;
    fake->responded = true;
    return ESP_OK;
}
