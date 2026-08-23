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
#include "web_server_setup_submit.h"

#define TEST_BODY_CAPACITY 1024U
#define TEST_SETUP_CODE "12345678"
#define TEST_OTHER_CODE "87654321"

typedef struct {
    app_v2_device_settings_t current;
    app_error_code_t settings_read_error;
    app_error_code_t password_error;
    app_error_code_t settings_replace_error;
    unsigned int settings_read_calls;
    unsigned int password_calls;
    unsigned int settings_replace_calls;
    unsigned int zero_calls;
    app_v2_device_settings_t committed_candidate;
    bool committed_changed;
    char observed_password[256];
    size_t observed_password_length;
} fake_setup_submit_t;

static app_error_code_t fake_settings_read(void *context, app_v2_device_settings_t *out_settings) {
    fake_setup_submit_t *fake = context;
    ++fake->settings_read_calls;
    if (fake->settings_read_error != APP_ERROR_NONE) {
        return fake->settings_read_error;
    }
    *out_settings = fake->current;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_password_create(void *context, const char *password,
                                             size_t password_length,
                                             app_v2_setup_password_material_t *out_material) {
    fake_setup_submit_t *fake = context;
    ++fake->password_calls;
    TEST_CHECK(password != NULL);
    TEST_CHECK(password_length < sizeof(fake->observed_password));
    memcpy(fake->observed_password, password, password_length);
    fake->observed_password[password_length] = '\0';
    fake->observed_password_length = password_length;
    if (fake->password_error != APP_ERROR_NONE) {
        return fake->password_error;
    }
    memset(out_material, 0, sizeof(*out_material));
    out_material->credential_version = APP_V2_CREDENTIAL_VERSION;
    out_material->password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    out_material->password_iterations = 120000U;
    memset(out_material->password_salt, 0x11, sizeof(out_material->password_salt));
    memset(out_material->password_verifier, 0x22, sizeof(out_material->password_verifier));
    return APP_ERROR_NONE;
}

static app_error_code_t
fake_settings_replace(void *context, const app_v2_device_settings_t *settings, bool *out_changed) {
    fake_setup_submit_t *fake = context;
    ++fake->settings_replace_calls;
    fake->committed_candidate = *settings;
    if (fake->settings_replace_error != APP_ERROR_NONE) {
        *out_changed = false;
        return fake->settings_replace_error;
    }
    *out_changed = true;
    fake->committed_changed = true;
    fake->current = *settings;
    return APP_ERROR_NONE;
}

static void fake_secure_zero(void *context, void *memory, size_t size) {
    fake_setup_submit_t *fake = context;
    memset(memory, 0, size);
    ++fake->zero_calls;
}

static web_setup_submit_ops_t operations(fake_setup_submit_t *fake) {
    return (web_setup_submit_ops_t){
        .context = fake,
        .settings_read = fake_settings_read,
        .password_create = fake_password_create,
        .settings_replace = fake_settings_replace,
        .secure_zero = fake_secure_zero,
    };
}

static app_v2_device_settings_t baseline_settings(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    settings.next_blob_id = 42U;
    settings.send_mode = APP_V2_SEND_MODE_PREVIEW;
    settings.snapshot_retention_target = 7U;
    return settings;
}

static void init_session(app_v2_setup_session_t *session, const char *code) {
    const app_v2_string_view_t view = {.data = code, .length = strlen(code)};
    TEST_CHECK_EQ_INT(APP_V2_SETUP_OK, app_v2_setup_session_init(session, view));
}

static void build_body(char *buffer, size_t capacity, const char *fields_json) {
    const int written = snprintf(buffer, capacity, "%s", fields_json);
    TEST_CHECK(written > 0 && (size_t)written < capacity);
}

static void build_request_body(char *buffer, size_t capacity, const char *setup_code,
                               const char *device_name, const char *ap_ssid,
                               const char *ap_passphrase, const char *admin_password,
                               bool require_confirmation) {
    const int written = snprintf(buffer, capacity,
                                 "{\"setupCode\":\"%s\",\"deviceName\":\"%s\",\"apSsid\":\"%s\","
                                 "\"apPassphrase\":\"%s\",\"adminPassword\":\"%s\","
                                 "\"requireSerialConfirmation\":%s}",
                                 setup_code, device_name, ap_ssid, ap_passphrase, admin_password,
                                 require_confirmation ? "true" : "false");
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

static void test_success_commits_and_consumes_code(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_OK, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, outcome.detail);
    TEST_CHECK(response.action.accepted);
    TEST_CHECK(response.restart_required);
    TEST_CHECK(response.action.connection_will_close);
    TEST_CHECK(!response.action.reprovisioning_required);

    TEST_CHECK_EQ_U64(1U, fake.settings_read_calls);
    TEST_CHECK_EQ_U64(1U, fake.password_calls);
    TEST_CHECK_EQ_U64(1U, fake.settings_replace_calls);
    TEST_CHECK_EQ_STRING("example-admin-password", fake.observed_password);

    TEST_CHECK(fake.committed_candidate.provisioned);
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard", fake.committed_candidate.device_name);
    TEST_CHECK_EQ_STRING("MacroKeyboard", fake.committed_candidate.ap_ssid);
    TEST_CHECK_EQ_STRING("example-passphrase", fake.committed_candidate.ap_passphrase);
    TEST_CHECK(!fake.committed_candidate.require_serial_confirmation);
    TEST_CHECK_EQ_U64(120000U, fake.committed_candidate.password_iterations);

    /* SPEC 12.3: setup must preserve configuration fields it does not name. */
    TEST_CHECK_EQ_U64(42U, fake.committed_candidate.next_blob_id);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_PREVIEW, fake.committed_candidate.send_mode);
    TEST_CHECK_EQ_U64(7U, fake.committed_candidate.snapshot_retention_target);

    TEST_CHECK(session.consumed);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
    TEST_CHECK(fake.zero_calls > 0U);
}

