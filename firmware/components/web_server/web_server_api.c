#include "web_server_internal.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "auth.h"
#include "device_controls.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "macro_limits.h"
#include "storage_object_json.h"
#include "web_api_core.h"
#include "web_api_handlers.h"
#include "web_api_response.h"
#include "web_http_status.h"

_Static_assert(WEB_API_RESPONSE_MAX_BYTES <= INT_MAX,
               "API response length must fit httpd_resp_send");
#include "web_request_policy.h"
#include "web_server.h"

#define WEB_API_SMALL_BODY_MAX_BYTES 4096U
#define WEB_API_WRAPPER_OVERHEAD_BYTES 256U

typedef struct {
    httpd_req_t *request;
    web_api_route_t route;
} web_http_request_context_t;

static app_error_code_t policy_get_header(void *context, const char *name, char *output,
                                          size_t output_size) {
    web_http_request_context_t *request_context = context;
    return request_context == NULL
               ? APP_ERROR_INVALID_ARGUMENT
               : web_server_get_header(request_context->request, name, output, output_size);
}

static app_error_code_t policy_validate_session(void *context, const char *session_token,
                                                const char *csrf_token) {
    (void)context;
    return csrf_token == NULL ? auth_session_validate_read_only(session_token)
                              : auth_session_validate(session_token, csrf_token);
}

static app_error_code_t policy_generate_request_id(void *context, char *output,
                                                   size_t output_size) {
    (void)context;
    if (output == NULL || output_size < APP_UUID_BUFFER_LENGTH) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_uuid_t request_id = {0};
    const app_error_code_t result = app_uuid_generate(&request_id);
    if (result == APP_ERROR_NONE) {
        memcpy(output, request_id.value, sizeof(request_id.value));
    }
    return result;
}

