/* -------------------------------------------------------------------------
 * GET /api/v1/diagnostics
 * ---------------------------------------------------------------------- */

static void test_diagnostics_get_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);

    cJSON *root = parse_response(&fake);
    /* reset_fakes() above chose every backend value (heap sizes, reset
     * reason, webfs/userdata capacity, blob count, USB/Wi-Fi/send state) to
     * equal contracts/v2/api/examples.json's own "diagnostics" fixture
     * exactly -- the same file webapp/tests/v2-api-contracts.test.ts
     * validates isDiagnosticsResponse(examples.diagnostics) against
     * (TODO_V2 V2-057's "consume the same checked-in examples from C and
     * TypeScript tests" bullet). The one field that cannot match literally
     * is "subsystems": the example leaves it "[]" as a documentation
     * placeholder, but a real device always reports all
     * WEB_DIAGNOSTICS_SUBSYSTEM_COUNT (8) entries (asserted separately
     * below) -- so this compares a copy of the response with "subsystems"
     * normalized to an empty array against the example, deep and
     * order-independent, rather than skip the comparison. */
    cJSON *comparable = cJSON_Duplicate(root, true);
    TEST_CHECK(comparable != NULL);
    TEST_CHECK(
        cJSON_ReplaceItemInObjectCaseSensitive(comparable, "subsystems", cJSON_CreateArray()));
    const cJSON *example = test_examples_fixture_get("diagnostics");
    TEST_CHECK(cJSON_Compare(comparable, example, true) != 0);
    cJSON_Delete(comparable);

    const cJSON *subsystems = cJSON_GetObjectItemCaseSensitive(root, "subsystems");
    TEST_CHECK_EQ_INT(8, cJSON_GetArraySize(subsystems));
    /* device_controls_get_health() above reports task_running=true and no
     * errors, so device_controls_health_derive_state() (the real,
     * unfaked device_controls_logic.c function) derives SUBSYSTEM_HEALTH_HEALTHY
     * -- proving the fake-to-pure-logic-to-JSON chain, not just that some
     * string came out. */
    const cJSON *controls_entry = NULL;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, subsystems) {
        const cJSON *name = cJSON_GetObjectItemCaseSensitive(entry, "name");
        if (strcmp(name->valuestring, "controls") == 0) {
            controls_entry = entry;
        }
    }
    TEST_CHECK(controls_entry != NULL);
    TEST_CHECK_EQ_STRING("healthy",
                         cJSON_GetObjectItemCaseSensitive(controls_entry, "state")->valuestring);
    cJSON_Delete(root);
}

static void test_diagnostics_release_fault_degrades_executor_subsystem(void) {
    reset_fakes();
    executor_health_record_cleanup(APP_ERROR_USB_NOT_READY, true);
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    cJSON *root = parse_response(&fake);
    const cJSON *subsystems = cJSON_GetObjectItemCaseSensitive(root, "subsystems");
    TEST_CHECK_EQ_INT(8, cJSON_GetArraySize(subsystems));

    const cJSON *executor_entry = NULL;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, subsystems) {
        const cJSON *name = cJSON_GetObjectItemCaseSensitive(entry, "name");
        if (cJSON_IsString(name) && strcmp(name->valuestring, "executor") == 0) {
            executor_entry = entry;
        }
    }
    TEST_CHECK(executor_entry != NULL);
    TEST_CHECK_EQ_STRING("failed",
                         cJSON_GetObjectItemCaseSensitive(executor_entry, "state")->valuestring);
    cJSON_Delete(root);
}

static void test_diagnostics_async_failure_degrades_existing_http_subsystem(void) {
    reset_fakes();
    http_health_record_async_failure(HTTP_ASYNC_FAILURE_COMPLETION, APP_ERROR_IO);
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    cJSON *root = parse_response(&fake);
    const cJSON *subsystems = cJSON_GetObjectItemCaseSensitive(root, "subsystems");
    TEST_CHECK_EQ_INT(8, cJSON_GetArraySize(subsystems));

    const cJSON *http_entry = NULL;
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, subsystems) {
        const cJSON *name = cJSON_GetObjectItemCaseSensitive(entry, "name");
        if (cJSON_IsString(name) && strcmp(name->valuestring, "http") == 0) {
            http_entry = entry;
        }
    }
    TEST_CHECK(http_entry != NULL);
    TEST_CHECK_EQ_STRING("failed",
                         cJSON_GetObjectItemCaseSensitive(http_entry, "state")->valuestring);
    cJSON_Delete(root);
}

static void test_diagnostics_get_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_diagnostics_get_unauthorized_expired_session(void) {
    reset_fakes();
    g_auth_session_validate_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_diagnostics_get_pending_reset_reports_recovery(void) {
    reset_fakes();
    g_factory_reset_state = FACTORY_RESET_STATE_PENDING;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
    cJSON *root = parse_response(&fake);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("reset_recovery_required",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK_EQ_STRING("factory reset recovery in progress",
                         cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);
    cJSON_Delete(root);
}

static void test_diagnostics_get_blob_scan_failure_maps_to_503(void) {
    reset_fakes();
    g_blob_diagnostics_result = APP_ERROR_STORAGE_UNAVAILABLE;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_bodyless(&request, &fake, "/api/v1/diagnostics", HTTP_GET);

    TEST_CHECK_EQ_INT(ESP_OK, api_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}
