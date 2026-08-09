/* SPEC 24.3 item: press and release sequence
 * SPEC 24.3 item: chords
 * SPEC 24.3 item: delays */
static void test_action_order_delay_and_status_progress(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(3U);
    request.plan.actions[0] = (macro_action_t){
        .type = MACRO_ACTION_CHORD,
        .modifiers = 2U,
        .usage = 4U,
    };
    request.plan.actions[1] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 25U};
    request.plan.actions[2] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 5U};
    request.plan.estimated_duration_ms = 71U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));

    executor_assert_relevant_call(&fake, 0U, "press", 2U);
    executor_assert_relevant_call(&fake, 1U, "wait", 8U);
    executor_assert_relevant_call(&fake, 2U, "release", 0U);
    executor_assert_relevant_call(&fake, 3U, "wait", 10U);
    executor_assert_relevant_call(&fake, 4U, "wait", 5U);
    executor_assert_relevant_call(&fake, 5U, "wait", 10U);
    executor_assert_relevant_call(&fake, 6U, "wait", 10U);
    executor_assert_relevant_call(&fake, 7U, "wait", 5U);
    executor_assert_relevant_call(&fake, 8U, "press", 0U);
    executor_assert_relevant_call(&fake, 9U, "wait", 8U);
    executor_assert_relevant_call(&fake, 10U, "release", 1U);
    executor_assert_relevant_call(&fake, 11U, "wait", 10U);
    executor_assert_relevant_call(&fake, 12U, "wait", 5U);
    executor_assert_relevant_call(&fake, 13U, "release", 2U);
    TEST_CHECK_EQ_U64(3U, fake.release_index);

    size_t last_index = 0U;
    bool saw_zero = false;
    bool saw_one = false;
    bool saw_two = false;
    for (size_t index = 0U; index < fake.snapshot_count; ++index) {
        const size_t action_index = fake.snapshots[index].action_index;
        TEST_CHECK(action_index >= last_index);
        last_index = action_index;
        saw_zero = saw_zero || action_index == 0U;
        saw_one = saw_one || action_index == 1U;
        saw_two = saw_two || action_index == 2U;
    }
    TEST_CHECK(saw_zero && saw_one && saw_two);
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
}

/* SPEC 24.3 item: cancel during text
 * SPEC 24.3 item: cancel during delay */
static void test_cancel_before_and_during_actions(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_cancel(&engine));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(0U, fake.wait_count);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
    executor_assert_terminal(&engine, EXECUTION_CANCELLED, APP_ERROR_EXECUTION_CANCELLED, false);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.cancel_on_wait = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(1U, fake.wait_count);
    TEST_CHECK_EQ_U64(2U, fake.release_index);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 100U};
    request.plan.estimated_duration_ms = 100U;
    fake.cancel_on_wait = 2U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_EXECUTION_CANCELLED, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(2U, fake.wait_count);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
}

/* FIX1 §12.4: accepted/started/completed timestamps and the redacted
 * current_action summary track the request from submission through each
 * action to completion. */
static void test_timestamps_and_current_action_track_execution(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    fake.now_ms = 1000U;
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    TEST_CHECK_EQ_STRING("none", engine.status.current_action);
    TEST_CHECK_EQ_U64(0U, engine.status.accepted_ms);

    macro_execution_request_t request = executor_make_request(2U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    request.plan.actions[1] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 20U};
    request.plan.estimated_duration_ms = 50U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_EQ_U64(1000U, request.accepted_ms);

    fake.now_ms = 1500U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));

    TEST_CHECK_EQ_U64(1000U, engine.status.accepted_ms);
    TEST_CHECK_EQ_U64(1500U, engine.status.started_ms);
    TEST_CHECK_EQ_U64(fake.now_ms, engine.status.completed_ms);
    TEST_CHECK_EQ_STRING("none", engine.status.current_action);

    /* Snapshot 0 is the initial RUNNING publish, taken before started_ms is
     * stamped and before the loop selects an action - current_action is still
     * "none" there. Every later snapshot (one per action) carries started_ms
     * and the action's current_action together. */
    bool saw_key = false;
    bool saw_delay = false;
    for (size_t index = 0U; index < fake.snapshot_count; ++index) {
        const macro_execution_status_t *snapshot = &fake.snapshots[index];
        TEST_CHECK_EQ_U64(1000U, snapshot->accepted_ms);
        TEST_CHECK_EQ_U64(0U, snapshot->completed_ms);
        if (strcmp(snapshot->current_action, "key") == 0) {
            TEST_CHECK_EQ_U64(0U, snapshot->action_index);
            TEST_CHECK_EQ_U64(1500U, snapshot->started_ms);
            saw_key = true;
        } else if (strcmp(snapshot->current_action, "delay") == 0) {
            TEST_CHECK_EQ_U64(1U, snapshot->action_index);
            TEST_CHECK_EQ_U64(1500U, snapshot->started_ms);
            saw_delay = true;
        }
    }
    TEST_CHECK(saw_key && saw_delay);
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
}

/* SPEC 24.3 item: timeout
 * SPEC_V2 §7.11: the absolute executor deadline is a fixed 310,000 ms ceiling
 * from the moment execution actually starts -- independent of this request's
 * own (much smaller) estimated duration, unlike the retired per-request
 * estimate+margin watchdog this replaced. Two delay slices are used so the
 * jump-past-deadline lands on the loop's pre-wait deadline check rather than
 * being invisible until an action that never comes. */
