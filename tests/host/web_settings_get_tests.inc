/* ---------------------------------------------------------------------- */
/* GET /api/v1/settings                                                    */
/* ---------------------------------------------------------------------- */

static void test_get_success(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char *json = NULL;

    const web_settings_get_outcome_t outcome = web_settings_get_handle(&ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_GET_OK, outcome.result);
    TEST_CHECK(json_contains(json, "\"deviceName\":\"Desk Macro Keyboard\""));
    TEST_CHECK(json_contains(json, "\"apSsid\":\"MacroKeyboard\""));
    TEST_CHECK(json_contains(json, "\"sendMode\":\"quick\""));
    TEST_CHECK(!json_contains(json, "example-passphrase"));
    TEST_CHECK(!json_contains(json, "password"));
    free(json);
}

static void test_get_success_with_optional_fields_present(void) {
    fake_t fake = {.current = provisioned_settings()};
    fake.current.station_configured = true;
    memcpy(fake.current.station_ssid, "OfficeWiFi", sizeof("OfficeWiFi"));
    memcpy(fake.current.station_passphrase, "station-example-passphrase",
           sizeof("station-example-passphrase"));
    memcpy(fake.current.last_selected_package_id, "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee",
           sizeof("aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee"));
    const web_settings_ops_t ops = operations(&fake);
    char *json = NULL;

    const web_settings_get_outcome_t outcome = web_settings_get_handle(&ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_GET_OK, outcome.result);
    TEST_CHECK(json_contains(json, "\"lastSelectedPackageId\":\"aaaaaaaa-bbbb-4ccc-8ddd-"
                                   "eeeeeeeeeeee\""));
    TEST_CHECK(json_contains(json, "\"stationConfigured\":true"));
    TEST_CHECK(json_contains(json, "\"stationSsid\":\"OfficeWiFi\""));
    TEST_CHECK(!json_contains(json, "station-example-passphrase"));
    free(json);
}

static void test_get_invalid_ops_rejected(void) {
    char *json = NULL;
    const web_settings_get_outcome_t outcome = web_settings_get_handle(NULL, &json);
    TEST_CHECK_EQ_INT(WEB_SETTINGS_GET_INTERNAL, outcome.result);
    TEST_CHECK_EQ_PTR(NULL, json);
}

static void test_get_unprovisioned_backend_record_is_internal_error(void) {
    fake_t fake = {0};
    app_v2_device_settings_init_unprovisioned(&fake.current);
    const web_settings_ops_t ops = operations(&fake);
    char *json = NULL;

    const web_settings_get_outcome_t outcome = web_settings_get_handle(&ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_GET_INTERNAL, outcome.result);
    TEST_CHECK_EQ_PTR(NULL, json);
}

static void test_get_backend_unavailable(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .settings_read_error = APP_ERROR_STORAGE_UNAVAILABLE,
    };
    const web_settings_ops_t ops = operations(&fake);
    char *json = NULL;

    const web_settings_get_outcome_t outcome = web_settings_get_handle(&ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_GET_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);
    TEST_CHECK_EQ_PTR(NULL, json);
}
