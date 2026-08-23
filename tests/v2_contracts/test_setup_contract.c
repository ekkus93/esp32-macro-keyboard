#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "setup_contract_v2.h"

static int failures = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            (void)fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #condition);           \
            ++failures;                                                                            \
        }                                                                                          \
    } while (0)

#define VIEW(text) ((app_v2_string_view_t){.data = (text), .length = sizeof(text) - 1U})

typedef struct {
    const uint32_t *values;
    size_t value_count;
    size_t index;
    bool fail;
} fake_random_t;

static bool fake_random_u32(void *context, uint32_t *out_value) {
    fake_random_t *fake = context;
    if (fake->fail || out_value == NULL || fake->index >= fake->value_count) {
        return false;
    }
    *out_value = fake->values[fake->index];
    ++fake->index;
    return true;
}

static app_v2_setup_password_material_t password_material(void) {
    app_v2_setup_password_material_t material = {
        .credential_version = APP_V2_CREDENTIAL_VERSION,
        .password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION,
        .password_iterations = UINT32_C(100000),
    };
    memset(material.password_salt, 0x11, sizeof(material.password_salt));
    memset(material.password_verifier, 0x22, sizeof(material.password_verifier));
    return material;
}

static app_v2_setup_request_t request_for_code(const char *code) {
    return (app_v2_setup_request_t){
        .setup_code = {.data = code, .length = strlen(code)},
        .device_name = VIEW("Desk Macro Keyboard"),
        .ap_ssid = VIEW("MacroKeyboard"),
        .ap_passphrase = VIEW("example-passphrase"),
        .admin_password = VIEW("example-admin-password"),
        .require_serial_confirmation = false,
    };
}

static void set_text(char *destination, size_t capacity, const char *text) {
    const size_t length = strlen(text);
    CHECK(length < capacity);
    if (length >= capacity) {
        return;
    }
    memset(destination, 0, capacity);
    memcpy(destination, text, length);
}

static app_v2_device_settings_t unprovisioned_with_preferences(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    settings.next_blob_id = UINT64_C(77);
    settings.send_mode = APP_V2_SEND_MODE_PREVIEW;
    settings.snapshot_retention_target = 9U;
    set_text(settings.last_selected_package_id, sizeof(settings.last_selected_package_id),
             "550e8400-e29b-41d4-a716-446655440000");
    CHECK(app_v2_device_settings_validate(&settings) == APP_V2_SETTINGS_OK);
    return settings;
}

static void test_random_generation_rejects_out_of_range_without_bias(void) {
    const uint32_t values[] = {UINT32_MAX, UINT32_C(12345678)};
    fake_random_t fake = {.values = values, .value_count = 2U};
    const app_v2_setup_random_ops_t operations = {
        .context = &fake,
        .random_u32 = fake_random_u32,
    };
    app_v2_setup_session_t session;
    CHECK(app_v2_setup_session_generate(&operations, &session) == APP_V2_SETUP_OK);
    CHECK(strcmp(session.code, "12345678") == 0);
    CHECK(!session.consumed);
    CHECK(fake.index == 2U);

    const uint32_t zero[] = {UINT32_C(0)};
    fake = (fake_random_t){.values = zero, .value_count = 1U};
    CHECK(app_v2_setup_session_generate(&operations, &session) == APP_V2_SETUP_OK);
    CHECK(strcmp(session.code, "00000000") == 0);
}

static void test_random_generation_fails_closed(void) {
    uint32_t rejected[APP_V2_SETUP_RANDOM_ATTEMPTS_MAX];
    for (size_t index = 0U; index < (size_t)APP_V2_SETUP_RANDOM_ATTEMPTS_MAX; ++index) {
        rejected[index] = UINT32_MAX;
    }
    fake_random_t fake = {
        .values = rejected,
        .value_count = (size_t)APP_V2_SETUP_RANDOM_ATTEMPTS_MAX,
    };
    const app_v2_setup_random_ops_t operations = {
        .context = &fake,
        .random_u32 = fake_random_u32,
    };
    app_v2_setup_session_t session;
    CHECK(app_v2_setup_session_generate(&operations, &session) == APP_V2_SETUP_RANDOM_FAILURE);
    const uint8_t zero_session[sizeof(session)] = {0};
    CHECK(memcmp(&session, zero_session, sizeof(session)) == 0);

    fake = (fake_random_t){.fail = true};
    CHECK(app_v2_setup_session_generate(&operations, &session) == APP_V2_SETUP_RANDOM_FAILURE);
    CHECK(memcmp(&session, zero_session, sizeof(session)) == 0);
}

