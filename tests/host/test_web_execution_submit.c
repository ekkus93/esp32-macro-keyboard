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
#define OTHER_SET_ID "11111111-1111-4111-8111-999999999999"
#define MACRO_ID "22222222-2222-4222-8222-222222222222"
#define OTHER_STEP_ID "44444444-4444-4444-8444-999999999999"
#define EXECUTION_ID "55555555-5555-4555-8555-555555555555"

typedef struct {
    app_error_code_t read_result;
    app_error_code_t compile_result;
    app_error_code_t uuid_result;
    app_error_code_t submit_result;
    uint32_t revision;
    bool macro_set_matches;
    size_t read_calls;
    size_t compile_calls;
    size_t uuid_calls;
    size_t submit_calls;
    size_t free_calls;
    macro_plan_t accepted_plan;
} fixture_t;

static app_uuid_t uuid(const char *text) {
    app_uuid_t value = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &value));
    return value;
}

static fixture_t fixture_defaults(void) {
    return (fixture_t){
        .revision = 7U,
        .macro_set_matches = true,
    };
}

static app_error_code_t read_macro(void *context, const app_uuid_t *set_id,
                                   const app_uuid_t *macro_id, macro_t *out_macro) {
    fixture_t *fixture = context;
    ++fixture->read_calls;
    TEST_CHECK_EQ_STRING(MACRO_ID, macro_id->value);
    TEST_CHECK_EQ_STRING(SET_ID, set_id->value);
    if (fixture->read_result != APP_ERROR_NONE) {
        return fixture->read_result;
    }
    *out_macro = (macro_t){
        .schema_version = 1U,
        .id = uuid(MACRO_ID),
        .revision = fixture->revision,
        .set_id = uuid(fixture->macro_set_matches ? SET_ID : OTHER_SET_ID),
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
    fixture_t *fixture = context;
    ++fixture->uuid_calls;
    if (fixture->uuid_result != APP_ERROR_NONE) {
        return fixture->uuid_result;
    }
    *out_uuid = uuid(EXECUTION_ID);
    return APP_ERROR_NONE;
}

static app_error_code_t submit(void *context, macro_execution_request_t *request) {
    fixture_t *fixture = context;
    ++fixture->submit_calls;
    TEST_CHECK_EQ_STRING(EXECUTION_ID, request->execution_id.value);
    TEST_CHECK_EQ_STRING(SET_ID, request->set_id.value);
    TEST_CHECK_EQ_STRING(MACRO_ID, request->macro_id.value);
    TEST_CHECK_EQ_U64(7U, request->macro_revision);
    TEST_CHECK_EQ_U64(2U, request->plan.action_count);
    if (fixture->submit_result == APP_ERROR_NONE) {
        fixture->accepted_plan = request->plan;
        request->plan = (macro_plan_t){0};
    }
    return fixture->submit_result;
}

static void release_accepted_plan(fixture_t *fixture) {
    free(fixture->accepted_plan.actions);
    fixture->accepted_plan = (macro_plan_t){0};
}

static web_execution_ops_t operations(fixture_t *fixture) {
    return (web_execution_ops_t){
        .context = fixture,
        .macro_read = read_macro,
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

static app_error_code_t submit_fixture(fixture_t *fixture,
                                       const web_execution_submit_request_t *submission,
                                       web_execution_accepted_t *accepted,
                                       macro_parse_error_t *parse_error) {
    const web_execution_ops_t ops = operations(fixture);
    return web_execution_submit_persisted(submission, &ops, accepted, parse_error);
}

static void test_success_and_global_fallback(void) {
    web_execution_accepted_t accepted = {0};
    macro_parse_error_t parse_error = {0};
    const web_execution_submit_request_t submission = request();

    fixture_t fixture = fixture_defaults();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         submit_fixture(&fixture, &submission, &accepted, &parse_error));
    TEST_CHECK_EQ_STRING(EXECUTION_ID, accepted.execution_id.value);
    TEST_CHECK_EQ_U64(2U, accepted.action_count);
    TEST_CHECK_EQ_U64(1U, fixture.read_calls);
    TEST_CHECK_EQ_U64(0U, fixture.free_calls);
    TEST_CHECK(fixture.accepted_plan.actions != NULL);
    release_accepted_plan(&fixture);
}

static void test_pre_compile_failures(void) {
    web_execution_accepted_t accepted = {0};
    macro_parse_error_t parse_error = {0};
    const web_execution_submit_request_t submission = request();

    fixture_t fixture = fixture_defaults();
    fixture.revision = 8U;
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         submit_fixture(&fixture, &submission, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(0U, fixture.compile_calls);
    TEST_CHECK_EQ_U64(0U, fixture.free_calls);

    fixture = fixture_defaults();
    fixture.read_result = APP_ERROR_NOT_FOUND;
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         submit_fixture(&fixture, &submission, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(1U, fixture.read_calls);
    TEST_CHECK_EQ_U64(0U, fixture.compile_calls);

    fixture = fixture_defaults();
    fixture.macro_set_matches = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         submit_fixture(&fixture, &submission, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(0U, fixture.compile_calls);
}

static void test_post_compile_cleanup_matrix(void) {
    web_execution_accepted_t accepted = {0};
    macro_parse_error_t parse_error = {0};
    const web_execution_submit_request_t submission = request();

    fixture_t fixture = fixture_defaults();
    fixture.compile_result = APP_ERROR_MACRO_SYNTAX;
    TEST_CHECK_APP_ERROR(APP_ERROR_MACRO_SYNTAX,
                         submit_fixture(&fixture, &submission, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(2U, parse_error.line);
    TEST_CHECK_EQ_U64(0U, fixture.free_calls);
    TEST_CHECK_EQ_U64(0U, fixture.uuid_calls);

    fixture = fixture_defaults();
    fixture.uuid_result = APP_ERROR_INTERNAL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         submit_fixture(&fixture, &submission, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(1U, fixture.free_calls);
    TEST_CHECK_EQ_U64(0U, fixture.submit_calls);

    static const app_error_code_t submit_failures[] = {
        APP_ERROR_USB_NOT_READY,
        APP_ERROR_EXECUTOR_BUSY,
        APP_ERROR_INTERNAL,
    };
    for (size_t index = 0U; index < sizeof(submit_failures) / sizeof(submit_failures[0]); ++index) {
        fixture = fixture_defaults();
        fixture.submit_result = submit_failures[index];
        TEST_CHECK_APP_ERROR(submit_failures[index],
                             submit_fixture(&fixture, &submission, &accepted, &parse_error));
        TEST_CHECK_EQ_U64(1U, fixture.free_calls);
        TEST_CHECK_EQ_U64(1U, fixture.submit_calls);
        TEST_CHECK(fixture.accepted_plan.actions == NULL);
    }
}

static void test_argument_validation(void) {
    fixture_t fixture = fixture_defaults();
    web_execution_ops_t ops = operations(&fixture);
    web_execution_submit_request_t submission = request();
    web_execution_accepted_t accepted = {0};
    macro_parse_error_t parse_error = {0};

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_execution_submit_persisted(NULL, &ops, &accepted, &parse_error));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_execution_submit_persisted(&submission, NULL, &accepted, &parse_error));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_execution_submit_persisted(&submission, &ops, NULL, &parse_error));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_execution_submit_persisted(&submission, &ops, &accepted, NULL));

    submission.macro_revision = 0U;
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_execution_submit_persisted(&submission, &ops, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(0U, fixture.read_calls);

    submission = request();
    ops.submit = NULL;
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_execution_submit_persisted(&submission, &ops, &accepted, &parse_error));
    TEST_CHECK_EQ_U64(0U, fixture.read_calls);
}

int main(void) {
    test_success_and_global_fallback();
    test_pre_compile_failures();
    test_post_compile_cleanup_matrix();
    test_argument_validation();
    return 0;
}
