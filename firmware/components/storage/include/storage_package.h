#ifndef STORAGE_PACKAGE_H
#define STORAGE_PACKAGE_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"

typedef enum {
    STORAGE_PACKAGE_KIND_SET = 0,
    STORAGE_PACKAGE_KIND_BACKUP,
} storage_package_kind_t;

typedef struct {
    storage_package_kind_t kind;
    size_t package_bytes;
    size_t set_count;
    size_t local_macro_count;
    size_t global_macro_count;
    size_t procedure_count;
    size_t progress_count;
} storage_package_summary_t;

app_error_code_t storage_package_validate(const char *data, size_t length,
                                          storage_package_kind_t expected_kind,
                                          storage_package_summary_t *out_summary);
app_error_code_t storage_package_export_set(const app_uuid_t *set_id, bool include_progress,
                                             char **out_data, size_t *out_length);
app_error_code_t storage_package_export_backup(bool include_progress, char **out_data,
                                                size_t *out_length);
app_error_code_t storage_package_replace_set(const app_uuid_t *target_set_id,
                                             uint32_t expected_revision, const char *data,
                                             size_t length, macro_set_t *out_set);
void storage_package_free(char *data);

#endif
