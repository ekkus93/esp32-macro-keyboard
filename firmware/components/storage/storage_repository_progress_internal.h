#ifndef STORAGE_REPOSITORY_PROGRESS_INTERNAL_H
#define STORAGE_REPOSITORY_PROGRESS_INTERNAL_H

#include "app_error.h"
#include "storage_repository.h"

app_error_code_t storage_progress_read_locked(const storage_procedure_identity_t *identity,
                                              storage_progress_snapshot_t *out_snapshot);

#endif
