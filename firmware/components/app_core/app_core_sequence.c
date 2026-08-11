#include "app_core_sequence.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_core_ops.h"
#include "app_error.h"
#include "app_operation_result.h"
#include "device_controls.h"
#include "device_settings_v2.h"
#include "provisioning_bootstrap.h"
#include "setup_contract_v2.h"
#include "web_server.h"

static bool operations_valid(const app_core_ops_t *operations) {
    return operations != NULL && operations->nvs_init != NULL &&
           operations->settings_init != NULL && operations->settings_read != NULL &&
           operations->bootstrap_derive != NULL && operations->setup_code_generate != NULL &&
           operations->storage_mount != NULL && operations->auth_init != NULL &&
           operations->usb_init != NULL && operations->executor_init != NULL &&
           operations->controls_init != NULL && operations->wifi_start != NULL &&
           operations->http_start != NULL && operations->http_stop != NULL &&
           operations->wifi_stop != NULL && operations->storage_unmount != NULL &&
           operations->auth_deinit != NULL && operations->usb_deinit != NULL &&
           operations->executor_deinit != NULL && operations->controls_deinit != NULL &&
           operations->settings_deinit != NULL && operations->nvs_deinit != NULL &&
           operations->http_owns_resources != NULL && operations->wifi_owns_resources != NULL &&
           operations->storage_owns_mount != NULL && operations->set_indicator != NULL &&
           operations->secure_zero != NULL && operations->log_event != NULL;
}

static void log_stage(const app_core_ops_t *operations, const char *stage,
                      app_error_code_t result) {
    const app_core_log_event_t event = {
        .type = APP_CORE_LOG_STAGE,
        .stage = stage,
        .primary_error = result,
        .cleanup_error = APP_ERROR_NONE,
        .cleanup_incomplete = false,
        .operation_id = 0U,
        .setup_code = NULL,
    };
    operations->log_event(operations->context, &event);
}

static void log_simple(const app_core_ops_t *operations, app_core_log_type_t type,
                       app_error_code_t primary, app_error_code_t cleanup,
                       bool cleanup_incomplete) {
    const app_core_log_event_t event = {
        .type = type,
        .stage = NULL,
        .primary_error = primary,
        .cleanup_error = cleanup,
        .cleanup_incomplete = cleanup_incomplete,
        .operation_id = 0U,
        .setup_code = NULL,
    };
    operations->log_event(operations->context, &event);
}

static void log_setup_code(const app_core_ops_t *operations, const char *setup_code) {
    (void)setup_code;
    const app_core_log_event_t event = {
        .type = APP_CORE_LOG_SETUP_CODE,
        .stage = NULL,
        .primary_error = APP_ERROR_NONE,
        .cleanup_error = APP_ERROR_NONE,
        .cleanup_incomplete = false,
        .operation_id = 0U,
        .setup_code = NULL,
    };
    operations->log_event(operations->context, &event);
}

app_error_code_t app_core_map_nvs_result(app_core_nvs_result_t result) {
    switch (result) {
    case APP_CORE_NVS_OK:
        return APP_ERROR_NONE;
    case APP_CORE_NVS_NO_FREE_PAGES:
    case APP_CORE_NVS_NEW_VERSION_FOUND:
        return APP_ERROR_STORAGE_CORRUPT;
    case APP_CORE_NVS_OTHER_FAILURE:
    default:
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }
}

typedef struct {
    bool nvs_initialized;
    bool settings_initialized;
    bool storage_mounted;
    bool auth_initialized;
    bool usb_initialized;
    bool executor_initialized;
    bool controls_initialized;
    bool wifi_started;
    bool http_started;
} app_core_owned_t;

typedef struct {
    app_v2_device_settings_t settings;
    provisioning_bootstrap_t bootstrap;
    char setup_code[APP_V2_SETUP_CODE_BUFFER_BYTES];
    web_server_config_t web;
} app_core_startup_secrets_t;

