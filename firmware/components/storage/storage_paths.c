#include "storage.h"

#include <stdio.h>

static app_error_code_t checked_format(char *buffer,
                                       size_t buffer_size,
                                       const char *format,
                                       const char *first,
                                       const char *second)
{
    if (buffer == NULL || buffer_size == 0U || format == NULL || first == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = second == NULL
                            ? snprintf(buffer, buffer_size, format, first)
                            : snprintf(buffer, buffer_size, format, first, second);
    if (written < 0 || (size_t)written >= buffer_size) {
        if (buffer_size > 0U) {
            buffer[0] = '\0';
        }
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_make_set_path(const app_uuid_t *set_id, char *buffer, size_t buffer_size)
{
    if (set_id == NULL || !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return checked_format(buffer, buffer_size, STORAGE_DATA_MOUNT "/sets/%s", set_id->value, NULL);
}

app_error_code_t storage_make_macro_path(const app_uuid_t *set_id,
                                         const app_uuid_t *macro_id,
                                         char *buffer,
                                         size_t buffer_size)
{
    if (set_id == NULL || macro_id == NULL || !app_uuid_is_valid_string(set_id->value) ||
        !app_uuid_is_valid_string(macro_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return checked_format(buffer, buffer_size, STORAGE_DATA_MOUNT "/sets/%s/macros/%s.json",
                          set_id->value, macro_id->value);
}
