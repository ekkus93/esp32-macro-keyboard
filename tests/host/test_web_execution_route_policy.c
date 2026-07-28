#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_executor.h"
#include "test_assert.h"
#include "web_api_core.h"
#include "web_execution_route_policy.h"

#define EXECUTION_ID "55555555-5555-4555-8555-555555555555"
#define OTHER_EXECUTION_ID "55555555-5555-4555-8555-999999999999"

static app_uuid_t uuid(const char *text) {
    app_uuid_t value = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &value));
    return value;
}

static macro_execution_status_t running_status(void) {
    return (macro_execution_status_t){
        .execution_id = uuid(EXECUTION_ID),
        .state = EXECUTION_RUNNING,
        .available = true,
    };
}

static web_api_path_t matching_path(void) {
    return (web_api_path_t){
        .route = WEB_API_ROUTE_EXECUTION_CANCEL,
        .has_execution_id = true,
        .execution_id = uuid(EXECUTION_ID),
    };
}

static web_execution_cancel_policy_t evaluate(const macro_execution_status_t *status,
                                              const web_api_path_t *path) {
    web_execution_cancel_policy_t policy = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_execution_cancel_policy_evaluate(status, path, &policy));
    return policy;
}

static void test_ready(void) {
    const macro_execution_status_t status = running_status();
    const web_api_path_t path = matching_path();
    const web_execution_cancel_policy_t policy = evaluate(&status, &path);
    TEST_CHECK(policy.permitted);
    TEST_CHECK_EQ_U64(0U, policy.status);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, policy.error);
    TEST_CHECK(policy.message == NULL);
}

static void test_unavailable_precedes_identity(void) {
    macro_execution_status_t status = running_status();
    status.available = false;
    web_api_path_t path = matching_path();
    path.execution_id = uuid(OTHER_EXECUTION_ID);
    const web_execution_cancel_policy_t policy = evaluate(&status, &path);
    TEST_CHECK(!policy.permitted);
    TEST_CHECK_EQ_U64(503U, policy.status);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, policy.error);
    TEST_CHECK_EQ_STRING("executor unavailable", policy.message);
}

static void test_not_found_matrix(void) {
    macro_execution_status_t status = running_status();
    web_api_path_t path = matching_path();

    path.has_execution_id = false;
    web_execution_cancel_policy_t policy = evaluate(&status, &path);
    TEST_CHECK_EQ_U64(404U, policy.status);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, policy.error);

    path = matching_path();
    path.execution_id = uuid(OTHER_EXECUTION_ID);
    policy = evaluate(&status, &path);
    TEST_CHECK_EQ_U64(404U, policy.status);
    TEST_CHECK_EQ_STRING("execution not found", policy.message);

    path = matching_path();
    memset(&status.execution_id, 0, sizeof(status.execution_id));
    policy = evaluate(&status, &path);
    TEST_CHECK_EQ_U64(404U, policy.status);
}

static void test_repeat_and_terminal_conflicts(void) {
    macro_execution_status_t status = running_status();
    const web_api_path_t path = matching_path();
    status.cancellation_requested = true;
    web_execution_cancel_policy_t policy = evaluate(&status, &path);
    TEST_CHECK_EQ_U64(409U, policy.status);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, policy.error);
    TEST_CHECK_EQ_STRING("cancellation already requested", policy.message);

    static const execution_state_t terminal_states[] = {
        EXECUTION_COMPLETED,
        EXECUTION_CANCELLED,
        EXECUTION_FAILED,
        EXECUTION_TIMED_OUT,
    };
    for (size_t index = 0U; index < sizeof(terminal_states) / sizeof(terminal_states[0]); ++index) {
        status = running_status();
        status.state = terminal_states[index];
        policy = evaluate(&status, &path);
        TEST_CHECK_EQ_U64(409U, policy.status);
        TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, policy.error);
        TEST_CHECK_EQ_STRING("execution is already terminal", policy.message);
    }
}

static void test_invalid_arguments_zero_output(void) {
    macro_execution_status_t status = running_status();
    web_api_path_t path = matching_path();
    web_execution_cancel_policy_t policy = {
        .permitted = true,
        .status = 999U,
        .error = APP_ERROR_INTERNAL,
        .message = "stale",
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_execution_cancel_policy_evaluate(NULL, &path, &policy));
    TEST_CHECK(!policy.permitted);
    TEST_CHECK_EQ_U64(0U, policy.status);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, policy.error);
    TEST_CHECK(policy.message == NULL);

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_execution_cancel_policy_evaluate(&status, NULL, &policy));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_execution_cancel_policy_evaluate(&status, &path, NULL));
}

int main(void) {
    test_ready();
    test_unavailable_precedes_identity();
    test_not_found_matrix();
    test_repeat_and_terminal_conflicts();
    test_invalid_arguments_zero_output();
    return 0;
}