static void teardown_stage(const app_core_ops_t *operations, app_operation_result_t *result,
                           bool should_run, bool *owned_flag,
                           app_error_code_t (*teardown)(void *context)) {
    if (!should_run) {
        return;
    }
    const app_error_code_t cleanup = teardown(operations->context);
    app_operation_record_cleanup(result, cleanup);
    if (cleanup == APP_ERROR_NONE) {
        *owned_flag = false;
    }
}

static app_operation_result_t cleanup_after_failure(const app_core_ops_t *operations,
                                                    app_core_owned_t *owned,
                                                    app_error_code_t primary_error) {
    app_operation_result_t result = app_operation_success();
    app_operation_record_primary(&result, primary_error);

    teardown_stage(operations, &result,
                   owned->http_started || operations->http_owns_resources(operations->context),
                   &owned->http_started, operations->http_stop);
    teardown_stage(operations, &result,
                   owned->wifi_started || operations->wifi_owns_resources(operations->context),
                   &owned->wifi_started, operations->wifi_stop);
    teardown_stage(operations, &result, owned->controls_initialized, &owned->controls_initialized,
                   operations->controls_deinit);
    teardown_stage(operations, &result, owned->executor_initialized, &owned->executor_initialized,
                   operations->executor_deinit);
    teardown_stage(operations, &result, owned->usb_initialized, &owned->usb_initialized,
                   operations->usb_deinit);
    teardown_stage(operations, &result, owned->auth_initialized, &owned->auth_initialized,
                   operations->auth_deinit);
    teardown_stage(operations, &result,
                   owned->storage_mounted || operations->storage_owns_mount(operations->context),
                   &owned->storage_mounted, operations->storage_unmount);
    teardown_stage(operations, &result, owned->settings_initialized, &owned->settings_initialized,
                   operations->settings_deinit);
    teardown_stage(operations, &result, owned->nvs_initialized, &owned->nvs_initialized,
                   operations->nvs_deinit);

    const app_error_code_t indicator =
        operations->set_indicator(operations->context, DEVICE_INDICATOR_FATAL);
    app_operation_record_cleanup(&result, indicator);
    return result;
}

static app_error_code_t finish_failure(const app_core_ops_t *operations, app_core_owned_t *owned,
                                       app_error_code_t primary_error) {
    const app_operation_result_t result = cleanup_after_failure(operations, owned, primary_error);
    if (result.cleanup_incomplete) {
        log_simple(operations, APP_CORE_LOG_CLEANUP_FAILED, result.primary_error,
                   result.cleanup_error, result.cleanup_incomplete);
    }
    return result.primary_error;
}

static void clear_startup_secrets(const app_core_ops_t *operations,
                                  app_core_startup_secrets_t *secrets) {
    operations->secure_zero(operations->context, secrets, sizeof(*secrets));
}

static app_error_code_t fail_with_secrets(const app_core_ops_t *operations, app_core_owned_t *owned,
                                          app_core_startup_secrets_t *secrets,
                                          app_error_code_t primary_error) {
    clear_startup_secrets(operations, secrets);
    return finish_failure(operations, owned, primary_error);
}

static void configure_setup_server(const app_v2_device_settings_t *settings, const char *setup_code,
                                   web_server_config_t *configuration) {
    *configuration = (web_server_config_t){
        .mode = WEB_SERVER_MODE_SETUP,
        .login_enabled = false,
    };
    memcpy(configuration->setup_device_name, settings->device_name,
           sizeof(configuration->setup_device_name));
    memcpy(configuration->setup_code, setup_code, sizeof(configuration->setup_code));
}

static void configure_normal_server(const app_v2_device_settings_t *settings,
                                    web_server_config_t *configuration) {
    *configuration = (web_server_config_t){
        .mode = WEB_SERVER_MODE_NORMAL,
        .login_enabled = true,
        .require_physical_confirmation = settings->require_serial_confirmation,
    };
    configuration->password_record.iterations = settings->password_iterations;
    memcpy(configuration->password_record.salt, settings->password_salt,
           sizeof(configuration->password_record.salt));
    memcpy(configuration->password_record.hash, settings->password_verifier,
           sizeof(configuration->password_record.hash));
}

