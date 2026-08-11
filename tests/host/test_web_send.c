#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_executor.h"
#include "test_assert.h"
#include "web_send.h"

typedef struct {
    bool submit_called;
    app_error_code_t submit_result;
    macro_execution_request_t last_request;
    macro_execution_status_t status_to_return;
    app_error_code_t cancel_result;
    bool cancel_called;
} fake_send_backend_t;

static app_error_code_t fake_submit(void *context, macro_execution_request_t *request) {
    fake_send_backend_t *fake = context;
    fake->submit_called = true;
    fake->last_request = *request;
    if (fake->submit_result == APP_ERROR_NONE) {
        macro_plan_v2_free(&request->plan);
        request->plan = (macro_plan_t){0};
    }
    return fake->submit_result;
}

static macro_execution_status_t fake_get_status(void *context) {
    fake_send_backend_t *fake = context;
    return fake->status_to_return;
}

static app_error_code_t fake_cancel(void *context) {
    fake_send_backend_t *fake = context;
    fake->cancel_called = true;
    return fake->cancel_result;
}

static web_send_ops_t fake_ops(fake_send_backend_t *fake) {
    return (web_send_ops_t){
        .context = fake,
        .submit = fake_submit,
        .get_status = fake_get_status,
        .cancel = fake_cancel,
    };
}

#define BODY_EXTRA_CAPACITY 4U

/* Allocates a buffer sized exactly strlen(text) + 1 + BODY_EXTRA_CAPACITY
 * bytes and writes `text` (NUL-terminated) into it, so the returned capacity
 * always matches the actual allocation -- passing a capacity larger than the
 * real allocation would let web_send_create_handle()'s bounded body scan
 * read past the end of the buffer. */
static char *dup_body(const char *text, size_t *out_capacity) {
    const size_t length = strlen(text);
    const size_t capacity = length + 1U + BODY_EXTRA_CAPACITY;
    char *copy = malloc(capacity);
    TEST_CHECK(copy != NULL);
    memcpy(copy, text, length + 1U);
    memset(copy + length + 1U, 0, BODY_EXTRA_CAPACITY);
    *out_capacity = capacity;
    return copy;
}

static void test_create_success(void) {
    fake_send_backend_t fake = {0};
    fake.submit_result = APP_ERROR_NONE;
    fake.status_to_return.available = true;
    const web_send_ops_t ops = fake_ops(&fake);

    size_t body_capacity = 0U;
    char *body =
        dup_body("{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}", &body_capacity);
    const web_send_create_outcome_t outcome = web_send_create_handle(body, body_capacity, &ops);
    free(body);

    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_OK, outcome.result);
    TEST_CHECK(fake.submit_called);
    TEST_CHECK(app_uuid_is_valid_string(outcome.accepted.id));
    TEST_CHECK(outcome.accepted.action_count > 0U);
    TEST_CHECK_EQ_U64(8U, fake.last_request.key_press_ms);
    TEST_CHECK_EQ_U64(15U, fake.last_request.inter_key_ms);
    TEST_CHECK_EQ_U64(1U, fake.last_request.macro_revision);
}

static void test_create_rejects_malformed_bodies(void) {
    fake_send_backend_t fake = {0};
    const web_send_ops_t ops = fake_ops(&fake);

    static const char *const invalid_bodies[] = {
        "",
        "not-json",
        "{}",
        "{\"source\":\"a\"}",
        "{\"source\":\"a\",\"keyPressMs\":8}",
        "{\"source\":\"a\",\"keyPressMs\":8,\"interKeyMs\":15,\"extra\":true}",
        "{\"source\":1,\"keyPressMs\":8,\"interKeyMs\":15}",
        "{\"source\":\"a\",\"keyPressMs\":\"8\",\"interKeyMs\":15}",
        "{\"source\":\"a\",\"keyPressMs\":-1,\"interKeyMs\":15}",
        "{\"source\":\"a\",\"keyPressMs\":8.5,\"interKeyMs\":15}",
        "[]",
        "{\"source\":\"a\",\"keyPressMs\":8,\"interKeyMs\":15} trailing",
    };
    for (size_t index = 0U; index < sizeof(invalid_bodies) / sizeof(invalid_bodies[0]); ++index) {
        fake.submit_called = false;
        size_t body_capacity = 0U;
        char *body = dup_body(invalid_bodies[index], &body_capacity);
        const web_send_create_outcome_t outcome = web_send_create_handle(body, body_capacity, &ops);
        free(body);
        TEST_CHECK_EQ_INT(WEB_SEND_CREATE_INVALID_BODY, outcome.result);
        TEST_CHECK(!fake.submit_called);
    }
}

