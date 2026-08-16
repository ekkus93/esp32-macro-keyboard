/* -------------------------------------------------------------------------
 * POST /api/v1/device/reset-settings
 * ---------------------------------------------------------------------- */

static void test_reset_settings_post_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/reset-settings", HTTP_POST,
                   "{\"confirmation\":\"RESET SETTINGS\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_reset_settings_calls);
    /* DEVICE_RESET_SETTINGS is not in restart_after_response()'s route set
     * (only DEVICE_RESTART/DEVICE_FACTORY_RESET are) -- device_controls
     * itself performs the reboot, web_server_api.c does not call
     * esp_restart() a second time. */
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": web_device_reset_accepted_json(false, true, ...)'s
     * two bool arguments (web_api_administration.c's call site) have no
     * backend dependency (both are fixed literals for this route), so this
     * diffs the live response against contracts/v2/api/examples.json's
     * "resetSettingsAccepted" example wholesale. */
    cJSON *root = parse_response(&fake);
    const cJSON *example = test_examples_fixture_get("resetSettingsAccepted");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_reset_settings_post_wrong_confirmation(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/reset-settings", HTTP_POST,
                   "{\"confirmation\":\"wrong\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    /* APP_ERROR_INVALID_ARGUMENT maps to 422 (web_api_http_status_for_error()),
     * not 400 -- SPEC_V2 13.2's validation-failure status. */
    TEST_CHECK_EQ_STRING("422 Unprocessable Entity", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_reset_settings_calls);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/factory-reset
 * ---------------------------------------------------------------------- */

static void test_factory_reset_post_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/factory-reset", HTTP_POST,
                   "{\"confirmation\":\"FACTORY RESET\",\"adminPassword\":\"example-password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_factory_reset_calls);
    /* The response must be the ONLY thing this path emits. device_controls
     * schedules the real restart (DEVICE_CONTROLS_RESTART_DELAY_MS); restarting
     * immediately here preempted that timer and reset the chip before lwIP put
     * the accepted response on the wire, so a real client received zero bytes
     * against a contract promising 202 -- measured in
     * docs/implementation-v2/H12_122_RESTART_RESPONSE_FINDING_2026-08-16.md.
     * A non-zero count here means that fast path came back. */
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": web_device_reset_accepted_json(true, false, ...)'s
     * call site for this route (web_api_administration.c) also passes two
     * fixed literals, so this diffs the live response against
     * contracts/v2/api/examples.json's "factoryResetAccepted" example
     * wholesale. */
    cJSON *root = parse_response(&fake);
    const cJSON *example = test_examples_fixture_get("factoryResetAccepted");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_factory_reset_post_precommit_failure_is_not_accepted(void) {
    reset_fakes();
    g_factory_reset_outcome = (device_controls_factory_reset_outcome_t){
        .durably_accepted = false,
        .recovery_required = false,
        .primary_error = APP_ERROR_STORAGE_UNAVAILABLE,
    };
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/factory-reset", HTTP_POST,
                   "{\"confirmation\":\"FACTORY RESET\",\"adminPassword\":\"example-password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_factory_reset_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

static void test_factory_reset_post_cleanup_failure_stays_accepted_for_recovery(void) {
    reset_fakes();
    g_factory_reset_outcome = (device_controls_factory_reset_outcome_t){
        .durably_accepted = true,
        .recovery_required = true,
        .primary_error = APP_ERROR_IO,
    };
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/factory-reset", HTTP_POST,
                   "{\"confirmation\":\"FACTORY RESET\",\"adminPassword\":\"example-password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_factory_reset_calls);
    /* The response must be the ONLY thing this path emits. device_controls
     * schedules the real restart (DEVICE_CONTROLS_RESTART_DELAY_MS); restarting
     * immediately here preempted that timer and reset the chip before lwIP put
     * the accepted response on the wire, so a real client received zero bytes
     * against a contract promising 202 -- measured in
     * docs/implementation-v2/H12_122_RESTART_RESPONSE_FINDING_2026-08-16.md.
     * A non-zero count here means that fast path came back. */
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);

    cJSON *root = parse_response(&fake);
    const cJSON *example = test_examples_fixture_get("factoryResetAccepted");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_factory_reset_post_incorrect_password(void) {
    reset_fakes();
    g_password_matches = false;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "/api/v1/device/factory-reset", HTTP_POST,
                   "{\"confirmation\":\"FACTORY RESET\",\"adminPassword\":\"wrong-password\"}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("403 Forbidden", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_factory_reset_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

/* -------------------------------------------------------------------------
 * GET/POST /api/v1/setup (provisioned-mode conflict; SPEC_V2 13.4). No
 * session is required (WEB_API_ROUTE_SETUP is exempted by
 * web_api_route_requires_session()), so these two need no Cookie header --
 * the live pipeline still reaches setup_route_response() the same way an
 * unauthenticated caller's real request would.
 * ---------------------------------------------------------------------- */

static void test_setup_get_not_found_when_provisioned(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_bodyless(&request, &fake, "/api/v1/setup", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("404 Not Found", fake.response_status);
}

static void test_setup_post_conflict_when_provisioned(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    /* web_api_route_requires_body(WEB_API_ROUTE_SETUP, POST) is true (so an
     * already-provisioned device's real setup submission body is accepted
     * up through the policy gate rather than rejected with 422 before it
     * can reach the 409 conflict response setup_route_response() answers --
     * see web_api_core.c's comment on that route) -- so this needs a real
     * JSON body/Content-Type, unlike the GET case above. */
    bind_json_body(&request, &fake, "/api/v1/setup", HTTP_POST, "{}");

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("409 Conflict", fake.response_status);
}
