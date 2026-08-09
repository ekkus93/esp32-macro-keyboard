#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api_contracts_v2.h"
#include "app_error.h"
#include "device_settings_v2.h"
#include "setup_contract_v2.h"
#include "test_assert.h"
#include "web_settings.h"

#define TEST_BODY_CAPACITY 1024U

typedef struct {
    app_v2_device_settings_t current;
    app_error_code_t settings_read_error;
    app_error_code_t settings_replace_error;
    app_error_code_t password_verify_error;
    app_error_code_t password_create_error;
    app_error_code_t invalidate_error;
    bool password_matches;
    bool bad_credential_material;
    unsigned int settings_read_calls;
    unsigned int settings_replace_calls;
    unsigned int password_verify_calls;
    unsigned int password_create_calls;
    unsigned int invalidate_calls;
    app_v2_device_settings_t committed_candidate;
    char observed_current_password[256];
    char observed_new_password[256];
} fake_t;

static app_error_code_t fake_settings_read(void *context, app_v2_device_settings_t *out_settings) {
    fake_t *fake = context;
    ++fake->settings_read_calls;
    if (fake->settings_read_error != APP_ERROR_NONE) {
        return fake->settings_read_error;
    }
    *out_settings = fake->current;
    return APP_ERROR_NONE;
}