static void test_setup_state_is_minimal_and_unprovisioned_only(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    app_v2_setup_state_response_t response;
    CHECK(app_v2_setup_state_from_settings(&settings, &response) == APP_V2_SETUP_OK);
    CHECK(!response.provisioned);
    CHECK(response.device_name.length == strlen("ESP32 Macro Keyboard"));
    CHECK(memcmp(response.device_name.data, "ESP32 Macro Keyboard", response.device_name.length) ==
          0);

    app_v2_setup_password_material_t material = password_material();
    app_v2_setup_session_t session;
    CHECK(app_v2_setup_session_init(&session, VIEW("12345678")) == APP_V2_SETUP_OK);
    app_v2_setup_request_t request = request_for_code("12345678");
    app_v2_device_settings_t provisioned;
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &settings, &material, &provisioned) ==
          APP_V2_SETUP_OK);
    CHECK(app_v2_setup_state_from_settings(&provisioned, &response) ==
          APP_V2_SETUP_ALREADY_PROVISIONED);
}

static void test_prepare_candidate_preserves_unrelated_settings(void) {
    app_v2_device_settings_t current = unprovisioned_with_preferences();
    app_v2_setup_session_t session;
    CHECK(app_v2_setup_session_init(&session, VIEW("12345678")) == APP_V2_SETUP_OK);
    app_v2_setup_request_t request = request_for_code("12345678");
    request.require_serial_confirmation = true;
    const app_v2_setup_password_material_t material = password_material();
    app_v2_device_settings_t candidate;

    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_OK);
    CHECK(candidate.provisioned);
    CHECK(strcmp(candidate.device_name, "Desk Macro Keyboard") == 0);
    CHECK(strcmp(candidate.ap_ssid, "MacroKeyboard") == 0);
    CHECK(strcmp(candidate.ap_passphrase, "example-passphrase") == 0);
    CHECK(candidate.require_serial_confirmation);
    CHECK(candidate.password_iterations == material.password_iterations);
    CHECK(memcmp(candidate.password_salt, material.password_salt,
                 sizeof(candidate.password_salt)) == 0);
    CHECK(memcmp(candidate.password_verifier, material.password_verifier,
                 sizeof(candidate.password_verifier)) == 0);

    CHECK(candidate.next_blob_id == current.next_blob_id);
    CHECK(candidate.send_mode == current.send_mode);
    CHECK(candidate.snapshot_retention_target == current.snapshot_retention_target);
    CHECK(strcmp(candidate.last_selected_package_id, current.last_selected_package_id) == 0);
    CHECK(!candidate.station_configured);
    CHECK(app_v2_device_settings_validate(&candidate) == APP_V2_SETTINGS_OK);
    CHECK(!session.consumed);
}

static void test_code_is_consumed_only_after_success_is_committed(void) {
    app_v2_device_settings_t current = unprovisioned_with_preferences();
    app_v2_setup_session_t session;
    CHECK(app_v2_setup_session_init(&session, VIEW("12345678")) == APP_V2_SETUP_OK);
    const app_v2_setup_password_material_t material = password_material();
    app_v2_setup_request_t request = request_for_code("12345678");
    app_v2_device_settings_t candidate;

    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_OK);
    CHECK(!session.consumed);
    CHECK(app_v2_setup_session_consume(&session) == APP_V2_SETUP_OK);
    CHECK(session.consumed);
    CHECK(session.code[0] == '\0');
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_CODE_CONSUMED);
    CHECK(app_v2_setup_session_consume(&session) == APP_V2_SETUP_CODE_CONSUMED);
}

