#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api_contracts_v2.h"
#include "app_error.h"
#include "device_settings_v2.h"
#include "settings_contract_v2.h"
#include "setup_contract_v2.h"
#include "test_assert.h"

#define TEST_UUID_A "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee"
#define TEST_UUID_B "11111111-2222-4333-9444-555555555555"

static app_v2_string_view_t view(const char *text) {
    return (app_v2_string_view_t){.data = text, .length = strlen(text)};
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

static void test_response_from_settings_excludes_secrets(void) {
    app_v2_device_settings_t settings = provisioned_settings();
    settings.station_configured = true;
    memcpy(settings.station_ssid, "OfficeWiFi", sizeof("OfficeWiFi"));
    memcpy(settings.station_passphrase, "station-example-passphrase",
           sizeof("station-example-passphrase"));
    memcpy(settings.last_selected_package_id, TEST_UUID_A, sizeof(TEST_UUID_A));

    app_v2_settings_response_t response = {0};
    const app_v2_settings_update_result_t result =
        app_v2_settings_response_from_settings(&settings, &response);

    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_UPDATE_OK, result);
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard", response.device_name.data);
    TEST_CHECK_EQ_STRING("MacroKeyboard", response.ap_ssid.data);
    TEST_CHECK(response.station_configured);
    TEST_CHECK(response.station_ssid.present);
    TEST_CHECK_EQ_STRING("OfficeWiFi", response.station_ssid.value.data);
    TEST_CHECK(response.last_selected_package_id.present);
    TEST_CHECK_EQ_STRING(TEST_UUID_A, response.last_selected_package_id.value.data);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_QUICK, response.send_mode);
    TEST_CHECK_EQ_U64(5U, response.snapshot_retention_target);
}

static void test_response_from_settings_unconfigured_station_hides_ssid(void) {
    app_v2_device_settings_t settings = provisioned_settings();
    app_v2_settings_response_t response = {0};

    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_UPDATE_OK,
                      app_v2_settings_response_from_settings(&settings, &response));
    TEST_CHECK(!response.station_configured);
    TEST_CHECK(!response.station_ssid.present);
    TEST_CHECK(!response.last_selected_package_id.present);
}

static void test_response_rejects_unprovisioned(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    app_v2_settings_response_t response = {0};
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_UPDATE_INVALID_CURRENT_SETTINGS,
                      app_v2_settings_response_from_settings(&settings, &response));
}

static void test_prepare_update_empty_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {0};
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_EMPTY,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_device_name(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_device_name = true,
        .device_name = view("New Name"),
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = true;
    bool reconnect = true;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_OK,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
    TEST_CHECK_EQ_STRING("New Name", candidate.device_name);
    TEST_CHECK(!restart);
    TEST_CHECK(!reconnect);
    /* Untouched fields are preserved. */
    TEST_CHECK_EQ_STRING("MacroKeyboard", candidate.ap_ssid);
    TEST_CHECK_EQ_STRING("example-passphrase", candidate.ap_passphrase);
}

static void test_prepare_update_invalid_device_name(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_device_name = true,
        .device_name = view(""),
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_snapshot_retention_target(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_snapshot_retention_target = true,
        .snapshot_retention_target = 42U,
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_OK,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
    TEST_CHECK_EQ_U64(42U, candidate.snapshot_retention_target);
}

static void test_prepare_update_snapshot_retention_target_over_max_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_snapshot_retention_target = true,
        .snapshot_retention_target = 101U,
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_SNAPSHOT_RETENTION_TARGET,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_last_selected_package_id_set_and_clear(void) {
    app_v2_device_settings_t current = provisioned_settings();
    memcpy(current.last_selected_package_id, TEST_UUID_A, sizeof(TEST_UUID_A));

    app_v2_settings_update_request_t clear_request = {
        .has_last_selected_package_id = true,
        .last_selected_package_id = {.present = false},
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_OK,
        app_v2_settings_prepare_update(&current, &clear_request, &candidate, &restart, &reconnect));
    TEST_CHECK_EQ_STRING("", candidate.last_selected_package_id);

    app_v2_settings_update_request_t set_request = {
        .has_last_selected_package_id = true,
        .last_selected_package_id = {.present = true, .value = view(TEST_UUID_B)},
    };
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_OK,
        app_v2_settings_prepare_update(&current, &set_request, &candidate, &restart, &reconnect));
    TEST_CHECK_EQ_STRING(TEST_UUID_B, candidate.last_selected_package_id);
}

