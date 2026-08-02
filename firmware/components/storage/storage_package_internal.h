#ifndef STORAGE_PACKAGE_INTERNAL_H
#define STORAGE_PACKAGE_INTERNAL_H

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
    app_error_code_t (*progress_read)(void *context, const storage_procedure_identity_t *identity,
                                      storage_progress_snapshot_t *out_snapshot);
} storage_package_export_ops_t;

typedef struct {
    void *context;
    app_error_code_t (*lock_take)(void *context);
    app_error_code_t (*lock_give)(void *context);
    app_error_code_t (*set_list)(void *context, storage_set_list_t *out_list);
    /* out_failed reports which object the read stopped on, so a failed backup
     * can name it instead of failing anonymously. It may be NULL. */
    app_error_code_t (*macro_list)(void *context, const storage_macro_location_t *location,
                                   storage_macro_list_t *out_list,
                                   storage_object_ref_t *out_failed);
    void (*macro_list_free)(void *context, storage_macro_list_t *list);
    app_error_code_t (*procedure_list)(void *context, const app_uuid_t *set_id,
                                       storage_procedure_list_t *out_list,
                                       storage_object_ref_t *out_failed);
    void (*procedure_list_free)(void *context, storage_procedure_list_t *list);
    app_error_code_t (*progress_read)(void *context, const storage_procedure_identity_t *identity,
                                      storage_progress_snapshot_t *out_snapshot);
} storage_package_backup_ops_t;

#ifndef ESP_PLATFORM
void storage_package_set_export_ops_for_test(const storage_package_export_ops_t *operations);
void storage_package_reset_export_ops_for_test(void);
void storage_package_set_backup_ops_for_test(const storage_package_backup_ops_t *operations);
void storage_package_reset_backup_ops_for_test(void);
#endif

#endif
