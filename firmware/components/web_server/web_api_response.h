#ifndef WEB_API_RESPONSE_H
#define WEB_API_RESPONSE_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"

#define WEB_API_RESPONSE_MAX_BYTES (512U * 1024U)

typedef void (*web_api_response_body_free_t)(void *body);

typedef struct {
    unsigned int status;
    char *body;
    size_t body_length;
    web_api_response_body_free_t body_free;
} web_api_response_t;

typedef struct {
    unsigned int status;
    app_error_code_t code;
    const char *message;
    /* Optional: the single request field that caused the error (SPEC_V2 13.2).
     * NULL when no single field is responsible. */
    const char *field;
    /* When true, byte_offset/line/column are populated and the emitted "code"
     * is the literal "macro_parse_error" (SPEC_V2 13.2's parser-error shape)
     * regardless of `code`. */
    bool has_parser_location;
    size_t byte_offset;
    size_t line;
    size_t column;
} web_api_error_spec_t;

app_error_code_t web_api_response_success(web_api_response_t *response, unsigned int status,
                                          const char *data_json);
/* A body-less success response (SPEC_V2 13.14's 204, e.g. change-password).
 * response->body/body_length/body_free stay NULL/0/NULL; response->status is
 * the only populated field, which callers must treat as the readiness
 * signal instead of a non-NULL body (see web_server_api.c's
 * dispatch_api_call()). */
app_error_code_t web_api_response_no_content(web_api_response_t *response, unsigned int status);
app_error_code_t web_api_response_take_json(web_api_response_t *response, unsigned int status,
                                            char *body, size_t body_length);
app_error_code_t web_api_response_error(web_api_response_t *response,
                                        const web_api_error_spec_t *error);
void web_api_response_free(web_api_response_t *response);

#endif
