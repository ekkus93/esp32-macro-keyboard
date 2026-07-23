#include "executor_test_fixture.h"

#include <stdlib.h>
#include <string.h>

#include "test_assert.h"

#define VALID_EXECUTION_ID "123e4567-e89b-42d3-a456-426614174000"
#define VALID_SET_ID "123e4567-e89b-42d3-b456-426614174001"
#define VALID_MACRO_ID "123e4567-e89b-42d3-8456-426614174002"

bool app_uuid_is_valid_string(const char *text)
{
    if (text == NULL || strlen(text) != 36U || text[8] != '-' || text[13] != '-' ||
        text[18] != '-' || text[23] != '-' || text[14] != '4') {
        return false;
    }
    for (size_t index = 0U; index < 36U; ++index) {
        if (index == 8U || index == 13U || index == 18U || index == 23U) {
            continue;
        }
        const char value = text[index];
        const bool hexadecimal = (value >= '0' && value <= '9') ||
                                 (value >= 'a' && value <= 'f');
        if (!hexadecimal) {
            return false;
        }
    }
    return text[19] == '8' || text[19] == '9' || text[19] == 'a' || text[19] == 'b';
}

void executor_fake_reset(executor_fake_t *fake)
{
    TEST_CHECK(fake != NULL);
    memset(fake, 0, sizeof(*fake));
    fake->usb_is_ready = true;
    fake->queue_result = true;
    fake_call_log_reset(&fake->calls);
}

void executor_clear_injected_failure(executor_fake_t *fake)
{
    TEST_CHECK(fake != NULL);
    fake->calls.failure_name = NULL;
    fake->calls.failure_occurrence = 0U;
    fake->calls.failure_seen = 0U;
}

static bool fake_lock(void *context)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(!fake->locked);
    if (fake_call_log_record(&fake->calls, "lock", 0U, 0U)) {
        return false;
    }
    fake->locked = true;
    return true;
}

static bool fake_unlock(void *context)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(fake->locked);
    if (fake->engine != NULL && fake->engine->status.state == EXECUTION_RUNNING) {
        TEST_CHECK(fake->snapshot_count < EXECUTOR_STATUS_SNAPSHOT_CAPACITY);
        fake->snapshots[fake->snapshot_count++] = fake->engine->status;
    }
    const bool fail = fake_call_log_record(&fake->calls, "unlock", 0U, 0U);
    fake->locked = false;
    return !fail;
}

static bool fake_queue_send(void *context, const macro_execution_request_t *request)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(request != NULL);
    if (fake_call_log_record(&fake->calls, "queue_send", request->plan.action_count, 0U) ||
        !fake->queue_result || fake->has_queued) {
        return false;
    }
    fake->queued = *request;
    fake->has_queued = true;
    return true;
}

static void fake_notify(void *context)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    (void)fake_call_log_record(&fake->calls, "notify", 0U, 0U);
}

static uint32_t fake_now_ms(void *context)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    (void)fake_call_log_record(&fake->calls, "now", fake->now_ms, 0U);
    return fake->now_ms;
}

static app_error_code_t fake_wait_ms(void *context, uint32_t milliseconds)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    ++fake->wait_count;
    (void)fake_call_log_record(&fake->calls, "wait", milliseconds, fake->wait_count);
    fake->now_ms += milliseconds;
    fake->now_ms += fake->extra_advance_on_wait_ms;
    if (fake->cancel_on_wait != 0U && fake->wait_count == fake->cancel_on_wait) {
        TEST_CHECK(fake->engine != NULL);
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_cancel(fake->engine));
    }
    if (fake->wait_failure_on != 0U && fake->wait_count == fake->wait_failure_on) {
        return fake->wait_failure_result;
    }
    return APP_ERROR_NONE;
}

static bool fake_usb_ready(void *context)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    (void)fake_call_log_record(&fake->calls, "usb_ready", fake->usb_is_ready ? 1U : 0U, 0U);
    return fake->usb_is_ready;
}

static app_error_code_t fake_press(void *context, uint8_t modifiers, uint8_t usage)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    (void)fake_call_log_record(&fake->calls, "press", modifiers, usage);
    return fake->press_result;
}

static app_error_code_t fake_release(void *context)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    (void)fake_call_log_record(&fake->calls, "release", fake->release_index, 0U);
    app_error_code_t result = APP_ERROR_NONE;
    if (fake->release_index < fake->release_result_count) {
        result = fake->release_results[fake->release_index];
    }
    ++fake->release_index;
    return result;
}

