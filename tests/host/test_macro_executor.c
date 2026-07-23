#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fake_call_log.h"
#include "macro_executor_engine.h"
#include "test_assert.h"

#define VALID_EXECUTION_ID "123e4567-e89b-42d3-a456-426614174000"
#define VALID_SET_ID "123e4567-e89b-42d3-b456-426614174001"
#define VALID_MACRO_ID "123e4567-e89b-42d3-8456-426614174002"

typedef struct {
    bool locked;
    bool usb_is_ready;
    bool queue_result;
    macro_execution_request_t queued;
    bool has_queued;
    uint32_t now_ms;
    size_t wait_count;
    size_t cancel_on_wait;
    macro_executor_engine_t *engine;
    app_error_code_t wait_result;
    app_error_code_t press_result;
    app_error_code_t release_results[8U];
    size_t release_result_count;
    size_t release_index;
    size_t free_count;
    fake_call_log_t calls;
} executor_fake_t;

bool app_uuid_is_valid_string(const char *text)
{
    if (text == NULL || strlen(text) != 36U || text[8] != '-' || text[13] != '-' ||
        text[18] != '-' || text[23] != '-' || text[14] != '4') {
        return false;
    }
    return text[19] == '8' || text[19] == '9' || text[19] == 'a' || text[19] == 'b';
}

static void fake_reset(executor_fake_t *fake)
{
    memset(fake, 0, sizeof(*fake));
    fake->usb_is_ready = true;
    fake->queue_result = true;
    fake_call_log_reset(&fake->calls);
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
    const bool fail = fake_call_log_record(&fake->calls, "unlock", 0U, 0U);
    fake->locked = false;
    return !fail;
}

