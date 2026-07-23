#ifndef STORAGE_REPOSITORY_H
#define STORAGE_REPOSITORY_H

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"

typedef struct {
    macro_set_t items[APP_MACRO_SETS_MAX];
    size_t count;
} storage_set_list_t;

app_error_code_t storage_repository_init(void);
app_error_code_t storage_set_list(storage_set_list_t *out_list);
app_error_code_t storage_set_read(const app_uuid_t *set_id, macro_set_t *out_set);
app_error_code_t storage_set_create(const macro_set_t *set);
app_error_code_t storage_set_update(const macro_set_t *replacement,
                                    uint32_t expected_revision,
                                    macro_set_t *out_updated);
app_error_code_t storage_set_delete(const app_uuid_t *set_id, uint32_t expected_revision);

#endif
