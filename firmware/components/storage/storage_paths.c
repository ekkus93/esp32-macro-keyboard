#include "storage.h"

#include <stdio.h>

app_error_code_t storage_make_set_path(const app_uuid_t *set_id,
                                       char *buffer,
                                       size_t buffer_size)
{
    if (set_id == NULL || buffer == NULL || buffer_size == 0U ||
        !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(buffer,
                                 buffer_size,
                                 STORAGE_DATA_MOUNT "/sets/%s",
                                 set_id->value);
    if (written < 0 || (size_t)written >= buffer_size) {
        buffer[0] = '\0';
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_make_macro_path(const app_uuid_t *set_id,
                                         const app_uuid_t *macro_id,
                                         char *buffer,
                                         size_t buffer_size)
{
    if (set_id == NULL || macro_id == NULL || buffer == NULL || buffer_size == 0U ||
        !app_uuid_is_valid_string(set_id->value) ||
        !app_uuid_is_valid_string(macro_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(buffer,
                                 buffer_size,
                                 STORAGE_DATA_MOUNT "/sets/%s/macros/%s.json",
                                 set_id->value,
                                 macro_id->value);
    if (written < 0 || (size_t)written >= buffer_size) {
        buffer[0] = '\0';
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}
