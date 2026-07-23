#ifndef EXECUTOR_TEST_FIXTURE_H
#define EXECUTOR_TEST_FIXTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fake_call_log.h"
#include "macro_executor_engine.h"

#define EXECUTOR_STATUS_SNAPSHOT_CAPACITY 512U

typedef struct {
    bool locked;
    bool usb_is_ready;
    bool queue_result;
    macro_execution_request_t queued;
    bool has_queued;
    uint32_t now_ms;
    size_t wait_count;
    size_t cancel_on_wait;
    size_t wait_failure_on;
    app_error_code_t wait_failure_result;
    uint32_t extra_advance_on_wait_ms;
    macro_executor_engine_t *engine;
    app_error_code_t press_result;
    app_error_code_t release_results[16U];
    size_t release_result_count;
    size_t release_index;
    size_t free_count;
    macro_execution_status_t snapshots[EXECUTOR_STATUS_SNAPSHOT_CAPACITY];
    size_t snapshot_count;
    fake_call_log_t calls;
} executor_fake_t;

void executor_fake_reset(executor_fake_t *fake);
void executor_clear_injected_failure(executor_fake_t *fake);
macro_executor_ops_t executor_make_ops(executor_fake_t *fake);
void executor_init_engine(macro_executor_engine_t *engine, executor_fake_t *fake);
macro_execution_request_t executor_make_request(size_t action_count);
void executor_free_unowned_request(macro_execution_request_t *request);
app_error_code_t executor_execute_queued(macro_executor_engine_t *engine,
                                         executor_fake_t *fake);
void executor_submit_single_key(macro_executor_engine_t *engine,
                                executor_fake_t *fake,
                                uint8_t usage);
void executor_assert_terminal(const macro_executor_engine_t *engine,
                              execution_state_t state,
                              app_error_code_t error,
                              bool expected_busy);
void executor_assert_relevant_call(const executor_fake_t *fake,
                                   size_t ordinal,
                                   const char *name,
                                   uint64_t argument0);

void executor_run_validation_tests(void);
void executor_run_execution_tests(void);
void executor_run_terminal_tests(void);

#endif
