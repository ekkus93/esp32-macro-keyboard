#ifndef FACTORY_RESET_STATE_CORE_H
#define FACTORY_RESET_STATE_CORE_H

#include <stdbool.h>
#include <stdint.h>

#include "factory_reset_state.h"

typedef struct {
    void *context;
    app_error_code_t (*read_value)(void *context, uint8_t *out_value);
    app_error_code_t (*write_value)(void *context, uint8_t value);
    app_error_code_t (*erase_value)(void *context);
} factory_reset_state_core_ops_t;

bool factory_reset_state_core_ops_is_valid(const factory_reset_state_core_ops_t *operations);
app_error_code_t factory_reset_state_core_read(const factory_reset_state_core_ops_t *operations,
                                               factory_reset_state_t *out_state);
app_error_code_t
factory_reset_state_core_mark_pending(const factory_reset_state_core_ops_t *operations);
app_error_code_t factory_reset_state_core_clear(const factory_reset_state_core_ops_t *operations);

#endif
