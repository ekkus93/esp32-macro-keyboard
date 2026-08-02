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

typedef struct {
    web_api_route_t route;
    web_api_method_t method;
} route_case_t;

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
        .content_length = web_api_route_requires_body(route, method) ? 2U : 0U,
        .body_limit = 256U,
    };
}

static const route_case_t route_cases[] = {
    {WEB_API_ROUTE_AUTH_SESSION, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_SETS, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_SETS, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SETS_ORDER, WEB_API_METHOD_PUT},
    {WEB_API_ROUTE_SET, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_SET, WEB_API_METHOD_PUT},
    {WEB_API_ROUTE_SET, WEB_API_METHOD_DELETE},
    {WEB_API_ROUTE_SET_DUPLICATE, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SET_SELECT, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SET_EXPORT, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_SET_IMPORT, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SET_MACROS, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_SET_MACROS, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_PUT},
    {WEB_API_ROUTE_SET_MACRO, WEB_API_METHOD_DELETE},
    {WEB_API_ROUTE_SET_MACRO_VALIDATE, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SET_MACRO_DUPLICATE, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SET_MACROS_REORDER, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SET_PROCEDURES, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_SET_PROCEDURES, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SET_PROCEDURE, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_SET_PROCEDURE, WEB_API_METHOD_PUT},
    {WEB_API_ROUTE_SET_PROCEDURE, WEB_API_METHOD_DELETE},
    {WEB_API_ROUTE_SET_PROCEDURES_REORDER, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_PROCEDURE_PROGRESS, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_PROCEDURE_PROGRESS, WEB_API_METHOD_PUT},
    {WEB_API_ROUTE_PROCEDURE_PROGRESS, WEB_API_METHOD_DELETE},
    {WEB_API_ROUTE_PROGRESS_COMPLETE, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_PROGRESS_SKIP, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_EXECUTIONS, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_EXECUTION_CURRENT, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_EXECUTION_CANCEL, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT},
    {WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_DEVICE_RESTART, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_DEVICE_RESET_SETTINGS, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_DEVICE_FACTORY_RESET, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_DIAGNOSTICS_STORAGE, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK, WEB_API_METHOD_POST},
    {WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_BACKUP, WEB_API_METHOD_GET},
    {WEB_API_ROUTE_RESTORE, WEB_API_METHOD_POST},
};

static void evaluate_success(const route_case_t *route_case) {
    fixture_t fixture = {
        .content_type = "application/json",
        .origin = "http://192.168.4.1",
        .validation_result = APP_ERROR_NONE,
        .confirmation_result = APP_ERROR_NONE,
    };
    const web_request_policy_ops_t ops = operations(&fixture);
    const web_request_policy_input_t policy = input(route_case->route, route_case->method);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK(web_api_route_allows_method(route_case->route, route_case->method));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_NONE, failure);
    TEST_CHECK_EQ_STRING(TOKEN, result.session_token);
    TEST_CHECK_EQ_STRING("generated-request-id", result.request_id);
    TEST_CHECK_EQ_U64(1U, fixture.validation_calls);
    TEST_CHECK_EQ_U64(web_api_route_requires_physical_confirmation(route_case->route) ? 1U : 0U,
                      fixture.confirmation_calls);
    TEST_CHECK(fixture.saw_null_csrf ==
               !web_api_route_requires_csrf(route_case->route, route_case->method));
}

static void evaluate_missing_header(const route_case_t *route_case, const char *header,
                                    web_request_policy_failure_t expected_failure,
                                    unsigned int expected_status) {
    fixture_t fixture = {
        .missing = header,
        .content_type = "application/json",
        .origin = "http://192.168.4.1",
        .validation_result = APP_ERROR_NONE,
        .confirmation_result = APP_ERROR_NONE,
    };
    const web_request_policy_ops_t ops = operations(&fixture);
    const web_request_policy_input_t policy = input(route_case->route, route_case->method);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    const app_error_code_t error = web_request_policy_evaluate(&policy, &ops, &result, &failure);
    TEST_CHECK(error != APP_ERROR_NONE);
    TEST_CHECK_EQ_INT(expected_failure, failure);
    TEST_CHECK_EQ_U64(expected_status, web_request_policy_http_status(failure, error));
}

