#ifndef DEVICE_SETTINGS_H
#define DEVICE_SETTINGS_H

#include <stdbool.h>

#include "app_error.h"
#include "device_settings_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

app_error_code_t device_settings_init(void);
app_error_code_t device_settings_deinit(void);
app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings);
app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed);
app_error_code_t device_settings_reset_noncredential(app_v2_device_settings_t *out_settings,
                                                     bool *out_changed);

#ifdef __cplusplus
}
#endif

#endif
