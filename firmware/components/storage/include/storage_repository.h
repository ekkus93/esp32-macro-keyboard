#ifndef STORAGE_REPOSITORY_H
#define STORAGE_REPOSITORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"

#define STORAGE_REFERENCE_DETAIL_MAX_IDS 16U

typedef struct {
    macro_set_t items[APP_MACRO_SETS_MAX];
    size_t count;
} storage_set_list_t;

typedef struct {
    macro_scope_t scope;
    bool has_set_id;
    app_uuid_t set_id;
} storage_macro_location_t;

typedef struct {
    macro_t *items;
    size_t count;
} storage_macro_list_t;

typedef struct {
    app_uuid_t ids[STORAGE_REFERENCE_DETAIL_MAX_IDS];
    size_t count;
    bool truncated;
} storage_reference_list_t;

typedef struct {
    procedure_t *items;
    size_t count;
} storage_procedure_list_t;

typedef struct {
    app_uuid_t set_id;
    app_uuid_t procedure_id;
} storage_procedure_identity_t;

typedef enum {
    STORAGE_PROGRESS_STATUS_CURRENT = 0,
    STORAGE_PROGRESS_STATUS_STALE,
} storage_progress_status_t;

typedef struct {
    procedure_progress_t progress;
    storage_progress_status_t status;
    uint32_t current_procedure_revision;
} storage_progress_snapshot_t;

/* Repository mutation lock lifecycle (FIX1 §9). Must be initialized before any
 * repository operation, including startup recovery, and torn down after the last
 * one. The take/give mechanism and test operations seam remain private. */
app_error_code_t storage_repository_lock_init(void);
app_error_code_t storage_repository_lock_deinit(void);

app_error_code_t storage_repository_init(void);
app_error_code_t storage_repository_deinit(void);
app_error_code_t storage_set_list(storage_set_list_t *out_list);
app_error_code_t storage_set_create(const macro_set_t *set);
app_error_code_t storage_set_read(const app_uuid_t *set_id, macro_set_t *out_set);
app_error_code_t storage_set_update(const macro_set_t *replacement, uint32_t expected_revision,
                                    macro_set_t *out_updated);
app_error_code_t storage_set_delete(const app_uuid_t *set_id, uint32_t expected_revision);

app_error_code_t storage_macro_list(const storage_macro_location_t *location,
                                    storage_macro_list_t *out_list);
void storage_macro_list_free(storage_macro_list_t *list);
app_error_code_t storage_macro_create(const storage_macro_location_t *location,
                                      const macro_t *macro);
app_error_code_t storage_macro_read(const storage_macro_location_t *location,
                                    const app_uuid_t *macro_id, macro_t *out_macro);
app_error_code_t storage_macro_update(const storage_macro_location_t *location,
                                      const macro_t *replacement, uint32_t expected_revision,
                                      macro_t *out_updated);
app_error_code_t storage_macro_delete(const storage_macro_location_t *location,
                                      const app_uuid_t *macro_id, uint32_t expected_revision,
                                      storage_reference_list_t *out_references);
app_error_code_t storage_macro_duplicate(const storage_macro_location_t *location,
                                         const app_uuid_t *source_id,
                                         const app_uuid_t *duplicate_id, const char *duplicate_name,
                                         macro_t *out_duplicate);
app_error_code_t storage_macro_reorder(const storage_macro_location_t *location,
                                       const app_uuid_t *ordered_ids, size_t count);

app_error_code_t storage_procedure_list(const app_uuid_t *set_id,
                                        storage_procedure_list_t *out_list);
void storage_procedure_list_free(storage_procedure_list_t *list);
app_error_code_t storage_procedure_create(const app_uuid_t *set_id, const procedure_t *procedure);
app_error_code_t storage_procedure_read(const app_uuid_t *set_id, const app_uuid_t *procedure_id,
                                        procedure_t *out_procedure);
app_error_code_t storage_procedure_update(const app_uuid_t *set_id, const procedure_t *replacement,
                                          uint32_t expected_revision, procedure_t *out_updated);
app_error_code_t storage_procedure_delete(const app_uuid_t *set_id, const app_uuid_t *procedure_id,
                                          uint32_t expected_revision);
app_error_code_t storage_procedure_reorder(const app_uuid_t *set_id, const app_uuid_t *ordered_ids,
                                           size_t count);

app_error_code_t storage_progress_read(const storage_procedure_identity_t *identity,
                                       storage_progress_snapshot_t *out_snapshot);
app_error_code_t storage_progress_update(const storage_procedure_identity_t *identity,
                                         const procedure_progress_t *replacement,
                                         storage_progress_snapshot_t *out_snapshot);
app_error_code_t storage_progress_reset(const storage_procedure_identity_t *identity,
                                        uint32_t expected_procedure_revision,
                                        storage_progress_snapshot_t *out_snapshot);

#endif