static void test_prepare_update_invalid_last_selected_package_id_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_last_selected_package_id = true,
        .last_selected_package_id = {.present = true, .value = view("not-a-uuid")},
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_LAST_SELECTED_PACKAGE_ID,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_access_point_sets_restart_and_reconnect(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_access_point = true,
        .access_point =
            {
                .ssid = view("NewAp"),
                .passphrase = view("new-example-passphrase"),
            },
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_OK,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
    TEST_CHECK_EQ_STRING("NewAp", candidate.ap_ssid);
    TEST_CHECK_EQ_STRING("new-example-passphrase", candidate.ap_passphrase);
    TEST_CHECK(restart);
    TEST_CHECK(reconnect);
}

static void test_prepare_update_access_point_invalid_passphrase_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_access_point = true,
        .access_point =
            {
                .ssid = view("NewAp"),
                .passphrase = view("short"),
            },
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_ACCESS_POINT_PASSPHRASE,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_station_set_requires_restart_only(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_station = true,
        .remove_station = false,
        .station =
            {
                .ssid = view("OfficeWiFi"),
                .passphrase = view("station-example-passphrase"),
            },
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_OK,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
    TEST_CHECK(candidate.station_configured);
    TEST_CHECK_EQ_STRING("OfficeWiFi", candidate.station_ssid);
    TEST_CHECK_EQ_STRING("station-example-passphrase", candidate.station_passphrase);
    TEST_CHECK(restart);
    TEST_CHECK(!reconnect);
}