static void test_wrong_malformed_and_reboot_stale_codes_fail(void) {
    app_v2_device_settings_t current = unprovisioned_with_preferences();
    app_v2_setup_password_material_t material = password_material();
    app_v2_device_settings_t candidate;
    app_v2_setup_session_t session;
    CHECK(app_v2_setup_session_init(&session, VIEW("12345678")) == APP_V2_SETUP_OK);

    app_v2_setup_request_t request = request_for_code("12345679");
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_CODE_MISMATCH);
    CHECK(!session.consumed);

    request = request_for_code("1234A678");
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_MALFORMED_CODE);
    request = request_for_code("1234567");
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_MALFORMED_CODE);

    CHECK(app_v2_setup_session_init(&session, VIEW("87654321")) == APP_V2_SETUP_OK);
    request = request_for_code("12345678");
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_CODE_MISMATCH);
}

static void test_strict_field_boundaries(void) {
    app_v2_device_settings_t current = unprovisioned_with_preferences();
    app_v2_setup_password_material_t material = password_material();
    app_v2_setup_session_t session;
    CHECK(app_v2_setup_session_init(&session, VIEW("12345678")) == APP_V2_SETUP_OK);
    app_v2_device_settings_t candidate;
    app_v2_setup_request_t request = request_for_code("12345678");

    char long_device_name[34];
    memset(long_device_name, 'd', sizeof(long_device_name));
    long_device_name[33] = '\0';
    request.device_name = (app_v2_string_view_t){.data = long_device_name, .length = 33U};
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_INVALID_DEVICE_NAME);

    request = request_for_code("12345678");
    const char invalid_utf8[] = {(char)0xc0, (char)0xaf};
    request.device_name =
        (app_v2_string_view_t){.data = invalid_utf8, .length = sizeof(invalid_utf8)};
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_INVALID_DEVICE_NAME);

    request = request_for_code("12345678");
    request.ap_ssid = (app_v2_string_view_t){.data = "", .length = 0U};
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_INVALID_AP_SSID);

    request = request_for_code("12345678");
    request.ap_passphrase = VIEW("1234567");
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_INVALID_AP_PASSPHRASE);

    request = request_for_code("12345678");
    request.admin_password = VIEW("12345678901");
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_INVALID_ADMIN_PASSWORD);

    request = request_for_code("12345678");
    material.password_iterations = 0U;
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate) ==
          APP_V2_SETUP_INVALID_PASSWORD_MATERIAL);
}

static void test_already_provisioned_is_rejected(void) {
    app_v2_device_settings_t current = unprovisioned_with_preferences();
    app_v2_setup_password_material_t material = password_material();
    app_v2_setup_session_t session;
    CHECK(app_v2_setup_session_init(&session, VIEW("12345678")) == APP_V2_SETUP_OK);
    app_v2_setup_request_t request = request_for_code("12345678");
    app_v2_device_settings_t provisioned;
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &current, &material, &provisioned) ==
          APP_V2_SETUP_OK);

    app_v2_device_settings_t candidate;
    CHECK(app_v2_setup_prepare_candidate(&session, &request, &provisioned, &material, &candidate) ==
          APP_V2_SETUP_ALREADY_PROVISIONED);
}

static void test_accepted_response_has_exact_v2_semantics(void) {
    app_v2_setup_accepted_t response = {0};
    app_v2_setup_accepted_response_init(&response);
    CHECK(response.action.accepted);
    CHECK(response.restart_required);
    CHECK(response.action.connection_will_close);
    CHECK(!response.action.reprovisioning_required);
}

int main(void) {
    test_random_generation_rejects_out_of_range_without_bias();
    test_random_generation_fails_closed();
    test_setup_state_is_minimal_and_unprovisioned_only();
    test_prepare_candidate_preserves_unrelated_settings();
    test_code_is_consumed_only_after_success_is_committed();
    test_wrong_malformed_and_reboot_stale_codes_fail();
    test_strict_field_boundaries();
    test_already_provisioned_is_rejected();
    test_accepted_response_has_exact_v2_semantics();

    if (failures != 0) {
        (void)fprintf(stderr, "%d v2 setup contract assertion(s) failed\n", failures);
        return 1;
    }
    (void)puts("v2 setup contract tests passed");
    return 0;
}
