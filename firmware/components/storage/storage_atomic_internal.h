#ifndef STORAGE_ATOMIC_INTERNAL_H
#define STORAGE_ATOMIC_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "storage_fs_ops.h"

typedef app_error_code_t (*storage_uuid_generate_fn)(void *context,
                                                     app_uuid_t *out_uuid);
typedef int (*storage_parent_sync_fn)(void *context, const char *path);

app_error_code_t storage_atomic_write_with_ops_and_parent_sync(
    const char *path,
    const void *data,
    size_t data_length,
    bool sync_required,
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context,
    storage_parent_sync_fn sync_parent_path,
    void *parent_sync_context);

app_error_code_t storage_atomic_write_with_ops(const char *path,
                                               const void *data,
                                               size_t data_length,
                                               bool sync_required,
                                               const storage_fs_ops_t *operations,
                                               storage_uuid_generate_fn generate_uuid,
                                               void *uuid_context);

#endif
