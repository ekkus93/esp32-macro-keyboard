/* V2-043: changing device UI preferences (send mode, snapshot retention target,
 * source-preview visibility, last-selected-package) must never create, modify,
 * or delete a repository blob (SPEC_V2 11.1: "UI preferences are device-wide
 * ... and do not make the repository dirty."). device_settings and the blob
 * repository are separate firmware components with no shared code path
 * (device_settings's CMakeLists REQUIRES app_contracts_v2, macro_model,
 * nvs_flash, freertos only -- never storage). This test proves that
 * independence behaviorally: it seeds a real on-disk repository directory,
 * drives a full sequence of preference changes against a completely separate
 * device-settings store, and asserts the repository directory is
 * byte-for-byte identical before and after. */

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_error.h"
#include "device_settings_core.h"
#include "device_settings_v2.h"
#include "storage_blob.h"
#include "storage_blob_internal.h"
#include "storage_fs_ops.h"
#include "test_assert.h"
#include "test_temp_dir.h"

#define CAPTURE_MAX 8U

typedef struct {
    storage_blob_entry_t entries[CAPTURE_MAX];
    size_t entry_count;
} blob_capture_t;

static app_error_code_t capture_entry(void *context, const storage_blob_entry_t *entry) {
    blob_capture_t *capture = context;
    if (capture == NULL || entry == NULL || capture->entry_count >= CAPTURE_MAX) {
        return APP_ERROR_INTERNAL;
    }
    capture->entries[capture->entry_count] = *entry;
    ++capture->entry_count;
    return APP_ERROR_NONE;
}

static void path_join(char *out_path, size_t path_size, const char *directory, const char *name) {
    const int written = snprintf(out_path, path_size, "%s/%s", directory, name);
    TEST_CHECK(written > 0 && (size_t)written < path_size);
}

static void write_blob_file(const char *repository, uint64_t id, const uint8_t *content,
                            size_t length) {
    char name[STORAGE_BLOB_FILENAME_CAPACITY];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_format_filename(id, name, sizeof(name)));
    char path[TEST_TEMP_DIR_PATH_MAX];
    path_join(path, sizeof(path), repository, name);
    const int descriptor = open(path, O_CREAT | O_EXCL | O_WRONLY, (mode_t)0600);
    TEST_CHECK(descriptor >= 0);
    TEST_CHECK(write(descriptor, content, length) == (ssize_t)length);
    TEST_CHECK(close(descriptor) == 0);
}

static void read_blob_file(const char *repository, uint64_t id, uint8_t *buffer, size_t buffer_size,
                           size_t *out_length) {
    char name[STORAGE_BLOB_FILENAME_CAPACITY];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_format_filename(id, name, sizeof(name)));
    char path[TEST_TEMP_DIR_PATH_MAX];
    path_join(path, sizeof(path), repository, name);
    const int descriptor = open(path, O_RDONLY);
    TEST_CHECK(descriptor >= 0);
    const ssize_t read_count = read(descriptor, buffer, buffer_size);
    TEST_CHECK(read_count >= 0);
    *out_length = (size_t)read_count;
    TEST_CHECK(close(descriptor) == 0);
}

/* Snapshots the two-blob repository (entry ids/sizes via a real scan, plus
 * exact bytes) so before/after states can be compared. */
static void snapshot_repository(const storage_fs_ops_t *operations, const char *repository,
                                uint8_t blob_one[16], size_t *blob_one_length, uint8_t blob_two[16],
                                size_t *blob_two_length, storage_blob_scan_summary_t *out_summary,
                                blob_capture_t *out_capture) {
    memset(out_capture, 0, sizeof(*out_capture));
    const storage_blob_scan_observer_t observer = {
        .context = out_capture,
        .visit_entry = capture_entry,
        .visit_invalid_name = NULL,
        .visit_temporary_file = NULL,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_blob_scan_with_ops(operations, repository, 3U,
                                                                    &observer, out_summary));
    read_blob_file(repository, 1U, blob_one, 16U, blob_one_length);
    read_blob_file(repository, 2U, blob_two, 16U, blob_two_length);
}

/* Minimal fake NVS-record store for device_settings_core, independent of the
 * repository directory above -- mirrors the fake in test_device_settings_core.c. */
typedef struct {
    uint8_t durable[APP_V2_SETTINGS_RECORD_BYTES];
    size_t durable_length;
    bool present;
} fake_settings_store_t;

