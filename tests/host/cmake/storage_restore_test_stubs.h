#ifndef STORAGE_RESTORE_TEST_STUBS_H
#define STORAGE_RESTORE_TEST_STUBS_H

#include <stdbool.h>

#include "app_error.h"
#include "app_uuid.h"

app_error_code_t storage_repository_set_index_presence(const app_uuid_t *set_id,
                                                       bool should_be_present);

#endif
