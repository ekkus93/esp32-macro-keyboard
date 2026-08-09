/* V2-063 (cancellation responsiveness): SPEC_V2 §7.4 "Cancellation MUST
 * remain responsive during ordinary typing and delay actions", and §15
 * "Network cancellation through `DELETE /api/v1/send` remains available
 * during typing and delays" alongside the serial-console `cancel` command.
 *
 * Both firmware entry points reach the exact same state machine:
 * `firmware/components/web_server/web_server_send.c`'s
 * executor_cancel_adapter() and `firmware/components/serial_console/
 * serial_console.c`'s command_cancel() both call macro_executor_cancel(),
 * which (aside from a shutting_down guard neither test needs) is exactly
 * macro_executor_engine_cancel() on the single global engine instance under
 * test throughout this file -- there is no per-transport cancellation state
 * to converge; they were already calling the same function. These tests
 * make that convergence, and the responsiveness bound CANCELLATION_SLICE_MS
 * (10 ms) provides, deterministic and explicit rather than implicit in the
 * production call graph. */

/* Two callers racing to cancel the same in-flight send at effectively the
 * same instant -- modeling a DELETE /api/v1/send request and a serial
 * `cancel` command arriving together -- resolve deterministically: the first
 * is recorded (NONE) and the second is the documented idempotent conflict
 * (CONFLICT), matching macro_executor_engine_cancel()'s single-request
 * contract. The run still ends CANCELLED after exactly one 10 ms
 * cancellation slice, proving the race neither loses the cancellation nor
 * delays it beyond the normal per-slice bound. */
static void test_network_and_serial_cancellation_converge_on_one_engine(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 5000U};
    request.plan.estimated_duration_ms = 5000U;
    fake.race_cancel_on_wait = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, fake.race_cancel_first_result);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, fake.race_cancel_second_result);
    TEST_CHECK_EQ_U64(1U, fake.wait_count);
    executor_assert_relevant_call(&fake, 0U, "wait", 10U);
    executor_assert_terminal(&engine, EXECUTION_CANCELLED, APP_ERROR_EXECUTION_CANCELLED, false);
}

/* The same race, but while the send is EXECUTION_AWAITING_CONFIRMATION rather
 * than running: SPEC_V2 §7.12 cancellation-while-awaiting-confirmation still
 * resolves through the identical macro_executor_engine_cancel() call, still
 * idempotently, and still types nothing. */
static void test_network_and_serial_cancellation_converge_during_confirmation_wait(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    request.require_confirmation = true;
    fake.race_cancel_on_wait = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, fake.race_cancel_first_result);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, fake.race_cancel_second_result);
    executor_assert_terminal(&engine, EXECUTION_CANCELLED, APP_ERROR_EXECUTION_CANCELLED, false);
    for (size_t index = 0U; index < fake.calls.call_count; ++index) {
        TEST_CHECK(strcmp(fake_call_log_at(&fake.calls, index)->name, "press") != 0);
    }
}

/* SPEC_V2 §7.4: cancellation during ordinary typing (a key/chord action's
 * press-duration dwell) is honored after exactly one CANCELLATION_SLICE_MS
 * (10 ms) slice, no matter how long the requested dwell is -- not after the
 * full dwell elapses. request.key_press_ms is set to the maximum the
 * contract allows (APP_DELAY_MAX_MS) specifically so a bound of "many
 * seconds" would be an observable regression, not a rounding difference. */
static void test_cancellation_during_typing_is_bounded_to_one_slice(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    request.key_press_ms = APP_DELAY_MAX_MS;
    request.inter_key_ms = 0U;
    fake.cancel_on_wait = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));

    TEST_CHECK_EQ_U64(1U, fake.wait_count);
    executor_assert_relevant_call(&fake, 0U, "press", 0U);
    executor_assert_relevant_call(&fake, 1U, "wait", 10U);
    executor_assert_relevant_call(&fake, 2U, "release", 0U);
    executor_assert_terminal(&engine, EXECUTION_CANCELLED, APP_ERROR_EXECUTION_CANCELLED, false);
}

/* SPEC_V2 §7.4: the same one-slice bound applies to a MACRO_ACTION_DELAY
 * directive, exercised here at the maximum single-delay duration
 * (APP_DELAY_MAX_MS) the parser accepts. */
static void test_cancellation_during_delay_directive_is_bounded_to_one_slice(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] =
        (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = APP_DELAY_MAX_MS};
    request.plan.estimated_duration_ms = APP_DELAY_MAX_MS;
    fake.cancel_on_wait = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));

    TEST_CHECK_EQ_U64(1U, fake.wait_count);
    executor_assert_relevant_call(&fake, 0U, "wait", 10U);
    executor_assert_terminal(&engine, EXECUTION_CANCELLED, APP_ERROR_EXECUTION_CANCELLED, false);
}

/* A cancellation that loses the race against a run's own natural completion
 * is still deterministic: once macro_executor_engine_execute() has returned,
 * the send is terminal and `busy` is clear, so a cancel arriving afterward
 * (from either call path) is NOT_FOUND rather than silently mutating an
 * already-finished send's flags. */
static void test_cancellation_racing_natural_completion_loses_cleanly(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);

    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, macro_executor_engine_cancel(&engine));
}

void executor_run_cancellation_race_tests(void) {
    test_network_and_serial_cancellation_converge_on_one_engine();
    test_network_and_serial_cancellation_converge_during_confirmation_wait();
    test_cancellation_during_typing_is_bounded_to_one_slice();
    test_cancellation_during_delay_directive_is_bounded_to_one_slice();
    test_cancellation_racing_natural_completion_loses_cleanly();
}
