#ifndef STORAGE_REPOSITORY_PROCEDURES_INTERNAL_H
#define STORAGE_REPOSITORY_PROCEDURES_INTERNAL_H

#include "app_error.h"
#include "macro_model.h"
#include "storage_repository.h"

app_error_code_t storage_procedure_list_locked(const app_uuid_t *set_id,
                                               storage_procedure_list_t *out_list);
/* As storage_procedure_list_locked, but reports which procedure the read
 * stopped on so the caller can name it. out_failed may be NULL. */
app_error_code_t storage_procedure_list_detail_locked(const app_uuid_t *set_id,
                                                      storage_procedure_list_t *out_list,
                                                      storage_object_ref_t *out_failed);
app_error_code_t storage_procedure_read_locked(const storage_procedure_identity_t *identity,
                                               procedure_t *out_procedure);

#endif