static void test_prepare_update_station_removal(void) {
    app_v2_device_settings_t current = provisioned_settings();
    current.station_configured = true;
    memcpy(current.station_ssid, "OfficeWiFi", sizeof("OfficeWiFi"));
    memcpy(current.station_passphrase, "station-example-passphrase",
           sizeof("station-example-passphrase"));
    app_v2_settings_update_request_t request = {
        .has_station = true,
        .remove_station = true,
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_OK,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
    TEST_CHECK(!candidate.station_configured);
    TEST_CHECK_EQ_STRING("", candidate.station_ssid);
    TEST_CHECK_EQ_STRING("", candidate.station_passphrase);
    TEST_CHECK(restart);
    TEST_CHECK(!reconnect);
}

static void test_prepare_update_invalid_station_ssid_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_station = true,
        .station =
            {
                .ssid = view(""),
                .passphrase = view("station-example-passphrase"),
            },
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_STATION_SSID,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_rejects_unprovisioned_current(void) {
    app_v2_device_settings_t current;
    app_v2_device_settings_init_unprovisioned(&current);
    app_v2_settings_update_request_t request = {
        .has_device_name = true,
        .device_name = view("New Name"),
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_CURRENT_SETTINGS,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_response_from_settings_invalid_arguments(void) {
    app_v2_device_settings_t settings = provisioned_settings();
    app_v2_settings_response_t response = {0};
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_UPDATE_INVALID_ARGUMENT,
                      app_v2_settings_response_from_settings(NULL, &response));
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_UPDATE_INVALID_ARGUMENT,
                      app_v2_settings_response_from_settings(&settings, NULL));
}

static void test_prepare_update_require_confirmation_and_send_mode_and_previews(void) {
    app_v2_device_settings_t current = provisioned_settings();
    current.require_serial_confirmation = false;
    current.show_macro_source_previews = false;
    app_v2_settings_update_request_t request = {
        .has_require_serial_confirmation = true,
        .require_serial_confirmation = true,
        .has_send_mode = true,
        .send_mode = APP_V2_SEND_MODE_PREVIEW,
        .has_show_macro_source_previews = true,
        .show_macro_source_previews = true,
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = true;
    bool reconnect = true;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_OK,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
    TEST_CHECK(candidate.require_serial_confirmation);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_PREVIEW, candidate.send_mode);
    TEST_CHECK(candidate.show_macro_source_previews);
    TEST_CHECK(!restart);
    TEST_CHECK(!reconnect);
}

static void test_prepare_update_device_name_multibyte_utf8_accepted(void) {
    app_v2_device_settings_t current = provisioned_settings();
    /* "Caf" + U+00E9 (2-byte) + " " + U+20AC (3-byte) + U+1F600 (4-byte). */
    const char name[] = "Caf\xC3\xA9 \xE2\x82\xAC\xF0\x9F\x98\x80";
    app_v2_settings_update_request_t request = {
        .has_device_name = true,
        .device_name = view(name),
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_OK,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
    TEST_CHECK_EQ_STRING(name, candidate.device_name);
}

static void test_prepare_update_device_name_invalid_leading_byte_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    /* 0x80 is a stray continuation byte, never valid as a lead byte. */
    const char name[] = "A\x80"
                        "B";
    app_v2_settings_update_request_t request = {
        .has_device_name = true,
        .device_name = {.data = name, .length = sizeof(name) - 1U},
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_device_name_truncated_multibyte_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    /* 0xC3 announces a 2-byte sequence but the string ends immediately. */
    const char name[] = "A\xC3";
    app_v2_settings_update_request_t request = {
        .has_device_name = true,
        .device_name = {.data = name, .length = sizeof(name) - 1U},
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_device_name_bad_continuation_byte_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    /* 0xC3 announces a 2-byte sequence; 'A' (0x41) is not a 10xxxxxx
     * continuation byte. */
    const char name[] = "A\xC3"
                        "AB";
    app_v2_settings_update_request_t request = {
        .has_device_name = true,
        .device_name = {.data = name, .length = sizeof(name) - 1U},
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_device_name_overlong_and_surrogate_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;

    /* 0xE0 0x80 0x80 is an overlong 3-byte encoding of U+0000 (< the 0x800
     * minimum a 3-byte sequence must encode). */
    const char overlong[] = "A\xE0\x80\x80"
                            "B";
    app_v2_settings_update_request_t overlong_request = {
        .has_device_name = true,
        .device_name = {.data = overlong, .length = sizeof(overlong) - 1U},
    };
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME,
                      app_v2_settings_prepare_update(&current, &overlong_request, &candidate,
                                                     &restart, &reconnect));

    /* 0xED 0xA0 0x80 encodes U+D800, a UTF-16 surrogate half that is never a
     * valid Unicode scalar value. */
    const char surrogate[] = "A\xED\xA0\x80"
                             "B";
    app_v2_settings_update_request_t surrogate_request = {
        .has_device_name = true,
        .device_name = {.data = surrogate, .length = sizeof(surrogate) - 1U},
    };
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME,
                      app_v2_settings_prepare_update(&current, &surrogate_request, &candidate,
                                                     &restart, &reconnect));
}

static void test_prepare_update_device_name_embedded_nul_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    const char name[] = {'A', 'B', '\0', 'C', 'D'};
    app_v2_settings_update_request_t request = {
        .has_device_name = true,
        .device_name = {.data = name, .length = sizeof(name)},
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_last_selected_package_id_bad_hyphen_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    /* Same shape as TEST_UUID_A but with the hyphen at index 8 replaced by a
     * hex digit. */
    app_v2_settings_update_request_t request = {
        .has_last_selected_package_id = true,
        .last_selected_package_id = {.present = true,
                                     .value = view("aaaaaaaaXbbbb-4ccc-8ddd-eeeeeeeeeeee")},
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_LAST_SELECTED_PACKAGE_ID,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_last_selected_package_id_bad_hex_digit_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    /* 'g' is not a lowercase hex digit. */
    app_v2_settings_update_request_t request = {
        .has_last_selected_package_id = true,
        .last_selected_package_id = {.present = true,
                                     .value = view("gaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee")},
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_LAST_SELECTED_PACKAGE_ID,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_last_selected_package_id_bad_variant_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    /* Version nibble '4' is correct; variant nibble 'c' is not one of
     * 8/9/a/b, so this fails only the variant check. */
    app_v2_settings_update_request_t request = {
        .has_last_selected_package_id = true,
        .last_selected_package_id = {.present = true,
                                     .value = view("aaaaaaaa-bbbb-4ccc-cddd-eeeeeeeeeeee")},
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_LAST_SELECTED_PACKAGE_ID,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_access_point_invalid_ssid_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_access_point = true,
        .access_point =
            {
                .ssid = view(""),
                .passphrase = view("new-example-passphrase"),
            },
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_ACCESS_POINT_SSID,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_invalid_station_passphrase_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {
        .has_station = true,
        .station =
            {
                .ssid = view("OfficeWiFi"),
                .passphrase = view("short"),
            },
    };
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_STATION_PASSPHRASE,
        app_v2_settings_prepare_update(&current, &request, &candidate, &restart, &reconnect));
}

static void test_prepare_update_invalid_arguments(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_settings_update_request_t request = {.has_device_name = true, .device_name = view("X")};
    app_v2_device_settings_t candidate = {0};
    bool restart = false;
    bool reconnect = false;
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_ARGUMENT,
        app_v2_settings_prepare_update(NULL, &request, &candidate, &restart, &reconnect));
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_ARGUMENT,
        app_v2_settings_prepare_update(&current, NULL, &candidate, &restart, &reconnect));
    TEST_CHECK_EQ_INT(
        APP_V2_SETTINGS_UPDATE_INVALID_ARGUMENT,
        app_v2_settings_prepare_update(&current, &request, NULL, &restart, &reconnect));
}

static void test_setup_preserves_uart_configured_station_before_provisioning(void) {
    app_v2_device_settings_t current;
    app_v2_device_settings_init_unprovisioned(&current);
    current.station_configured = true;
    memcpy(current.station_ssid, "BenchWiFi", sizeof("BenchWiFi"));
    memcpy(current.station_passphrase, "bench-passphrase", sizeof("bench-passphrase"));
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_OK, app_v2_device_settings_validate(&current));

    app_v2_setup_session_t session = {0};
    TEST_CHECK_EQ_INT(APP_V2_SETUP_OK,
                      app_v2_setup_session_init(&session, view("12345678")));
    const app_v2_setup_request_t request = {
        .setup_code = view("12345678"),
        .device_name = view("Desk Macro Keyboard"),
        .ap_ssid = view("MacroKeyboard"),
        .ap_passphrase = view("example-passphrase"),
        .admin_password = view("example-admin-password"),
        .require_serial_confirmation = false,
    };
    app_v2_setup_password_material_t material = {
        .credential_version = APP_V2_CREDENTIAL_VERSION,
        .password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION,
        .password_iterations = 5500U,
    };
    memset(material.password_salt, 0x55, sizeof(material.password_salt));
    memset(material.password_verifier, 0x66, sizeof(material.password_verifier));

    app_v2_device_settings_t candidate = {0};
    TEST_CHECK_EQ_INT(
        APP_V2_SETUP_OK,
        app_v2_setup_prepare_candidate(&session, &request, &current, &material, &candidate));
    TEST_CHECK(candidate.provisioned);
    TEST_CHECK(candidate.station_configured);
    TEST_CHECK_EQ_STRING("BenchWiFi", candidate.station_ssid);
    TEST_CHECK_EQ_STRING("bench-passphrase", candidate.station_passphrase);
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_OK, app_v2_device_settings_validate(&candidate));
}

