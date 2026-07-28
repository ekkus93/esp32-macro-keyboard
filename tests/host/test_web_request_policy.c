#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "test_assert.h"
#include "web_request_policy.h"

#define TOKEN "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"

typedef struct {
    const char *missing;
    const char *content_type;
    const char *origin;
    const char *request_id;
    app_error_code_t validation_result;
    app_error_code_t confirmation_result;
    size_t validation_calls;
    size_t confirmation_calls;
    bool saw_null_csrf;
} fixture_t;

static app_error_code_t get_header(void *context, const char *name, char *output,
                                   size_t output_size) {
    fixture_t *fixture = context;
    if (fixture->missing != NULL && strcmp(fixture->missing, name) == 0) {
        output[0] = '\0';
        return APP_ERROR_AUTH_REQUIRED;
    }
    const char *value = NULL;
    if (strcmp(name, "Content-Type") == 0) {
        value = fixture->content_type;
    } else if (strcmp(name, "Host") == 0) {
        value = "192.168.4.1";
    } else if (strcmp(name, "Origin") == 0) {
        value = fixture->origin;
    } else if (strcmp(name, "Cookie") == 0) {
        value = "MKSESSION=" TOKEN;
    } else if (strcmp(name, "X-CSRF-Token") == 0) {
        value = TOKEN;
    } else if (strcmp(name, "X-Request-ID") == 0) {
        value = fixture->request_id;
    }
    if (value == NULL) {
        output[0] = '\0';
        return APP_ERROR_AUTH_REQUIRED;
    }
    TEST_CHECK(strlen(value) < output_size);
    memcpy(output, value, strlen(value) + 1U);
    return APP_ERROR_NONE;
}

static app_error_code_t validate(void *context, const char *session_token, const char *csrf_token) {
    fixture_t *fixture = context;
    TEST_CHECK_EQ_STRING(TOKEN, session_token);
    fixture->saw_null_csrf = csrf_token == NULL;
    if (csrf_token != NULL) {
        TEST_CHECK_EQ_STRING(TOKEN, csrf_token);
    }
    ++fixture->validation_calls;
    return fixture->validation_result;
}

static app_error_code_t generate(void *context, char *output, size_t output_size) {
    (void)context;
    const char *value = "generated-request-id";
    TEST_CHECK(strlen(value) < output_size);
    memcpy(output, value, strlen(value) + 1U);
    return APP_ERROR_NONE;
}

static app_error_code_t confirm(void *context) {
    fixture_t *fixture = context;
    ++fixture->confirmation_calls;
    return fixture->confirmation_result;
}

static web_request_policy_ops_t operations(fixture_t *fixture) {
    return (web_request_policy_ops_t){
        .context = fixture,
        .get_header = get_header,
        .validate_session = validate,
        .generate_request_id = generate,
        .confirm = confirm,
    };
}

static web_request_policy_input_t input(web_api_route_t route, web_api_method_t method) {
    return (web_request_policy_input_t){
        .route = route,
        .method = method,
        .content_length = method == WEB_API_METHOD_GET ? 0U : 2U,
        .body_limit = 256U,
    };
}

static void test_success_and_generated_request_id(void) {
    fixture_t fixture = {
        .content_type = "application/json",
        .origin = "http://192.168.4.1",
        .validation_result = APP_ERROR_NONE,
        .confirmation_result = APP_ERROR_NONE,
    };
    const web_request_policy_ops_t ops = operations(&fixture);
    const web_request_policy_input_t policy = input(WEB_API_ROUTE_SETS, WEB_API_METHOD_POST);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_STRING(TOKEN, result.session_token);
    TEST_CHECK_EQ_STRING("generated-request-id", result.request_id);
    TEST_CHECK_EQ_U64(1U, fixture.validation_calls);
}

static void test_get_does_not_require_csrf(void) {
    fixture_t fixture = {
        .missing = "X-CSRF-Token",
        .validation_result = APP_ERROR_NONE,
    };
    const web_request_policy_ops_t ops = operations(&fixture);
    const web_request_policy_input_t policy = input(WEB_API_ROUTE_SETS, WEB_API_METHOD_GET);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK(fixture.saw_null_csrf);
    TEST_CHECK_EQ_U64(1U, fixture.validation_calls);
}

static void test_failure_statuses(void) {
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;

    fixture_t fixture = {
        .content_type = "text/plain",
        .origin = "http://192.168.4.1",
    };
    web_request_policy_ops_t ops = operations(&fixture);
    web_request_policy_input_t policy = input(WEB_API_ROUTE_SETS, WEB_API_METHOD_POST);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_CONTENT_TYPE, failure);
    TEST_CHECK_EQ_U64(415U, web_request_policy_http_status(failure, APP_ERROR_INVALID_ARGUMENT));

    fixture = (fixture_t){
        .content_type = "application/json",
        .origin = "http://invalid.local",
    };
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_ORIGIN, failure);
    TEST_CHECK_EQ_U64(403U, web_request_policy_http_status(failure, APP_ERROR_AUTH_REQUIRED));

    fixture = (fixture_t){
        .content_type = "application/json",
        .origin = "http://192.168.4.1",
        .missing = "Cookie",
    };
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_COOKIE, failure);
    TEST_CHECK_EQ_U64(401U, web_request_policy_http_status(failure, APP_ERROR_AUTH_REQUIRED));

    fixture = (fixture_t){
        .content_type = "application/json",
        .origin = "http://192.168.4.1",
        .missing = "X-CSRF-Token",
    };
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_CSRF, failure);

    fixture = (fixture_t){
        .content_type = "application/json",
        .origin = "http://192.168.4.1",
        .validation_result = APP_ERROR_AUTH_FAILED,
    };
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_FAILED,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_SESSION, failure);

    fixture = (fixture_t){
        .content_type = "application/json",
        .origin = "http://192.168.4.1",
        .validation_result = APP_ERROR_NONE,
        .request_id = "bad/request",
    };
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_REQUEST_ID, failure);

    fixture = (fixture_t){
        .content_type = "application/json",
        .origin = "http://192.168.4.1",
        .validation_result = APP_ERROR_NONE,
        .confirmation_result = APP_ERROR_TIMEOUT,
    };
    ops = operations(&fixture);
    policy = input(WEB_API_ROUTE_DEVICE_FACTORY_RESET, WEB_API_METHOD_POST);
    policy.content_length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION, failure);
    TEST_CHECK_EQ_U64(403U, web_request_policy_http_status(failure, APP_ERROR_TIMEOUT));
}

static void test_body_limit_precedes_headers(void) {
    fixture_t fixture = {0};
    const web_request_policy_ops_t ops = operations(&fixture);
    web_request_policy_input_t policy = input(WEB_API_ROUTE_RESTORE, WEB_API_METHOD_POST);
    policy.content_length = 1025U;
    policy.body_limit = 1024U;
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_BODY_LIMIT, failure);
    TEST_CHECK_EQ_U64(413U, web_request_policy_http_status(failure, APP_ERROR_INVALID_ARGUMENT));
    TEST_CHECK_EQ_U64(0U, fixture.validation_calls);
}

int main(void) {
    test_success_and_generated_request_id();
    test_get_does_not_require_csrf();
    test_failure_statuses();
    test_body_limit_precedes_headers();
    return 0;
}