static void test_create_parse_error_reports_location(void) {
    fake_send_backend_t fake = {0};
    const web_send_ops_t ops = fake_ops(&fake);
    size_t body_capacity = 0U;
    char *body =
        dup_body("{\"source\":\"{DELAY:-1}\",\"keyPressMs\":8,\"interKeyMs\":15}", &body_capacity);
    const web_send_create_outcome_t outcome = web_send_create_handle(body, body_capacity, &ops);
    free(body);
    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_PARSE_ERROR, outcome.result);
    TEST_CHECK(!fake.submit_called);
    TEST_CHECK_EQ_INT(APP_ERROR_MACRO_SYNTAX, outcome.parse_error.code);
}

static void test_create_busy(void) {
    fake_send_backend_t fake = {0};
    fake.submit_result = APP_ERROR_EXECUTOR_BUSY;
    const web_send_ops_t ops = fake_ops(&fake);
    size_t body_capacity = 0U;
    char *body =
        dup_body("{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}", &body_capacity);
    const web_send_create_outcome_t outcome = web_send_create_handle(body, body_capacity, &ops);
    free(body);
    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_BUSY, outcome.result);
    TEST_CHECK(fake.submit_called);
}

static void test_create_usb_not_ready(void) {
    fake_send_backend_t fake = {0};
    fake.submit_result = APP_ERROR_USB_NOT_READY;
    const web_send_ops_t ops = fake_ops(&fake);
    size_t body_capacity = 0U;
    char *body =
        dup_body("{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}", &body_capacity);
    const web_send_create_outcome_t outcome = web_send_create_handle(body, body_capacity, &ops);
    free(body);
    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_EQ_INT(APP_ERROR_USB_NOT_READY, outcome.detail);
}

static void test_create_backend_internal_error(void) {
    fake_send_backend_t fake = {0};
    fake.submit_result = APP_ERROR_INVALID_ARGUMENT;
    const web_send_ops_t ops = fake_ops(&fake);
    size_t body_capacity = 0U;
    char *body =
        dup_body("{\"source\":\"first\",\"keyPressMs\":0,\"interKeyMs\":15}", &body_capacity);
    const web_send_create_outcome_t outcome = web_send_create_handle(body, body_capacity, &ops);
    free(body);
    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_INTERNAL, outcome.result);
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT, outcome.detail);
}

static void test_create_invalid_ops_or_null_body(void) {
    fake_send_backend_t fake = {0};
    const web_send_ops_t incomplete_ops = {.context = &fake};
    size_t body_capacity = 0U;
    char *body =
        dup_body("{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}", &body_capacity);
    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_INTERNAL,
                      web_send_create_handle(body, body_capacity, &incomplete_ops).result);
    free(body);

    const web_send_ops_t ops = fake_ops(&fake);
    TEST_CHECK_EQ_INT(WEB_SEND_CREATE_INVALID_BODY, web_send_create_handle(NULL, 0U, &ops).result);
}

static void test_get_never_sent(void) {
    fake_send_backend_t fake = {0};
    fake.status_to_return = (macro_execution_status_t){.state = EXECUTION_IDLE, .available = true};
    const web_send_ops_t ops = fake_ops(&fake);
    const web_send_get_outcome_t outcome = web_send_get_handle(&ops, 0U);
    TEST_CHECK_EQ_INT(WEB_SEND_GET_NEVER_SENT, outcome.result);
}

