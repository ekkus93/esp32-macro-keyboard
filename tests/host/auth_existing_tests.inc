static void test_password_boundaries(void) {
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    auth_password_record_t record;
    char password[130U];
    memset(password, 'a', sizeof(password));

    TEST_CHECK(auth_core_password_create(&core, password, 11U, &record) ==
               APP_ERROR_INVALID_ARGUMENT);
    TEST_CHECK(auth_core_password_create(&core, password, 12U, &record) == APP_ERROR_NONE);
    TEST_CHECK_EQ_U64(120000U, record.iterations);
    static const uint8_t expected_salt[AUTH_SALT_BYTES] = {
        0x75U, 0xcdU, 0x25U, 0x4bU, 0x84U, 0xe2U, 0xeaU, 0xf2U,
        0xa6U, 0x81U, 0x20U, 0x67U, 0x43U, 0x34U, 0xb2U, 0x6eU,
    };
    TEST_CHECK_EQ_BUFFER(expected_salt, record.salt, sizeof(expected_salt));
    bool matches = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_password_verify(&core, password, 12U, &record, &matches));
    TEST_CHECK(matches);
    password[0] = 'b';
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         auth_core_password_verify(&core, password, 12U, &record, &matches));
    TEST_CHECK(!matches);
    password[0] = 'a';
    TEST_CHECK(auth_core_password_create(&core, password, 128U, &record) == APP_ERROR_NONE);
    TEST_CHECK(auth_core_password_create(&core, password, 129U, &record) ==
               APP_ERROR_INVALID_ARGUMENT);
    password[4] = '\0';
    TEST_CHECK(auth_core_password_create(&core, password, 12U, &record) ==
               APP_ERROR_INVALID_ARGUMENT);
}

static void test_derive_failures_zero_outputs(void) {
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    auth_password_record_t record;
    memset(&record, 0xa5, sizeof(record));
    fake.derive_fail = true;
    static const char password[] = "abcdefghijkl";
    TEST_CHECK(auth_core_password_create(&core, password, sizeof(password) - 1U, &record) ==
               APP_ERROR_INTERNAL);
    static const uint8_t zeros[sizeof(record)] = {0};
    TEST_CHECK_EQ_BUFFER(zeros, &record, sizeof(record));
    TEST_CHECK(fake.zero_count >= 1U);

    fake.derive_fail = false;
    TEST_CHECK(auth_core_password_create(&core, password, sizeof(password) - 1U, &record) ==
               APP_ERROR_NONE);
    fake.derive_fail = true;
    const size_t zero_before = fake.zero_count;
    bool matches = true;
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INTERNAL,
        auth_core_password_verify(&core, password, sizeof(password) - 1U, &record, &matches));
    TEST_CHECK(!matches);
    TEST_CHECK(fake.zero_count == zero_before + 1U);
    TEST_CHECK_EQ_U64(AUTH_HASH_BYTES, fake.last_zero_length);
}

/* SPEC 24.4 item: authentication and logout */
static void test_sessions(void) {
    auth_fake_t fake;
    fake_reset(&fake);
    fake.now_us = 100U;
    auth_core_t core;
    init_core(&core, &fake);
    auth_session_view_t session;
    TEST_CHECK(auth_core_session_create(&core, &session) == APP_ERROR_NONE);
    TEST_CHECK(strlen(session.session_token) == AUTH_TOKEN_HEX_BYTES - 1U);
    TEST_CHECK(strlen(session.csrf_token) == AUTH_TOKEN_HEX_BYTES - 1U);
    TEST_CHECK(strcmp(session.session_token, session.csrf_token) != 0);
    const uint64_t original_expiry = session.expires_at_us;
    TEST_CHECK(auth_core_session_validate(&core, session.session_token, session.csrf_token) ==
               APP_ERROR_NONE);
    TEST_CHECK(auth_core_session_validate_read_only(&core, session.session_token) ==
               APP_ERROR_NONE);
    char restored_csrf[AUTH_TOKEN_HEX_BYTES] = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE, auth_core_session_get_csrf_token(&core, session.session_token,
                                                         restored_csrf,
                                                         sizeof(restored_csrf)));
    TEST_CHECK_EQ_STRING(session.csrf_token, restored_csrf);

    fake.now_us += 1000000U;
    TEST_CHECK(auth_core_session_validate(&core, session.session_token, session.csrf_token) ==
               APP_ERROR_NONE);
    TEST_CHECK(core.sessions[0].view.expires_at_us > original_expiry);

    char wrong[AUTH_TOKEN_HEX_BYTES];
    memcpy(wrong, session.csrf_token, sizeof(wrong));
    wrong[0] = wrong[0] == '0' ? '1' : '0';
    const uint64_t expiry_before_failure = core.sessions[0].view.expires_at_us;
    TEST_CHECK(auth_core_session_validate(&core, session.session_token, wrong) ==
               APP_ERROR_AUTH_REQUIRED);
    TEST_CHECK_EQ_U64(expiry_before_failure, core.sessions[0].view.expires_at_us);
    TEST_CHECK(auth_core_session_validate(&core, "short", wrong) == APP_ERROR_AUTH_REQUIRED);
    TEST_CHECK(auth_core_session_validate_read_only(&core, wrong) == APP_ERROR_AUTH_REQUIRED);
    memset(restored_csrf, 'x', sizeof(restored_csrf));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_AUTH_REQUIRED,
        auth_core_session_get_csrf_token(&core, wrong, restored_csrf, sizeof(restored_csrf)));
    TEST_CHECK_EQ_U64(0U, restored_csrf[0]);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         auth_core_session_get_csrf_token(
                             &core, session.session_token, restored_csrf, 1U));
    TEST_CHECK_EQ_U64(0U, restored_csrf[0]);
    TEST_CHECK(auth_core_session_validate_read_only(NULL, session.session_token) ==
               APP_ERROR_AUTH_REQUIRED);

    TEST_CHECK(auth_core_session_logout(&core, session.session_token) == APP_ERROR_NONE);
    TEST_CHECK_APP_ERROR(
        APP_ERROR_AUTH_REQUIRED,
        auth_core_session_get_csrf_token(&core, session.session_token, restored_csrf,
                                         sizeof(restored_csrf)));
    TEST_CHECK(auth_core_session_logout(&core, session.session_token) == APP_ERROR_NOT_FOUND);
}

