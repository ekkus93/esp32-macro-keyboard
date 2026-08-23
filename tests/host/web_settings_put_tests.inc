/* ---------------------------------------------------------------------- */
/* PUT /api/v1/settings                                                    */
/* ---------------------------------------------------------------------- */

static void test_put_device_name_success(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":\"New Name\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK_EQ_STRING("New Name", fake.committed_candidate.device_name);
    TEST_CHECK(json_contains(json, "\"deviceName\":\"New Name\""));
    TEST_CHECK(json_contains(json, "\"restartRequired\":false"));
    TEST_CHECK(json_contains(json, "\"reconnectRequired\":false"));
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
    free(json);
}

static void test_put_access_point_sets_restart_and_reconnect(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"accessPoint\":{\"ssid\":\"NewAp\",\"passphrase\":\"new-example-passphrase\"}}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK_EQ_STRING("NewAp", fake.committed_candidate.ap_ssid);
    TEST_CHECK(json_contains(json, "\"restartRequired\":true"));
    TEST_CHECK(json_contains(json, "\"reconnectRequired\":true"));
    free(json);
}

static void test_put_last_selected_package_id_null_clears(void) {
    fake_t fake = {.current = provisioned_settings()};
    memcpy(fake.current.last_selected_package_id, "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee",
           sizeof("aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee"));
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"lastSelectedPackageId\":null}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK_EQ_STRING("", fake.committed_candidate.last_selected_package_id);
    TEST_CHECK(json_contains(json, "\"lastSelectedPackageId\":null"));
    free(json);
}

static void test_put_station_removal(void) {
    fake_t fake = {.current = provisioned_settings()};
    fake.current.station_configured = true;
    memcpy(fake.current.station_ssid, "OfficeWiFi", sizeof("OfficeWiFi"));
    memcpy(fake.current.station_passphrase, "station-example-passphrase",
           sizeof("station-example-passphrase"));
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"station\":null}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK(!fake.committed_candidate.station_configured);
    TEST_CHECK(json_contains(json, "\"stationConfigured\":false"));
    TEST_CHECK(json_contains(json, "\"restartRequired\":true"));
    TEST_CHECK(json_contains(json, "\"reconnectRequired\":false"));
    free(json);
}

static void test_put_empty_object_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_EMPTY, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_put_unknown_field_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"unknownField\":true}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_read_calls);
}

static void test_put_duplicate_field_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":\"A\",\"deviceName\":\"B\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_wrong_type_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"requireSerialConfirmation\":\"false\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_invalid_send_mode_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"sendMode\":\"eventually\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_SEND_MODE, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_read_calls);
}

static void test_put_access_point_missing_passphrase_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"accessPoint\":{\"ssid\":\"NewAp\"}}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_station_wrong_type_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"station\":\"OfficeWiFi\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_invalid_device_name_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":\"\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_DEVICE_NAME, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_put_snapshot_retention_out_of_range_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"snapshotRetentionTarget\":101}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_SNAPSHOT_RETENTION_TARGET, outcome.result);
}

static void test_put_settings_read_backend_unavailable(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .settings_read_error = APP_ERROR_STORAGE_UNAVAILABLE,
    };
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":\"New Name\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);
}

static void test_put_settings_replace_backend_unavailable(void) {
    fake_t fake = {
        .current = provisioned_settings(),
        .settings_replace_error = APP_ERROR_STORAGE_FULL,
    };
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":\"New Name\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL, outcome.detail);
}

static void test_put_invalid_ops_wipes_body(void) {
    web_settings_ops_t ops = {0}; /* missing settings_read/settings_replace */
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":\"New Name\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INTERNAL, outcome.result);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
    TEST_CHECK_EQ_PTR(NULL, json);
}

