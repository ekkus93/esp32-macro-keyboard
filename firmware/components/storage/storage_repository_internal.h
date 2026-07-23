#ifndef STORAGE_REPOSITORY_INTERNAL_H
#define STORAGE_REPOSITORY_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"

#define STORAGE_SET_FILE_MAX_BYTES 4096U
#define STORAGE_INDEX_FILE_MAX_BYTES 4096U
#define STORAGE_SCHEMA_FILE_PATH STORAGE_DATA_MOUNT "/schema.json"
#define STORAGE_SET_INDEX_FILE_PATH STORAGE_DATA_MOUNT "/set-index.json"
#define STORAGE_GLOBAL_ORDER_FILE_PATH STORAGE_DATA_MOUNT "/global/macro-order.json"

typedef struct {
    app_uuid_t ids[APP_MACRO_SETS_MAX];
    size_t count;
} storage_set_index_t;

app_error_code_t storage_repository_map_file_error(void);
app_error_code_t storage_repository_read_bounded_file(const char *path,
                                                      size_t maximum,
                                                      char **out_data,
                                                      size_t *out_length);
bool storage_repository_directory_has_entries(const char *path);
app_error_code_t storage_repository_ensure_initial_file(const char *path,
                                                        const char *contents);
app_error_code_t storage_repository_set_file_path(const app_uuid_t *set_id,
                                                  char *buffer,
                                                  size_t buffer_size);
app_error_code_t storage_repository_remove_manifest(const app_uuid_t *transaction_id);
app_error_code_t storage_repository_make_directory(const char *path);
app_error_code_t storage_repository_remove_tree(const char *path);
app_error_code_t storage_repository_parse_set_json(const char *data,
                                                   size_t length,
                                                   macro_set_t *out_set);
app_error_code_t storage_repository_serialize_set_json(const macro_set_t *set,
                                                       char **out_json,
                                                       size_t *out_length);
app_error_code_t storage_repository_load_index(storage_set_index_t *out_index);
app_error_code_t storage_repository_load_index_path(const char *path,
                                                    storage_set_index_t *out_index);
app_error_code_t storage_repository_write_index(const storage_set_index_t *index);
app_error_code_t storage_repository_set_index_presence(const app_uuid_t *set_id,
                                                       bool should_be_present);

#endif