static void test_require_serial_confirmation_is_carried_through(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", true);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_OK, outcome.result);
    TEST_CHECK(fake.committed_candidate.require_serial_confirmation);
}

/* SPEC 12.3: the submitted AP SSID/passphrase become the device's access
 * point identity. An unprovisioned record always starts with empty AP
 * fields, so a successful submission is exactly the boot-to-boot transition
 * a device would apply on restart. */
static void test_reconnect_ap_credential_transition(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    TEST_CHECK_EQ_STRING("", fake.current.ap_ssid);
    TEST_CHECK_EQ_STRING("", fake.current.ap_passphrase);
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "NewNetworkName",
                       "new-network-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_OK, outcome.result);
    TEST_CHECK_EQ_STRING("NewNetworkName", fake.committed_candidate.ap_ssid);
    TEST_CHECK_EQ_STRING("new-network-passphrase", fake.committed_candidate.ap_passphrase);
    /* A boot that re-reads settings now sees the new AP identity. */
    TEST_CHECK_EQ_STRING("NewNetworkName", fake.current.ap_ssid);
    TEST_CHECK(response.restart_required);
}

static void test_wrong_code_rejected_before_commit(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_OTHER_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_CODE_MISMATCH, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK(!session.consumed);
    TEST_CHECK(!response.action.accepted);
}

/* Every unprovisioned boot generates a fresh code (Cutover A); a code
 * supplied from a previous boot's session is simply a code that does not
 * match the one currently held, i.e. the same CODE_MISMATCH path. */
static void test_reboot_stale_code_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t previous_boot_session;
    init_session(&previous_boot_session, TEST_OTHER_CODE);
    app_v2_setup_session_t current_boot_session;
    init_session(&current_boot_session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_OTHER_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &current_boot_session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_CODE_MISMATCH, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK(!previous_boot_session.consumed);
}