static bool fake_queue_send(void *context, const macro_execution_request_t *request)
{
    executor_fake_t *fake = context;
    TEST_CHECK(fake != NULL);
    TEST_CHECK(request != NULL);
    (void)fake_call_log_record(&fake->calls, "queue_send", request->plan.action_count, 0U);
    if (!fake->queue_result || fake->has_queued) {
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
    if (fake->cancel_on_wait != 0U && fake->wait_count == fake->cancel_on_wait) {
        TEST_CHECK(fake->engine != NULL);
        TEST_CHECK(macro_executor_engine_cancel(fake->engine) == APP_ERROR_NONE);
    }
    return fake->wait_result;
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

static macro_executor_ops_t make_ops(executor_fake_t *fake)
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

static macro_execution_request_t make_request(size_t action_count)
{
    macro_execution_request_t request = {0};
    memcpy(request.execution_id.value, VALID_EXECUTION_ID, sizeof(VALID_EXECUTION_ID));
    memcpy(request.set_id.value, VALID_SET_ID, sizeof(VALID_SET_ID));
    memcpy(request.macro_id.value, VALID_MACRO_ID, sizeof(VALID_MACRO_ID));
    request.macro_revision = 1U;
    request.key_press_ms = 8U;
    request.inter_key_ms = 15U;
    request.plan.actions = calloc(action_count, sizeof(*request.plan.actions));
    TEST_CHECK(request.plan.actions != NULL);
    request.plan.action_count = action_count;
    request.plan.estimated_duration_ms = 100U;
    return request;
}

static void init_engine(macro_executor_engine_t *engine, executor_fake_t *fake)
{
    TEST_CHECK(engine != NULL);
    const macro_executor_ops_t ops = make_ops(fake);
    TEST_CHECK(macro_executor_engine_init(engine, &ops) == APP_ERROR_NONE);
    fake->engine = engine;
    free_context = fake;
}

static void test_validation_and_ownership(void)
{
    executor_fake_t fake;
    fake_reset(&fake);
    macro_executor_engine_t engine;
    init_engine(&engine, &fake);
    TEST_CHECK(macro_executor_engine_submit(&engine, NULL) == APP_ERROR_INVALID_ARGUMENT);

    macro_execution_request_t request = make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    fake.usb_is_ready = false;
    TEST_CHECK(macro_executor_engine_submit(&engine, &request) == APP_ERROR_USB_NOT_READY);
    TEST_CHECK(request.plan.actions != NULL);
    fake.usb_is_ready = true;
    fake.queue_result = false;
    TEST_CHECK(macro_executor_engine_submit(&engine, &request) == APP_ERROR_INTERNAL);
    TEST_CHECK(request.plan.actions != NULL);
    TEST_CHECK(!engine.busy);

    fake.queue_result = true;
    TEST_CHECK(macro_executor_engine_submit(&engine, &request) == APP_ERROR_NONE);
    TEST_CHECK(request.plan.actions == NULL);
    TEST_CHECK(fake.has_queued);
    TEST_CHECK(engine.busy);

    macro_execution_request_t second = make_request(1U);
    TEST_CHECK(macro_executor_engine_submit(&engine, &second) == APP_ERROR_EXECUTOR_BUSY);
    TEST_CHECK(second.plan.actions != NULL);
    free(second.plan.actions);
    second.plan.actions = NULL;

    TEST_CHECK(macro_executor_engine_execute(&engine, &fake.queued) == APP_ERROR_NONE);
    TEST_CHECK_EQ_U64(1U, fake.free_count);
    TEST_CHECK(!engine.busy);
}

static void test_successful_action_order(void)
{
    executor_fake_t fake;
    fake_reset(&fake);
    macro_executor_engine_t engine;
    init_engine(&engine, &fake);
    macro_execution_request_t request = make_request(2U);
    request.plan.actions[0] = (macro_action_t){
        .type = MACRO_ACTION_CHORD,
        .modifiers = 2U,
        .usage = 4U,
    };
    request.plan.actions[1] = (macro_action_t){
        .type = MACRO_ACTION_DELAY,
        .delay_ms = 25U,
    };
    TEST_CHECK(macro_executor_engine_submit(&engine, &request) == APP_ERROR_NONE);
    TEST_CHECK(macro_executor_engine_execute(&engine, &fake.queued) == APP_ERROR_NONE);
    const macro_execution_status_t status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(status.state == EXECUTION_COMPLETED);
    TEST_CHECK(status.error == APP_ERROR_NONE);
    TEST_CHECK(status.release_error == APP_ERROR_NONE);
    TEST_CHECK_EQ_U64(2U, status.action_count);
    TEST_CHECK_EQ_U64(1U, status.action_index);
    TEST_CHECK_EQ_U64(2U, fake.release_index);
    TEST_CHECK_EQ_U64(1U, fake.free_count);

    size_t press_index = SIZE_MAX;
    size_t first_release_index = SIZE_MAX;
    for (size_t index = 0U; index < fake.calls.call_count; ++index) {
        const fake_call_t *call = fake_call_log_at(&fake.calls, index);
        if (strcmp(call->name, "press") == 0) {
            press_index = index;
        } else if (strcmp(call->name, "release") == 0 && first_release_index == SIZE_MAX) {
            first_release_index = index;
        }
    }
    TEST_CHECK(press_index != SIZE_MAX);
    TEST_CHECK(first_release_index != SIZE_MAX);
    TEST_CHECK(press_index < first_release_index);
}

static void test_cancellation_and_timeout(void)
{
    executor_fake_t fake;
    fake_reset(&fake);
    macro_executor_engine_t engine;
    init_engine(&engine, &fake);
    macro_execution_request_t request = make_request(1U);
    request.plan.actions[0] = (macro_action_t){
        .type = MACRO_ACTION_DELAY,
        .delay_ms = 100U,
    };
    fake.cancel_on_wait = 2U;
    TEST_CHECK(macro_executor_engine_submit(&engine, &request) == APP_ERROR_NONE);
    TEST_CHECK(macro_executor_engine_execute(&engine, &fake.queued) ==
               APP_ERROR_EXECUTION_CANCELLED);
    TEST_CHECK(macro_executor_engine_get_status(&engine).state == EXECUTION_CANCELLED);
    TEST_CHECK(fake.wait_count == 2U);
    TEST_CHECK(!engine.busy);

    fake_reset(&fake);
    init_engine(&engine, &fake);
    request = make_request(1U);
    request.plan.actions[0] = (macro_action_t){
        .type = MACRO_ACTION_DELAY,
        .delay_ms = 100U,
    };
    request.plan.estimated_duration_ms = 0U;
    fake.now_ms = UINT32_MAX - 5U;
    TEST_CHECK(macro_executor_engine_submit(&engine, &request) == APP_ERROR_NONE);
    fake.wait_result = APP_ERROR_TIMEOUT;
    TEST_CHECK(macro_executor_engine_execute(&engine, &fake.queued) == APP_ERROR_TIMEOUT);
    TEST_CHECK(macro_executor_engine_get_status(&engine).state == EXECUTION_FAILED);
}

static void test_release_errors_and_idle_cancel(void)
{
    executor_fake_t fake;
    fake_reset(&fake);
    macro_executor_engine_t engine;
    init_engine(&engine, &fake);
    TEST_CHECK(macro_executor_engine_cancel(&engine) == APP_ERROR_NOT_FOUND);

    macro_execution_request_t request = make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    fake.press_result = APP_ERROR_IO;
    fake.release_results[0] = APP_ERROR_NONE;
    fake.release_results[1] = APP_ERROR_USB_NOT_READY;
    fake.release_result_count = 2U;
    TEST_CHECK(macro_executor_engine_submit(&engine, &request) == APP_ERROR_NONE);
    TEST_CHECK(macro_executor_engine_execute(&engine, &fake.queued) == APP_ERROR_IO);
    const macro_execution_status_t status = macro_executor_engine_get_status(&engine);
    TEST_CHECK(status.state == EXECUTION_FAILED);
    TEST_CHECK(status.error == APP_ERROR_IO);
    TEST_CHECK(status.release_error == APP_ERROR_USB_NOT_READY);
    TEST_CHECK_EQ_U64(2U, fake.release_index);
}

int main(void)
{
    test_validation_and_ownership();
    test_successful_action_order();
    test_cancellation_and_timeout();
    test_release_errors_and_idle_cancel();
    puts("macro executor tests passed");
    return EXIT_SUCCESS;
}
