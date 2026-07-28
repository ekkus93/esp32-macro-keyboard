#ifndef STORAGE_REPOSITORY_PROCEDURES_INTERNAL_H
#define STORAGE_REPOSITORY_PROCEDURES_INTERNAL_H

#include "app_error.h"
#include "macro_model.h"
#include "storage_repository.h"

app_error_code_t storage_procedure_read_locked(const storage_procedure_identity_t *identity,
                                               procedure_t *out_procedure);

#endif
