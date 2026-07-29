#ifndef STORAGE_PACKAGE_INTERNAL_H
#define STORAGE_PACKAGE_INTERNAL_H

#ifndef ESP_PLATFORM

#include "storage_package.h"
#include "storage_repository.h"

typedef struct {
    void *context;
    app_error_code_t (*lock_take)(void *context);
    app_error_code_t (*lock_give)(void *context);
    app_error_code_t (*set_read)(void *context, const app_uuid_t *set_id, macro_set_t *out_set);
    app_error_code_t (*macro_list)(void *context, const storage_macro_location_t *location,
                                   storage_macro_list_t *out_list);
    void (*macro_list_free)(void *context, storage_macro_list_t *list);
    app_error_code_t (*procedure_list)(void *context, const app_uuid_t *set_id,
                                       storage_procedure_list_t *out_list);
    void (*procedure_list_free)(void *context, storage_procedure_list_t *list);
    app_error_code_t (*progress_read)(void *context,
                                      const storage_procedure_identity_t *identity,
                                      storage_progress_snapshot_t *out_snapshot);
} storage_package_export_ops_t;

void storage_package_set_export_ops_for_test(const storage_package_export_ops_t *operations);
void storage_package_reset_export_ops_for_test(void);

#endif

#endif
