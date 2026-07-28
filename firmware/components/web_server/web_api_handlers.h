#ifndef WEB_API_HANDLERS_H
#define WEB_API_HANDLERS_H

#include <stddef.h>

#include "app_error.h"
#include "auth.h"
#include "web_api_core.h"
#include "web_api_response.h"

typedef struct {
    web_api_method_t method;
    web_api_path_t path;
    const char *body;
    size_t body_length;
    char session_token[AUTH_TOKEN_HEX_BYTES];
} web_api_call_t;

app_error_code_t web_api_handle_sets(const web_api_call_t *call, web_api_response_t *response);
app_error_code_t web_api_handle_macros(const web_api_call_t *call, web_api_response_t *response);
app_error_code_t web_api_handle_procedures(const web_api_call_t *call,
                                           web_api_response_t *response);
app_error_code_t web_api_handle_execution(const web_api_call_t *call, web_api_response_t *response);
app_error_code_t web_api_handle_administration(const web_api_call_t *call,
                                               web_api_response_t *response);
app_error_code_t web_api_dispatch(const web_api_call_t *call, web_api_response_t *response);

#endif
