#ifndef WEB_REQUEST_POLICY_H
#define WEB_REQUEST_POLICY_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "auth.h"
#include "web_api_core.h"
#include "web_server_adapter.h"

typedef enum {
    WEB_REQUEST_POLICY_FAILURE_NONE = 0,
    WEB_REQUEST_POLICY_FAILURE_CONTENT_TYPE,
    WEB_REQUEST_POLICY_FAILURE_BODY_LIMIT,
    WEB_REQUEST_POLICY_FAILURE_COOKIE,
    WEB_REQUEST_POLICY_FAILURE_SESSION,
    WEB_REQUEST_POLICY_FAILURE_REQUEST_ID,
    WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION,
} web_request_policy_failure_t;

typedef app_error_code_t (*web_request_generate_id_fn)(void *context, char *output,
                                                       size_t output_size);
typedef app_error_code_t (*web_request_confirm_fn)(void *context);

typedef struct {
    void *context;
    web_adapter_get_header_fn get_header;
    web_adapter_validate_session_fn validate_session;
    web_request_generate_id_fn generate_request_id;
    web_request_confirm_fn confirm;
} web_request_policy_ops_t;

typedef struct {
    web_api_route_t route;
    web_api_method_t method;
    size_t content_length;
    size_t body_limit;
} web_request_policy_input_t;

typedef struct {
    char session_token[AUTH_TOKEN_HEX_BYTES];
    char request_id[WEB_API_REQUEST_ID_MAX_BYTES + 1U];
} web_request_policy_result_t;

app_error_code_t web_request_policy_evaluate(const web_request_policy_input_t *input,
                                             const web_request_policy_ops_t *operations,
                                             web_request_policy_result_t *out_result,
                                             web_request_policy_failure_t *out_failure);
unsigned int web_request_policy_http_status(web_request_policy_failure_t failure,
                                            app_error_code_t error);

#endif
