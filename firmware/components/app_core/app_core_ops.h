#ifndef APP_CORE_OPS_H
#define APP_CORE_OPS_H

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "auth.h"
#include "device_controls.h"
#include "web_server.h"

typedef enum {
    APP_CORE_NVS_OK = 0,
    APP_CORE_NVS_NO_FREE_PAGES,
    APP_CORE_NVS_NEW_VERSION_FOUND,
    APP_CORE_NVS_OTHER_FAILURE
} app_core_nvs_result_t;

typedef enum {
    APP_CORE_LOG_STAGE = 0,
    APP_CORE_LOG_STORAGE_DEGRADED,
    APP_CORE_LOG_DEVELOPMENT_CREDENTIALS,
    APP_CORE_LOG_PROVISIONING_REQUIRED,
    APP_CORE_LOG_CLEANUP_FAILED
} app_core_log_type_t;

typedef struct {
    app_core_log_type_t type;
    const char *stage;
    app_error_code_t primary_error;
    app_error_code_t secondary_error;
    const char *ssid;
    const char *ap_passphrase;
    const char *web_password;
} app_core_log_event_t;

typedef struct {
    void *context;
    app_core_nvs_result_t (*nvs_init)(void *context);
    app_error_code_t (*storage_mount)(void *context);
    app_error_code_t (*storage_recover)(void *context);
    app_error_code_t (*repository_init)(void *context);
    app_error_code_t (*auth_init)(void *context);
    app_error_code_t (*usb_init)(void *context);
    app_error_code_t (*executor_init)(void *context);
    app_error_code_t (*controls_init)(void *context);
    app_error_code_t (*random_fill)(void *context, uint8_t *output, size_t length);
    app_error_code_t (*password_create)(void *context,
                                        const char *password,
                                        size_t password_length,
                                        auth_password_record_t *out_record);
    app_error_code_t (*wifi_start)(void *context,
                                   const char *ssid,
                                   const char *passphrase);
    app_error_code_t (*http_start)(void *context,
                                   const web_server_config_t *configuration);
    app_error_code_t (*http_stop)(void *context);
    app_error_code_t (*wifi_stop)(void *context);
    app_error_code_t (*storage_unmount)(void *context);
    app_error_code_t (*set_indicator)(void *context,
                                      device_indicator_state_t indicator);
    void (*secure_zero)(void *context, void *memory, size_t length);
    void (*log_event)(void *context, const app_core_log_event_t *event);
} app_core_ops_t;

#endif