static app_error_code_t policy_confirm(void *context) {
    web_http_request_context_t *request_context = context;
    if (request_context == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (!web_api_physical_confirmation_required(
            request_context->route, server_configuration.require_physical_confirmation)) {
        return APP_ERROR_NONE;
    }
    return device_controls_wait_for_confirmation(APP_PHYSICAL_CONFIRM_TIMEOUT_MS);
}

static app_error_code_t method_from_request(const httpd_req_t *request,
                                            web_api_method_t *out_method) {
    if (request == NULL || out_method == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (request->method) {
    case HTTP_GET:
        *out_method = WEB_API_METHOD_GET;
        return APP_ERROR_NONE;
    case HTTP_POST:
        *out_method = WEB_API_METHOD_POST;
        return APP_ERROR_NONE;
    case HTTP_PUT:
        *out_method = WEB_API_METHOD_PUT;
        return APP_ERROR_NONE;
    case HTTP_DELETE:
        *out_method = WEB_API_METHOD_DELETE;
        return APP_ERROR_NONE;
    default:
        return APP_ERROR_INVALID_ARGUMENT;
    }
}

static size_t route_body_limit(web_api_route_t route) {
    switch (route) {
    case WEB_API_ROUTE_SET:
    case WEB_API_ROUTE_SETS:
        return STORAGE_SET_FILE_MAX_BYTES + WEB_API_WRAPPER_OVERHEAD_BYTES;
    case WEB_API_ROUTE_SET_MACRO:
    case WEB_API_ROUTE_GLOBAL_MACRO:
    case WEB_API_ROUTE_SET_MACRO_VALIDATE:
    case WEB_API_ROUTE_GLOBAL_MACRO_VALIDATE:
        return STORAGE_MACRO_FILE_MAX_BYTES + WEB_API_WRAPPER_OVERHEAD_BYTES;
    case WEB_API_ROUTE_SET_PROCEDURE:
    case WEB_API_ROUTE_SET_PROCEDURES:
        return STORAGE_PROCEDURE_FILE_MAX_BYTES + WEB_API_WRAPPER_OVERHEAD_BYTES;
    case WEB_API_ROUTE_PROCEDURE_PROGRESS:
        return STORAGE_PROGRESS_FILE_MAX_BYTES + WEB_API_WRAPPER_OVERHEAD_BYTES;
    case WEB_API_ROUTE_SET_IMPORT:
    case WEB_API_ROUTE_RESTORE:
        return APP_IMPORT_PACKAGE_MAX_BYTES;
    default:
        return WEB_API_SMALL_BODY_MAX_BYTES;
    }
}

static const char *status_text(unsigned int status) {
    switch (status) {
    case WEB_HTTP_STATUS_OK:
        return "200 OK";
    case WEB_HTTP_STATUS_CREATED:
        return "201 Created";
    case WEB_HTTP_STATUS_ACCEPTED:
        return "202 Accepted";
    case WEB_HTTP_STATUS_BAD_REQUEST:
        return "400 Bad Request";
    case WEB_HTTP_STATUS_UNAUTHORIZED:
        return "401 Unauthorized";
    case WEB_HTTP_STATUS_FORBIDDEN:
        return "403 Forbidden";
    case WEB_HTTP_STATUS_NOT_FOUND:
        return "404 Not Found";
    case WEB_HTTP_STATUS_METHOD_NOT_ALLOWED:
        return "405 Method Not Allowed";
    case WEB_HTTP_STATUS_CONFLICT:
        return "409 Conflict";
    case WEB_HTTP_STATUS_PAYLOAD_TOO_LARGE:
        return "413 Payload Too Large";
    case WEB_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE:
        return "415 Unsupported Media Type";
    case WEB_HTTP_STATUS_UNPROCESSABLE_ENTITY:
        return "422 Unprocessable Entity";
    case WEB_HTTP_STATUS_TOO_MANY_REQUESTS:
        return "429 Too Many Requests";
    case WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR:
        return "500 Internal Server Error";
    case WEB_HTTP_STATUS_SERVICE_UNAVAILABLE:
        return "503 Service Unavailable";
    case WEB_HTTP_STATUS_INSUFFICIENT_STORAGE:
        return "507 Insufficient Storage";
    default:
        return "500 Internal Server Error";
    }
}

static const char *policy_failure_message(web_request_policy_failure_t failure) {
    switch (failure) {
    case WEB_REQUEST_POLICY_FAILURE_CONTENT_TYPE:
        return "application/json content type required";
    case WEB_REQUEST_POLICY_FAILURE_BODY_LIMIT:
        return "request body exceeds route limit";
    case WEB_REQUEST_POLICY_FAILURE_HOST:
        return "host validation failed";
    case WEB_REQUEST_POLICY_FAILURE_ORIGIN:
        return "origin validation failed";
    case WEB_REQUEST_POLICY_FAILURE_COOKIE:
    case WEB_REQUEST_POLICY_FAILURE_SESSION:
        return "authentication required";
    case WEB_REQUEST_POLICY_FAILURE_CSRF:
        return "CSRF validation failed";
    case WEB_REQUEST_POLICY_FAILURE_REQUEST_ID:
        return "invalid request ID";
    case WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION:
        return "physical confirmation failed";
    case WEB_REQUEST_POLICY_FAILURE_NONE:
    default:
        return "request policy failed";
    }
}

static app_error_code_t set_error_response(web_api_response_t *response, unsigned int status,
                                           app_error_code_t error, const char *message) {
    return web_api_response_error(response, &(web_api_error_spec_t){
                                                .status = status,
                                                .code = error,
                                                .message = message,
                                            });
}

static esp_err_t send_api_response(httpd_req_t *request, const char *request_id,
                                   const web_api_response_t *response) {
    if (request == NULL || response == NULL || response->body == NULL ||
        response->body_length == 0U || httpd_resp_set_type(request, "application/json") != ESP_OK ||
        httpd_resp_set_status(request, status_text(response->status)) != ESP_OK ||
        httpd_resp_set_hdr(request, "Cache-Control", "no-store") != ESP_OK) {
        return ESP_FAIL;
    }
    if (request_id != NULL && request_id[0] != '\0' &&
        httpd_resp_set_hdr(request, "X-Request-ID", request_id) != ESP_OK) {
        return ESP_FAIL;
    }
    return httpd_resp_send(request, response->body, (int)response->body_length);
}

static app_error_code_t apply_request_policy(httpd_req_t *request, const web_api_call_t *call,
                                             size_t body_limit,
                                             web_request_policy_result_t *out_policy,
                                             web_request_policy_failure_t *out_failure) {
    web_http_request_context_t request_context = {
        .request = request,
        .route = call->path.route,
    };
    const web_request_policy_ops_t operations = {
        .context = &request_context,
        .get_header = policy_get_header,
        .validate_session = policy_validate_session,
        .generate_request_id = policy_generate_request_id,
        .confirm = policy_confirm,
    };
    const web_request_policy_input_t input = {
        .route = call->path.route,
        .method = call->method,
        .content_length = request->content_len,
        .body_limit = body_limit,
    };
    return web_request_policy_evaluate(&input, &operations, out_policy, out_failure);
}

static app_error_code_t read_call_body(httpd_req_t *request, size_t body_limit, char **out_body,
                                       size_t *out_length) {
    *out_body = NULL;
    *out_length = 0U;
    const size_t content_length = request->content_len;
    if (content_length == 0U) {
        return APP_ERROR_NONE;
    }
    if (content_length > body_limit || content_length == SIZE_MAX) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *body = calloc(content_length + 1U, 1U);
    if (body == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const app_error_code_t result =
        read_bounded_body(request, body, content_length + 1U, body_limit);
    if (result != APP_ERROR_NONE) {
        free(body);
        return result;
    }
    *out_body = body;
    *out_length = content_length;
    return APP_ERROR_NONE;
}

static bool restart_after_response(const web_api_call_t *call, const web_api_response_t *response) {
    return response->status == WEB_HTTP_STATUS_ACCEPTED &&
           (call->path.route == WEB_API_ROUTE_DEVICE_RESTART ||
            call->path.route == WEB_API_ROUTE_DEVICE_FACTORY_RESET);
}

static app_error_code_t prepare_api_call(httpd_req_t *request, web_api_call_t *call,
                                         web_api_response_t *response, size_t *out_body_limit,
                                         bool *out_response_ready) {
    app_error_code_t result = method_from_request(request, &call->method);
    if (result == APP_ERROR_NONE) {
        result = web_api_parse_path(request->uri, &call->path);
    }
    if (result != APP_ERROR_NONE) {
        const unsigned int status =
            result == APP_ERROR_NOT_FOUND ? WEB_HTTP_STATUS_NOT_FOUND : WEB_HTTP_STATUS_BAD_REQUEST;
        result = set_error_response(response, status, result,
                                    status == WEB_HTTP_STATUS_NOT_FOUND ? "route not found"
                                                                        : "invalid API path");
        *out_response_ready = result == APP_ERROR_NONE;
        return result;
    }
    if (!web_api_route_allows_method(call->path.route, call->method)) {
        result = set_error_response(response, WEB_HTTP_STATUS_METHOD_NOT_ALLOWED,
                                    APP_ERROR_INVALID_ARGUMENT, "method not allowed");
        *out_response_ready = result == APP_ERROR_NONE;
        return result;
    }
    *out_body_limit = route_body_limit(call->path.route);
    if (!web_api_route_requires_body(call->path.route, call->method) &&
        request->content_len != 0U) {
        result = set_error_response(response, WEB_HTTP_STATUS_UNPROCESSABLE_ENTITY,
                                    APP_ERROR_INVALID_ARGUMENT,
                                    "request body is not allowed for this route");
        *out_response_ready = result == APP_ERROR_NONE;
    }
    return result;
}

static app_error_code_t authorize_and_read_api_call(httpd_req_t *request, web_api_call_t *call,
                                                    size_t body_limit,
                                                    web_request_policy_result_t *policy,
                                                    web_api_response_t *response, char **out_body,
                                                    bool *out_response_ready) {
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    app_error_code_t result = apply_request_policy(request, call, body_limit, policy, &failure);
    if (result != APP_ERROR_NONE) {
        result = set_error_response(response, web_request_policy_http_status(failure, result),
                                    result, policy_failure_message(failure));
        *out_response_ready = result == APP_ERROR_NONE;
        return result;
    }
    memcpy(call->session_token, policy->session_token, sizeof(call->session_token));
    result = read_call_body(request, body_limit, out_body, &call->body_length);
    call->body = *out_body == NULL ? "" : *out_body;
    if (result != APP_ERROR_NONE) {
        result = set_error_response(response, web_api_http_status_for_error(result), result,
                                    "could not read request body");
        *out_response_ready = result == APP_ERROR_NONE;
    }
    return result;
}

static bool dispatch_api_call(const web_api_call_t *call, web_api_response_t *response) {
    app_error_code_t result = web_api_dispatch(call, response);
    if (result != APP_ERROR_NONE && response->body == NULL) {
        result = set_error_response(response, web_api_http_status_for_error(result), result,
                                    "API operation failed");
    }
    return result == APP_ERROR_NONE && response->body != NULL;
}

esp_err_t api_handler(httpd_req_t *request) {
    web_api_response_t response = {0};
    web_request_policy_result_t policy = {0};
    web_api_call_t call = {0};
    char *body = NULL;
    size_t body_limit = WEB_API_SMALL_BODY_MAX_BYTES;
    bool response_ready = false;

    app_error_code_t result =
        prepare_api_call(request, &call, &response, &body_limit, &response_ready);
    if (!response_ready && result == APP_ERROR_NONE) {
        result = authorize_and_read_api_call(request, &call, body_limit, &policy, &response, &body,
                                             &response_ready);
    }
    if (!response_ready && result == APP_ERROR_NONE) {
        response_ready = dispatch_api_call(&call, &response);
    }
    if (!response_ready) {
        web_api_response_free(&response);
        (void)set_error_response(&response, WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR,
                                 APP_ERROR_INTERNAL, "response encoding failed");
    }

    const bool should_restart = response.body != NULL && restart_after_response(&call, &response);
    const esp_err_t send_result = send_api_response(request, policy.request_id, &response);
    free(body);
    web_api_response_free(&response);
    if (send_result == ESP_OK && should_restart) {
        esp_restart();
    }
    return send_result;
}
