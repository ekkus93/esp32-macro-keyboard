#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"
#include "storage_repository.h"
#include "test_assert.h"
#include "web_execution_submit.h"

#define SET_ID "11111111-1111-4111-8111-111111111111"
#define MACRO_ID "22222222-2222-4222-8222-222222222222"
#define EXECUTION_ID "55555555-5555-4555-8555-555555555555"

typedef struct {
    app_error_code_t read_result;
    app_error_code_t compile_result;
    app_error_code_t submit_result;
    uint32_t revision;
    size_t compile_calls;
    size_t submit_calls;
    size_t free_calls;
} fixture_t;

static app_uuid_t uuid(const char *text) {
    app_uuid_t value = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &value));
    return value;
}

static app_error_code_t read_macro(void *context, const storage_macro_location_t *location,
                                   const app_uuid_t *macro_id, macro_t *out_macro) {
    fixture_t *fixture = context;
    TEST_CHECK(location->scope == MACRO_SCOPE_SET);
    TEST_CHECK_EQ_STRING(MACRO_ID, macro_id->value);
    if (fixture->read_result != APP_ERROR_NONE) {
        return fixture->read_result;
    }
    *out_macro = (macro_t){
        .schema_version = 1U,
        .id = uuid(MACRO_ID),
        .revision = fixture->revision,
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = uuid(SET_ID),
        .source_length = 1U,
        .key_press_ms = 8U,
        .inter_key_ms = 15U,
    };
    memcpy(out_macro->name, "M", 2U);
    out_macro->source = malloc(2U);
    TEST_CHECK(out_macro->source != NULL);
    memcpy(out_macro->source, "a", 2U);
    return APP_ERROR_NONE;
}

static app_error_code_t read_procedure(void *context, const app_uuid_t *set_id,
                                       const app_uuid_t *procedure_id, procedure_t *out_procedure) {
    (void)context;
    (void)set_id;
    (void)procedure_id;
    (void)out_procedure;
    return APP_ERROR_NOT_FOUND;
}

static app_error_code_t compile_macro(void *context, const char *source, size_t source_length,
                                      const macro_compile_options_t *options,
                                      macro_plan_t *out_plan, macro_parse_error_t *out_error) {
    fixture_t *fixture = context;
    TEST_CHECK_EQ_STRING("a", source);
    TEST_CHECK_EQ_U64(1U, source_length);
    TEST_CHECK_EQ_U64(8U, options->key_press_ms);
    ++fixture->compile_calls;
    if (fixture->compile_result != APP_ERROR_NONE) {
        out_error->code = fixture->compile_result;
        out_error->line = 2U;
        return fixture->compile_result;
    }
    out_plan->actions = calloc(2U, sizeof(*out_plan->actions));
    TEST_CHECK(out_plan->actions != NULL);
    out_plan->action_count = 2U;
    out_plan->estimated_duration_ms = 38U;
    return APP_ERROR_NONE;
}

static void free_plan(void *context, macro_plan_t *plan) {
    fixture_t *fixture = context;
    ++fixture->free_calls;
    free(plan->actions);
    *plan = (macro_plan_t){0};
}

static app_error_code_t generate_uuid(void *context, app_uuid_t *out_uuid) {
    (void)context;
    *out_uuid = uuid(EXECUTION_ID);
    return APP_ERROR_NONE;
}

static app_error_code_t submit(void *context, macro_execution_request_t *request) {
    fixture_t *fixture = context;
    ++fixture->submit_calls;
    TEST_CHECK_EQ_STRING(EXECUTION_ID, request->execution_id.value);
    TEST_CHECK_EQ_U64(2U, request->plan.action_count);
    if (fixture->submit_result == APP_ERROR_NONE) {
        request->plan = (macro_plan_t){0};
    }
    return fixture->submit_result;
}

static web_execution_ops_t operations(fixture_t *fixture) {
    return (web_execution_ops_t){
        .context = fixture,
        .macro_read = read_macro,
        .procedure_read = read_procedure,
        .compile = compile_macro,
        .plan_free = free_plan,
        .uuid_generate = generate_uuid,
        .submit = submit,
    };
}

static web_execution_submit_request_t request(void) {
    return (web_execution_submit_request_t){
        .set_id = uuid(SET_ID),
        .macro_id = uuid(MACRO_ID),
        .macro_revision = 7U,
    };
}

static void test_success_and_failures(void) {
    web_execution_accepted_t accepted = {0};
    macro_parse_error_t parse_error = {0};
    const web_execution_submit_request_t submission = request();

    fixture_t fixture = {.revision = 7U};
    web_execution_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, web_execution_submit_persisted(&submission, &ops, &accepted, &parse_error));
    TEST_CHECK_EQ_STRING(EXECUTION_ID, accepted.execution_id.value);
    TEST_CHECK_EQ_U64(2U, accepted.action_count);
    TEST_CHECK_EQ_U64(0U, fixture.free_calls);

    fixture = (fixture_t){.revision = 8U};
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, web_execution_submit_persisted(
                                                 &submission, &ops, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(0U, fixture.compile_calls);

    fixture = (fixture_t){.revision = 7U, .compile_result = APP_ERROR_MACRO_SYNTAX};
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_MACRO_SYNTAX, web_execution_submit_persisted(
                                                     &submission, &ops, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(2U, parse_error.line);

    fixture = (fixture_t){.revision = 7U, .submit_result = APP_ERROR_USB_NOT_READY};
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_USB_NOT_READY, web_execution_submit_persisted(
                                                      &submission, &ops, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(1U, fixture.free_calls);

    fixture = (fixture_t){.revision = 7U, .submit_result = APP_ERROR_EXECUTOR_BUSY};
    ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTOR_BUSY, web_execution_submit_persisted(
                                                      &submission, &ops, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(1U, fixture.free_calls);
}

int main(void) {
    test_success_and_failures();
    return 0;
}