static void test_get_running_reports_cached_duration(void) {
    fake_send_backend_t fake = {0};
    app_uuid_t id = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_generate(&id));
    fake.status_to_return = (macro_execution_status_t){
        .state = EXECUTION_RUNNING,
        .available = true,
        .execution_id = id,
        .action_index = 2U,
        .action_count = 9U,
        .cancellation_requested = false,
        .error = APP_ERROR_NONE,
        .release_error = APP_ERROR_NONE,
    };
    const web_send_ops_t ops = fake_ops(&fake);
    const web_send_get_outcome_t outcome = web_send_get_handle(&ops, 214U);
    TEST_CHECK_EQ_INT(WEB_SEND_GET_OK, outcome.result);
    TEST_CHECK_EQ_STRING(id.value, outcome.status.id);
    TEST_CHECK_EQ_STRING("running", outcome.status.state);
    TEST_CHECK_EQ_U64(2U, outcome.status.action_index);
    TEST_CHECK_EQ_U64(9U, outcome.status.action_count);
    TEST_CHECK_EQ_U64(214U, outcome.status.estimated_duration_ms);
    TEST_CHECK_EQ_STRING("", outcome.status.error);
    TEST_CHECK_EQ_STRING("", outcome.status.release_error);
}

static void test_get_reports_terminal_errors(void) {
    fake_send_backend_t fake = {0};
    fake.status_to_return = (macro_execution_status_t){
        .state = EXECUTION_FAILED,
        .available = true,
        .error = APP_ERROR_USB_NOT_READY,
        .release_error = APP_ERROR_TIMEOUT,
    };
    const web_send_ops_t ops = fake_ops(&fake);
    const web_send_get_outcome_t outcome = web_send_get_handle(&ops, 0U);
    TEST_CHECK_EQ_INT(WEB_SEND_GET_OK, outcome.result);
    TEST_CHECK_EQ_STRING("failed", outcome.status.state);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_USB_NOT_READY), outcome.status.error);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_TIMEOUT), outcome.status.release_error);
}

static void test_get_unavailable_backend(void) {
    fake_send_backend_t fake = {0};
    fake.status_to_return =
        (macro_execution_status_t){.state = EXECUTION_RUNNING, .available = false};
    const web_send_ops_t ops = fake_ops(&fake);
    TEST_CHECK_EQ_INT(WEB_SEND_GET_INTERNAL, web_send_get_handle(&ops, 0U).result);
}

static void test_get_unavailable_release_fault_remains_visible(void) {
    fake_send_backend_t fake = {0};
    fake.status_to_return = (macro_execution_status_t){
        .state = EXECUTION_FAILED,
        .available = false,
        .error = APP_ERROR_IO,
        .release_error = APP_ERROR_USB_NOT_READY,
    };
    const web_send_ops_t ops = fake_ops(&fake);
    const web_send_get_outcome_t outcome = web_send_get_handle(&ops, 0U);
    TEST_CHECK_EQ_INT(WEB_SEND_GET_OK, outcome.result);
    TEST_CHECK_EQ_STRING("failed", outcome.status.state);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_IO), outcome.status.error);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_USB_NOT_READY),
                         outcome.status.release_error);
}

static void test_cancel_never_sent(void) {
    fake_send_backend_t fake = {0};
    fake.status_to_return = (macro_execution_status_t){.state = EXECUTION_IDLE};
    const web_send_ops_t ops = fake_ops(&fake);
    TEST_CHECK_EQ_INT(WEB_SEND_CANCEL_NEVER_SENT, web_send_cancel_handle(&ops));
    TEST_CHECK(!fake.cancel_called);
}

