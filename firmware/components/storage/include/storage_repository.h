#ifndef STORAGE_REPOSITORY_H
#define STORAGE_REPOSITORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"

typedef struct {
    macro_package_t items[APP_MACRO_SETS_MAX];
    size_t count;
} storage_package_list_t;

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
app_error_code_t storage_package_list(storage_package_list_t *out_list);
app_error_code_t storage_package_create(const macro_package_t *set);
app_error_code_t storage_package_read(const app_uuid_t *set_id, macro_package_t *out_package);
app_error_code_t storage_package_update(const macro_package_t *replacement,
                                        uint32_t expected_revision, macro_package_t *out_updated);
app_error_code_t storage_package_delete(const app_uuid_t *set_id, uint32_t expected_revision);
app_error_code_t storage_package_duplicate(const app_uuid_t *source_id, uint32_t expected_revision,
                                           const app_uuid_t *duplicate_id,
                                           const char *duplicate_name,
                                           macro_package_t *out_duplicate);
app_error_code_t storage_package_reorder(const app_uuid_t *ordered_ids, size_t count);

/* The active set lives in the index (SPEC 12.3), not in NVS: it is a property of
 * the macro-set repository, and keeping it beside the set order is what lets
 * deleting the active set clear it in the same atomic index write.
 *
 * `storage_package_select` requires the set to exist. Selection is always explicit;
 * nothing infers or auto-switches the active set (SPEC 10.1). */
app_error_code_t storage_active_package_read(bool *out_has_active_package,
                                             app_uuid_t *out_package_id);

/* Bytes every set file currently occupies, excluding `exclude_package_id` when it is
 * non-NULL. Measures the filesystem rather than trusting the per-object limits,
 * which is what SPEC 10.7 requires: those limits alone would permit 20 MB of
 * user data on a 512 KiB partition. */
app_error_code_t storage_repository_measure_user_data(const app_uuid_t *exclude_package_id,
                                                      size_t *out_bytes);
app_error_code_t storage_package_select(const app_uuid_t *set_id);

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
