#include "factory_reset_state_core.h"

#include <stddef.h>
#include <stdint.h>

bool factory_reset_state_core_ops_is_valid(const factory_reset_state_core_ops_t *operations) {
    return operations != NULL && operations->read_value != NULL &&
           operations->write_value != NULL && operations->erase_value != NULL;
}

app_error_code_t factory_reset_state_core_read(const factory_reset_state_core_ops_t *operations,
                                               factory_reset_state_t *out_state) {
    if (!factory_reset_state_core_ops_is_valid(operations) || out_state == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_state = FACTORY_RESET_STATE_NONE;
    uint8_t stored = 0U;
    const app_error_code_t result = operations->read_value(operations->context, &stored);
    if (result == APP_ERROR_NOT_FOUND) {
        return APP_ERROR_NONE;
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (stored != (uint8_t)FACTORY_RESET_STATE_PENDING) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_state = FACTORY_RESET_STATE_PENDING;
    return APP_ERROR_NONE;
}

app_error_code_t
factory_reset_state_core_mark_pending(const factory_reset_state_core_ops_t *operations) {
    if (!factory_reset_state_core_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return operations->write_value(operations->context, (uint8_t)FACTORY_RESET_STATE_PENDING);
}

app_error_code_t factory_reset_state_core_clear(const factory_reset_state_core_ops_t *operations) {
    if (!factory_reset_state_core_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_error_code_t result = operations->erase_value(operations->context);
    return result == APP_ERROR_NOT_FOUND ? APP_ERROR_NONE : result;
}
