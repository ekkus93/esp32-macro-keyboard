#include "web_server_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "auth.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "macro_executor.h"
#include "macro_parser.h"
#include "web_api_core.h"
#include "web_api_response.h"
#include "web_http_status.h"
#include "web_send.h"

#define SEND_CONTENT_TYPE_MAX_BYTES 64U

/* The estimated duration of the send macro_executor_get_status() is
 * currently reporting, or 0 if none has ever been accepted. Populated on
 * every successful POST /api/v1/send; macro_execution_status_t does not
 * itself carry this value (see web_send.h's module comment). Like
 * `setup_session` in web_server_common.c, this is small httpd-adapter-level
 * state, not part of the host-testable web_send.c core. */
static uint32_t last_send_estimated_duration_ms;

static app_error_code_t executor_submit_adapter(void *context, macro_execution_request_t *request) {
    (void)context;
    return macro_executor_submit(request);
}

static app_error_code_t confirmation_policy_adapter(void *context, bool *out_required) {
    (void)context;
    if (out_required == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_v2_device_settings_t settings = {0};
    const app_error_code_t result = device_settings_read(&settings);
    if (result == APP_ERROR_NONE) {
        *out_required = settings.require_serial_confirmation;
    }
    memset(&settings, 0, sizeof(settings));
    return result;
}

static macro_execution_status_t executor_status_adapter(void *context) {
    (void)context;
    return macro_executor_get_status();
}

static app_error_code_t executor_cancel_adapter(void *context) {
    (void)context;
    return macro_executor_cancel();
}

static web_send_ops_t send_ops(void) {
    return (web_send_ops_t){
        .context = NULL,
        .submit = executor_submit_adapter,
        .get_require_confirmation = confirmation_policy_adapter,
        .get_status = executor_status_adapter,
        .cancel = executor_cancel_adapter,
    };
}

static esp_err_t send_json_response(httpd_req_t *request, char *json, const char *status) {
    if (json == NULL) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "send response encoding failed");
    }
    const esp_err_t result = send_json(request, json, status);
    free(json);
    return result;
}

static esp_err_t send_parser_error(httpd_req_t *request, const macro_parse_error_t *parse_error) {
    web_api_response_t response = {0};
    const app_error_code_t result =
        web_api_response_error(&response, &(web_api_error_spec_t){
                                              .status = WEB_HTTP_STATUS_UNPROCESSABLE_ENTITY,
                                              .code = APP_ERROR_MACRO_SYNTAX,
                                              .message = parse_error->message,
                                              .field = "source",
                                              .has_parser_location = true,
                                              .byte_offset = parse_error->byte_offset,
                                              .line = parse_error->line,
                                              .column = parse_error->column,
                                          });
    if (result != APP_ERROR_NONE) {
        web_api_response_free(&response);
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "send response encoding failed");
    }
    const esp_err_t send_result = send_json(request, response.body, "422 Unprocessable Entity");
    web_api_response_free(&response);
    return send_result;
}

static esp_err_t send_create_error(httpd_req_t *request, web_send_create_outcome_t outcome) {
    switch (outcome.result) {
    case WEB_SEND_CREATE_INVALID_BODY:
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "invalid send request");
    case WEB_SEND_CREATE_PARSE_ERROR:
        return send_parser_error(request, &outcome.parse_error);
    case WEB_SEND_CREATE_BUSY:
        return send_error(request, "409 Conflict", APP_ERROR_CONFLICT, "a send is already active");
    case WEB_SEND_CREATE_BACKEND_UNAVAILABLE:
        return send_error(request, "503 Service Unavailable", outcome.detail,
                          "send backend unavailable");
    case WEB_SEND_CREATE_OK:
    case WEB_SEND_CREATE_INTERNAL:
    default:
        return send_error(request, "500 Internal Server Error",
                          outcome.detail == APP_ERROR_NONE ? APP_ERROR_INTERNAL : outcome.detail,
                          "send could not be accepted");
    }
}

