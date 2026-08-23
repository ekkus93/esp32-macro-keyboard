/* -------------------------------------------------------------------------
 * GET/PUT /api/v1/settings
 * ---------------------------------------------------------------------- */

static void test_settings_get_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/settings", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard",
                         cJSON_GetObjectItemCaseSensitive(root, "deviceName")->valuestring);

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": provisioned_settings() above already produces every
     * field contracts/v2/api/examples.json's "settings" example claims
     * (app_v2_device_settings_init_unprovisioned()'s own defaults already
     * match sendMode/snapshotRetentionTarget/
     * requireSerialConfirmation/lastSelectedPackageId, and
     * provisioned_settings() overrides deviceName/apSsid to the same
     * strings), so this diffs the whole live response against it wholesale. */
    const cJSON *example = test_examples_fixture_get("settings");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_settings_get_rejects_oversized_request_id(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    char request_id[WEB_API_REQUEST_ID_MAX_BYTES + 2U];
    memset(request_id, 'a', sizeof(request_id) - 1U);
    request_id[sizeof(request_id) - 1U] = '\0';
    fake_httpd_add_request_header(&fake, "X-Request-ID", request_id);
    bind_bodyless(&request, &fake, "/api/v1/settings", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
    TEST_CHECK(fake_httpd_response_header(&fake, "X-Request-ID") == NULL);
}

static void test_settings_put_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings", HTTP_PUT,
                   "{\"deviceName\":\"New Device Name\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("New Device Name", g_device_settings_record.device_name);
    cJSON *root = parse_response(&fake);
    const cJSON *settings = cJSON_GetObjectItemCaseSensitive(root, "settings");
    TEST_CHECK(settings != NULL);
    TEST_CHECK_EQ_STRING("New Device Name",
                         cJSON_GetObjectItemCaseSensitive(settings, "deviceName")->valuestring);
    cJSON_Delete(root);
}

/* TODO_V2 V2-057 "consume the same checked-in examples from C and TypeScript
 * tests": submits contracts/v2/api/examples.json's own "settingsUpdate" body
 * verbatim against the "settings" example's baseline (provisioned_settings()
 * above already matches it, see test_settings_get_valid()), so the response
 * can be diffed against "settingsUpdated" wholesale, including
 * "restartRequired"/"reconnectRequired".
 *
 * Both examples.json's "settingsUpdated" AND docs/SPEC_V2.md 13.9's own
 * inline JSON for this exact request previously showed both flags false,
 * which contradicted the very next sentence in SPEC_V2 13.9 ("Changing
 * access-point credentials sets both flags to true") and
 * apply_access_point() (settings_contract_v2.c), which does exactly that
 * unconditionally whenever a request carries "accessPoint" -- which this
 * request does. Confirmed as a stale example (not a code defect), reported
 * to and fixed by Phil per this project's frozen-spec discipline -- see
 * docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md for the
 * original discrepancy writeup. examples.json and SPEC_V2.md now both read
 * true/true, matching this test's separate exact assertion below. */