static void test_password_change_validate_ok(void) {
    app_v2_device_settings_t current = provisioned_settings();
    TEST_CHECK_EQ_INT(APP_V2_PASSWORD_CHANGE_OK,
                      app_v2_password_change_validate(&current, view("new-example-password")));
}

static void test_password_change_validate_too_short_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    TEST_CHECK_EQ_INT(APP_V2_PASSWORD_CHANGE_INVALID_NEW_PASSWORD,
                      app_v2_password_change_validate(&current, view("short")));
}

static void test_password_change_validate_unprovisioned_rejected(void) {
    app_v2_device_settings_t current;
    app_v2_device_settings_init_unprovisioned(&current);
    TEST_CHECK_EQ_INT(APP_V2_PASSWORD_CHANGE_INVALID_CURRENT_SETTINGS,
                      app_v2_password_change_validate(&current, view("new-example-password")));
}

static void test_password_change_prepare_candidate_merges_material_only(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_setup_password_material_t material = {
        .credential_version = APP_V2_CREDENTIAL_VERSION,
        .password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION,
        .password_iterations = 5500U,
    };
    memset(material.password_salt, 0x55, sizeof(material.password_salt));
    memset(material.password_verifier, 0x66, sizeof(material.password_verifier));

    app_v2_device_settings_t candidate = {0};
    TEST_CHECK(app_v2_password_change_prepare_candidate(&current, &material, &candidate));
    TEST_CHECK_EQ_U64(5500U, candidate.password_iterations);
    TEST_CHECK_EQ_BUFFER(material.password_salt, candidate.password_salt,
                         sizeof(material.password_salt));
    TEST_CHECK_EQ_BUFFER(material.password_verifier, candidate.password_verifier,
                         sizeof(material.password_verifier));
    /* Everything else is preserved untouched. */
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard", candidate.device_name);
    TEST_CHECK_EQ_STRING("MacroKeyboard", candidate.ap_ssid);
    TEST_CHECK(candidate.provisioned);
}

