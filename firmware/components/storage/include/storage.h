#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"

#define STORAGE_WEB_MOUNT "/web"
#define STORAGE_DATA_MOUNT "/data"
#define STORAGE_WEB_PARTITION "webfs"
#define STORAGE_DATA_PARTITION "userdata"

typedef enum {
    STORAGE_TRANSACTION_IMPORT_SET = 0,
    STORAGE_TRANSACTION_REPLACE_SET,
    STORAGE_TRANSACTION_DUPLICATE_SET,
    STORAGE_TRANSACTION_DELETE_SET,
    STORAGE_TRANSACTION_RESTORE,
    STORAGE_TRANSACTION_MIGRATE
} storage_transaction_type_t;

typedef enum {
    STORAGE_TRANSACTION_PREPARED = 0,
    STORAGE_TRANSACTION_STAGED,
    STORAGE_TRANSACTION_BACKED_UP,
    STORAGE_TRANSACTION_ACTIVATED,
    STORAGE_TRANSACTION_INDEXED,
    STORAGE_TRANSACTION_COMPLETE
} storage_transaction_phase_t;

typedef struct {
    uint32_t schema_version;
    app_uuid_t id;
    storage_transaction_type_t type;
    storage_transaction_phase_t phase;
    char source[APP_PATH_MAX_BYTES];
    char staging[APP_PATH_MAX_BYTES];
    char destination[APP_PATH_MAX_BYTES];
    char backup[APP_PATH_MAX_BYTES];
    uint32_t expected_revision;
    uint32_t replacement_revision;
} storage_transaction_manifest_t;

app_error_code_t storage_mount_all(void);
app_error_code_t storage_unmount_all(void);
app_error_code_t storage_prepare_directories(void);
app_error_code_t storage_make_set_path(const app_uuid_t *set_id, char *buffer, size_t buffer_size);
app_error_code_t storage_make_macro_path(const app_uuid_t *set_id,
                                         const app_uuid_t *macro_id,
                                         char *buffer,
                                         size_t buffer_size);
app_error_code_t storage_atomic_write(const char *path,
                                      const void *data,
                                      size_t data_length,
                                      bool sync_required);
app_error_code_t storage_transaction_recover_all(void);
app_error_code_t storage_transaction_write_manifest(const storage_transaction_manifest_t *manifest);

#endif
