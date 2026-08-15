#ifndef APP_CORE_OPS_H
#define APP_CORE_OPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "device_controls.h"
#include "device_settings_v2.h"
#include "provisioning_bootstrap.h"
#include "setup_contract_v2.h"
#include "web_server.h"

typedef enum {
    APP_CORE_NVS_OK = 0,
    APP_CORE_NVS_NO_FREE_PAGES,
    APP_CORE_NVS_NEW_VERSION_FOUND,
    APP_CORE_NVS_OTHER_FAILURE
} app_core_nvs_result_t;

typedef enum {
    APP_CORE_LOG_STAGE = 0,
    APP_CORE_LOG_PROVISIONING_REQUIRED,
    APP_CORE_LOG_CLEANUP_FAILED
} app_core_log_type_t;

typedef struct {
    app_core_log_type_t type;
    const char *stage;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
    uint32_t operation_id;
} app_core_log_event_t;

typedef struct {
    const char *ap_ssid;
    const char *ap_passphrase;
    bool station_configured;
    const char *station_ssid;
    const char *station_passphrase;
} app_core_wifi_configuration_t;

typedef struct {
    void *context;
    app_core_nvs_result_t (*nvs_init)(void *context);
    app_error_code_t (*settings_init)(void *context);
    app_error_code_t (*settings_read)(void *context, app_v2_device_settings_t *out_settings);
    app_error_code_t (*bootstrap_derive)(void *context, provisioning_bootstrap_t *out_bootstrap);
    app_error_code_t (*setup_code_generate)(void *context,
                                            char out_code[APP_V2_SETUP_CODE_BUFFER_BYTES]);
    app_error_code_t (*show_setup_code)(void *context, const char *setup_code);
    app_error_code_t (*storage_mount)(void *context);
    app_error_code_t (*auth_init)(void *context);
    app_error_code_t (*usb_init)(void *context);
    app_error_code_t (*executor_init)(void *context);
    app_error_code_t (*controls_init)(void *context);
    app_error_code_t (*wifi_start)(void *context,
                                   const app_core_wifi_configuration_t *configuration);
    app_error_code_t (*http_start)(void *context, const web_server_config_t *configuration);
    app_error_code_t (*http_stop)(void *context);
    app_error_code_t (*wifi_stop)(void *context);
    app_error_code_t (*storage_unmount)(void *context);
    app_error_code_t (*auth_deinit)(void *context);
    app_error_code_t (*usb_deinit)(void *context);
    app_error_code_t (*executor_deinit)(void *context);
    app_error_code_t (*controls_deinit)(void *context);
    app_error_code_t (*settings_deinit)(void *context);
    app_error_code_t (*nvs_deinit)(void *context);
    bool (*http_owns_resources)(void *context);
    bool (*wifi_owns_resources)(void *context);
    bool (*storage_owns_mount)(void *context);
    app_error_code_t (*set_indicator)(void *context, device_indicator_state_t indicator);
    void (*secure_zero)(void *context, void *memory, size_t length);
    void (*log_event)(void *context, const app_core_log_event_t *event);
} app_core_ops_t;

#endif
