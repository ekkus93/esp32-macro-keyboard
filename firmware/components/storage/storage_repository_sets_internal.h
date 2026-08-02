#ifndef STORAGE_REPOSITORY_SETS_INTERNAL_H
#define STORAGE_REPOSITORY_SETS_INTERNAL_H

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"

app_error_code_t storage_set_read_locked(const app_uuid_t *set_id, macro_set_t *out_set);

#endif
