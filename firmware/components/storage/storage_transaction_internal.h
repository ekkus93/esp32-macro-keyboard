#ifndef STORAGE_TRANSACTION_INTERNAL_H
#define STORAGE_TRANSACTION_INTERNAL_H

#include <stdbool.h>

#include "app_error.h"
#include "app_uuid.h"
#include "storage.h"
#include "storage_atomic_internal.h"
#include "storage_fs_ops.h"

typedef app_error_code_t (*storage_transaction_set_index_presence_fn)(
    void *context,
    const app_uuid_t *set_id,
    bool should_be_present);

app_error_code_t storage_transaction_write_manifest_with_ops(
    const storage_transaction_manifest_t *manifest,
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context);

app_error_code_t storage_transaction_recover_all_with_ops(
    const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid,
    void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence,
    void *index_context);

#endif