static void test_malformed_code_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), "1234", "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_MALFORMED_CODE, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_reused_code_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    TEST_CHECK_EQ_INT(APP_V2_SETUP_OK, app_v2_setup_session_consume(&session));
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_CODE_CONSUMED, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_already_provisioned_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    fake.current.provisioned = true;
    fake.current.password_iterations = 120000U;
    memset(fake.current.password_salt, 0x33, sizeof(fake.current.password_salt));
    memset(fake.current.password_verifier, 0x44, sizeof(fake.current.password_verifier));
    TEST_CHECK_EQ_INT(3U,
                      snprintf(fake.current.ap_ssid, sizeof(fake.current.ap_ssid), "%s", "OLD"));
    TEST_CHECK_EQ_INT(8U, snprintf(fake.current.ap_passphrase, sizeof(fake.current.ap_passphrase),
                                   "%s", "old-pass"));
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_ALREADY_PROVISIONED, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK(!session.consumed);
}

static void test_invalid_device_name_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_DEVICE_NAME, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK(!session.consumed);
}

static void test_invalid_ap_ssid_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_AP_SSID, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_invalid_ap_passphrase_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "short", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_AP_PASSPHRASE, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
}

static void test_invalid_admin_password_maps_from_password_create(void) {
    fake_setup_submit_t fake = {
        .current = baseline_settings(),
        .password_error = APP_ERROR_INVALID_ARGUMENT,
    };
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "short", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_ADMIN_PASSWORD, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK(!session.consumed);
}

static void test_settings_unavailable(void) {
    fake_setup_submit_t fake = {
        .current = baseline_settings(),
        .settings_read_error = APP_ERROR_STORAGE_UNAVAILABLE,
    };
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_SETTINGS_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);
    TEST_CHECK_EQ_U64(0U, fake.password_calls);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK(!session.consumed);
}

static void test_password_derivation_internal_failure(void) {
    fake_setup_submit_t fake = {
        .current = baseline_settings(),
        .password_error = APP_ERROR_INTERNAL,
    };
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_PASSWORD_DERIVATION_FAILED, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
    TEST_CHECK_EQ_U64(0U, fake.settings_replace_calls);
    TEST_CHECK(!session.consumed);
}

/* SPEC 12.3 / TODO V2-040: the setup code must not be spent on a failed
 * commit. This proves it by retrying the exact same session after a
 * simulated storage failure and observing the retry succeed. */
static void test_commit_failure_does_not_consume_code(void) {
    fake_setup_submit_t fake = {
        .current = baseline_settings(),
        .settings_replace_error = APP_ERROR_STORAGE_UNAVAILABLE,
    };
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t first_outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_COMMIT_FAILED, first_outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, first_outcome.detail);
    TEST_CHECK_EQ_U64(1U, fake.settings_replace_calls);
    TEST_CHECK(!session.consumed);

    fake.settings_replace_error = APP_ERROR_NONE;
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);
    const web_setup_submit_outcome_t retry_outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_OK, retry_outcome.result);
    TEST_CHECK_EQ_U64(2U, fake.settings_replace_calls);
    TEST_CHECK(session.consumed);
    TEST_CHECK(response.action.accepted);
}

static void test_unknown_field_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_body(
        body, sizeof(body),
        "{\"setupCode\":\"12345678\",\"deviceName\":\"Desk\",\"apSsid\":\"Macro\","
        "\"apPassphrase\":\"example-passphrase\",\"adminPassword\":\"example-admin-password\","
        "\"requireSerialConfirmation\":false,\"alwaysSelectPackage\":true}");
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_BODY, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_read_calls);
}

static void test_missing_field_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_body(
        body, sizeof(body),
        "{\"setupCode\":\"12345678\",\"deviceName\":\"Desk\",\"apSsid\":\"Macro\","
        "\"apPassphrase\":\"example-passphrase\",\"adminPassword\":\"example-admin-password\"}");
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_BODY, outcome.result);
}

static void test_duplicate_field_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"setupCode\":\"12345678\",\"setupCode\":\"12345678\",\"deviceName\":\"Desk\","
               "\"apSsid\":\"Macro\",\"apPassphrase\":\"example-passphrase\","
               "\"adminPassword\":\"example-admin-password\",\"requireSerialConfirmation\":false}");
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_BODY, outcome.result);
}

