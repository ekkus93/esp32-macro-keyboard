#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device_controls_logic.h"
#include "test_assert.h"

static void test_basic_logic(void) {
    TEST_CHECK(device_controls_level_is_pressed(0, 0));
    TEST_CHECK(!device_controls_level_is_pressed(1, 0));
    TEST_CHECK(device_controls_level_is_pressed(1, 1));
    TEST_CHECK(!device_controls_level_is_pressed(0, 1));
    TEST_CHECK(!device_controls_level_is_pressed(0, -1));
    TEST_CHECK(!device_controls_level_is_pressed(1, 2));

    device_controls_debounce_t button = {0};
    TEST_CHECK(!device_controls_debounce_update(&button, true));
    TEST_CHECK(!device_controls_debounce_update(&button, true));
    TEST_CHECK(device_controls_debounce_update(&button, true));
    for (size_t index = 0U; index < 300U; ++index) {
        TEST_CHECK(!device_controls_debounce_update(&button, true));
    }
    TEST_CHECK_EQ_U64(DEVICE_CONTROLS_DEBOUNCE_SAMPLES, button.candidate_count);
    TEST_CHECK(!device_controls_debounce_update(&button, false));
    TEST_CHECK(!device_controls_debounce_update(&button, false));
    TEST_CHECK(!device_controls_debounce_update(&button, false));
    TEST_CHECK(!button.stable);
    TEST_CHECK(!device_controls_debounce_update(NULL, true));

    for (size_t index = 0U; index < 99U; ++index) {
        TEST_CHECK(!device_controls_debounce_update(&button, (index % 2U) == 0U));
        TEST_CHECK_EQ_U64(1U, button.candidate_count);
    }
}

typedef struct {
    device_indicator_state_t state;
    uint32_t elapsed_ms;
    bool expected;
} indicator_case_t;

static void test_indicators(void) {
    static const indicator_case_t cases[] = {
        {DEVICE_INDICATOR_READY, 0U, true},
        {DEVICE_INDICATOR_READY, UINT32_MAX, true},
        {DEVICE_INDICATOR_BOOTING, 0U, true},
        {DEVICE_INDICATOR_BOOTING, 249U, true},
        {DEVICE_INDICATOR_BOOTING, 250U, false},
        {DEVICE_INDICATOR_BOOTING, 1000U, true},
        {DEVICE_INDICATOR_EXECUTING, 99U, true},
        {DEVICE_INDICATOR_EXECUTING, 100U, false},
        {DEVICE_INDICATOR_EXECUTING, 200U, true},
        {DEVICE_INDICATOR_DEGRADED, 249U, true},
        {DEVICE_INDICATOR_DEGRADED, 250U, false},
        {DEVICE_INDICATOR_DEGRADED, 500U, true},
        {DEVICE_INDICATOR_DEGRADED, 750U, false},
        {DEVICE_INDICATOR_DEGRADED, 2000U, true},
        {DEVICE_INDICATOR_FATAL, 249U, true},
        {DEVICE_INDICATOR_FATAL, 250U, false},
        {DEVICE_INDICATOR_FATAL, 500U, true},
        {(device_indicator_state_t)99, 0U, false},
    };
    for (size_t index = 0U; index < sizeof(cases) / sizeof(cases[0]); ++index) {
        TEST_CHECK_EQ_INT(cases[index].expected,
                          device_controls_indicator_on((device_indicator_phase_t){
                              .state = cases[index].state,
                              .elapsed_ms = cases[index].elapsed_ms,
                          }));
    }
}