static app_error_code_t
fake_settings_replace(void *context, const app_v2_device_settings_t *settings, bool *out_changed) {
    fake_t *fake = context;
    ++fake->settings_replace_calls;
    fake->committed_candidate = *settings;
    if (fake->settings_replace_error != APP_ERROR_NONE) {
        *out_changed = false;
        return fake->settings_replace_error;
    }
    *out_changed = true;
    fake->current = *settings;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_password_verify(void *context, const char *password,
                                             size_t password_length,
                                             const app_v2_device_settings_t *settings,
                                             bool *out_matches) {
    fake_t *fake = context;
    (void)settings;
    ++fake->password_verify_calls;
    TEST_CHECK(password_length < sizeof(fake->observed_current_password));
    memcpy(fake->observed_current_password, password, password_length);
    fake->observed_current_password[password_length] = '\0';
    if (fake->password_verify_error != APP_ERROR_NONE) {
        return fake->password_verify_error;
    }
    *out_matches = fake->password_matches;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_password_create(void *context, const char *password,
                                             size_t password_length,
                                             app_v2_setup_password_material_t *out_material) {
    fake_t *fake = context;
    ++fake->password_create_calls;
    TEST_CHECK(password_length < sizeof(fake->observed_new_password));
    memcpy(fake->observed_new_password, password, password_length);
    fake->observed_new_password[password_length] = '\0';
    if (fake->password_create_error != APP_ERROR_NONE) {
        return fake->password_create_error;
    }
    memset(out_material, 0, sizeof(*out_material));
    out_material->credential_version =
        (uint16_t)(fake->bad_credential_material ? APP_V2_CREDENTIAL_VERSION + 1U
                                                 : APP_V2_CREDENTIAL_VERSION);
    out_material->password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    out_material->password_iterations = 5500U;
    memset(out_material->password_salt, 0x77, sizeof(out_material->password_salt));
    memset(out_material->password_verifier, 0x88, sizeof(out_material->password_verifier));
    return APP_ERROR_NONE;
}

static app_error_code_t fake_invalidate_all_sessions(void *context) {
    fake_t *fake = context;
    ++fake->invalidate_calls;
    return fake->invalidate_error;
}

static web_settings_ops_t operations(fake_t *fake) {
    return (web_settings_ops_t){
        .context = fake,
        .settings_read = fake_settings_read,
        .settings_replace = fake_settings_replace,
        .password_verify = fake_password_verify,
        .password_create = fake_password_create,
        .invalidate_all_sessions = fake_invalidate_all_sessions,
    };
}

static app_v2_device_settings_t provisioned_settings(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    settings.provisioned = true;
    settings.credential_version = APP_V2_CREDENTIAL_VERSION;
    settings.password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    settings.password_iterations = 120000U;
    memset(settings.password_salt, 0x11, sizeof(settings.password_salt));
    memset(settings.password_verifier, 0x22, sizeof(settings.password_verifier));
    memcpy(settings.device_name, "Desk Macro Keyboard", sizeof("Desk Macro Keyboard"));
    memcpy(settings.ap_ssid, "MacroKeyboard", sizeof("MacroKeyboard"));
    memcpy(settings.ap_passphrase, "example-passphrase", sizeof("example-passphrase"));
    return settings;
}

static void build_body(char *buffer, size_t capacity, const char *json) {
    const int written = snprintf(buffer, capacity, "%s", json);
    TEST_CHECK(written > 0 && (size_t)written < capacity);
}

static bool buffer_all_zero(const char *buffer, size_t length) {
    for (size_t index = 0U; index < length; ++index) {
        if (buffer[index] != '\0') {
            return false;
        }
    }
    return true;
}

static bool json_contains(const char *json, const char *needle) {
    return json != NULL && strstr(json, needle) != NULL;
}

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

static void test_put_show_macro_source_previews_wrong_type_rejected(void) {
    fake_t fake = {.current = provisioned_settings()};
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"showMacroSourcePreviews\":\"yes\"}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_INVALID_BODY, outcome.result);
}

static void test_put_show_macro_source_previews_success(void) {
    fake_t fake = {.current = provisioned_settings()};
    fake.current.show_macro_source_previews = false;
    const web_settings_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"showMacroSourcePreviews\":true}");
    char *json = NULL;

    const web_settings_put_outcome_t outcome =
        web_settings_put_handle(body, sizeof(body), &ops, &json);

    TEST_CHECK_EQ_INT(WEB_SETTINGS_PUT_OK, outcome.result);
    TEST_CHECK(fake.committed_candidate.show_macro_source_previews);
    TEST_CHECK(json_contains(json, "\"showMacroSourcePreviews\":true"));
    free(json);
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
    build_body(body, sizeof(body), "{\"accessPoint\":{\"ssid\":\"NewAp\",\"passphrase\":\"short12\"}}");
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
    build_body(body, sizeof(body), "{\"station\":{\"ssid\":\"OfficeWiFi\",\"passphrase\":\"short12\"}}");
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
    TEST_CHECK_EQ_U64(1U, fake.invalidate_calls);
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
    TEST_CHECK_EQ_U64(0U, fake.invalidate_calls);
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

    TEST_CHECK_EQ_INT(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_EQ_U64(1U, fake.settings_replace_calls);
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

int main(void) {
    test_get_success();
    test_get_success_with_optional_fields_present();
    test_get_invalid_ops_rejected();
    test_get_unprovisioned_backend_record_is_internal_error();
    test_get_backend_unavailable();
    test_put_device_name_success();
    test_put_access_point_sets_restart_and_reconnect();
    test_put_last_selected_package_id_null_clears();
    test_put_station_removal();
    test_put_empty_object_rejected();
    test_put_unknown_field_rejected();
    test_put_duplicate_field_rejected();
    test_put_wrong_type_rejected();
    test_put_invalid_send_mode_rejected();
    test_put_access_point_missing_passphrase_rejected();
    test_put_station_wrong_type_rejected();
    test_put_invalid_device_name_rejected();
    test_put_snapshot_retention_out_of_range_rejected();
    test_put_settings_read_backend_unavailable();
    test_put_settings_replace_backend_unavailable();
    test_put_invalid_ops_wipes_body();
    test_put_body_without_nul_terminator_rejected();
    test_put_embedded_nul_escape_rejected();
    test_put_malformed_json_rejected();
    test_put_trailing_content_rejected();
    test_put_non_object_top_level_rejected();
    test_put_device_name_wrong_type_rejected();
    test_put_access_point_extra_credential_field_rejected();
    test_put_require_serial_confirmation_success();
    test_put_send_mode_preview_success();
    test_put_send_mode_quick_success();
    test_put_snapshot_retention_negative_rejected();
    test_put_snapshot_retention_fractional_rejected();
    test_put_show_macro_source_previews_wrong_type_rejected();
    test_put_show_macro_source_previews_success();
    test_put_last_selected_package_id_wrong_type_rejected();
    test_put_last_selected_package_id_valid_string_success();
    test_put_station_valid_object_success();
    test_put_send_mode_wrong_type_rejected();
    test_put_access_point_ssid_too_long_rejected();
    test_put_access_point_passphrase_too_short_rejected();
    test_put_station_ssid_too_long_rejected();
    test_put_station_passphrase_too_short_rejected();
    test_put_last_selected_package_id_malformed_uuid_rejected();
    test_change_password_success();
    test_change_password_incorrect_current_password();
    test_change_password_new_password_too_short_rejected();
    test_change_password_missing_field_rejected();
    test_change_password_settings_replace_backend_unavailable();
    test_change_password_invalidate_failure();
    test_change_password_invalid_ops_wipes_body();
    test_change_password_duplicate_field_rejected();
    test_change_password_unknown_field_rejected();
    test_change_password_wrong_type_field_rejected();
    test_change_password_settings_read_backend_unavailable();
    test_change_password_verify_backend_unavailable();
    test_change_password_create_backend_unavailable();
    test_change_password_prepare_candidate_failure_is_internal();
    puts("web settings tests passed");
    return EXIT_SUCCESS;
}
