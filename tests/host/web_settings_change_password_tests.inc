/* ---------------------------------------------------------------------- */
/* POST /api/v1/settings/change-password                                   */
/* ---------------------------------------------------------------------- */

static void test_change_password_success(void) {
    fake_t fake = {.current = provisioned_settings(), .password_matches = true};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_OK, outcome.result);
    TEST_CHECK_EQ_STRING("old-example-password", fake.observed_current_password);
    TEST_CHECK_EQ_STRING("new-example-password", fake.observed_new_password);
    TEST_CHECK_EQ_U64(5500U, fake.committed_candidate.password_iterations);
    TEST_CHECK_EQ_U64(1U, fake.settings_replace_calls);
    TEST_CHECK_EQ_U64(1U, fake.password_transition_begin_calls);
    TEST_CHECK_EQ_U64(1U, fake.password_activate_calls);
    TEST_CHECK_EQ_U64(1U, fake.invalidate_calls);
    TEST_CHECK_EQ_U64(1U, fake.password_transition_end_calls);
    TEST_CHECK(!fake.password_transition_active);
    TEST_CHECK_EQ_U64(fake.committed_candidate.password_iterations,
                      fake.active_candidate.password_iterations);
    TEST_CHECK(memcmp(fake.committed_candidate.password_salt, fake.active_candidate.password_salt,
                      sizeof(fake.active_candidate.password_salt)) == 0);
    TEST_CHECK(memcmp(fake.committed_candidate.password_verifier,
                      fake.active_candidate.password_verifier,
                      sizeof(fake.active_candidate.password_verifier)) == 0);
    /* Everything else preserved. */
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard", fake.committed_candidate.device_name);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_change_password_incorrect_current_password(void) {
    fake_t fake = {.current = provisioned_settings(), .password_matches = false};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"wrong-password\",\"newPassword\":\"new-example-password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INCORRECT_CURRENT_PASSWORD, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_calls);
    TEST_CHECK_EQ_U64(0U, fake.password_create_calls);
}

static void test_change_password_new_password_too_short_rejected(void) {
    fake_t fake = {.current = provisioned_settings(), .password_matches = true};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"short\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INVALID_NEW_PASSWORD, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.password_verify_calls);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_change_password_missing_field_rejected(void) {
    fake_t fake = {.current = provisioned_settings(), .password_matches = true};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"currentPassword\":\"old-example-password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INVALID_BODY, outcome.result);
}

static void test_change_password_settings_replace_backend_unavailable(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .password_matches = true,
        .settings_replace_error = APP_ERROR_STORAGE_FULL,
    };
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL, outcome.detail);
    TEST_CHECK_EQ_U64(1U, fake.password_transition_begin_calls);
    TEST_CHECK_EQ_U64(0U, fake.password_activate_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_calls);
    TEST_CHECK_EQ_U64(1U, fake.password_transition_end_calls);
    TEST_CHECK(!fake.password_transition_active);
}

static void test_change_password_transition_begin_failure_changes_nothing(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .password_matches = true,
        .password_transition_begin_error = APP_ERROR_CONFLICT,
    };
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, outcome.detail);
    TEST_CHECK_EQ_U64(1U, fake.password_transition_begin_calls);
    TEST_CHECK_EQ_U64(0U, fake.settings_read_calls);
    TEST_CHECK_EQ_U64(0U, fake.password_verify_calls);
    TEST_CHECK_EQ_U64(0U, fake.password_create_calls);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK_EQ_U64(0U, fake.password_activate_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_calls);
    TEST_CHECK_EQ_U64(0U, fake.password_transition_end_calls);
    TEST_CHECK(!fake.password_transition_active);
}

static void test_change_password_invalidate_failure(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .password_matches = true,
        .invalidate_error = APP_ERROR_INTERNAL,
    };
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_COMMITTED_SESSION_INVALIDATION_INCOMPLETE,
                      outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
    TEST_CHECK_EQ_U64(1U, fake.settings_replace_calls);
    TEST_CHECK_EQ_U64(1U, fake.password_activate_calls);
    TEST_CHECK_EQ_U64(1U, fake.invalidate_calls);
    TEST_CHECK_EQ_U64(1U, fake.password_transition_end_calls);
    TEST_CHECK(!fake.password_transition_active);
    TEST_CHECK_EQ_U64(fake.committed_candidate.password_iterations,
                      fake.active_candidate.password_iterations);
}

static void test_change_password_invalid_ops_wipes_body(void) {
    fake_t fake = {.current = provisioned_settings(), .password_matches = true};
    web_settings_ops_t ops = operations(&fake);
    ops.password_verify = NULL; /* makes ops_valid_change_password() reject it */
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INTERNAL, outcome.result);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_change_password_duplicate_field_rejected(void) {
    fake_t fake = {.current = provisioned_settings(), .password_matches = true};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"currentPassword\":\"dup\","
               "\"newPassword\":\"new-example-password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INVALID_BODY, outcome.result);
}

static void test_change_password_unknown_field_rejected(void) {
    fake_t fake = {.current = provisioned_settings(), .password_matches = true};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\",\"extra\":\"x\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INVALID_BODY, outcome.result);
}

static void test_change_password_wrong_type_field_rejected(void) {
    fake_t fake = {.current = provisioned_settings(), .password_matches = true};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":123,\"newPassword\":\"new-example-password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INVALID_BODY, outcome.result);
}

static void test_change_password_settings_read_backend_unavailable(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .password_matches = true,
        .settings_read_error = APP_ERROR_STORAGE_UNAVAILABLE,
    };
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);
    TEST_CHECK_EQ_U64(0U, fake.password_verify_calls);
}

static void test_change_password_verify_backend_unavailable(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .password_verify_error = APP_ERROR_INTERNAL,
    };
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
    TEST_CHECK_EQ_U64(0U, fake.password_create_calls);
}

static void test_change_password_create_backend_unavailable(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .password_matches = true,
        .password_create_error = APP_ERROR_INTERNAL,
    };
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_change_password_prepare_candidate_failure_is_internal(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .password_matches = true,
        .bad_credential_material = true,
    };
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
               "password\"}");

    const web_change_password_outcome_t outcome =
        web_change_password_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_INTERNAL, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK_EQ_U64(0U, fake.invalidate_calls);
}
