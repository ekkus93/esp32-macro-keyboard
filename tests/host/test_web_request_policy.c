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
    app_error_code_t request_id_result;
    app_error_code_t validation_result;
    app_error_code_t confirmation_result;
    size_t request_id_generation_calls;
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
        return fixture->request_id_result == APP_ERROR_NONE ? APP_ERROR_AUTH_REQUIRED
                                                            : fixture->request_id_result;
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
    fixture_t *fixture = context;
    ++fixture->request_id_generation_calls;
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
        {WEB_API_ROUTE_BLOB_COLLECTION, WEB_API_METHOD_GET},
        {WEB_API_ROUTE_BLOB_COLLECTION, WEB_API_METHOD_POST},
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
            .content_type = cases[index].route == WEB_API_ROUTE_BLOB_COLLECTION
                                ? "application/gzip"
                                : "application/json",
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

    fixture = (fixture_t){.content_type = "application/json"};
    ops = operations(&fixture);
    policy = input(WEB_API_ROUTE_BLOB_COLLECTION, WEB_API_METHOD_POST);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_CONTENT_TYPE, failure);
    TEST_CHECK_EQ_U64(0U, fixture.validation_calls);

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

static void test_request_id_header_failure_is_not_silently_replaced(void) {
    fixture_t fixture = {
        .content_type = "application/json",
        .request_id_result = APP_ERROR_INVALID_ARGUMENT,
        .validation_result = APP_ERROR_NONE,
    };
    const web_request_policy_ops_t ops = operations(&fixture);
    const web_request_policy_input_t policy = input(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_REQUEST_ID, failure);
    TEST_CHECK_EQ_U64(1U, fixture.validation_calls);
    TEST_CHECK_EQ_U64(0U, fixture.request_id_generation_calls);
    TEST_CHECK_EQ_STRING("", result.request_id);
}

/* GET /api/v1/diagnostics (WEB_API_ROUTE_DIAGNOSTICS_FULL) reaches
 * web_diagnostics_handle() only through this exact same generic-pipeline gate
 * (web_server_api.c's apply_request_policy(), called from
 * web_api_handle_call_with_body() before web_api_dispatch() is ever reached)
 * -- unlike status/limits/send, which have their own dedicated httpd_uri_t
 * registrations and never touch web_request_policy.c at all (see
 * test_web_server_status_limits_route.c / test_web_server_send_route.c's
 * header comments). test_fail_closed_ordering() above already proves the
 * cookie/content-type/body-limit/confirmation failure modes are route-
 * agnostic (WEB_API_ROUTE_SETTINGS/BLOB_COLLECTION/DEVICE_FACTORY_RESET); this
 * exercises the identical missing-cookie ("unauthorized") and invalid-session
 * ("expired-session") failures specifically for WEB_API_ROUTE_DIAGNOSTICS_FULL
 * so that route's own matrix entry is not just inferred by analogy. */
static void test_diagnostics_route_unauthorized_and_expired_session(void) {
    fixture_t fixture = {.missing = "Cookie", .content_type = "application/json"};
    web_request_policy_ops_t ops = operations(&fixture);
    web_request_policy_input_t policy = input(WEB_API_ROUTE_DIAGNOSTICS_FULL, WEB_API_METHOD_GET);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_COOKIE, failure);
    TEST_CHECK_EQ_U64(0U, fixture.validation_calls);

    fixture = (fixture_t){
        .content_type = "application/json",
        .validation_result = APP_ERROR_AUTH_REQUIRED, /* Simulates an expired session. */
    };
    ops = operations(&fixture);
    policy = input(WEB_API_ROUTE_DIAGNOSTICS_FULL, WEB_API_METHOD_GET);
    TEST_CHECK_APP_ERROR(APP_ERROR_AUTH_REQUIRED,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_INT(WEB_REQUEST_POLICY_FAILURE_SESSION, failure);
    TEST_CHECK_EQ_U64(1U, fixture.validation_calls);
}

/* SPEC 13.4: GET/POST /api/v1/setup on a provisioned device must answer
 * 404/409 without requiring a session -- unlike every other route in
 * test_success_matrix(), which all require one. */
static void test_setup_route_requires_no_session(void) {
    fixture_t fixture = {
        .missing = "Cookie",
        .content_type = "application/json",
    };
    web_request_policy_ops_t ops = operations(&fixture);
    web_request_policy_input_t policy = input(WEB_API_ROUTE_SETUP, WEB_API_METHOD_GET);
    web_request_policy_result_t result = {0};
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_U64(0U, fixture.validation_calls);
    TEST_CHECK_EQ_STRING("", result.session_token);

    fixture = (fixture_t){.missing = "Cookie", .content_type = "application/json"};
    ops = operations(&fixture);
    policy = input(WEB_API_ROUTE_SETUP, WEB_API_METHOD_POST);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_request_policy_evaluate(&policy, &ops, &result, &failure));
    TEST_CHECK_EQ_U64(0U, fixture.validation_calls);
    TEST_CHECK_EQ_U64(0U, fixture.confirmation_calls);
}

int main(void) {
    test_success_matrix();
    test_fail_closed_ordering();
    test_request_id_header_failure_is_not_silently_replaced();
    test_diagnostics_route_unauthorized_and_expired_session();
    test_setup_route_requires_no_session();
    return 0;
}