typedef struct {
    device_controls_engine_t *engine;
    bool complete_stop_during_wait;
    bool locked;
    bool fail_lock;
    bool fail_unlock;
    bool stop_requested;
    bool confirmation_signal_result;
    bool stopped_signal_result;
    app_error_code_t cancel_result;
    app_error_code_t write_result;
    app_error_code_t wait_result;
    app_error_code_t safe_result;
    app_error_code_t reset_results[3];
    unsigned int confirmation_signals;
    unsigned int stopped_signals;
    unsigned int cancel_calls;
    unsigned int write_calls;
    unsigned int request_stop_calls;
    unsigned int wait_calls;
    unsigned int safe_calls;
    unsigned int reset_calls[3];
    unsigned int delete_confirmation_calls;
    unsigned int delete_stopped_calls;
    bool confirmation_deleted;
    bool stopped_deleted;
    bool use_after_free;
} fake_controls_t;

static bool fake_lock(void *context) {
    fake_controls_t *fake = context;
    if (fake->fail_lock || fake->locked) {
        return false;
    }
    fake->locked = true;
    return true;
}

static bool fake_unlock(void *context) {
    fake_controls_t *fake = context;
    if (fake->fail_unlock || !fake->locked) {
        return false;
    }
    fake->locked = false;
    return true;
}

static bool fake_confirmation(void *context) {
    fake_controls_t *fake = context;
    fake->use_after_free = fake->use_after_free || fake->confirmation_deleted;
    ++fake->confirmation_signals;
    return fake->confirmation_signal_result;
}

static bool fake_stopped(void *context) {
    fake_controls_t *fake = context;
    fake->use_after_free = fake->use_after_free || fake->stopped_deleted;
    ++fake->stopped_signals;
    return fake->stopped_signal_result;
}

static app_error_code_t fake_cancel(void *context) {
    fake_controls_t *fake = context;
    ++fake->cancel_calls;
    return fake->cancel_result;
}

static app_error_code_t fake_write(void *context, bool enabled) {
    fake_controls_t *fake = context;
    (void)enabled;
    ++fake->write_calls;
    return fake->write_result;
}

static void fake_request_stop(void *context) {
    fake_controls_t *fake = context;
    fake->stop_requested = true;
    ++fake->request_stop_calls;
}

static void fake_clear_stop(void *context) {
    fake_controls_t *fake = context;
    fake->stop_requested = false;
}

