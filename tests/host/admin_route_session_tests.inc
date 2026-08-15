/* -------------------------------------------------------------------------
 * GET /api/v1/auth/session
 * ---------------------------------------------------------------------- */

static void test_session_get_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/auth/session", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);
    TEST_CHECK(fake_httpd_response_header(&fake, "X-Request-ID") != NULL);

    cJSON *root = parse_response(&fake);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "authenticated")));
    TEST_CHECK_EQ_U64(
        86400U,
        (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "idleExpiresInSeconds")->valuedouble);

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": reset_fakes()'s g_idle_seconds/g_absolute_seconds
     * already equal contracts/v2/api/examples.json's "session" example
     * exactly, so this diffs the whole live response against it wholesale. */
    const cJSON *example = test_examples_fixture_get("session");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_session_get_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_bodyless(&request, &fake, "/api/v1/auth/session", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING(
        "auth_required",
        cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(root, "error"), "code")
            ->valuestring);
    cJSON_Delete(root);
}

static void test_session_get_unauthorized_expired_session(void) {
    reset_fakes();
    g_auth_session_validate_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/auth/session", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/restart
 * ---------------------------------------------------------------------- */

static void test_restart_post_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/device/restart", HTTP_POST);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_restart_calls);
    /* api_handler() itself calls esp_restart() after a successful 202
     * restart/factory-reset response is sent -- this is the one behavior
     * only the live api_handler() path (not web_api_handle_administration()
     * called directly) can prove. */
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_esp_restart_calls);

    cJSON *root = parse_response(&fake);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "accepted")));

    /* TODO_V2 V2-057 "consume the same checked-in examples from C and
     * TypeScript tests": web_device_restart_accepted_json() has no backend
     * dependency (every field is a fixed literal), so this diffs the live
     * response against contracts/v2/api/examples.json's "restartAccepted"
     * example wholesale. */
    const cJSON *example = test_examples_fixture_get("restartAccepted");
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    cJSON_Delete(root);
}

static void test_restart_post_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_bodyless(&request, &fake, "/api/v1/device/restart", HTTP_POST);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_restart_calls);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}
