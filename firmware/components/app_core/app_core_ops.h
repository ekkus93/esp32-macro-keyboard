#ifndef APP_CORE_OPS_H
#define APP_CORE_OPS_H

#include <stdbool.h>
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

/* Structured startup log event. The error fields mirror app_operation_result_t
 * causality (see the support component): the primary error is never overwritten
 * by a later cleanup error, the first cleanup error is preserved separately, and
 * cleanup_incomplete records whether teardown ran to completion. Per FIX1 SPEC
 * §3.2 the event also carries the affected subsystem (stage) and a stable
 * operation identifier when one exists (0 when none, as during startup). These
 * structured fields must never carry credentials, tokens, cookies, or macro
 * source; the ssid/ap_passphrase/web_password fields are populated only for the
 * development-only DEVELOPMENT_CREDENTIALS event. */
typedef struct {
    app_core_log_type_t type;
    const char *stage;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
    uint32_t operation_id;
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
    app_error_code_t (*password_create)(void *context, const char *password, size_t password_length,
                                        auth_password_record_t *out_record);
    app_error_code_t (*wifi_start)(void *context, const char *ssid, const char *passphrase);
    app_error_code_t (*http_start)(void *context, const web_server_config_t *configuration);
    app_error_code_t (*http_stop)(void *context);
    app_error_code_t (*wifi_stop)(void *context);
    app_error_code_t (*storage_unmount)(void *context);
    /* Reverse-teardown operations and residual-ownership queries used by the
     * exhaustive failure cleanup. http/wifi ownership can outlive a failed start
     * (a partial start still owns resources), so cleanup consults these queries in
     * addition to the tracked "started" flags. */
    app_error_code_t (*repository_deinit)(void *context);
    app_error_code_t (*auth_deinit)(void *context);
    app_error_code_t (*usb_deinit)(void *context);
    app_error_code_t (*executor_deinit)(void *context);
    app_error_code_t (*controls_deinit)(void *context);
    app_error_code_t (*nvs_deinit)(void *context);
    bool (*http_owns_resources)(void *context);
    bool (*wifi_owns_resources)(void *context);
    bool (*storage_owns_mount)(void *context);
    app_error_code_t (*set_indicator)(void *context, device_indicator_state_t indicator);
    void (*secure_zero)(void *context, void *memory, size_t length);
    void (*log_event)(void *context, const app_core_log_event_t *event);
} app_core_ops_t;

#endif
