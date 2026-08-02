#ifndef STORAGE_REPOSITORY_MACROS_INTERNAL_H
#define STORAGE_REPOSITORY_MACROS_INTERNAL_H

#include "app_error.h"
#include "macro_model.h"
#include "storage_repository.h"

app_error_code_t storage_macro_list_locked(const storage_macro_location_t *location,
                                           storage_macro_list_t *out_list);
/* As storage_macro_list_locked, but reports which macro the read stopped on so
 * the caller can name it. out_failed may be NULL. */
/* out_skips, when non-NULL, makes the read tolerant: objects that are
 * individually unusable are stepped over and recorded instead of aborting the
 * whole list. Device-level errors still abort, and out_failed then names the
 * object the read stopped on. Either may be NULL. */
app_error_code_t storage_macro_list_detail_locked(const storage_macro_location_t *location,
                                                  storage_macro_list_t *out_list,
                                                  storage_object_ref_t *out_failed,
                                                  storage_skip_record_t *out_skips);
app_error_code_t storage_macro_read_locked(const storage_macro_location_t *location,
                                           const app_uuid_t *macro_id, macro_t *out_macro);

#endif
