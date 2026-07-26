#include "app_operation_result.h"

#include <stddef.h>

#include "app_error.h"

void app_operation_record_primary(app_operation_result_t *result, app_error_code_t error) {
    if (result != NULL && error != APP_ERROR_NONE && result->primary_error == APP_ERROR_NONE) {
        result->primary_error = error;
    }
}

void app_operation_record_cleanup(app_operation_result_t *result, app_error_code_t error) {
    if (result != NULL && error != APP_ERROR_NONE) {
        if (result->cleanup_error == APP_ERROR_NONE) {
            result->cleanup_error = error;
        }
        result->cleanup_incomplete = true;
    }
}
