#ifndef STORAGE_SET_TREE_INTERNAL_H
#define STORAGE_SET_TREE_INTERNAL_H

#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"

app_error_code_t storage_set_tree_validate(const char *path, const app_uuid_t *set_id,
                                           uint32_t expected_revision);

#endif
