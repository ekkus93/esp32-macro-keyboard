#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "device_settings_core.h"
#include "device_settings_v2.h"
#include "test_assert.h"

typedef struct {
    uint8_t durable[APP_V2_SETTINGS_RECORD_BYTES];
    size_t durable_length;
    bool present;
    app_error_code_t read_error;
    app_error_code_t replace_error;
    unsigned int read_calls;
    unsigned int replace_calls;
    unsigned int zero_calls;
} fake_settings_store_t;

static app_error_code_t fake_read(void *context, uint8_t *record, size_t capacity,
                                  size_t *out_length) {
    fake_settings_store_t *fake = context;
    ++fake->read_calls;
    if (fake->read_error != APP_ERROR_NONE) {
        return fake->read_error;
    }
    if (!fake->present) {
        return APP_ERROR_NOT_FOUND;
    }
    if (record == NULL || out_length == NULL || fake->durable_length > capacity) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    memcpy(record, fake->durable, fake->durable_length);
    *out_length = fake->durable_length;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_replace(void *context, const uint8_t *record, size_t length) {
    fake_settings_store_t *fake = context;
    ++fake->replace_calls;
    if (fake->replace_error != APP_ERROR_NONE) {
        return fake->replace_error;
    }
    if (record == NULL || length != APP_V2_SETTINGS_RECORD_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(fake->durable, record, length);
    fake->durable_length = length;
    fake->present = true;
    return APP_ERROR_NONE;
}

static void fake_zero(void *context, void *memory, size_t length) {
    fake_settings_store_t *fake = context;
    memset(memory, 0, length);
    ++fake->zero_calls;
}

static device_settings_core_ops_t fake_operations(fake_settings_store_t *fake) {
    return (device_settings_core_ops_t){
        .context = fake,
        .read_record = fake_read,
        .replace_record_atomic = fake_replace,
        .secure_zero = fake_zero,
    };
}

static void init_core(device_settings_core_t *core, fake_settings_store_t *fake) {
    memset(fake, 0, sizeof(*fake));
    const device_settings_core_ops_t operations = fake_operations(fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_init(core, &operations));
}

static app_v2_device_settings_t configured_settings(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    settings.provisioned = true;
    settings.credential_version = APP_V2_CREDENTIAL_VERSION;
    settings.password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    settings.password_iterations = 120000U;
    memset(settings.password_salt, 0x11, sizeof(settings.password_salt));
    memset(settings.password_verifier, 0x22, sizeof(settings.password_verifier));
    settings.next_blob_id = UINT64_C(42);
    settings.send_mode = APP_V2_SEND_MODE_PREVIEW;
    settings.snapshot_retention_target = 9U;
    settings.show_macro_source_previews = true;
    settings.require_serial_confirmation = true;
    settings.station_configured = true;
    TEST_CHECK(snprintf(settings.last_selected_package_id,
                        sizeof(settings.last_selected_package_id), "%s",
                        "123e4567-e89b-42d3-a456-426614174000") > 0);
    TEST_CHECK(snprintf(settings.device_name, sizeof(settings.device_name), "%s", "Desk Keyboard") >
               0);
    TEST_CHECK(snprintf(settings.ap_ssid, sizeof(settings.ap_ssid), "%s", "Macro Keyboard") > 0);
    TEST_CHECK(snprintf(settings.ap_passphrase, sizeof(settings.ap_passphrase), "%s",
                        "example-ap-passphrase") > 0);
    TEST_CHECK(snprintf(settings.station_ssid, sizeof(settings.station_ssid), "%s",
                        "example-station") > 0);
    TEST_CHECK(snprintf(settings.station_passphrase, sizeof(settings.station_passphrase), "%s",
                        "example-station-passphrase") > 0);
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_OK, app_v2_device_settings_validate(&settings));
    return settings;
}

static void seed_durable(fake_settings_store_t *fake, const app_v2_device_settings_t *settings) {
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_OK, app_v2_device_settings_encode(settings, fake->durable,
                                                                        sizeof(fake->durable)));
    fake->durable_length = sizeof(fake->durable);
    fake->present = true;
}

static void test_init_validation(void) {
    fake_settings_store_t fake = {0};
    device_settings_core_t core;
    device_settings_core_ops_t operations = fake_operations(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, device_settings_core_init(NULL, &operations));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, device_settings_core_init(&core, NULL));
    operations.read_record = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, device_settings_core_init(&core, &operations));
}

