#ifndef DEVICE_SETTINGS_CORE_H
#define DEVICE_SETTINGS_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "device_settings_v2.h"

typedef struct {
    void *context;
    app_error_code_t (*read_record)(void *context, uint8_t *record, size_t capacity,
                                    size_t *out_length);
    app_error_code_t (*replace_record_atomic)(void *context, const uint8_t *record,
                                              size_t length);
    void (*secure_zero)(void *context, void *memory, size_t length);
} device_settings_core_ops_t;

typedef struct {
    device_settings_core_ops_t ops;
    app_v2_device_settings_t current;
    bool loaded;
    bool record_present;
} device_settings_core_t;

app_error_code_t device_settings_core_init(device_settings_core_t *core,
                                           const device_settings_core_ops_t *operations);
app_error_code_t device_settings_core_load(device_settings_core_t *core,
                                           app_v2_device_settings_t *out_settings);
app_error_code_t device_settings_core_replace(device_settings_core_t *core,
                                              const app_v2_device_settings_t *settings,
                                              bool *out_changed);
app_error_code_t device_settings_core_reset_noncredential(device_settings_core_t *core,
                                                          app_v2_device_settings_t *out_settings,
                                                          bool *out_changed);
void device_settings_core_deinit(device_settings_core_t *core);

#endif
