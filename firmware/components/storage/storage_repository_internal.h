#ifndef STORAGE_REPOSITORY_INTERNAL_H
#define STORAGE_REPOSITORY_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"
#include "storage_atomic_internal.h"
#include "storage_fs_ops.h"
#include "storage_object_json.h"

/* SPEC 13.3: /data holds index.json and sets/, and nothing else. The separate
 * schema.json marker is gone -- the index carries schema_version itself, and a
 * second file recording the same number is a second thing to keep in step. */
#define STORAGE_INDEX_FILE_PATH STORAGE_DATA_MOUNT "/index.json"

/* SPEC 12.3. `revision` is the index's own revision, not any set's.
 * `has_active_set` is false until the user selects one -- SPEC 10.1 forbids
 * inferring or automatically switching the active set. */
typedef struct {
    uint32_t revision;
    bool has_active_set;
    app_uuid_t active_set_id;
    app_uuid_t ids[APP_MACRO_SETS_MAX];
    size_t count;
} storage_set_index_t;

app_error_code_t storage_repository_map_file_error(void);
app_error_code_t storage_repository_map_error_number(int error_number);
app_error_code_t storage_repository_read_bounded_file_with_ops(const char *path, size_t maximum,
                                                               char **out_data, size_t *out_length,
                                                               const storage_fs_ops_t *operations);
app_error_code_t storage_repository_read_bounded_file(const char *path, size_t maximum,
                                                      char **out_data, size_t *out_length);
app_error_code_t storage_repository_directory_has_entries_with_ops(
    const char *path, const storage_fs_ops_t *operations, bool *out_has_entries);
app_error_code_t storage_repository_directory_has_entries(const char *path, bool *out_has_entries);
app_error_code_t
storage_repository_ensure_initial_file_with_ops(const char *path, const char *contents,
                                                const storage_fs_ops_t *operations);
app_error_code_t storage_repository_ensure_initial_file(const char *path, const char *contents);
/* Deletes a file whose contents could not be parsed. Replaces quarantine:
 * damaged data is discarded and the failure is reported, not archived. */
app_error_code_t storage_repository_discard_corrupt_file(const char *path);
app_error_code_t storage_repository_make_directory_with_ops(const char *path,
                                                            const storage_fs_ops_t *operations);
app_error_code_t storage_repository_make_directory(const char *path);
app_error_code_t storage_repository_remove_tree_with_ops(const char *path,
                                                         const storage_fs_ops_t *operations);
app_error_code_t storage_repository_remove_tree(const char *path);
app_error_code_t storage_repository_parse_index(const char *data, size_t length,
                                                storage_set_index_t *out_index);
app_error_code_t storage_repository_load_index(storage_set_index_t *out_index);
app_error_code_t storage_repository_load_index_path(const char *path,
                                                    storage_set_index_t *out_index);
app_error_code_t storage_repository_write_index(const storage_set_index_t *index);
app_error_code_t storage_repository_set_index_presence(const app_uuid_t *set_id,
                                                       bool should_be_present);

#endif