static executor_fake_t *free_context;

static void fake_plan_free(macro_plan_t *plan)
{
    TEST_CHECK(plan != NULL);
    TEST_CHECK(free_context != NULL);
    ++free_context->free_count;
    free(plan->actions);
    plan->actions = NULL;
    plan->action_count = 0U;
    plan->estimated_duration_ms = 0U;
}

macro_executor_ops_t executor_make_ops(executor_fake_t *fake)
{
    return (macro_executor_ops_t){
        .context = fake,
        .lock = fake_lock,
        .unlock = fake_unlock,
        .queue_send = fake_queue_send,
        .notify_executor = fake_notify,
        .now_ms = fake_now_ms,
        .wait_ms = fake_wait_ms,
        .usb_ready = fake_usb_ready,
        .usb_press = fake_press,
        .usb_release_all = fake_release,
        .plan_free = fake_plan_free,
    };
}

void executor_init_engine(macro_executor_engine_t *engine, executor_fake_t *fake)
{
    TEST_CHECK(engine != NULL);
    const macro_executor_ops_t ops = executor_make_ops(fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_init(engine, &ops));
    fake->engine = engine;
    free_context = fake;
}

macro_execution_request_t executor_make_request(size_t action_count)
{
    macro_execution_request_t request = {0};
    memcpy(request.execution_id.value, VALID_EXECUTION_ID, sizeof(VALID_EXECUTION_ID));
    memcpy(request.set_id.value, VALID_SET_ID, sizeof(VALID_SET_ID));
    memcpy(request.macro_id.value, VALID_MACRO_ID, sizeof(VALID_MACRO_ID));
    request.macro_revision = 1U;
    request.key_press_ms = 8U;
    request.inter_key_ms = 15U;
    const size_t allocation_count = action_count == 0U ? 1U : action_count;
    request.plan.actions = calloc(allocation_count, sizeof(*request.plan.actions));
    TEST_CHECK(request.plan.actions != NULL);
    request.plan.action_count = action_count;
    request.plan.estimated_duration_ms = 100U;
    return request;
}

void executor_free_unowned_request(macro_execution_request_t *request)
{
    TEST_CHECK(request != NULL);
    free(request->plan.actions);
    request->plan.actions = NULL;
    request->plan.action_count = 0U;
}

static macro_execution_request_t take_queued(executor_fake_t *fake)
{
    TEST_CHECK(fake != NULL);
    TEST_CHECK(fake->has_queued);
    macro_execution_request_t request = fake->queued;
    memset(&fake->queued, 0, sizeof(fake->queued));
    fake->has_queued = false;
    return request;
}

app_error_code_t executor_execute_queued(macro_executor_engine_t *engine,
                                         executor_fake_t *fake)
{
    macro_execution_request_t request = take_queued(fake);
    return macro_executor_engine_execute(engine, &request);
}

void executor_submit_single_key(macro_executor_engine_t *engine,
                                executor_fake_t *fake,
                                uint8_t usage)
{
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = usage};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(engine, &request));
    TEST_CHECK(request.plan.actions == NULL);
    TEST_CHECK(fake->has_queued);
}

void executor_assert_terminal(const macro_executor_engine_t *engine,
                              execution_state_t state,
                              app_error_code_t error,
                              bool expected_busy)
{
    TEST_CHECK(engine != NULL);
    TEST_CHECK(engine->status.state == state);
    TEST_CHECK_APP_ERROR(error, engine->status.error);
    TEST_CHECK(engine->busy == expected_busy);
    TEST_CHECK(!engine->cancellation_requested);
}

void executor_assert_relevant_call(const executor_fake_t *fake,
                                   size_t ordinal,
                                   const char *name,
                                   uint64_t argument0)
{
    size_t seen = 0U;
    for (size_t index = 0U; index < fake->calls.call_count; ++index) {
        const fake_call_t *call = fake_call_log_at(&fake->calls, index);
        if (strcmp(call->name, "press") != 0 && strcmp(call->name, "wait") != 0 &&
            strcmp(call->name, "release") != 0) {
            continue;
        }
        if (seen == ordinal) {
            TEST_CHECK_EQ_STRING(name, call->name);
            TEST_CHECK_EQ_U64(argument0, call->argument0);
            return;
        }
        ++seen;
    }
    test_fail(__FILE__, __LINE__, "relevant executor call exists", "missing call");
}