static void test_watchdog_and_wait_failures(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 20U};
    request.plan.estimated_duration_ms = 20U;
    fake.extra_advance_on_wait_ms = APP_V2_EXECUTOR_ABSOLUTE_DEADLINE_MS;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(1U, fake.wait_count);
    TEST_CHECK_EQ_U64(1U, fake.release_index);
    /* A watchdog timeout is its own terminal state, distinct from a failure. */
    executor_assert_terminal(&engine, EXECUTION_TIMED_OUT, APP_ERROR_TIMEOUT, false);

    executor_fake_reset(&fake);
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    fake.wait_failure_on = 2U;
    fake.wait_failure_result = APP_ERROR_IO;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, executor_execute_queued(&engine, &fake));
    TEST_CHECK_EQ_U64(2U, fake.release_index);
    executor_assert_terminal(&engine, EXECUTION_FAILED, APP_ERROR_IO, false);
}

/* An action whose type is outside the parser's own vocabulary (defensive:
 * macro_parser never emits one) still gets a safe "none" current_action
 * string from action_type_string()'s default case, and execute_action()
 * rejects it as APP_ERROR_INVALID_ARGUMENT rather than running it as a key or
 * chord press. */
static void test_unknown_action_type_is_rejected_safely(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(1U);
    request.plan.actions[0] = (macro_action_t){.type = (macro_action_type_t)99, .usage = 4U};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, executor_execute_queued(&engine, &fake));

    executor_assert_terminal(&engine, EXECUTION_FAILED, APP_ERROR_INVALID_ARGUMENT, false);
    bool saw_unknown_action_snapshot = false;
    for (size_t index = 0U; index < fake.snapshot_count; ++index) {
        if (fake.snapshots[index].action_index == 0U &&
            strcmp(fake.snapshots[index].current_action, "none") == 0) {
            saw_unknown_action_snapshot = true;
        }
    }
    TEST_CHECK(saw_unknown_action_snapshot);
    for (size_t index = 0U; index < fake.calls.call_count; ++index) {
        TEST_CHECK(strcmp(fake_call_log_at(&fake.calls, index)->name, "press") != 0);
    }
}

/* SPEC_V2 §7.11's absolute deadline is also checked once per action in the
 * main execution loop, immediately before that action runs -- not only
 * inside cancellable_delay()'s own per-slice check. Pushing the deadline past
 * during the first action's own wait (a delay short enough to finish in a
 * single slice, so cancellable_delay() never re-checks the deadline itself)
 * makes the *second* action's pre-run check in the main loop the one that
 * catches it, and that second action never runs. */
static void test_main_loop_deadline_check_before_next_action(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    macro_execution_request_t request = executor_make_request(2U);
    request.plan.actions[0] = (macro_action_t){.type = MACRO_ACTION_DELAY, .delay_ms = 5U};
    request.plan.actions[1] = (macro_action_t){.type = MACRO_ACTION_KEY, .usage = 4U};
    request.plan.estimated_duration_ms = 5U;
    fake.extra_advance_on_wait_ms = APP_V2_EXECUTOR_ABSOLUTE_DEADLINE_MS;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_executor_engine_submit(&engine, &request));
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, executor_execute_queued(&engine, &fake));

    executor_assert_terminal(&engine, EXECUTION_TIMED_OUT, APP_ERROR_TIMEOUT, false);
    TEST_CHECK_EQ_U64(1U, fake.wait_count);
    for (size_t index = 0U; index < fake.calls.call_count; ++index) {
        TEST_CHECK(strcmp(fake_call_log_at(&fake.calls, index)->name, "press") != 0);
    }
}

/* A lock() failure on the very first publish (the accepted-send publish at
 * the top of macro_executor_engine_execute(), before the main loop or any
 * confirmation phase) is reported by publish_step() and still drives the
 * engine through the normal finish_execution() path, because that failed
 * publish's own finish_execution() call gets a fresh, working lock(). The
 * engine recovers (not latched unavailable) since that finish_execution()
 * call's own publish/reset both succeed. */
static void test_initial_publish_lock_failure_still_finishes_and_recovers(void) {
    executor_fake_t fake;
    executor_fake_reset(&fake);
    macro_executor_engine_t engine;
    executor_init_engine(&engine, &fake);
    executor_submit_single_key(&engine, &fake, 4U);
    /* fake_call_log_fail_on()'s occurrence counter starts from this call, not
     * from executor_fake_reset() -- submit()'s own lock/unlock already
     * happened above and does not count. Occurrence 1 is execute()'s own
     * initial publish, the very first lock() call inside execute() itself. */
    fake_call_log_fail_on(&fake.calls, "lock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, executor_execute_queued(&engine, &fake));

    executor_assert_terminal(&engine, EXECUTION_FAILED, APP_ERROR_INTERNAL, false);
    for (size_t index = 0U; index < fake.calls.call_count; ++index) {
        TEST_CHECK(strcmp(fake_call_log_at(&fake.calls, index)->name, "press") != 0);
    }
    executor_clear_injected_failure(&fake);

    /* The engine is not latched unavailable: a fresh submission succeeds. */
    executor_submit_single_key(&engine, &fake, 5U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, executor_execute_queued(&engine, &fake));
    executor_assert_terminal(&engine, EXECUTION_COMPLETED, APP_ERROR_NONE, false);
}

void executor_run_execution_tests(void) {
    test_action_order_delay_and_status_progress();
    test_cancel_before_and_during_actions();
    test_timestamps_and_current_action_track_execution();
    test_watchdog_and_wait_failures();
    test_unknown_action_type_is_rejected_safely();
    test_main_loop_deadline_check_before_next_action();
    test_initial_publish_lock_failure_still_finishes_and_recovers();
}
