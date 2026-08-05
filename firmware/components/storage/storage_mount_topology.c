#include "storage.h"

#include <stdint.h>

#include "app_error.h"
#include "storage_blob.h"
#include "storage_blob_internal.h"
#include "storage_fs_ops.h"

#ifdef ESP_PLATFORM
#include "esp_err.h"
#include "nvs.h"
#endif

#define STORAGE_BLOB_NVS_NAMESPACE "storage"
#define STORAGE_BLOB_NVS_NEXT_ID_KEY "next_blob_id"

static app_error_code_t read_persisted_next_id(uint64_t *out_next_id) {
    if (out_next_id == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_next_id = 0U;

#ifdef ESP_PLATFORM
    nvs_handle_t handle = 0U;
    esp_err_t result = nvs_open(STORAGE_BLOB_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (result == ESP_ERR_NVS_NOT_FOUND) {
        return APP_ERROR_NONE;
    }
    if (result != ESP_OK) {
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }

    result = nvs_get_u64(handle, STORAGE_BLOB_NVS_NEXT_ID_KEY, out_next_id);
    nvs_close(handle);
    if (result == ESP_ERR_NVS_NOT_FOUND) {
        *out_next_id = 0U;
        return APP_ERROR_NONE;
    }
    if (result == ESP_ERR_NVS_TYPE_MISMATCH) {
        *out_next_id = 0U;
        return APP_ERROR_STORAGE_CORRUPT;
    }
    if (result != ESP_OK) {
        *out_next_id = 0U;
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }
#else
    /* Host tests exercise the scan and next-ID derivation with explicit values.
     * Device builds compile the NVS branch above. */
#endif

    return APP_ERROR_NONE;
}

app_error_code_t storage_prepare_directories(void) {
    const storage_fs_ops_t *operations = storage_fs_ops_posix();
    const app_error_code_t prepared =
        storage_blob_prepare_directory_with_ops(operations, STORAGE_BLOB_DIRECTORY);
    if (prepared != APP_ERROR_NONE) {
        return prepared;
    }

    uint64_t persisted_next_id = 0U;
    const app_error_code_t counter_result = read_persisted_next_id(&persisted_next_id);
    if (counter_result != APP_ERROR_NONE) {
        return counter_result;
    }
    return storage_blob_scan_startup(persisted_next_id);
}
