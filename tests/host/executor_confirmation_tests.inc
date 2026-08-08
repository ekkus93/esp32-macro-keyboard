/* SPEC_V2 §7.12: "When physical confirmation is enabled, an accepted send
 * enters `awaiting_confirmation` for at most 60 seconds. The serial-console
 * `confirm` command starts it. Expiry produces `timed_out` and types
 * nothing." These tests exercise macro_executor_engine's require_confirmation
 * request field end to end: the state transition into and out of
 * EXECUTION_AWAITING_CONFIRMATION, the 60-second timeout, cancellation while
 * awaiting confirmation, and macro_executor_engine_confirm()'s own result
 * contract. */

static macro_execution_request_t confirmation_single_key_request(uint8_t usage) {
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = usage};
    request.require_confirmation = true;
    return request;
}

static void test_confirm_before_any_send_is_not_found(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, macro_executor_engine_confirm(&engine));
}

static void test_confirm_while_running_without_confirmation_is_not_found(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    /* Queued but not yet executed: busy, but status.state has not moved off
     * EXECUTION_IDLE yet, so there is nothing to confirm. */
    executor_submit_single_key(&engine, &fake, 4U);
    TEST_CHECK(engine.busy);
    TEST_CHECK(engine.status.state == EXECUTION_IDLE);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, macro_executor_engine_confirm(&engine));

    /* A require_confirmation == false execute() never visits
     * EXECUTION_AWAITING_CONFIRMATION at all: confirming a plain running send
     * is still not found. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, macro_executor_engine_confirm(&engine));
}

/* SPEC_V2 §7.12: confirming releases the send to run; a second confirm of the
 * same send is an idempotent conflict, matching macro_executor_engine_cancel's
 * convention. No action executes before macro_executor_engine_confirm() is
 * called. */
static void test_confirmation_granted_runs_the_macro_and_is_idempotent(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = confirmation_single_key_request(4U);
    fake.confirm_on_wait = 1U;
    fake.capture_second_confirm = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));

    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, fake.second_confirm_result);
    /* Exactly one 10 ms slice was spent awaiting confirmation (confirm_on_wait
     * == 1) before the press proceeded. */
    executor_assert_relevant_call(&fake, 0U, "wait", 10U);
    executor_assert_relevant_call(&fake, 1U, "press", 0U);
}

/* SPEC_V2 §7.12: a cancellation while awaiting confirmation ends the send
 * having typed nothing -- no press ever happens. */
static void test_cancellation_while_awaiting_confirmation_types_nothing(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = confirmation_single_key_request(4U);
    fake.cancel_on_wait = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));

    executor_assert_terminal(&engine, EXECUTION_CANCELLED, APP_ERROR_EXECUTION_CANCELLED, false);
    /* Only finish_execution()'s unconditional terminal release_all ran -- no
     * per-action release, because no action ever started. */
    TEST_CHECK_EQ_U64(1U, fake.release_index);
    for (size_t index = 0U; index < fake.calls.call_count; ++index) {
        TEST_CHECK(strcmp(fake_call_log_at(&fake.calls, index)->name, "press") != 0);
    }
}

/* SPEC_V2 §7.12: "Expiry produces timed_out and types nothing." Jumps the fake
 * clock past the 60-second confirmation deadline in a single wait_ms() call
 * (the same technique test_watchdog_and_wait_failures() uses for the
 * execution-phase absolute deadline) rather than running out every 10 ms
 * slice for real. */
static void test_confirmation_expiry_times_out_and_types_nothing(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);

    macro_execution_request_t request = confirmation_single_key_request(4U);
    fake.extra_advance_on_wait_ms = APP_V2_SERIAL_CONFIRMATION_TIMEOUT_SECONDS * 1000U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, executor_execute_queued(&engine, &fake));

    executor_assert_terminal(&engine, EXECUTION_TIMED_OUT, APP_ERROR_TIMEOUT, false);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
    TEST_CHECK_EQ_U64(1U, fake.wait_count);
    for (size_t index = 0U; index < fake.calls.call_count; ++index) {
        TEST_CHECK(strcmp(fake_call_log_at(&fake.calls, index)->name, "press") != 0);
    }
}

void executor_run_confirmation_tests(void) {
    test_confirm_before_any_send_is_not_found();
    test_confirm_while_running_without_confirmation_is_not_found();
    test_confirmation_granted_runs_the_macro_and_is_idempotent();
    test_cancellation_while_awaiting_confirmation_types_nothing();
    test_confirmation_expiry_times_out_and_types_nothing();
}
