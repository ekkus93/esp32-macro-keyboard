#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "device_settings_v2.h"

static int failures = 0;

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            (void)fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, #condition);           \
            ++failures;                                                                            \
        }                                                                                          \
    } while (0)

static void copy_text(char *destination, size_t capacity, const char *source) {
    const size_t length = strlen(source);
    CHECK(length < capacity);
    if (length >= capacity) {
        return;
    }
    memset(destination, 0, capacity);
    memcpy(destination, source, length);
}

static void make_provisioned(app_v2_device_settings_t *settings) {
    app_v2_device_settings_init_unprovisioned(settings);
    settings->provisioned = true;
    settings->password_iterations = UINT32_C(100000);
    memset(settings->password_salt, 0x11, sizeof(settings->password_salt));
    memset(settings->password_verifier, 0x22, sizeof(settings->password_verifier));
    settings->next_blob_id = UINT64_C(42);
    settings->send_mode = APP_V2_SEND_MODE_PREVIEW;
    settings->snapshot_retention_target = 9U;
    settings->require_serial_confirmation = true;
    settings->station_configured = true;
    copy_text(settings->last_selected_package_id, sizeof(settings->last_selected_package_id),
              "550e8400-e29b-41d4-a716-446655440000");
    copy_text(settings->device_name, sizeof(settings->device_name), "Desk Macro Keyboard");
    copy_text(settings->ap_ssid, sizeof(settings->ap_ssid), "MacroKeyboard");
    copy_text(settings->ap_passphrase, sizeof(settings->ap_passphrase), "example-passphrase");
    copy_text(settings->station_ssid, sizeof(settings->station_ssid), "OfficeWiFi");
    copy_text(settings->station_passphrase, sizeof(settings->station_passphrase),
              "station-example-passphrase");
}

static bool settings_equal(const app_v2_device_settings_t *left,
                           const app_v2_device_settings_t *right) {
    return left->provisioned == right->provisioned &&
           left->credential_version == right->credential_version &&
           left->password_algorithm_version == right->password_algorithm_version &&
           left->password_iterations == right->password_iterations &&
           memcmp(left->password_salt, right->password_salt, sizeof(left->password_salt)) == 0 &&
           memcmp(left->password_verifier, right->password_verifier,
                  sizeof(left->password_verifier)) == 0 &&
           left->next_blob_id == right->next_blob_id && left->send_mode == right->send_mode &&
           left->snapshot_retention_target == right->snapshot_retention_target &&
           left->require_serial_confirmation == right->require_serial_confirmation &&
           left->station_configured == right->station_configured &&
           strcmp(left->last_selected_package_id, right->last_selected_package_id) == 0 &&
           strcmp(left->device_name, right->device_name) == 0 &&
           strcmp(left->ap_ssid, right->ap_ssid) == 0 &&
           strcmp(left->ap_passphrase, right->ap_passphrase) == 0 &&
           strcmp(left->station_ssid, right->station_ssid) == 0 &&
           strcmp(left->station_passphrase, right->station_passphrase) == 0;
}

static void test_unprovisioned_round_trip(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    CHECK(app_v2_device_settings_validate(&settings) == APP_V2_SETTINGS_OK);

    uint8_t record[APP_V2_SETTINGS_RECORD_BYTES];
    CHECK(app_v2_device_settings_encode(&settings, record, sizeof(record)) == APP_V2_SETTINGS_OK);
    app_v2_device_settings_t decoded;
    memset(&decoded, 0xa5, sizeof(decoded));
    CHECK(app_v2_device_settings_decode(record, sizeof(record), &decoded) == APP_V2_SETTINGS_OK);
    CHECK(settings_equal(&settings, &decoded));
}