static void test_settings_put_valid_matches_example(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings", HTTP_PUT,
                   "{\"deviceName\":\"Desk Macro Keyboard\",\"requireSerialConfirmation\":false,"
                   "\"sendMode\":\"quick\",\"snapshotRetentionTarget\":5,"
                   "\"lastSelectedPackageId\":null,"
                   "\"accessPoint\":{\"ssid\":\"MacroKeyboard\",\"passphrase\":\"new-example-"
                   "passphrase\"},\"station\":{\"ssid\":\"OfficeWiFi\",\"passphrase\":\"station-"
                   "example-passphrase\"}}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);

    cJSON *root = parse_response(&fake);
    /* The real, prose-conformant values for this exact request -- see this
     * test's own comment. */
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "restartRequired")));
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "reconnectRequired")));

    const cJSON *example = test_examples_fixture_get("settingsUpdated");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_settings_put_unauthorized_expired_session(void) {
    reset_fakes();
    g_auth_session_validate_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings", HTTP_PUT,
                   "{\"deviceName\":\"New Device Name\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    /* The policy gate ran (and rejected) before dispatch reached
     * handle_settings_put(), so the backend was never touched. */
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard", g_device_settings_record.device_name);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/settings/change-password
 * ---------------------------------------------------------------------- */

static void test_change_password_post_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings/change-password", HTTP_POST,
                   "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
                   "password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    /* This is exactly the case that caught web_server_api.c's status_text()
     * bug: WEB_HTTP_STATUS_NO_CONTENT (204) had no case in that switch, so
     * a *successful* change-password response was previously sent to the
     * client as "500 Internal Server Error" even though the password was
     * already changed and every session already invalidated -- a defect no
     * existing test (all of which call web_api_handle_administration()
     * directly and only ever inspect response.status, not the wire status
     * line send_api_response()/status_text() actually produce) could catch.
     * Fixed in the same commit as this test, see web_server_api.c. */
    TEST_CHECK_EQ_STRING("204 No Content", fake.response_status);
    TEST_CHECK_EQ_U64(0U, fake.response_body_length);
    TEST_CHECK_EQ_STRING("MKSESSION=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0",
                         fake_httpd_response_header(&fake, "Set-Cookie"));
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_logout_all_calls);
    TEST_CHECK_EQ_U64(AUTH_PBKDF2_ITERATIONS,
                      (uint64_t)server_configuration.password_record.iterations);
    TEST_CHECK_EQ_U64(0x77U, (uint64_t)server_configuration.password_record.salt[0]);
    TEST_CHECK_EQ_U64(0x88U, (uint64_t)server_configuration.password_record.hash[0]);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

static void test_change_password_post_committed_invalidation_failure_is_explicit(void) {
    reset_fakes();
    g_logout_all_result = APP_ERROR_INTERNAL;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings/change-password", HTTP_POST,
                   "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
                   "password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("409 Conflict", fake.response_status);
    TEST_CHECK_EQ_STRING("MKSESSION=; HttpOnly; SameSite=Strict; Path=/; Max-Age=0",
                         fake_httpd_response_header(&fake, "Set-Cookie"));
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_logout_all_calls);
    TEST_CHECK_EQ_U64(AUTH_PBKDF2_ITERATIONS,
                      (uint64_t)g_device_settings_record.password_iterations);
    TEST_CHECK_EQ_U64(AUTH_PBKDF2_ITERATIONS,
                      (uint64_t)server_configuration.password_record.iterations);
    TEST_CHECK_EQ_U64(0x77U, (uint64_t)g_device_settings_record.password_salt[0]);
    TEST_CHECK_EQ_U64(0x77U, (uint64_t)server_configuration.password_record.salt[0]);

    cJSON *root = parse_response(&fake);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK(error != NULL);
    TEST_CHECK_EQ_STRING("auth_state_incomplete",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK_EQ_STRING(
        "password changed; session invalidation incomplete; sign in with the new password",
        cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);
    cJSON_Delete(root);
}

static void test_change_password_post_busy_before_commit_keeps_cookie(void) {
    reset_fakes();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_server_password_transition_begin());
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings/change-password", HTTP_POST,
                   "{\"currentPassword\":\"old-example-password\",\"newPassword\":\"new-example-"
                   "password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
    TEST_CHECK(fake_httpd_response_header(&fake, "Set-Cookie") == NULL);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_logout_all_calls);
    TEST_CHECK_EQ_U64(120000U, (uint64_t)g_device_settings_record.password_iterations);
    cJSON *root = parse_response(&fake);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("conflict", cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    cJSON_Delete(root);
    web_server_password_transition_end();
}

static void test_change_password_post_incorrect_current_password(void) {
    reset_fakes();
    g_password_matches = false;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/settings/change-password", HTTP_POST,
                   "{\"currentPassword\":\"wrong-password\",\"newPassword\":\"new-example-"
                   "password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("403 Forbidden", fake.response_status);
    TEST_CHECK(fake_httpd_response_header(&fake, "Set-Cookie") == NULL);
}
