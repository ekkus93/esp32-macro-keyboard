#include "web_request_policy.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "web_api_core.h"
#include "web_cookie.h"
#include "web_http_status.h"

#define WEB_POLICY_HEADER_BYTES 256U

static void clear_result(web_request_policy_result_t *result) {
    if (result != NULL) {
        memset(result, 0, sizeof(*result));
    }
}

static app_error_code_t fail(web_request_policy_result_t *result,
                             web_request_policy_failure_t *out_failure,
                             web_request_policy_failure_t failure, app_error_code_t error) {
    clear_result(result);
    *out_failure = failure;
    return error;
}

static bool operations_valid(const web_request_policy_ops_t *operations) {
    return operations != NULL && operations->get_header != NULL &&
           operations->validate_session != NULL && operations->generate_request_id != NULL;
}

static app_error_code_t read_required_header(const web_request_policy_ops_t *operations,
                                             const char *name, char *buffer, size_t buffer_size) {
    const app_error_code_t result =
        operations->get_header(operations->context, name, buffer, buffer_size);
    return result == APP_ERROR_NONE && buffer[0] != '\0' ? APP_ERROR_NONE : APP_ERROR_AUTH_REQUIRED;
}

static app_error_code_t establish_request_id(const web_request_policy_ops_t *operations,
                                             web_request_policy_result_t *out_result,
                                             web_request_policy_failure_t *out_failure) {
    char supplied[WEB_API_REQUEST_ID_MAX_BYTES + 1U] = {0};
    const app_error_code_t header_result =
        operations->get_header(operations->context, "X-Request-ID", supplied, sizeof(supplied));
    if (header_result == APP_ERROR_NONE) {
        if (!web_api_request_id_is_valid(supplied)) {
            return fail(out_result, out_failure, WEB_REQUEST_POLICY_FAILURE_REQUEST_ID,
                        APP_ERROR_INVALID_ARGUMENT);
        }
        memcpy(out_result->request_id, supplied, strlen(supplied) + 1U);
        return APP_ERROR_NONE;
    }
    const app_error_code_t generate_result = operations->generate_request_id(
        operations->context, out_result->request_id, sizeof(out_result->request_id));
    if (generate_result != APP_ERROR_NONE || !web_api_request_id_is_valid(out_result->request_id)) {
        return fail(out_result, out_failure, WEB_REQUEST_POLICY_FAILURE_REQUEST_ID,
                    generate_result == APP_ERROR_NONE ? APP_ERROR_INTERNAL : generate_result);
    }
    return APP_ERROR_NONE;
}

static app_error_code_t enforce_body_content_type(const web_request_policy_input_t *input,
                                                  const web_request_policy_ops_t *operations,
                                                  web_request_policy_result_t *out_result,
                                                  web_request_policy_failure_t *out_failure) {
    if (input->content_length > input->body_limit) {
        return fail(out_result, out_failure, WEB_REQUEST_POLICY_FAILURE_BODY_LIMIT,
                    APP_ERROR_INVALID_ARGUMENT);
    }
    if (web_api_route_requires_body(input->route, input->method) || input->content_length > 0U) {
        char content_type[WEB_POLICY_HEADER_BYTES] = {0};
        if (operations->get_header(operations->context, "Content-Type", content_type,
                                   sizeof(content_type)) != APP_ERROR_NONE ||
            !web_api_content_type_is_json(content_type)) {
            return fail(out_result, out_failure, WEB_REQUEST_POLICY_FAILURE_CONTENT_TYPE,
                        APP_ERROR_INVALID_ARGUMENT);
        }
    }
    return APP_ERROR_NONE;
}

static app_error_code_t enforce_session(const web_request_policy_input_t *input,
                                        const web_request_policy_ops_t *operations,
                                        web_request_policy_result_t *out_result,
                                        web_request_policy_failure_t *out_failure) {
    if (!web_api_route_requires_session(input->route)) {
        return APP_ERROR_NONE;
    }
    char cookie[WEB_POLICY_HEADER_BYTES] = {0};
    if (read_required_header(operations, "Cookie", cookie, sizeof(cookie)) != APP_ERROR_NONE ||
        web_cookie_extract_session(cookie, out_result->session_token,
                                   sizeof(out_result->session_token)) != APP_ERROR_NONE) {
        return fail(out_result, out_failure, WEB_REQUEST_POLICY_FAILURE_COOKIE,
                    APP_ERROR_AUTH_REQUIRED);
    }
    /* The session cookie is HttpOnly and SameSite=Strict, so a cross-site page
     * cannot cause it to be sent at all -- which is the attack a CSRF token
     * defends against. Carrying both was one mechanism doing another's job
     * (SPEC 16.2). */
    const app_error_code_t validation =
        operations->validate_session(operations->context, out_result->session_token);
    if (validation != APP_ERROR_NONE) {
        return fail(out_result, out_failure, WEB_REQUEST_POLICY_FAILURE_SESSION, validation);
    }
    return APP_ERROR_NONE;
}

static app_error_code_t enforce_physical_confirmation(const web_request_policy_input_t *input,
                                                      const web_request_policy_ops_t *operations,
                                                      web_request_policy_result_t *out_result,
                                                      web_request_policy_failure_t *out_failure) {
    if (!web_api_route_requires_physical_confirmation(input->route)) {
        return APP_ERROR_NONE;
    }
    if (operations->confirm == NULL) {
        return fail(out_result, out_failure, WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION,
                    APP_ERROR_INTERNAL);
    }
    const app_error_code_t result = operations->confirm(operations->context);
    if (result != APP_ERROR_NONE) {
        return fail(out_result, out_failure, WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION,
                    result);
    }
    return APP_ERROR_NONE;
}

app_error_code_t web_request_policy_evaluate(const web_request_policy_input_t *input,
                                             const web_request_policy_ops_t *operations,
                                             web_request_policy_result_t *out_result,
                                             web_request_policy_failure_t *out_failure) {
    clear_result(out_result);
    if (out_failure != NULL) {
        *out_failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    }
    if (input == NULL || !operations_valid(operations) || out_result == NULL ||
        out_failure == NULL || input->route == WEB_API_ROUTE_UNKNOWN || input->body_limit == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_error_code_t result = enforce_body_content_type(input, operations, out_result, out_failure);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = enforce_session(input, operations, out_result, out_failure);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = establish_request_id(operations, out_result, out_failure);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    return enforce_physical_confirmation(input, operations, out_result, out_failure);
}

unsigned int web_request_policy_http_status(web_request_policy_failure_t failure,
                                            app_error_code_t error) {
    switch (failure) {
    case WEB_REQUEST_POLICY_FAILURE_CONTENT_TYPE:
        return WEB_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE;
    case WEB_REQUEST_POLICY_FAILURE_BODY_LIMIT:
        return WEB_HTTP_STATUS_PAYLOAD_TOO_LARGE;
    case WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION:
        return WEB_HTTP_STATUS_FORBIDDEN;
    case WEB_REQUEST_POLICY_FAILURE_COOKIE:
    case WEB_REQUEST_POLICY_FAILURE_SESSION:
        return WEB_HTTP_STATUS_UNAUTHORIZED;
    case WEB_REQUEST_POLICY_FAILURE_REQUEST_ID:
        return error == APP_ERROR_INTERNAL ? WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR
                                           : WEB_HTTP_STATUS_BAD_REQUEST;
    case WEB_REQUEST_POLICY_FAILURE_NONE:
    default:
        return web_api_http_status_for_error(error);
    }
}
