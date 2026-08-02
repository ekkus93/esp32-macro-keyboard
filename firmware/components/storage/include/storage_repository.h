#ifndef STORAGE_REPOSITORY_H
#define STORAGE_REPOSITORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"

typedef struct {
    macro_set_t items[APP_MACRO_SETS_MAX];
    size_t count;
} storage_set_list_t;

/* Identifies the single object a bulk read stopped on. List reads abort at the
 * first unreadable object and otherwise discard which one it was, which left
 * callers reporting failures they could not attribute to anything. */
typedef struct {
    bool has_id;
    app_uuid_t id;
} storage_object_ref_t;

/* Collects the objects a tolerant bulk read stepped over. `total` counts every
 * skipped object; `count` counts the ones that fit `items`, so a caller can
 * always tell that more were dropped than it can enumerate. */
typedef struct {
    storage_object_ref_t *items;
    size_t capacity;
    size_t count;
    size_t total;
} storage_skip_record_t;

typedef struct {
    macro_t *items;
    size_t count;
} storage_macro_list_t;

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
app_error_code_t storage_set_duplicate(const app_uuid_t *source_id, uint32_t expected_revision,
                                       const app_uuid_t *duplicate_id, const char *duplicate_name,
                                       macro_set_t *out_duplicate);
app_error_code_t storage_set_reorder(const app_uuid_t *ordered_ids, size_t count);

/* The active set lives in the index (SPEC 12.3), not in NVS: it is a property of
 * the macro-set repository, and keeping it beside the set order is what lets
 * deleting the active set clear it in the same atomic index write.
 *
 * `storage_set_select` requires the set to exist. Selection is always explicit;
 * nothing infers or auto-switches the active set (SPEC 10.1). */
app_error_code_t storage_active_set_read(bool *out_has_active_set, app_uuid_t *out_set_id);
app_error_code_t storage_set_select(const app_uuid_t *set_id);

/* Every macro is addressed by its owning set (SPEC §7.2). */
app_error_code_t storage_macro_list(const app_uuid_t *set_id, storage_macro_list_t *out_list);
void storage_macro_list_free(storage_macro_list_t *list);
app_error_code_t storage_macro_create(const app_uuid_t *set_id, const macro_t *macro);
app_error_code_t storage_macro_read(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                    macro_t *out_macro);
app_error_code_t storage_macro_update(const app_uuid_t *set_id, const macro_t *replacement,
                                      uint32_t expected_revision, macro_t *out_updated);
app_error_code_t storage_macro_delete(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                      uint32_t expected_revision);
app_error_code_t storage_macro_duplicate(const app_uuid_t *set_id, const app_uuid_t *source_id,
                                         const app_uuid_t *duplicate_id, const char *duplicate_name,
                                         macro_t *out_duplicate);
app_error_code_t storage_macro_reorder(const app_uuid_t *set_id, const app_uuid_t *ordered_ids,
                                       size_t count);

#endif
