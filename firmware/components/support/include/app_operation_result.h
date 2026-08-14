#ifndef APP_OPERATION_RESULT_H
#define APP_OPERATION_RESULT_H

#include <stdbool.h>

#include "app_error.h"

/* Commit/durability state is meaningful only for operations that activate a
 * canonical value. NOT_APPLICABLE keeps existing non-commit users explicit;
 * NOT_COMMITTED means activation did not occur; COMMITTED means activation and
 * its required durability acknowledgement completed; COMMIT_UNCERTAIN means the
 * canonical activation point was crossed but final durability acknowledgement
 * failed, so callers must reconcile state rather than assume nothing changed. */
typedef enum {
    APP_OPERATION_COMMIT_NOT_APPLICABLE = 0,
    APP_OPERATION_NOT_COMMITTED,
    APP_OPERATION_COMMITTED,
    APP_OPERATION_COMMIT_UNCERTAIN,
} app_operation_commit_state_t;

/* A structured result for operations that perform primary work and may also run
 * cleanup that can fail independently. It preserves the first primary error and,
 * separately, the first cleanup/release/durability error, records whether cleanup
 * completed, and can carry commit certainty for mutation paths. This does not
 * replace or collapse the stable app_error_code_t API. */
typedef struct {
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
    app_operation_commit_state_t commit_state;
} app_operation_result_t;

static inline app_operation_result_t app_operation_success(void) {
    return (app_operation_result_t){
        .primary_error = APP_ERROR_NONE,
        .cleanup_error = APP_ERROR_NONE,
        .cleanup_incomplete = false,
        .commit_state = APP_OPERATION_COMMIT_NOT_APPLICABLE,
    };
}

static inline bool app_operation_result_ok(app_operation_result_t result) {
    return result.primary_error == APP_ERROR_NONE && result.cleanup_error == APP_ERROR_NONE &&
           !result.cleanup_incomplete && result.commit_state != APP_OPERATION_NOT_COMMITTED &&
           result.commit_state != APP_OPERATION_COMMIT_UNCERTAIN;
}

void app_operation_record_primary(app_operation_result_t *result, app_error_code_t error);
void app_operation_record_cleanup(app_operation_result_t *result, app_error_code_t error);

/* Stable single-error compatibility mapping for public APIs that cannot yet
 * return the structured result. The initiating error remains authoritative;
 * cleanup/release/durability error is returned only when no primary error exists. */
app_error_code_t app_operation_result_error(app_operation_result_t result);

#endif