static void test_put_body_without_nul_terminator_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    memset(body, 'x', sizeof(body));
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_put_embedded_nul_escape_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":\"A\\u0000B\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_put_malformed_json_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_trailing_content_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":\"New Name\"}garbage");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_non_object_top_level_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "[]");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_device_name_wrong_type_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"deviceName\":123}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_access_point_extra_credential_field_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"accessPoint\":{\"ssid\":\"NewAp\",\"passphrase\":\"new-example-passphrase\","
               "\"extra\":\"x\"}}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_require_serial_confirmation_success(void) {
    fake_t fake = {.current = provisioned_settings()};
    fake.current.require_serial_confirmation = false;
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"requireSerialConfirmation\":true}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK(fake.committed_candidate.require_serial_confirmation);
    TEST_CHECK(json_contains(json, "\"requireSerialConfirmation\":true"));
    free(json);
}

static void test_put_send_mode_preview_success(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"sendMode\":\"preview\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_PREVIEW, fake.committed_candidate.send_mode);
    TEST_CHECK(json_contains(json, "\"sendMode\":\"preview\""));
    free(json);
}

static void test_put_send_mode_quick_success(void) {
    fake_t fake = {.current = provisioned_settings()};
    fake.current.send_mode = APP_V2_SEND_MODE_PREVIEW;
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"sendMode\":\"quick\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_QUICK, fake.committed_candidate.send_mode);
    TEST_CHECK(json_contains(json, "\"sendMode\":\"quick\""));
    free(json);
}

static void test_put_snapshot_retention_negative_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"snapshotRetentionTarget\":-1}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_snapshot_retention_fractional_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"snapshotRetentionTarget\":50.5}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_last_selected_package_id_wrong_type_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"lastSelectedPackageId\":42}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_last_selected_package_id_valid_string_success(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"lastSelectedPackageId\":\"aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK_EQ_STRING("aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee",
                         fake.committed_candidate.last_selected_package_id);
    TEST_CHECK(json_contains(json, "\"lastSelectedPackageId\":\"aaaaaaaa-bbbb-4ccc-8ddd-"
                                   "eeeeeeeeeeee\""));
    free(json);
}

static void test_put_station_valid_object_success(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"station\":{\"ssid\":\"OfficeWiFi\",\"passphrase\":\"station-example-"
               "passphrase\"}}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK(fake.committed_candidate.station_configured);
    TEST_CHECK_EQ_STRING("OfficeWiFi", fake.committed_candidate.station_ssid);
    TEST_CHECK(json_contains(json, "\"stationConfigured\":true"));
    TEST_CHECK(json_contains(json, "\"restartRequired\":true"));
    TEST_CHECK(json_contains(json, "\"reconnectRequired\":false"));
    free(json);
}

static void test_put_send_mode_wrong_type_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"sendMode\":42}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

/* The five tests below each carry a validly-shaped field past
 * exact_settings_put_fields()/populate_settings_update_request() so that
 * map_prepare_update_result() maps app_v2_settings_prepare_update()'s
 * contract-level rejection to its web_settings_put_result_t counterpart --
 * distinct from the field-shape rejections above (missing/extra/wrong-typed
 * fields), which never reach app_v2_settings_prepare_update() at all. */

static void test_put_access_point_ssid_too_long_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    /* APP_V2_WIFI_SSID_MAX_BYTES is 32; this ssid is 33 bytes. */
    build_body(body, sizeof(body),
               "{\"accessPoint\":{\"ssid\":\"123456789012345678901234567890123\","
               "\"passphrase\":\"new-example-passphrase\"}}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_ACCESS_POINT_SSID, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_put_access_point_passphrase_too_short_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    /* APP_V2_WIFI_PASSPHRASE_MIN_BYTES is 8; "short12" is 7 bytes. */
    build_body(body, sizeof(body),
               "{\"accessPoint\":{\"ssid\":\"NewAp\",\"passphrase\":\"short12\"}}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_ACCESS_POINT_PASSPHRASE, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_put_station_ssid_too_long_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"station\":{\"ssid\":\"123456789012345678901234567890123\","
               "\"passphrase\":\"station-example-passphrase\"}}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_STATION_SSID, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_put_station_passphrase_too_short_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"station\":{\"ssid\":\"OfficeWiFi\",\"passphrase\":\"short12\"}}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_STATION_PASSPHRASE, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_put_last_selected_package_id_malformed_uuid_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"lastSelectedPackageId\":\"not-a-uuid\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_LAST_SELECTED_PACKAGE_ID, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}
