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
    app_error_code_t validation_result;
    app_error_code_t confirmation_result;
    size_t validation_calls;
    size_t confirmation_calls;
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
    } else if (strcmp(name, "Cookie") == 0) {
        value = "MKSESSION=" TOKEN;
    } else if (strcmp(name, "X-Request-ID") == 0) {
        output[0] = '\0';
        return APP_ERROR_AUTH_REQUIRED;
    }
    if (value == NULL) {
        output[0] = '\0';
        return APP_ERROR_AUTH_REQUIRED;
    }
    TEST_CHECK(strlen(value) < output_size);
    memcpy(output, value, strlen(value) + 1U);
    return APP_ERROR_NONE;
}

static app_error_code_t validate(void *context, const char *session_token) {
    fixture_t *fixture = context;
    TEST_CHECK_EQ_STRING(TOKEN, session_token);
    ++fixture->validation_calls;
    return fixture->validation_result;
}

static app_error_code_t generate(void *context, char *output, size_t output_size) {
    (void)context;
    static const char value[] = "generated-request-id";
    TEST_CHECK(sizeof(value) <= output_size);
    memcpy(output, value, sizeof(value));
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

static void test_success_matrix(void) {
    static const struct {
        web_api_route_t route;
        web_api_method_t method;
    } cases[] = {
        {WEB_API_ROUTE_AUTH_SESSION, WEB_API_METHOD_GET},
        {WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET},
        {WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT},
        {WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD, WEB_API_METHOD_POST},
        {WEB_API_ROUTE_DEVICE_RESTART, WEB_API_METHOD_POST},
        {WEB_API_ROUTE_DEVICE_RESET_SETTINGS, WEB_API_METHOD_POST},
        {WEB_API_ROUTE_DEVICE_FACTORY_RESET, WEB_API_METHOD_POST},
        {WEB_API_ROUTE_DIAGNOSTICS_FULL, WEB_API_METHOD_GET},
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        fixture_t fixture = {
            .content_type = "application/json",
            .validation_result = APP_ERROR_NONE,
            .confirmation_result = APP_ERROR_NONE,
        };
        const web_request_policy_ops_t ops = operations(&fixture);
        const web_request_policy_input_t policy = input(cases[index].route, cases[index].method);
        web_request_policy_result_t result = {0};
        web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                             web_request_policy_evaluate(&policy, &ops, &result, &failure));
        TEST_CHECK_EQ_STRING(TOKEN, result.session_token);
        TEST_CHECK_EQ_U64(1U, fixture.validation_calls);
        TEST_CHECK_EQ_U64(web_api_route_requires_physical_confirmation(cases[index].route) ? 1U
                                                                                           : 0U,
                          fixture.confirmation_calls);
    }
}

static void test_fail_closed_ordering(void) {
    fixture_t fixture = {
        .missing = "Cookie",
        .content_type = "application/json",
    };
    web_request_policy_ops_t ops = operations(&fixture);
    web_request_policy_input_t policy = input(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_COOKIE, failure);

    fixture = (fixture_t){.content_type = "text/plain"};
    ops = operations(&fixture);
    policy = input(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_CONTENT_TYPE, failure);

    fixture = (fixture_t){0};
    ops = operations(&fixture);
    policy = input(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_PUT);
    policy.content_length = 257U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_BODY_LIMIT, failure);
    TEST_CHECK_EQ_U64(0U, fixture.validation_calls);

    fixture = (fixture_t){
        .content_type = "application/json",
        .validation_result = APP_ERROR_NONE,
        .confirmation_result = APP_ERROR_TIMEOUT,
    };
    ops = operations(&fixture);
    policy = input(WEB_API_ROUTE_DEVICE_FACTORY_RESET, WEB_API_METHOD_POST);
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_PHYSICAL_CONFIRMATION, failure);
}

int main(void) {
    test_success_matrix();
    test_fail_closed_ordering();
    return 0;
}