static void test_provisioned_round_trip(void) {
    app_v2_device_settings_t settings;
    make_provisioned(&settings);
    CHECK(app_v2_device_settings_validate(&settings) == APP_V2_SETTINGS_OK);

    uint8_t record[APP_V2_SETTINGS_RECORD_BYTES];
    CHECK(app_v2_device_settings_encode(&settings, record, sizeof(record)) == APP_V2_SETTINGS_OK);
    app_v2_device_settings_t decoded;
    memset(&decoded, 0, sizeof(decoded));
    CHECK(app_v2_device_settings_decode(record, sizeof(record), &decoded) == APP_V2_SETTINGS_OK);
    CHECK(settings_equal(&settings, &decoded));
}

static void test_length_version_and_header_rejection(void) {
    app_v2_device_settings_t settings;
    make_provisioned(&settings);
    uint8_t record[APP_V2_SETTINGS_RECORD_BYTES];
    CHECK(app_v2_device_settings_encode(&settings, record, sizeof(record)) == APP_V2_SETTINGS_OK);

    app_v2_device_settings_t decoded;
    CHECK(app_v2_device_settings_decode(record, sizeof(record) - 1U, &decoded) ==
          APP_V2_SETTINGS_INVALID_LENGTH);
    CHECK(app_v2_device_settings_encode(&settings, record, sizeof(record) - 1U) ==
          APP_V2_SETTINGS_INVALID_LENGTH);

    uint8_t modified[APP_V2_SETTINGS_RECORD_BYTES];
    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_MAGIC] ^= UINT8_C(0x01);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_CORRUPT);

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_RECORD_VERSION] = UINT8_C(0x02);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_UNSUPPORTED_VERSION);

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_RECORD_LENGTH] = UINT8_C(0x00);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_CORRUPT);

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_CREDENTIAL_VERSION] = UINT8_C(0x02);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_UNSUPPORTED_VERSION);

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_PASSWORD_ALGORITHM] = UINT8_C(0x02);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_UNSUPPORTED_VERSION);
}

static void test_enum_boolean_reserved_and_string_rejection(void) {
    app_v2_device_settings_t settings;
    make_provisioned(&settings);
    uint8_t record[APP_V2_SETTINGS_RECORD_BYTES];
    CHECK(app_v2_device_settings_encode(&settings, record, sizeof(record)) == APP_V2_SETTINGS_OK);
    app_v2_device_settings_t decoded;
    uint8_t modified[APP_V2_SETTINGS_RECORD_BYTES];

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_SEND_MODE] = UINT8_C(2);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_CORRUPT);

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_RETENTION_TARGET] = UINT8_C(101);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_CORRUPT);

    static const size_t boolean_offsets[] = {
        APP_V2_SETTINGS_OFFSET_RESERVED_SHOW_SOURCE,
        APP_V2_SETTINGS_OFFSET_REQUIRE_CONFIRMATION,
        APP_V2_SETTINGS_OFFSET_PROVISIONED,
        APP_V2_SETTINGS_OFFSET_STATION_CONFIGURED,
    };
    for (size_t index = 0U; index < sizeof(boolean_offsets) / sizeof(boolean_offsets[0]); ++index) {
        memcpy(modified, record, sizeof(modified));
        modified[boolean_offsets[index]] = UINT8_C(2);
        CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
              APP_V2_SETTINGS_CORRUPT);
    }

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_RESERVED] = UINT8_C(1);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_CORRUPT);

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_LAST_SELECTED_PACKAGE] = (uint8_t)'G';
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_CORRUPT);

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_DEVICE_NAME] = UINT8_C(0xc0);
    modified[APP_V2_SETTINGS_OFFSET_DEVICE_NAME + 1U] = UINT8_C(0x80);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_CORRUPT);

    memcpy(modified, record, sizeof(modified));
    modified[APP_V2_SETTINGS_OFFSET_DEVICE_NAME + strlen("Desk Macro Keyboard") + 1U] = UINT8_C(1);
    CHECK(app_v2_device_settings_decode(modified, sizeof(modified), &decoded) ==
          APP_V2_SETTINGS_CORRUPT);
}

