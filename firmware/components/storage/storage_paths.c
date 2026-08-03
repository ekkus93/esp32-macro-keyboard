#include "storage.h"

#include <stdio.h>

#include "app_error.h"
#include "app_uuid.h"

/* One file per set (SPEC 13.3). There is no set directory, no macros/ child, and
 * no order file, so this is the only path helper the repository needs. */
app_error_code_t storage_make_package_path(const app_uuid_t *set_id, char *buffer,
                                           size_t buffer_size) {
    if (set_id == NULL || buffer == NULL || buffer_size == 0U ||
        !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written =
        snprintf(buffer, buffer_size, STORAGE_DATA_MOUNT "/sets/%s.json", set_id->value);
    if (written < 0 || (size_t)written >= buffer_size) {
        buffer[0] = '\0';
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}
