#include "storage.h"

#include "app_error.h"

app_error_code_t storage_prepare_directories(void) {
    /* Phase 2 deliberately removes the firmware-owned package and macro
     * repository. The data partition remains mounted for the bounded blob
     * repository introduced in Phase 3, but startup creates no v1 topology. */
    return APP_ERROR_NONE;
}
