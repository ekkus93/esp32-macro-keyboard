#ifndef STORAGE_REPOSITORY_SETS_INTERNAL_H
#define STORAGE_REPOSITORY_SETS_INTERNAL_H

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"

app_error_code_t storage_set_read_locked(const app_uuid_t *set_id, macro_set_t *out_set);

#ifndef ESP_PLATFORM
typedef struct {
    void *context;
    app_error_code_t (*clear_active_set_if_matches)(void *context, const app_uuid_t *set_id);
} storage_repository_set_settings_ops_t;

void storage_repository_sets_set_settings_ops_for_test(
    const storage_repository_set_settings_ops_t *operations);
void storage_repository_sets_reset_settings_ops_for_test(void);
#endif

#endif
