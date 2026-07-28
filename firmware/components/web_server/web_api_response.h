#ifndef WEB_API_RESPONSE_H
#define WEB_API_RESPONSE_H

#include <stddef.h>

#include "app_error.h"

#define WEB_API_RESPONSE_MAX_BYTES (512U * 1024U)

typedef struct {
    unsigned int status;
    char *body;
    size_t body_length;
} web_api_response_t;

typedef struct {
    unsigned int status;
    app_error_code_t code;
    const char *message;
    const char *details_json;
} web_api_error_spec_t;

app_error_code_t web_api_response_success(web_api_response_t *response, unsigned int status,
                                          const char *data_json);
app_error_code_t web_api_response_error(web_api_response_t *response,
                                        const web_api_error_spec_t *error);
void web_api_response_free(web_api_response_t *response);

#endif