static void test_missing_record_uses_defaults_without_write(void) {
    fake_settings_store_t fake;
    device_settings_core_t core;
    init_core(&core, &fake);

    app_v2_device_settings_t loaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_load(&core, &loaded));
    TEST_CHECK(!loaded.provisioned);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_QUICK, loaded.send_mode);
    TEST_CHECK_EQ_U64(5U, loaded.snapshot_retention_target);
    TEST_CHECK(!loaded.show_macro_source_previews);
    TEST_CHECK_EQ_U64(1U, fake.read_calls);
    TEST_CHECK_EQ_U64(0U, fake.replace_calls);

    bool changed = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_replace(&core, &loaded, &changed));
    TEST_CHECK(!changed);
    TEST_CHECK_EQ_U64(0U, fake.replace_calls);

    app_v2_device_settings_t cached;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_load(&core, &cached));
    TEST_CHECK_EQ_U64(1U, fake.read_calls);
}

static void test_load_valid_and_reject_corruption(void) {
    fake_settings_store_t fake;
    device_settings_core_t core;
    init_core(&core, &fake);
    const app_v2_device_settings_t expected = configured_settings();
    seed_durable(&fake, &expected);

    app_v2_device_settings_t loaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_load(&core, &loaded));
    TEST_CHECK_EQ_U64(expected.next_blob_id, loaded.next_blob_id);
    TEST_CHECK(strcmp(expected.device_name, loaded.device_name) == 0);
    TEST_CHECK(strcmp(expected.last_selected_package_id, loaded.last_selected_package_id) == 0);

    init_core(&core, &fake);
    seed_durable(&fake, &expected);
    fake.durable_length = APP_V2_SETTINGS_RECORD_BYTES - 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, device_settings_core_load(&core, &loaded));
    TEST_CHECK(!core.loaded);

    init_core(&core, &fake);
    seed_durable(&fake, &expected);
    fake.durable[APP_V2_SETTINGS_OFFSET_RECORD_VERSION] ^= 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, device_settings_core_load(&core, &loaded));
    TEST_CHECK(!core.loaded);
}

static void test_replace_and_failure_preservation(void) {
    fake_settings_store_t fake;
    device_settings_core_t core;
    init_core(&core, &fake);

    app_v2_device_settings_t replacement = configured_settings();
    bool changed = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_settings_core_replace(&core, &replacement, &changed));
    TEST_CHECK(changed);
    TEST_CHECK_EQ_U64(1U, fake.replace_calls);

    device_settings_core_t reloaded_core;
    const device_settings_core_ops_t operations = fake_operations(&fake);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_init(&reloaded_core, &operations));
    app_v2_device_settings_t reloaded;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_load(&reloaded_core, &reloaded));
    TEST_CHECK(strcmp(reloaded.device_name, replacement.device_name) == 0);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_PREVIEW, reloaded.send_mode);

    app_v2_device_settings_t failed = replacement;
    TEST_CHECK(snprintf(failed.device_name, sizeof(failed.device_name), "%s", "Failed Update") > 0);
    uint8_t durable_before[APP_V2_SETTINGS_RECORD_BYTES];
    memcpy(durable_before, fake.durable, sizeof(durable_before));
    fake.replace_error = APP_ERROR_IO;
    changed = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, device_settings_core_replace(&core, &failed, &changed));
    TEST_CHECK(!changed);
    TEST_CHECK_EQ_BUFFER(durable_before, fake.durable, sizeof(durable_before));

    app_v2_device_settings_t after_failure;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_load(&core, &after_failure));
    TEST_CHECK(strcmp(after_failure.device_name, replacement.device_name) == 0);

    failed = replacement;
    failed.snapshot_retention_target = 101U;
    fake.replace_error = APP_ERROR_NONE;
    const unsigned int writes_before = fake.replace_calls;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_settings_core_replace(&core, &failed, &changed));
    TEST_CHECK_EQ_U64(writes_before, fake.replace_calls);
}

static void test_reset_preserves_credentials_and_blob_counter(void) {
    fake_settings_store_t fake;
    device_settings_core_t core;
    init_core(&core, &fake);
    const app_v2_device_settings_t original = configured_settings();
    seed_durable(&fake, &original);

    app_v2_device_settings_t reset;
    bool changed = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_settings_core_reset_noncredential(&core, &reset, &changed));
    TEST_CHECK(changed);
    TEST_CHECK(reset.provisioned);
    TEST_CHECK_EQ_U64(original.credential_version, reset.credential_version);
    TEST_CHECK_EQ_U64(original.password_iterations, reset.password_iterations);
    TEST_CHECK_EQ_BUFFER(original.password_salt, reset.password_salt, sizeof(reset.password_salt));
    TEST_CHECK_EQ_BUFFER(original.password_verifier, reset.password_verifier,
                         sizeof(reset.password_verifier));
    TEST_CHECK_EQ_U64(original.next_blob_id, reset.next_blob_id);
    TEST_CHECK(strcmp(original.ap_ssid, reset.ap_ssid) == 0);
    TEST_CHECK(strcmp(original.ap_passphrase, reset.ap_passphrase) == 0);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_QUICK, reset.send_mode);
    TEST_CHECK_EQ_U64(5U, reset.snapshot_retention_target);
    TEST_CHECK(!reset.show_macro_source_previews);
    TEST_CHECK(!reset.require_serial_confirmation);
    TEST_CHECK(!reset.station_configured);
    TEST_CHECK(reset.last_selected_package_id[0] == '\0');
    TEST_CHECK(reset.station_ssid[0] == '\0');
    TEST_CHECK(reset.station_passphrase[0] == '\0');
}

