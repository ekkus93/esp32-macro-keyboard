#include "storage.h"

#include <stdint.h>

#include "app_error.h"
#include "storage_blob.h"
#include "storage_blob_internal.h"
#include "storage_fs_ops.h"

app_error_code_t storage_prepare_directories(void) {
    const storage_fs_ops_t *operations = storage_fs_ops_posix();
    const app_error_code_t prepared =
        storage_blob_prepare_directory_with_ops(operations, STORAGE_BLOB_DIRECTORY);
    if (prepared != APP_ERROR_NONE) {
        return prepared;
    }
    return storage_blob_scan_startup(UINT64_C(0));
}