static app_error_code_t start_network(const app_core_ops_t *operations, app_core_owned_t *owned,
                                      const app_core_wifi_configuration_t *wifi,
                                      const web_server_config_t *web) {
    app_error_code_t result = operations->wifi_start(operations->context, wifi);
    log_stage(operations, "wifi", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->wifi_started = true;

    result = operations->http_start(operations->context, web);
    log_stage(operations, "http", result);
    if (result == APP_ERROR_NONE) {
        owned->http_started = true;
    }
    return result;
}

static app_error_code_t start_normal_mode(const app_core_ops_t *operations, app_core_owned_t *owned,
                                          app_core_startup_secrets_t *secrets) {
    app_error_code_t result = operations->auth_init(operations->context);
    log_stage(operations, "authentication", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->auth_initialized = true;

    result = operations->usb_init(operations->context);
    log_stage(operations, "usb", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->usb_initialized = true;

    result = operations->executor_init(operations->context);
    log_stage(operations, "executor", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->executor_initialized = true;

    result = operations->controls_init(operations->context);
    log_stage(operations, "controls", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->controls_initialized = true;

    configure_normal_server(&secrets->settings, &secrets->web);
    const app_core_wifi_configuration_t wifi = {
        .ap_ssid = secrets->settings.ap_ssid,
        .ap_passphrase = secrets->settings.ap_passphrase,
        .station_configured = secrets->settings.station_configured,
        .station_ssid = secrets->settings.station_ssid,
        .station_passphrase = secrets->settings.station_passphrase,
    };
    return start_network(operations, owned, &wifi, &secrets->web);
}

static app_error_code_t start_setup_mode(const app_core_ops_t *operations, app_core_owned_t *owned,
                                         app_core_startup_secrets_t *secrets) {
    log_simple(operations, APP_CORE_LOG_PROVISIONING_REQUIRED, APP_ERROR_AUTH_REQUIRED,
               APP_ERROR_NONE, false);

    app_error_code_t result = operations->auth_init(operations->context);
    log_stage(operations, "authentication", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->auth_initialized = true;

    result = operations->bootstrap_derive(operations->context, &secrets->bootstrap);
    log_stage(operations, "setup_bootstrap", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    result = operations->setup_code_generate(operations->context, secrets->setup_code);
    log_stage(operations, "setup_code", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    log_setup_code(operations, secrets->setup_code);

    configure_setup_server(&secrets->settings, secrets->setup_code, &secrets->web);
    const app_core_wifi_configuration_t wifi = {
        .ap_ssid = secrets->bootstrap.ap_ssid,
        .ap_passphrase = secrets->bootstrap.ap_passphrase,
        .station_configured = false,
        .station_ssid = NULL,
        .station_passphrase = NULL,
    };
    return start_network(operations, owned, &wifi, &secrets->web);
}

app_error_code_t app_core_sequence_start(const app_core_ops_t *operations) {
    if (!operations_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_core_owned_t owned = {0};
    app_core_startup_secrets_t secrets = {0};

    app_error_code_t result =
        operations->set_indicator(operations->context, DEVICE_INDICATOR_BOOTING);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }

    result = app_core_map_nvs_result(operations->nvs_init(operations->context));
    log_stage(operations, "nvs", result);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }
    owned.nvs_initialized = true;

    result = operations->settings_init(operations->context);
    log_stage(operations, "settings_init", result);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }
    owned.settings_initialized = true;

    result = operations->settings_read(operations->context, &secrets.settings);
    log_stage(operations, "settings_read", result);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }

    result = operations->storage_mount(operations->context);
    log_stage(operations, "storage_mount", result);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }
    owned.storage_mounted = true;

    result = secrets.settings.provisioned ? start_normal_mode(operations, &owned, &secrets)
                                          : start_setup_mode(operations, &owned, &secrets);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }

    clear_startup_secrets(operations, &secrets);
    result = operations->set_indicator(operations->context, DEVICE_INDICATOR_READY);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    return APP_ERROR_NONE;
}