static app_error_code_t fake_wait(void *context, uint32_t timeout_ms) {
    fake_controls_t *fake = context;
    if (timeout_ms == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++fake->wait_calls;
    if (fake->complete_stop_during_wait && fake->engine != NULL) {
        const device_controls_poll_result_t stopped =
            device_controls_engine_poll(fake->engine,
                                        (device_controls_poll_input_t){.stop_requested = true});
        TEST_CHECK(!stopped.continue_running);
    }
    return fake->wait_result;
}

static app_error_code_t fake_safe(void *context) {
    fake_controls_t *fake = context;
    ++fake->safe_calls;
    return fake->safe_result;
}

static app_error_code_t fake_reset(void *context, device_controls_pin_t pin) {
    fake_controls_t *fake = context;
    const size_t index = (size_t)pin;
    if (index >= 3U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++fake->reset_calls[index];
    return fake->reset_results[index];
}

static void fake_delete_confirmation(void *context) {
    fake_controls_t *fake = context;
    ++fake->delete_confirmation_calls;
    fake->confirmation_deleted = true;
}

static void fake_delete_stopped(void *context) {
    fake_controls_t *fake = context;
    ++fake->delete_stopped_calls;
    fake->stopped_deleted = true;
}

static device_controls_ops_t fake_ops(fake_controls_t *fake) {
    return (device_controls_ops_t){
        .context = fake,
        .lock = fake_lock,
        .unlock = fake_unlock,
        .signal_confirmation = fake_confirmation,
        .signal_stopped = fake_stopped,
        .cancel_execution = fake_cancel,
        .write_indicator = fake_write,
        .request_stop = fake_request_stop,
        .clear_stop_request = fake_clear_stop,
        .wait_stopped = fake_wait,
        .set_safe_output = fake_safe,
        .reset_pin = fake_reset,
        .delete_confirmation_signal = fake_delete_confirmation,
        .delete_stopped_signal = fake_delete_stopped,
    };
}

static void fake_init(fake_controls_t *fake) {
    memset(fake, 0, sizeof(*fake));
    fake->confirmation_signal_result = true;
    fake->stopped_signal_result = true;
}

static void invalidate_operation(device_controls_ops_t *operations, size_t index) {
    switch (index) {
    case 0U:
        operations->lock = NULL;
        break;
    case 1U:
        operations->unlock = NULL;
        break;
    case 2U:
        operations->signal_confirmation = NULL;
        break;
    case 3U:
        operations->signal_stopped = NULL;
        break;
    case 4U:
        operations->cancel_execution = NULL;
        break;
    case 5U:
        operations->write_indicator = NULL;
        break;
    case 6U:
        operations->request_stop = NULL;
        break;
    case 7U:
        operations->clear_stop_request = NULL;
        break;
    case 8U:
        operations->wait_stopped = NULL;
        break;
    case 9U:
        operations->set_safe_output = NULL;
        break;
    case 10U:
        operations->reset_pin = NULL;
        break;
    case 11U:
        operations->delete_confirmation_signal = NULL;
        break;
    case 12U:
        operations->delete_stopped_signal = NULL;
        break;
    default:
        break;
    }
}

static void acquire_all(device_controls_engine_t *engine) {
    for (device_controls_resource_t resource = DEVICE_CONTROLS_RESOURCE_CONFIRM_SIGNAL;
         resource <= DEVICE_CONTROLS_RESOURCE_STATUS_PIN;
         resource = (device_controls_resource_t)(resource + 1)) {
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                             device_controls_engine_acquire_resource(engine, resource));
    }
}

static void start_engine(device_controls_engine_t *engine, fake_controls_t *fake) {
    const device_controls_ops_t operations = fake_ops(fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_engine_init(engine, &operations));
    fake->engine = engine;
    acquire_all(engine);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_engine_task_started(engine));
}

static device_controls_poll_result_t poll_button(device_controls_engine_t *engine,
                                                 bool confirmation,
                                                 bool cancel,
                                                 uint32_t elapsed_ms) {
    return device_controls_engine_poll(engine,
                                       (device_controls_poll_input_t){
                                           .confirmation_pressed = confirmation,
                                           .cancel_pressed = cancel,
                                           .elapsed_ms = elapsed_ms,
                                       });
}

static void test_validation_and_partial_init(void) {
    fake_controls_t fake;
    fake_init(&fake);
    device_controls_engine_t engine = {0};
    device_controls_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_controls_engine_init(NULL, &operations));
    for (size_t index = 0U; index < 13U; ++index) {
        operations = fake_ops(&fake);
        invalidate_operation(&operations, index);
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                             device_controls_engine_init(&engine, &operations));
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         device_controls_engine_acquire_resource(
                             NULL, DEVICE_CONTROLS_RESOURCE_CONFIRM_PIN));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, device_controls_engine_task_started(NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_controls_engine_deinit(NULL, 100U));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_controls_engine_deinit(&engine, 0U));
    TEST_CHECK(!device_controls_engine_owns_resources(NULL));
    TEST_CHECK(!device_controls_engine_task_stop_confirmed(NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         device_controls_engine_poll(NULL,
                                                     (device_controls_poll_input_t){0})
                             .error);

    operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_engine_init(&engine, &operations));
    fake.engine = &engine;
    TEST_CHECK(device_controls_engine_task_stop_confirmed(&engine));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_controls_engine_acquire_resource(
                             &engine, (device_controls_resource_t)99));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_engine_acquire_resource(
                             &engine, DEVICE_CONTROLS_RESOURCE_CONFIRM_PIN));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_engine_deinit(&engine, 100U));
    TEST_CHECK_EQ_U64(1U, fake.reset_calls[DEVICE_CONTROLS_PIN_CONFIRM]);
}

