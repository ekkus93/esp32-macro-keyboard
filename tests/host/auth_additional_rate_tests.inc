static void test_complete_rate_limit_matrix(void) {
    const uint32_t source_a = UINT32_C(0x0a000001);
    const uint32_t source_b = UINT32_C(0x0a000002);
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    uint32_t retry = UINT32_MAX;

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_login_attempt_allowed(&core, source_a, &retry));
    for (size_t failure = 1U; failure <= 4U; ++failure) {
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_failure(&core, source_a));
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                             auth_core_login_attempt_allowed(&core, source_a, &retry));
        TEST_CHECK_EQ_U64(0U, retry);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_failure(&core, source_a));
    TEST_CHECK_APP_ERROR(APP_ERROR_RATE_LIMITED,
                         auth_core_login_attempt_allowed(&core, source_a, &retry));
    TEST_CHECK_EQ_U64(300U, retry);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_login_attempt_allowed(&core, source_b, &retry));
    TEST_CHECK_EQ_U64(0U, retry);

    fake.now_us = 1000000U;
    TEST_CHECK_APP_ERROR(APP_ERROR_RATE_LIMITED,
                         auth_core_login_attempt_allowed(&core, source_a, &retry));
    TEST_CHECK_EQ_U64(299U, retry);
    fake.now_us = AUTH_CORE_LOCKOUT_US;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_login_attempt_allowed(&core, source_a, &retry));

    fake_reset(&fake);
    init_core(&core, &fake);
    for (size_t failure = 0U; failure < 4U; ++failure) {
        fake.now_us = (uint64_t)failure * UINT64_C(15000000);
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_failure(&core, source_a));
    }
    fake.now_us = UINT64_C(61000000);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_failure(&core, source_a));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_login_attempt_allowed(&core, source_a, &retry));
    TEST_CHECK_EQ_U64(0U, retry);
    fake.now_us = UINT64_C(62000000);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_failure(&core, source_a));
    TEST_CHECK_APP_ERROR(APP_ERROR_RATE_LIMITED,
                         auth_core_login_attempt_allowed(&core, source_a, &retry));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_success(&core, source_a));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_login_attempt_allowed(&core, source_a, &retry));

    fake_reset(&fake);
    init_core(&core, &fake);
    for (size_t slot = 0U; slot < AUTH_RATE_LIMIT_SOURCE_MAX; ++slot) {
        const uint32_t source = UINT32_C(0x0a000100) + (uint32_t)slot;
        for (size_t failure = 0U; failure < AUTH_RATE_LIMIT_FAILURE_MAX; ++failure) {
            TEST_CHECK_APP_ERROR(APP_ERROR_NONE, auth_core_login_record_failure(&core, source));
        }
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_RATE_LIMITED,
                         auth_core_login_record_failure(&core, UINT32_C(0x0a00ffff)));

    fake_call_log_fail_on(&fake.calls, "lock", 1U);
    retry = UINT32_MAX;
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         auth_core_login_attempt_allowed(&core, source_a, &retry));
    TEST_CHECK_EQ_U64(0U, retry);
    fake.calls.failure_name = NULL;
    fake.calls.failure_occurrence = 0U;
    fake.calls.failure_seen = 0U;

    const auth_core_t before_unlock = core;
    fake_call_log_fail_on(&fake.calls, "unlock", 1U);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, auth_core_login_record_success(&core, source_a));
    TEST_CHECK_EQ_BUFFER(before_unlock.rate_limits, core.rate_limits, sizeof(core.rate_limits));
}
