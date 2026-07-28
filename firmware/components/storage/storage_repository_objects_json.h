#ifndef STORAGE_REPOSITORY_OBJECTS_JSON_H
#define STORAGE_REPOSITORY_OBJECTS_JSON_H

#include <stddef.h>

#include "app_error.h"
#include "macro_model.h"

#define STORAGE_MACRO_FILE_MAX_BYTES 8192U
#define STORAGE_PROCEDURE_FILE_MAX_BYTES (256U * 1024U)
#define STORAGE_PROGRESS_FILE_MAX_BYTES (32U * 1024U)
#define STORAGE_ORDER_FILE_MAX_BYTES 8192U
#define STORAGE_ORDER_MAX_IDS APP_STEPS_PER_PROCEDURE_MAX

typedef struct {
    app_uuid_t ids[STORAGE_ORDER_MAX_IDS];
    size_t count;
} storage_uuid_order_t;

app_error_code_t storage_repository_parse_macro_json(const char *data, size_t length,
                                                     macro_t *out_macro);
app_error_code_t storage_repository_serialize_macro_json(const macro_t *macro, char **out_json,
                                                         size_t *out_length);
app_error_code_t storage_repository_parse_procedure_json(const char *data, size_t length,
                                                         procedure_t *out_procedure);
app_error_code_t storage_repository_serialize_procedure_json(const procedure_t *procedure,
                                                             char **out_json, size_t *out_length);
app_error_code_t storage_repository_parse_progress_json(const char *data, size_t length,
                                                        procedure_progress_t *out_progress);
app_error_code_t storage_repository_serialize_progress_json(const procedure_progress_t *progress,
                                                            char **out_json, size_t *out_length);
app_error_code_t storage_repository_parse_order_json(const char *data, size_t length,
                                                     storage_uuid_order_t *out_order,
                                                     size_t maximum_count);
app_error_code_t storage_repository_serialize_order_json(const storage_uuid_order_t *order,
                                                         size_t maximum_count, char **out_json,
                                                         size_t *out_length);

#endif