/* SPEC 24.4 item: session expiry */
static void test_session_expiry_and_capacity(void) {
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    auth_session_view_t sessions[APP_SESSION_TABLE_MAX];
    for (size_t index = 0U; index < APP_SESSION_TABLE_MAX; ++index) {
        TEST_CHECK(auth_core_session_create(&core, &sessions[index]) == APP_ERROR_NONE);
    }
    auth_session_view_t extra;
    TEST_CHECK(auth_core_session_create(&core, &extra) == APP_ERROR_CONFLICT);

    fake.now_us = sessions[0].expires_at_us;
    TEST_CHECK(auth_core_session_validate(&core, sessions[0].session_token,
                                          sessions[0].csrf_token) == APP_ERROR_AUTH_REQUIRED);
    TEST_CHECK(auth_core_session_create(&core, &extra) == APP_ERROR_NONE);
}

/* SPEC 24.4 item: rate limiting */
static void test_rate_limit(void) {
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    uint32_t retry = UINT32_MAX;
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_NONE);
    TEST_CHECK_EQ_U64(0U, retry);
    for (size_t index = 0U; index < 5U; ++index) {
        TEST_CHECK(auth_core_login_record_failure(&core) == APP_ERROR_NONE);
    }
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_RATE_LIMITED);
    TEST_CHECK_EQ_U64(60U, retry);
    fake.now_us = 1500000U;
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_RATE_LIMITED);
    TEST_CHECK_EQ_U64(59U, retry);
    fake.now_us = 60000000U;
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_NONE);
    TEST_CHECK_EQ_U64(0U, retry);

    TEST_CHECK(auth_core_login_record_failure(&core) == APP_ERROR_NONE);
    TEST_CHECK(auth_core_login_record_success(&core) == APP_ERROR_NONE);
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_NONE);

    fake.now_us = 50000000U;
    TEST_CHECK(auth_core_login_attempt_allowed(&core, &retry) == APP_ERROR_INTERNAL);
}

static void test_strict_password_create_sequence(void) {
    auth_fake_t fake;
    fake_reset(&fake);
    auth_core_t core;
    init_core(&core, &fake);
    char password[13U];
    memset(password, 'a', 12U);
    password[12] = '\0';
    auth_password_record_t record;

    /*
     * Strict fake enforcement (UNIT_TESTS1 L915): creating a password record
     * draws salt bytes from the random source, then derives the hash -- exactly
     * one "random" followed by one "derive" and nothing else. Strict mode aborts
     * on any unexpected, missing, or out-of-order operation.
     */
    fake_call_log_reset(&fake.calls);
    static const char *const expected[] = {"random", "derive"};
    fake_call_log_set_strict(&fake.calls, true);
    for (size_t index = 0U; index < (sizeof(expected) / sizeof(expected[0])); ++index) {
        fake_call_log_expect(&fake.calls, expected[index]);
    }
    TEST_CHECK(auth_core_password_create(&core, password, 12U, &record) == APP_ERROR_NONE);
    fake_call_log_verify(&fake.calls);
}

static void auth_run_existing_tests(void) {
    test_strict_password_create_sequence();
    test_password_boundaries();
    test_derive_failures_zero_outputs();
    test_sessions();
    test_session_expiry_and_capacity();
    test_rate_limit();
}