esp_err_t send_create_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    char session_token[AUTH_TOKEN_HEX_BYTES] = {0};
    const app_error_code_t auth_result = authorize_mutation(request, session_token);
    memset(session_token, 0, sizeof(session_token));
    if (auth_result != APP_ERROR_NONE) {
        return send_error(request, "401 Unauthorized", auth_result, "authentication required");
    }

    char content_type[SEND_CONTENT_TYPE_MAX_BYTES] = {0};
    if (web_server_get_header(request, "Content-Type", content_type, sizeof(content_type)) !=
            APP_ERROR_NONE ||
        !web_api_content_type_is_json(content_type)) {
        return send_error(request, "415 Unsupported Media Type", APP_ERROR_INVALID_ARGUMENT,
                          "application/json content type required");
    }

    const size_t content_length = request->content_len;
    if (content_length == 0U) {
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "send body is required");
    }
    if (content_length > (size_t)APP_V2_JSON_BODY_MAX_BYTES) {
        return send_error(request, "413 Payload Too Large", APP_ERROR_INVALID_ARGUMENT,
                          "send body exceeds route limit");
    }

    /* Heap-allocated so an 8 KiB body does not become an 8 KiB httpd task
     * stack frame -- see web_server_setup.c's identical rationale
     * (scripts/check-stack-usage.sh fails any unallowlisted frame over 4096
     * bytes). */
    const size_t body_capacity = (size_t)APP_V2_JSON_BODY_MAX_BYTES + 1U;
    char *body = calloc(body_capacity, 1U);
    if (body == NULL) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "send request allocation failed");
    }
    const app_error_code_t body_result =
        read_bounded_body(request, body, body_capacity, (size_t)APP_V2_JSON_BODY_MAX_BYTES);
    if (body_result != APP_ERROR_NONE) {
        memset(body, 0, body_capacity);
        free(body);
        return send_error(request, "400 Bad Request", body_result, "invalid request body");
    }

    const web_send_ops_t operations = send_ops();
    const web_send_create_outcome_t outcome =
        web_send_create_handle(body, body_capacity, &operations);
    free(body);
    if (outcome.result != WEB_SEND_CREATE_OK) {
        return send_create_error(request, outcome);
    }
    last_send_estimated_duration_ms = outcome.accepted.estimated_duration_ms;
    char *json = NULL;
    if (web_send_accepted_json(&outcome.accepted, &json) != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "send response encoding failed");
    }
    return send_json_response(request, json, "202 Accepted");
}

esp_err_t send_get_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    char session_token[AUTH_TOKEN_HEX_BYTES] = {0};
    const app_error_code_t auth_result = authorize_mutation(request, session_token);
    memset(session_token, 0, sizeof(session_token));
    if (auth_result != APP_ERROR_NONE) {
        return send_error(request, "401 Unauthorized", auth_result, "authentication required");
    }
    const web_send_ops_t operations = send_ops();
    const web_send_get_outcome_t outcome =
        web_send_get_handle(&operations, last_send_estimated_duration_ms);
    switch (outcome.result) {
    case WEB_SEND_GET_NEVER_SENT:
        return send_error(request, "404 Not Found", APP_ERROR_NOT_FOUND, "no send has been sent");
    case WEB_SEND_GET_INTERNAL:
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "send status unavailable");
    case WEB_SEND_GET_OK:
    default:
        break;
    }
    char *json = NULL;
    if (web_send_status_json(&outcome.status, &json) != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "send response encoding failed");
    }
    return send_json_response(request, json, "200 OK");
}

esp_err_t send_cancel_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    char session_token[AUTH_TOKEN_HEX_BYTES] = {0};
    const app_error_code_t auth_result = authorize_mutation(request, session_token);
    memset(session_token, 0, sizeof(session_token));
    if (auth_result != APP_ERROR_NONE) {
        return send_error(request, "401 Unauthorized", auth_result, "authentication required");
    }
    const web_send_ops_t operations = send_ops();
    const web_send_cancel_result_t result = web_send_cancel_handle(&operations);
    switch (result) {
    case WEB_SEND_CANCEL_NEVER_SENT:
        return send_error(request, "404 Not Found", APP_ERROR_NOT_FOUND, "no send has been sent");
    case WEB_SEND_CANCEL_INTERNAL:
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "cancellation could not be recorded");
    case WEB_SEND_CANCEL_ACCEPTED:
    default:
        return send_json(request, "{\"accepted\":true}", "202 Accepted");
    }
}
