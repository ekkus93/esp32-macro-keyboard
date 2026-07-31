#ifndef STORAGE_TRANSACTION_INTERNAL_H
#define STORAGE_TRANSACTION_INTERNAL_H

#include <stdbool.h>

#include "app_error.h"
#include "app_uuid.h"
#include "storage.h"
#include "storage_atomic_internal.h"
#include "storage_fs_ops.h"

typedef app_error_code_t (*storage_transaction_set_index_presence_fn)(void *context,
                                                                      const app_uuid_t *set_id,
                                                                      bool should_be_present);
typedef app_error_code_t (*storage_transaction_validate_set_fn)(
    void *context, const char *path, const app_uuid_t *set_id, uint32_t expected_revision);
typedef app_error_code_t (*storage_transaction_validate_repository_fn)(void *context,
                                                                       const char *root);
typedef app_error_code_t (*storage_transaction_remove_tree_fn)(void *context, const char *path);

app_error_code_t storage_transaction_write_manifest_with_ops(
    const storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context);

/* Read and validate a transaction manifest at `path` (raw binary, exactly
 * sizeof(manifest) bytes, shape-valid), requiring its id to equal `expected_id`.
 * Reads via `operations` and modifies no file. */
app_error_code_t
storage_transaction_read_manifest_with_ops(const char *path, const app_uuid_t *expected_id,
                                           storage_transaction_manifest_t *out_manifest,
                                           const storage_fs_ops_t *operations);

app_error_code_t storage_transaction_recover_replace_with_ops(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context);

app_error_code_t storage_transaction_recover_restore_with_ops(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_validate_repository_fn validate_repository, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context);

app_error_code_t storage_transaction_recover_restores_with_ops(
    const storage_fs_ops_t *operations, storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_validate_repository_fn validate_repository, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context);

app_error_code_t storage_transaction_recover_all_with_ops(
    const storage_fs_ops_t *operations, storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context);

#endif
