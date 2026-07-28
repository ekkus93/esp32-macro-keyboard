#ifndef STORAGE_REPOSITORY_MACROS_INTERNAL_H
#define STORAGE_REPOSITORY_MACROS_INTERNAL_H

#include "app_error.h"
#include "macro_model.h"
#include "storage_repository.h"

app_error_code_t storage_macro_list_locked(const storage_macro_location_t *location,
                                           storage_macro_list_t *out_list);
app_error_code_t storage_macro_read_locked(const storage_macro_location_t *location,
                                           const app_uuid_t *macro_id, macro_t *out_macro);

#endif