static void test_health_categories(void) {
    fake_controls_t fake;
    fake_init(&fake);
    device_controls_engine_t engine;
    const device_controls_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_engine_init(&engine, &operations));
    fake.engine = &engine;
    TEST_CHECK_APP_ERROR(
        APP_ERROR_IO,
        device_controls_engine_record_failure(
            &engine, APP_ERROR_IO, DEVICE_CONTROLS_FAILURE_GPIO_CONFIGURATION, false));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INTERNAL,
        device_controls_engine_record_failure(
            &engine, APP_ERROR_INTERNAL, DEVICE_CONTROLS_FAILURE_TASK_START, false));
    const device_controls_health_t health = device_controls_engine_get_health(&engine);
    TEST_CHECK(health.gpio_configuration_failed);
    TEST_CHECK(health.task_start_failed);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_controls_engine_record_failure(
                             &engine, APP_ERROR_NONE, DEVICE_CONTROLS_FAILURE_GENERIC, false));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_engine_deinit(&engine, 100U));
}

static void debounce_press(device_controls_engine_t *engine, bool confirmation, bool cancel) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, poll_button(engine, confirmation, cancel, 0U).error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, poll_button(engine, confirmation, cancel, 20U).error);
}

static void test_runtime_failures(void) {
    fake_controls_t fake;
    fake_init(&fake);
    device_controls_engine_t engine;
    start_engine(&engine, &fake);

    fake.confirmation_signal_result = false;
    debounce_press(&engine, true, false);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         poll_button(&engine, true, false, 40U).error);
    device_controls_health_t health = device_controls_engine_get_health(&engine);
    TEST_CHECK(health.confirmation_signal_failed);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, health.last_confirmation_error);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, poll_button(&engine, false, false, 60U).error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, poll_button(&engine, false, false, 80U).error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, poll_button(&engine, false, false, 100U).error);
    fake.cancel_result = APP_ERROR_IO;
    debounce_press(&engine, false, true);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, poll_button(&engine, false, true, 160U).error);
    health = device_controls_engine_get_health(&engine);
    TEST_CHECK(health.cancel_request_failed);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, health.last_cancel_error);

    fake.cancel_result = APP_ERROR_NOT_FOUND;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, poll_button(&engine, false, false, 180U).error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, poll_button(&engine, false, false, 200U).error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, poll_button(&engine, false, false, 220U).error);
    debounce_press(&engine, false, true);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, poll_button(&engine, false, true, 280U).error);

    device_controls_poll_result_t result = device_controls_engine_poll(
        &engine,
        (device_controls_poll_input_t){
            .confirmation_pressed = true,
            .cancel_pressed = true,
            .gpio_read_error = APP_ERROR_IO,
        });
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, result.error);
    health = device_controls_engine_get_health(&engine);
    TEST_CHECK(health.gpio_read_failed);

    fake.write_result = APP_ERROR_IO;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, poll_button(&engine, false, false, 300U).error);
    TEST_CHECK(device_controls_engine_get_health(&engine).indicator_output_failed);
}

static void test_stop_failures(void) {
    fake_controls_t fake;
    fake_init(&fake);
    fake.wait_result = APP_ERROR_TIMEOUT;
    device_controls_engine_t engine;
    start_engine(&engine, &fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT,
                         device_controls_engine_deinit(&engine, 100U));
    TEST_CHECK_EQ_U64(1U, fake.request_stop_calls);
    TEST_CHECK_EQ_U64(0U, fake.safe_calls);
    TEST_CHECK(device_controls_engine_owns_resources(&engine));
    device_controls_health_t health = device_controls_engine_get_health(&engine);
    TEST_CHECK(health.task_stop_failed);
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, health.cleanup_error);

    fake_init(&fake);
    fake.stopped_signal_result = false;
    fake.wait_result = APP_ERROR_TIMEOUT;
    fake.complete_stop_during_wait = true;
    start_engine(&engine, &fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_engine_deinit(&engine, 100U));
    health = device_controls_engine_get_health(&engine);
    TEST_CHECK(!health.task_running);
    TEST_CHECK(health.task_stop_failed);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, health.cleanup_error);
    TEST_CHECK_EQ_U64(0U, fake.delete_confirmation_calls);
}