static void evaluate_origin_failure(const route_case_t *route_case) {
    fixture_t fixture = {
        .content_type = "application/json",
        .origin = "http://invalid.local",
        .validation_result = APP_ERROR_NONE,
        .confirmation_result = APP_ERROR_NONE,
    };
    const web_request_policy_ops_t ops = operations(&fixture);
    const web_request_policy_input_t policy = input(route_case->route, route_case->method);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_ORIGIN, failure);
    TEST_CHECK_EQ_U64(403U, web_request_policy_http_status(failure, APP_ERROR_AUTH_REQUIRED));
}

static void evaluate_content_type_failure(const route_case_t *route_case) {
    if (!web_api_route_requires_body(route_case->route, route_case->method)) {
        return;
    }
    fixture_t fixture = {
        .content_type = "text/plain",
        .origin = "http://192.168.4.1",
    };
    const web_request_policy_ops_t ops = operations(&fixture);
    const web_request_policy_input_t policy = input(route_case->route, route_case->method);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_CONTENT_TYPE, failure);
    TEST_CHECK_EQ_U64(415U, web_request_policy_http_status(failure, APP_ERROR_INVALID_ARGUMENT));
}

static void evaluate_confirmation_failure(const route_case_t *route_case) {
    if (!web_api_route_requires_physical_confirmation(route_case->route)) {
        return;
    }
    fixture_t fixture = {
        .content_type = "application/json",
        .origin = "http://192.168.4.1",
        .validation_result = APP_ERROR_NONE,
        .confirmation_result = APP_ERROR_TIMEOUT,
    };
    const web_request_policy_ops_t ops = operations(&fixture);
    const web_request_policy_input_t policy = input(route_case->route, route_case->method);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION, failure);
    TEST_CHECK_EQ_U64(403U, web_request_policy_http_status(failure, APP_ERROR_TIMEOUT));
}

static void evaluate_body_limit_failure(const route_case_t *route_case) {
    fixture_t fixture = {0};
    const web_request_policy_ops_t ops = operations(&fixture);
    web_request_policy_input_t policy = input(route_case->route, route_case->method);
    policy.content_length = 257U;
    policy.body_limit = 256U;
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_BODY_LIMIT, failure);
    TEST_CHECK_EQ_U64(413U, web_request_policy_http_status(failure, APP_ERROR_INVALID_ARGUMENT));
    TEST_CHECK_EQ_U64(0U, fixture.validation_calls);
}

static void test_complete_route_policy_matrix(void) {
    for (size_t index = 0U; index < sizeof(route_cases) / sizeof(route_cases[0]); ++index) {
        const route_case_t *route_case = &route_cases[index];
        evaluate_success(route_case);
        evaluate_missing_header(route_case, "Host", WEB_REQUEST_POLICY_FAILURE_HOST, 403U);
        evaluate_origin_failure(route_case);
        evaluate_missing_header(route_case, "Cookie", WEB_REQUEST_POLICY_FAILURE_COOKIE, 401U);
        if (web_api_route_requires_csrf(route_case->route, route_case->method)) {
            evaluate_missing_header(route_case, "X-CSRF-Token", WEB_REQUEST_POLICY_FAILURE_CSRF,
                                    403U);
        }
        evaluate_content_type_failure(route_case);
        evaluate_confirmation_failure(route_case);
        evaluate_body_limit_failure(route_case);
    }
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
    test_complete_route_policy_matrix();
    test_success_and_generated_request_id();
    test_get_does_not_require_csrf();
    test_failure_statuses();
    test_body_limit_precedes_headers();
    return 0;
}