static void test_wrong_type_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_body(
        body, sizeof(body),
        "{\"setupCode\":\"12345678\",\"deviceName\":\"Desk\",\"apSsid\":\"Macro\","
        "\"apPassphrase\":\"example-passphrase\",\"adminPassword\":\"example-admin-password\","
        "\"requireSerialConfirmation\":\"false\"}");
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_BODY, outcome.result);
}

static void test_trailing_garbage_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_body(
        body, sizeof(body),
        "{\"setupCode\":\"12345678\",\"deviceName\":\"Desk\",\"apSsid\":\"Macro\","
        "\"apPassphrase\":\"example-passphrase\",\"adminPassword\":\"example-admin-password\","
        "\"requireSerialConfirmation\":false}garbage");
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_BODY, outcome.result);
}

static void test_not_an_object_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "[]");
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_BODY, outcome.result);
}

static void test_malformed_json_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{not json");
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_BODY, outcome.result);
}

static void test_embedded_nul_escape_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY];
    build_body(
        body, sizeof(body),
        "{\"setupCode\":\"12345678\",\"deviceName\":\"Desk\\u0000name\",\"apSsid\":\"Macro\","
        "\"apPassphrase\":\"example-passphrase\",\"adminPassword\":\"example-admin-password\","
        "\"requireSerialConfirmation\":false}");
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_BODY, outcome.result);
}

static void test_empty_body_rejected(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    char body[TEST_BODY_CAPACITY] = {0};
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};

    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, sizeof(body), &ops, &session, &response);

    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INVALID_BODY, outcome.result);
}

static void test_invalid_arguments(void) {
    fake_setup_submit_t fake = {.current = baseline_settings()};
    app_v2_setup_session_t session;
    init_session(&session, TEST_SETUP_CODE);
    const web_setup_submit_ops_t ops = operations(&fake);
    app_v2_setup_accepted_t response = {0};
    char body[TEST_BODY_CAPACITY];
    build_request_body(body, sizeof(body), TEST_SETUP_CODE, "Desk Macro Keyboard", "MacroKeyboard",
                       "example-passphrase", "example-admin-password", false);

    TEST_CHECK_EQ_INT(
        WEB_SETUP_SUBMIT_INTERNAL,
        web_setup_submit_handle(NULL, sizeof(body), &ops, &session, &response).result);
    TEST_CHECK_EQ_INT(
        WEB_SETUP_SUBMIT_INTERNAL,
        web_setup_submit_handle(body, sizeof(body), NULL, &session, &response).result);
    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INTERNAL,
                      web_setup_submit_handle(body, sizeof(body), &ops, NULL, &response).result);
    TEST_CHECK_EQ_INT(WEB_SETUP_SUBMIT_INTERNAL,
                      web_setup_submit_handle(body, sizeof(body), &ops, &session, NULL).result);
}

int main(void) {
    test_success_commits_and_consumes_code();
    test_require_serial_confirmation_is_carried_through();
    test_reconnect_ap_credential_transition();
    test_wrong_code_rejected_before_commit();
    test_reboot_stale_code_rejected();
    test_malformed_code_rejected();
    test_reused_code_rejected();
    test_already_provisioned_rejected();
    test_invalid_device_name_rejected();
    test_invalid_ap_ssid_rejected();
    test_invalid_ap_passphrase_rejected();
    test_invalid_admin_password_maps_from_password_create();
    test_settings_unavailable();
    test_password_derivation_internal_failure();
    test_commit_failure_does_not_consume_code();
    test_unknown_field_rejected();
    test_missing_field_rejected();
    test_duplicate_field_rejected();
    test_wrong_type_rejected();
    test_trailing_garbage_rejected();
    test_not_an_object_rejected();
    test_malformed_json_rejected();
    test_embedded_nul_escape_rejected();
    test_empty_body_rejected();
    test_invalid_arguments();
    puts("web server setup submit tests passed");
    return EXIT_SUCCESS;
}