static void test_cancel_accepted_and_idempotent(void) {
    static const app_error_code_t idempotent_results[] = {
        APP_ERROR_NONE,
        APP_ERROR_CONFLICT,
        APP_ERROR_NOT_FOUND,
    };
    for (size_t index = 0U; index < sizeof(idempotent_results) / sizeof(idempotent_results[0]);
         ++index) {
        fake_send_backend_t fake = {0};
        fake.status_to_return = (macro_execution_status_t){.state = EXECUTION_RUNNING};
        fake.cancel_result = idempotent_results[index];
        const web_send_ops_t ops = fake_ops(&fake);
        TEST_CHECK_EQ_INT(WEB_SEND_CANCEL_ACCEPTED, web_send_cancel_handle(&ops));
        TEST_CHECK(fake.cancel_called);
    }
}

static void test_cancel_internal_error(void) {
    fake_send_backend_t fake = {0};
    fake.status_to_return = (macro_execution_status_t){.state = EXECUTION_RUNNING};
    fake.cancel_result = APP_ERROR_INTERNAL;
    const web_send_ops_t ops = fake_ops(&fake);
    TEST_CHECK_EQ_INT(WEB_SEND_CANCEL_INTERNAL, web_send_cancel_handle(&ops));
}

static void test_accepted_json_exact_shape(void) {
    const web_send_accepted_t accepted = {
        .id = "550e8400-e29b-41d4-a716-446655440000",
        .action_count = 9U,
        .estimated_duration_ms = 214U,
    };
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_send_accepted_json(&accepted, &json));
    cJSON *root = cJSON_Parse(json);
    TEST_CHECK(root != NULL);
    TEST_CHECK_EQ_STRING("550e8400-e29b-41d4-a716-446655440000",
                         cJSON_GetObjectItemCaseSensitive(root, "id")->valuestring);
    TEST_CHECK_EQ_STRING("running", cJSON_GetObjectItemCaseSensitive(root, "state")->valuestring);
    TEST_CHECK_EQ_U64(9U,
                      (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "actionCount")->valuedouble);
    TEST_CHECK_EQ_U64(
        214U, (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "estimatedDurationMs")->valuedouble);
    cJSON_Delete(root);
    free(json);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_send_accepted_json(NULL, &json));
}

static void test_status_json_exact_shape(void) {
    const web_send_status_view_t status = {
        .id = "550e8400-e29b-41d4-a716-446655440000",
        .state = "completed",
        .action_index = 9U,
        .action_count = 9U,
        .estimated_duration_ms = 214U,
        .cancellation_requested = false,
        .error = "",
        .release_error = "",
    };
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_send_status_json(&status, &json));
    cJSON *root = cJSON_Parse(json);
    TEST_CHECK(root != NULL);
    TEST_CHECK_EQ_STRING("completed", cJSON_GetObjectItemCaseSensitive(root, "state")->valuestring);
    TEST_CHECK(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(root, "cancellationRequested")));
    TEST_CHECK_EQ_STRING("", cJSON_GetObjectItemCaseSensitive(root, "error")->valuestring);
    TEST_CHECK_EQ_STRING("", cJSON_GetObjectItemCaseSensitive(root, "releaseError")->valuestring);
    cJSON_Delete(root);
    free(json);
}

int main(void) {
    test_create_success();
    test_create_rejects_malformed_bodies();
    test_create_parse_error_reports_location();
    test_create_busy();
    test_create_usb_not_ready();
    test_create_backend_internal_error();
    test_create_invalid_ops_or_null_body();
    test_get_never_sent();
    test_get_running_reports_cached_duration();
    test_get_reports_terminal_errors();
    test_get_unavailable_backend();
    test_get_unavailable_release_fault_remains_visible();
    test_cancel_never_sent();
    test_cancel_accepted_and_idempotent();
    test_cancel_internal_error();
    test_accepted_json_exact_shape();
    test_status_json_exact_shape();
    puts("web send tests passed");
    return EXIT_SUCCESS;
}