static void test_password_change_validate_null_current_rejected(void) {
    TEST_CHECK_EQ_INT(APP_V2_PASSWORD_CHANGE_INVALID_ARGUMENT,
                      app_v2_password_change_validate(NULL, view("new-example-password")));
}

static void test_password_change_prepare_candidate_bad_credential_version_rejected(void) {
    app_v2_device_settings_t current = provisioned_settings();
    /* Wrong credential_version makes the merged candidate fail
     * app_v2_device_settings_validate(), so the function must reject it
     * rather than commit a corrupt record. */
    app_v2_setup_password_material_t material = {
        .credential_version = (uint16_t)(APP_V2_CREDENTIAL_VERSION + 1U),
        .password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION,
        .password_iterations = 5500U,
    };
    memset(material.password_salt, 0x55, sizeof(material.password_salt));
    memset(material.password_verifier, 0x66, sizeof(material.password_verifier));

    app_v2_device_settings_t candidate = {0};
    TEST_CHECK(!app_v2_password_change_prepare_candidate(&current, &material, &candidate));
}

static void test_password_change_prepare_candidate_invalid_arguments(void) {
    app_v2_device_settings_t current = provisioned_settings();
    app_v2_setup_password_material_t material = {0};
    app_v2_device_settings_t candidate = {0};
    TEST_CHECK(!app_v2_password_change_prepare_candidate(NULL, &material, &candidate));
    TEST_CHECK(!app_v2_password_change_prepare_candidate(&current, NULL, &candidate));
    TEST_CHECK(!app_v2_password_change_prepare_candidate(&current, &material, NULL));
}

int main(void) {
    test_response_from_settings_excludes_secrets();
    test_response_from_settings_unconfigured_station_hides_ssid();
    test_response_rejects_unprovisioned();
    test_prepare_update_empty_rejected();
    test_prepare_update_device_name();
    test_prepare_update_invalid_device_name();
    test_prepare_update_snapshot_retention_target();
    test_prepare_update_snapshot_retention_target_over_max_rejected();
    test_prepare_update_last_selected_package_id_set_and_clear();
    test_prepare_update_invalid_last_selected_package_id_rejected();
    test_prepare_update_access_point_sets_restart_and_reconnect();
    test_prepare_update_access_point_invalid_passphrase_rejected();
    test_prepare_update_station_set_requires_restart_only();
    test_prepare_update_station_removal();
    test_prepare_update_invalid_station_ssid_rejected();
    test_prepare_update_invalid_station_passphrase_rejected();
    test_prepare_update_rejects_unprovisioned_current();
    test_prepare_update_invalid_arguments();
    test_response_from_settings_invalid_arguments();
    test_setup_preserves_uart_configured_station_before_provisioning();
    test_prepare_update_require_confirmation_and_send_mode_and_previews();
    test_prepare_update_device_name_multibyte_utf8_accepted();
    test_prepare_update_device_name_invalid_leading_byte_rejected();
    test_prepare_update_device_name_truncated_multibyte_rejected();
    test_prepare_update_device_name_bad_continuation_byte_rejected();
    test_prepare_update_device_name_overlong_and_surrogate_rejected();
    test_prepare_update_device_name_embedded_nul_rejected();
    test_prepare_update_last_selected_package_id_bad_hyphen_rejected();
    test_prepare_update_last_selected_package_id_bad_hex_digit_rejected();
    test_prepare_update_last_selected_package_id_bad_variant_rejected();
    test_prepare_update_access_point_invalid_ssid_rejected();
    test_password_change_validate_ok();
    test_password_change_validate_too_short_rejected();
    test_password_change_validate_unprovisioned_rejected();
    test_password_change_validate_null_current_rejected();
    test_password_change_prepare_candidate_merges_material_only();
    test_password_change_prepare_candidate_bad_credential_version_rejected();
    test_password_change_prepare_candidate_invalid_arguments();
    puts("settings contract v2 tests passed");
    return EXIT_SUCCESS;
}