/* SPEC_V2.md §11.4 "Factory reset": erases device configuration, credentials,
 * and provisioning state (repository blobs are a separate storage concern --
 * device_controls erases those independently). */
static void test_factory_reset_erases_everything(void) {
    fake_settings_store_t fake;
    device_settings_core_t core;
    init_core(&core, &fake);
    const app_v2_device_settings_t original = configured_settings();
    seed_durable(&fake, &original);

    app_v2_device_settings_t reset;
    bool changed = false;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_settings_core_factory_reset(&core, &reset, &changed));
    TEST_CHECK(changed);
    TEST_CHECK(!reset.provisioned);
    /* init_unprovisioned() sets these two version fields even in the
     * unprovisioned default; only the derived credential material clears. */
    TEST_CHECK_EQ_U64(APP_V2_CREDENTIAL_VERSION, reset.credential_version);
    TEST_CHECK_EQ_U64(APP_V2_PASSWORD_ALGORITHM_VERSION, reset.password_algorithm_version);
    TEST_CHECK_EQ_U64(0U, reset.password_iterations);
    static const uint8_t zero_salt[APP_V2_PASSWORD_SALT_BYTES] = {0};
    static const uint8_t zero_verifier[APP_V2_PASSWORD_VERIFIER_BYTES] = {0};
    TEST_CHECK_EQ_BUFFER(zero_salt, reset.password_salt, sizeof(zero_salt));
    TEST_CHECK_EQ_BUFFER(zero_verifier, reset.password_verifier, sizeof(zero_verifier));
    TEST_CHECK_EQ_U64(0U, reset.next_blob_id);
    TEST_CHECK(reset.ap_ssid[0] == '\0');
    TEST_CHECK(reset.ap_passphrase[0] == '\0');
    TEST_CHECK(reset.station_ssid[0] == '\0');
    TEST_CHECK(reset.station_passphrase[0] == '\0');
    TEST_CHECK(!reset.station_configured);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_QUICK, reset.send_mode);
    TEST_CHECK_EQ_U64(5U, reset.snapshot_retention_target);
    TEST_CHECK(!reset.show_macro_source_previews);
    TEST_CHECK(!reset.require_serial_confirmation);
    TEST_CHECK(reset.last_selected_package_id[0] == '\0');
    TEST_CHECK(strcmp(reset.device_name, "ESP32 Macro Keyboard") == 0);

    /* Re-run on an already-unprovisioned device: idempotent, no redundant
     * write, and still reports success. */
    const unsigned int writes_before = fake.replace_calls;
    app_v2_device_settings_t reset_again;
    bool changed_again = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_settings_core_factory_reset(&core, &reset_again, &changed_again));
    TEST_CHECK(!changed_again);
    TEST_CHECK_EQ_U64(writes_before, fake.replace_calls);
    TEST_CHECK(!reset_again.provisioned);

    /* Invalid arguments and a durable-write failure are both surfaced and
     * leave the caller's output buffers cleared. */
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_settings_core_factory_reset(NULL, &reset, &changed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_settings_core_factory_reset(&core, NULL, &changed));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         device_settings_core_factory_reset(&core, &reset, NULL));

    /* A durable-write failure needs a genuine change pending -- against an
     * already-unprovisioned core the candidate record is identical to the
     * current one, so device_settings_core_replace() short-circuits before
     * ever calling replace_record_atomic(). Re-seed a provisioned record on a
     * fresh core so the write is actually attempted. */
    fake_settings_store_t failing_fake;
    device_settings_core_t failing_core;
    init_core(&failing_core, &failing_fake);
    seed_durable(&failing_fake, &original);
    failing_fake.replace_error = APP_ERROR_IO;
    app_v2_device_settings_t failed = {.provisioned = true};
    TEST_CHECK_APP_ERROR(APP_ERROR_IO,
                         device_settings_core_factory_reset(&failing_core, &failed, &changed));
    TEST_CHECK_EQ_U64(0U, failed.credential_version);
    TEST_CHECK(!failed.provisioned);
}

int main(void) {
    test_init_validation();
    test_missing_record_uses_defaults_without_write();
    test_load_valid_and_reject_corruption();
    test_replace_and_failure_preservation();
    test_reset_preserves_credentials_and_blob_counter();
    test_factory_reset_erases_everything();
    puts("V2 device settings store tests passed");
    return EXIT_SUCCESS;
}
