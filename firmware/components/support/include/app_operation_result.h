#ifndef APP_OPERATION_RESULT_H
#define APP_OPERATION_RESULT_H

#include <stdbool.h>

#include "app_error.h"

/* A structured result for operations that perform primary work and may also run
 * cleanup that can fail independently. It preserves the first primary error and,
 * separately, the first cleanup error, and records whether cleanup completed —
 * so a cleanup failure after a successful primary action is never presented as a
 * plain success, and a primary failure is never overwritten by a later cleanup
 * error. This does not replace or collapse the stable app_error_code_t API. */
typedef struct {
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} app_operation_result_t;

static inline app_operation_result_t app_operation_success(void) {
    return (app_operation_result_t){
        .primary_error = APP_ERROR_NONE,
        .cleanup_error = APP_ERROR_NONE,
        .cleanup_incomplete = false,
    };
}

static inline bool app_operation_result_ok(app_operation_result_t result) {
    return result.primary_error == APP_ERROR_NONE && result.cleanup_error == APP_ERROR_NONE &&
           !result.cleanup_incomplete;
}

void app_operation_record_primary(app_operation_result_t *result, app_error_code_t error);
void app_operation_record_cleanup(app_operation_result_t *result, app_error_code_t error);

#endif