static app_error_code_t fake_read(void *context, uint8_t *record, size_t capacity,
                                  size_t *out_length) {
    fake_settings_store_t *fake = context;
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
    if (record == NULL || length != APP_V2_SETTINGS_RECORD_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memcpy(fake->durable, record, length);
    fake->durable_length = length;
    fake->present = true;
    return APP_ERROR_NONE;
}

static void fake_zero(void *context, void *memory, size_t length) {
    (void)context;
    memset(memory, 0, length);
}

static app_v2_device_settings_t provisioned_settings(void) {
    app_v2_device_settings_t settings;
    app_v2_device_settings_init_unprovisioned(&settings);
    settings.provisioned = true;
    settings.credential_version = APP_V2_CREDENTIAL_VERSION;
    settings.password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    settings.password_iterations = 120000U;
    memset(settings.password_salt, 0x33, sizeof(settings.password_salt));
    memset(settings.password_verifier, 0x44, sizeof(settings.password_verifier));
    settings.next_blob_id = UINT64_C(7);
    TEST_CHECK(snprintf(settings.device_name, sizeof(settings.device_name), "%s",
                        "Isolation Test Keyboard") > 0);
    TEST_CHECK(snprintf(settings.ap_ssid, sizeof(settings.ap_ssid), "%s", "Macro Keyboard") > 0);
    TEST_CHECK(snprintf(settings.ap_passphrase, sizeof(settings.ap_passphrase), "%s",
                        "isolation-ap-passphrase") > 0);
    TEST_CHECK_EQ_INT(APP_V2_SETTINGS_OK, app_v2_device_settings_validate(&settings));
    return settings;
}

static void test_preference_changes_never_touch_repository(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char repository[TEST_TEMP_DIR_PATH_MAX];
    path_join(repository, sizeof(repository), directory.path, "repository");
    const storage_fs_ops_t *operations = storage_fs_ops_posix();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_blob_prepare_directory_with_ops(operations, repository));

    const uint8_t content_one[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    const uint8_t content_two[5] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee};
    write_blob_file(repository, 1U, content_one, sizeof(content_one));
    write_blob_file(repository, 2U, content_two, sizeof(content_two));

    uint8_t before_one[16] = {0};
    uint8_t before_two[16] = {0};
    size_t before_one_length = 0U;
    size_t before_two_length = 0U;
    storage_blob_scan_summary_t before_summary;
    blob_capture_t before_capture;
    snapshot_repository(operations, repository, before_one, &before_one_length, before_two,
                        &before_two_length, &before_summary, &before_capture);
    TEST_CHECK_EQ_U64(2U, before_summary.valid_count);
    TEST_CHECK_EQ_U64(0U, before_summary.invalid_name_count);
    TEST_CHECK_EQ_U64(0U, before_summary.temporary_file_count);
    TEST_CHECK_EQ_U64(sizeof(content_one), before_one_length);
    TEST_CHECK_EQ_U64(sizeof(content_two), before_two_length);

    /* A completely independent device-settings store: no reference to
     * `repository`, `operations`, or any storage type. */
    fake_settings_store_t fake = {0};
    device_settings_core_t core;
    const device_settings_core_ops_t settings_operations = {
        .context = &fake,
        .read_record = fake_read,
        .replace_record_atomic = fake_replace,
        .secure_zero = fake_zero,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_init(&core, &settings_operations));

    /* Drive every V2-043 preference through several distinct values, plus a
     * full non-credential reset. */
    app_v2_device_settings_t settings = provisioned_settings();
    bool changed = false;

    settings.send_mode = APP_V2_SEND_MODE_PREVIEW;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_replace(&core, &settings, &changed));
    TEST_CHECK(changed);

    settings.snapshot_retention_target = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_replace(&core, &settings, &changed));
    TEST_CHECK(changed);

    settings.snapshot_retention_target = 100U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_replace(&core, &settings, &changed));
    TEST_CHECK(changed);

    TEST_CHECK(snprintf(settings.last_selected_package_id,
                        sizeof(settings.last_selected_package_id), "%s",
                        "11111111-1111-4111-8111-111111111111") > 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_replace(&core, &settings, &changed));
    TEST_CHECK(changed);

    settings.send_mode = APP_V2_SEND_MODE_QUICK;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, device_settings_core_replace(&core, &settings, &changed));
    TEST_CHECK(changed);

    app_v2_device_settings_t reset_result;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_settings_core_reset_noncredential(&core, &reset_result, &changed));
    TEST_CHECK(changed);
    TEST_CHECK_EQ_INT(APP_V2_SEND_MODE_QUICK, reset_result.send_mode);
    TEST_CHECK_EQ_U64(5U, reset_result.snapshot_retention_target);

    uint8_t after_one[16] = {0};
    uint8_t after_two[16] = {0};
    size_t after_one_length = 0U;
    size_t after_two_length = 0U;
    storage_blob_scan_summary_t after_summary;
    blob_capture_t after_capture;
    snapshot_repository(operations, repository, after_one, &after_one_length, after_two,
                        &after_two_length, &after_summary, &after_capture);

    TEST_CHECK_EQ_U64(before_summary.valid_count, after_summary.valid_count);
    TEST_CHECK_EQ_U64(before_summary.invalid_name_count, after_summary.invalid_name_count);
    TEST_CHECK_EQ_U64(before_summary.temporary_file_count, after_summary.temporary_file_count);
    TEST_CHECK_EQ_U64(before_summary.max_id, after_summary.max_id);
    TEST_CHECK_EQ_U64(before_capture.entry_count, after_capture.entry_count);
    for (size_t index = 0U; index < before_capture.entry_count; ++index) {
        TEST_CHECK_EQ_U64(before_capture.entries[index].id, after_capture.entries[index].id);
        TEST_CHECK_EQ_U64(before_capture.entries[index].stored_bytes,
                          after_capture.entries[index].stored_bytes);
    }
    TEST_CHECK_EQ_U64(before_one_length, after_one_length);
    TEST_CHECK_EQ_BUFFER(before_one, after_one, before_one_length);
    TEST_CHECK_EQ_U64(before_two_length, after_two_length);
    TEST_CHECK_EQ_BUFFER(before_two, after_two, before_two_length);

    test_temp_dir_remove(&directory);
}

int main(void) {
    test_preference_changes_never_touch_repository();
    puts("device settings / repository isolation tests passed");
    return EXIT_SUCCESS;
}