static void test_provisioning_and_station_invariants(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    settings.password_iterations = 1U;
    CHECK(app_v2_device_settings_validate(&settings) == APP_V2_SETTINGS_CORRUPT);

    make_provisioned(&settings);
    settings.password_iterations = 0U;
    CHECK(app_v2_device_settings_validate(&settings) == APP_V2_SETTINGS_CORRUPT);

    make_provisioned(&settings);
    settings.password_salt[0] = 0U;
    memset(settings.password_salt, 0, sizeof(settings.password_salt));
    CHECK(app_v2_device_settings_validate(&settings) == APP_V2_SETTINGS_CORRUPT);

    make_provisioned(&settings);
    settings.station_configured = false;
    CHECK(app_v2_device_settings_validate(&settings) == APP_V2_SETTINGS_CORRUPT);

    make_provisioned(&settings);
    settings.station_configured = false;
    memset(settings.station_ssid, 0, sizeof(settings.station_ssid));
    memset(settings.station_passphrase, 0, sizeof(settings.station_passphrase));
    CHECK(app_v2_device_settings_validate(&settings) == APP_V2_SETTINGS_OK);
}

static void test_reset_settings_preserves_credentials_and_counter(void) {
    app_v2_device_settings_t settings;
    make_provisioned(&settings);
    const uint16_t credential_version = settings.credential_version;
    const uint16_t password_algorithm_version = settings.password_algorithm_version;
    const uint32_t iterations = settings.password_iterations;
    uint8_t salt[APP_V2_PASSWORD_SALT_BYTES];
    uint8_t verifier[APP_V2_PASSWORD_VERIFIER_BYTES];
    memcpy(salt, settings.password_salt, sizeof(salt));
    memcpy(verifier, settings.password_verifier, sizeof(verifier));
    const uint64_t next_blob_id = settings.next_blob_id;
    char ap_ssid[sizeof(settings.ap_ssid)];
    char ap_passphrase[sizeof(settings.ap_passphrase)];
    memcpy(ap_ssid, settings.ap_ssid, sizeof(ap_ssid));
    memcpy(ap_passphrase, settings.ap_passphrase, sizeof(ap_passphrase));

    CHECK(app_v2_device_settings_reset_noncredential(&settings) == APP_V2_SETTINGS_OK);
    CHECK(settings.provisioned);
    CHECK(settings.credential_version == credential_version);
    CHECK(settings.password_algorithm_version == password_algorithm_version);
    CHECK(settings.password_iterations == iterations);
    CHECK(memcmp(settings.password_salt, salt, sizeof(salt)) == 0);
    CHECK(memcmp(settings.password_verifier, verifier, sizeof(verifier)) == 0);
    CHECK(settings.next_blob_id == next_blob_id);
    CHECK(strcmp(settings.ap_ssid, ap_ssid) == 0);
    CHECK(strcmp(settings.ap_passphrase, ap_passphrase) == 0);
    CHECK(strcmp(settings.device_name, "ESP32 Macro Keyboard") == 0);
    CHECK(!settings.require_serial_confirmation);
    CHECK(settings.send_mode == APP_V2_SEND_MODE_QUICK);
    CHECK(settings.snapshot_retention_target == 5U);
    CHECK(settings.last_selected_package_id[0] == '\0');
    CHECK(!settings.station_configured);
    CHECK(settings.station_ssid[0] == '\0');
    CHECK(settings.station_passphrase[0] == '\0');
}

int main(void) {
    test_unprovisioned_round_trip();
    test_provisioned_round_trip();
    test_length_version_and_header_rejection();
    test_enum_boolean_reserved_and_string_rejection();
    test_provisioning_and_station_invariants();
    test_reset_settings_preserves_credentials_and_counter();

    if (failures != 0) {
        (void)fprintf(stderr, "%d v2 settings test assertion(s) failed\n", failures);
        return 1;
    }
    (void)printf("all v2 device-settings contract tests passed\n");
    return 0;
}
