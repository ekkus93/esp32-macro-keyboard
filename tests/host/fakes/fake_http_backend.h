#ifndef FAKE_HTTP_BACKEND_H
#define FAKE_HTTP_BACKEND_H

#include <stddef.h>

#include "fake_call_log.h"

#define FAKE_HTTP_BODY_CAPACITY 2048U
#define FAKE_HTTP_HEADER_CAPACITY 16U
#define FAKE_HTTP_HEADER_NAME_MAX 64U
#define FAKE_HTTP_HEADER_VALUE_MAX 256U

typedef struct {
    char name[FAKE_HTTP_HEADER_NAME_MAX];
    char value[FAKE_HTTP_HEADER_VALUE_MAX];
} fake_http_header_t;

typedef struct {
    char body[FAKE_HTTP_BODY_CAPACITY];
    size_t body_length;
    size_t body_cursor;
    size_t receive_chunk;
    fake_http_header_t headers[FAKE_HTTP_HEADER_CAPACITY];
    size_t header_count;
    char response[FAKE_HTTP_BODY_CAPACITY];
    size_t response_length;
    int status;
    fake_call_log_t calls;
} fake_http_backend_t;

void fake_http_backend_reset(fake_http_backend_t *http);
void fake_http_backend_set_body(fake_http_backend_t *http,
                                const char *body,
                                size_t receive_chunk);
void fake_http_backend_add_header(fake_http_backend_t *http,
                                  const char *name,
                                  const char *value);
const char *fake_http_backend_get_header(fake_http_backend_t *http, const char *name);
int fake_http_backend_receive(fake_http_backend_t *http, char *output, size_t capacity);
int fake_http_backend_send(fake_http_backend_t *http,
                           int status,
                           const char *data,
                           size_t length);

#endif
