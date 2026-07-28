#ifndef STORAGE_REPOSITORY_ORDER_H
#define STORAGE_REPOSITORY_ORDER_H

#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "storage_repository_objects_json.h"

app_error_code_t storage_repository_load_order_locked(const char *path, size_t maximum_count,
                                                      storage_uuid_order_t *out_order);
app_error_code_t storage_repository_write_order_locked(const char *path, size_t maximum_count,
                                                       const storage_uuid_order_t *order);
bool storage_repository_order_contains(const storage_uuid_order_t *order, const app_uuid_t *item_id,
                                       size_t *out_index);
app_error_code_t storage_repository_order_append(storage_uuid_order_t *order, size_t maximum_count,
                                                 const app_uuid_t *item_id);
app_error_code_t storage_repository_order_remove(storage_uuid_order_t *order,
                                                 const app_uuid_t *item_id);
bool storage_repository_order_same_members(const storage_uuid_order_t *left,
                                           const storage_uuid_order_t *right);

#endif