static void test_cleanup_retry_and_idempotence(void) {
    fake_controls_t fake;
    fake_init(&fake);
    fake.complete_stop_during_wait = true;
    fake.safe_result = APP_ERROR_IO;
    fake.reset_results[DEVICE_CONTROLS_PIN_CONFIRM] = APP_ERROR_INTERNAL;
    device_controls_engine_t engine;
    start_engine(&engine, &fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         device_controls_engine_deinit(&engine, 100U));
    TEST_CHECK_EQ_U64(1U, fake.safe_calls);
    TEST_CHECK_EQ_U64(1U, fake.reset_calls[DEVICE_CONTROLS_PIN_CONFIRM]);
    TEST_CHECK_EQ_U64(1U, fake.reset_calls[DEVICE_CONTROLS_PIN_CANCEL]);
    TEST_CHECK_EQ_U64(1U, fake.reset_calls[DEVICE_CONTROLS_PIN_STATUS]);
    TEST_CHECK_EQ_U64(1U, fake.delete_confirmation_calls);
    TEST_CHECK_EQ_U64(1U, fake.delete_stopped_calls);
    TEST_CHECK(device_controls_engine_owns_resources(&engine));

    fake.safe_result = APP_ERROR_NONE;
    fake.reset_results[DEVICE_CONTROLS_PIN_CONFIRM] = APP_ERROR_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_engine_deinit(&engine, 100U));
    TEST_CHECK(!device_controls_engine_owns_resources(&engine));
    TEST_CHECK(device_controls_engine_task_stop_confirmed(&engine));
    const unsigned int writes = fake.write_calls;
    const unsigned int confirmations = fake.confirmation_signals;
    const unsigned int stopped = fake.stopped_signals;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_engine_deinit(&engine, 100U));
    const device_controls_poll_result_t after =
        poll_button(&engine, true, true, 100U);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, after.error);
    TEST_CHECK(!after.continue_running);
    TEST_CHECK_EQ_U64(writes, fake.write_calls);
    TEST_CHECK_EQ_U64(confirmations, fake.confirmation_signals);
    TEST_CHECK_EQ_U64(stopped, fake.stopped_signals);
    TEST_CHECK(!fake.use_after_free);
}

static void test_lock_failures_and_empty_health(void) {
    fake_controls_t fake;
    fake_init(&fake);
    device_controls_engine_t engine;
    const device_controls_ops_t operations = fake_ops(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_engine_init(&engine, &operations));
    fake.engine = &engine;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_controls_engine_set_indicator(
                             &engine, (device_indicator_state_t)99));
    fake.fail_lock = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_engine_set_indicator(
                             &engine, DEVICE_INDICATOR_READY));
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_engine_get_health(&engine).last_error);
    fake.fail_lock = false;
    fake.fail_unlock = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_engine_set_indicator(
                             &engine, DEVICE_INDICATOR_READY));
    fake.fail_unlock = false;
    fake.locked = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_controls_engine_deinit(&engine, 100U));

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_controls_engine_get_health(NULL).last_error);
    device_controls_engine_t empty = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_engine_get_health(&empty).last_error);
}

int main(void) {
    test_basic_logic();
    test_indicators();
    test_validation_and_partial_init();
    test_health_categories();
    test_runtime_failures();
    test_stop_failures();
    test_cleanup_retry_and_idempotence();
    test_lock_failures_and_empty_health();
    puts("device controls tests passed");
    return EXIT_SUCCESS;
}
