#include "app_core_sequence.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_core_ops.h"
#include "app_error.h"
#include "app_operation_result.h"
#include "device_controls.h"
#include "provisioning.h"
#include "provisioning_bootstrap.h"
#include "web_server.h"

static bool operations_valid(const app_core_ops_t *operations) {
    return operations != NULL && operations->nvs_init != NULL &&
           operations->provisioning_init != NULL &&
           operations->provisioning_load != NULL &&
           operations->bootstrap_derive != NULL &&
           operations->storage_mount != NULL &&
           operations->storage_recover != NULL &&
           operations->repository_init != NULL &&
           operations->auth_init != NULL && operations->usb_init != NULL &&
           operations->executor_init != NULL &&
           operations->controls_init != NULL &&
           operations->wifi_start != NULL && operations->http_start != NULL &&
           operations->http_stop != NULL && operations->wifi_stop != NULL &&
           operations->storage_unmount != NULL &&
           operations->repository_deinit != NULL &&
           operations->auth_deinit != NULL &&
           operations->usb_deinit != NULL &&
           operations->executor_deinit != NULL &&
           operations->controls_deinit != NULL &&
           operations->provisioning_deinit != NULL &&
           operations->nvs_deinit != NULL &&
           operations->http_owns_resources != NULL &&
           operations->wifi_owns_resources != NULL &&
           operations->storage_owns_mount != NULL &&
           operations->provisioning_owns_resources != NULL &&
           operations->set_indicator != NULL &&
           operations->secure_zero != NULL && operations->log_event != NULL;
}

static void log_stage(const app_core_ops_t *operations,
                      const char *stage,
                      app_error_code_t result) {
    const app_core_log_event_t event = {
        .type = APP_CORE_LOG_STAGE,
        .stage = stage,
        .primary_error = result,
        .cleanup_error = APP_ERROR_NONE,
        .cleanup_incomplete = false,
        .operation_id = 0U,
        .ssid = NULL,
        .ap_passphrase = NULL,
        .setup_code = NULL,
    };
    operations->log_event(operations->context, &event);
}

static void log_simple(const app_core_ops_t *operations,
                       app_core_log_type_t type,
                       app_error_code_t primary,
                       app_error_code_t cleanup,
                       bool cleanup_incomplete) {
    const app_core_log_event_t event = {
        .type = type,
        .stage = NULL,
        .primary_error = primary,
        .cleanup_error = cleanup,
        .cleanup_incomplete = cleanup_incomplete,
        .operation_id = 0U,
        .ssid = NULL,
        .ap_passphrase = NULL,
        .setup_code = NULL,
    };
    operations->log_event(operations->context, &event);
}

static void log_manufacturing_credentials(
    const app_core_ops_t *operations,
    const provisioning_bootstrap_t *bootstrap) {
    const app_core_log_event_t event = {
        .type = APP_CORE_LOG_MANUFACTURING_CREDENTIALS,
        .stage = NULL,
        .primary_error = APP_ERROR_NONE,
        .cleanup_error = APP_ERROR_NONE,
        .cleanup_incomplete = false,
        .operation_id = 0U,
        .ssid = bootstrap->ap_ssid,
        .ap_passphrase = bootstrap->ap_passphrase,
        .setup_code = bootstrap->setup_code,
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
    bool provisioning_initialized;
    bool storage_mounted;
    bool repository_initialized;
    bool auth_initialized;
    bool usb_initialized;
    bool executor_initialized;
    bool controls_initialized;
    bool wifi_started;
    bool http_started;
} app_core_owned_t;

typedef struct {
    provisioning_config_t provisioning;
    provisioning_bootstrap_t bootstrap;
    web_server_config_t web;
} app_core_startup_secrets_t;

typedef struct {
    const char *ssid;
    const char *passphrase;
    const web_server_config_t *web_configuration;
} app_core_network_start_t;

static void teardown_stage(const app_core_ops_t *operations,
                           app_operation_result_t *result,
                           bool should_run,
                           bool *owned_flag,
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

static app_operation_result_t cleanup_after_failure(
    const app_core_ops_t *operations,
    app_core_owned_t *owned,
    app_error_code_t primary_error) {
    app_operation_result_t result = app_operation_success();
    app_operation_record_primary(&result, primary_error);

    teardown_stage(operations,
                   &result,
                   owned->http_started ||
                       operations->http_owns_resources(operations->context),
                   &owned->http_started,
                   operations->http_stop);
    teardown_stage(operations,
                   &result,
                   owned->wifi_started ||
                       operations->wifi_owns_resources(operations->context),
                   &owned->wifi_started,
                   operations->wifi_stop);
    teardown_stage(operations,
                   &result,
                   owned->controls_initialized,
                   &owned->controls_initialized,
                   operations->controls_deinit);
    teardown_stage(operations,
                   &result,
                   owned->executor_initialized,
                   &owned->executor_initialized,
                   operations->executor_deinit);
    teardown_stage(operations,
                   &result,
                   owned->usb_initialized,
                   &owned->usb_initialized,
                   operations->usb_deinit);
    teardown_stage(operations,
                   &result,
                   owned->auth_initialized,
                   &owned->auth_initialized,
                   operations->auth_deinit);
    teardown_stage(operations,
                   &result,
                   owned->repository_initialized,
                   &owned->repository_initialized,
                   operations->repository_deinit);
    teardown_stage(operations,
                   &result,
                   owned->storage_mounted ||
                       operations->storage_owns_mount(operations->context),
                   &owned->storage_mounted,
                   operations->storage_unmount);
    teardown_stage(operations,
                   &result,
                   owned->provisioning_initialized ||
                       operations->provisioning_owns_resources(
                           operations->context),
                   &owned->provisioning_initialized,
                   operations->provisioning_deinit);
    teardown_stage(operations,
                   &result,
                   owned->nvs_initialized,
                   &owned->nvs_initialized,
                   operations->nvs_deinit);

    const app_error_code_t indicator = operations->set_indicator(
        operations->context, DEVICE_INDICATOR_FATAL);
    app_operation_record_cleanup(&result, indicator);
    return result;
}

static app_error_code_t finish_failure(const app_core_ops_t *operations,
                                       app_core_owned_t *owned,
                                       app_error_code_t primary_error) {
    const app_operation_result_t result =
        cleanup_after_failure(operations, owned, primary_error);
    if (result.cleanup_incomplete) {
        log_simple(operations,
                   APP_CORE_LOG_CLEANUP_FAILED,
                   result.primary_error,
                   result.cleanup_error,
                   result.cleanup_incomplete);
    }
    return result.primary_error;
}

static void clear_startup_secrets(
    const app_core_ops_t *operations,
    app_core_startup_secrets_t *secrets) {
    operations->secure_zero(
        operations->context, secrets, sizeof(*secrets));
}

static app_error_code_t fail_with_secrets(
    const app_core_ops_t *operations,
    app_core_owned_t *owned,
    app_core_startup_secrets_t *secrets,
    app_error_code_t primary_error) {
    clear_startup_secrets(operations, secrets);
    return finish_failure(operations, owned, primary_error);
}

static void configure_setup_server(
    const app_core_policy_t *policy,
    const provisioning_bootstrap_t *bootstrap,
    web_server_config_t *configuration) {
    *configuration = (web_server_config_t){
        .mode = WEB_SERVER_MODE_SETUP,
        .login_enabled = false,
        .setup_physical_confirmation_required = true,
        .setup_manufacturing_bypass =
            policy->manufacturing_provisioning_enabled,
    };
    memcpy(configuration->setup_device_id,
           bootstrap->device_id,
           sizeof(configuration->setup_device_id));
    memcpy(configuration->setup_ap_ssid,
           bootstrap->ap_ssid,
           sizeof(configuration->setup_ap_ssid));
    memcpy(configuration->setup_code,
           bootstrap->setup_code,
           sizeof(configuration->setup_code));
}

static void configure_normal_server(
    const provisioning_config_t *provisioning,
    web_server_config_t *configuration) {
    *configuration = (web_server_config_t){
        .mode = WEB_SERVER_MODE_NORMAL,
        .login_enabled = true,
        .password_record = provisioning->password_record,
    };
}

static app_error_code_t start_network(
    const app_core_ops_t *operations,
    app_core_owned_t *owned,
    const app_core_network_start_t *network) {
    app_error_code_t result = operations->wifi_start(
        operations->context, network->ssid, network->passphrase);
    log_stage(operations, "wifi", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->wifi_started = true;

    result = operations->http_start(
        operations->context, network->web_configuration);
    log_stage(operations, "http", result);
    if (result == APP_ERROR_NONE) {
        owned->http_started = true;
    }
    return result;
}

static app_error_code_t start_normal_mode(
    const app_core_ops_t *operations,
    app_core_owned_t *owned,
    app_core_startup_secrets_t *secrets,
    bool *out_storage_degraded) {
    app_error_code_t result =
        operations->storage_recover(operations->context);
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        *out_storage_degraded = true;
        log_simple(operations,
                   APP_CORE_LOG_STORAGE_DEGRADED,
                   APP_ERROR_STORAGE_CORRUPT,
                   APP_ERROR_NONE,
                   false);
    } else {
        log_stage(operations, "storage_recovery", result);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }

    result = operations->repository_init(operations->context);
    log_stage(operations, "storage_repository", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->repository_initialized = true;

    result = operations->auth_init(operations->context);
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

    configure_normal_server(&secrets->provisioning, &secrets->web);
    const app_core_network_start_t network = {
        .ssid = secrets->provisioning.ap_ssid,
        .passphrase = secrets->provisioning.ap_passphrase,
        .web_configuration = &secrets->web,
    };
    return start_network(operations, owned, &network);
}

static app_error_code_t start_setup_mode(
    const app_core_ops_t *operations,
    const app_core_policy_t *policy,
    app_core_owned_t *owned,
    app_core_startup_secrets_t *secrets) {
    log_simple(operations,
               APP_CORE_LOG_PROVISIONING_REQUIRED,
               APP_ERROR_AUTH_REQUIRED,
               APP_ERROR_NONE,
               false);

    app_error_code_t result = operations->auth_init(operations->context);
    log_stage(operations, "authentication", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->auth_initialized = true;

    result = operations->controls_init(operations->context);
    log_stage(operations, "controls", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    owned->controls_initialized = true;

    result = operations->bootstrap_derive(
        operations->context, &secrets->bootstrap);
    log_stage(operations, "setup_bootstrap", result);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    configure_setup_server(policy, &secrets->bootstrap, &secrets->web);
    if (policy->manufacturing_provisioning_enabled) {
        log_manufacturing_credentials(operations, &secrets->bootstrap);
    }
    const app_core_network_start_t network = {
        .ssid = secrets->bootstrap.ap_ssid,
        .passphrase = secrets->bootstrap.ap_passphrase,
        .web_configuration = &secrets->web,
    };
    return start_network(operations, owned, &network);
}

app_error_code_t app_core_sequence_start(const app_core_ops_t *operations,
                                         const app_core_policy_t *policy) {
    if (!operations_valid(operations) || policy == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_core_owned_t owned = {0};
    app_core_startup_secrets_t secrets = {0};
    bool storage_degraded = false;

    app_error_code_t result = operations->set_indicator(
        operations->context, DEVICE_INDICATOR_BOOTING);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }

    result = app_core_map_nvs_result(
        operations->nvs_init(operations->context));
    log_stage(operations, "nvs", result);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }
    owned.nvs_initialized = true;

    result = operations->provisioning_init(operations->context);
    log_stage(operations, "provisioning_init", result);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }
    owned.provisioning_initialized = true;

    result = operations->provisioning_load(
        operations->context, &secrets.provisioning);
    log_stage(operations, "provisioning_load", result);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }

    result = operations->storage_mount(operations->context);
    log_stage(operations, "storage_mount", result);
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }
    owned.storage_mounted = true;

    if (secrets.provisioning.provisioned) {
        result = start_normal_mode(
            operations, &owned, &secrets, &storage_degraded);
    } else {
        result = start_setup_mode(operations, policy, &owned, &secrets);
    }
    if (result != APP_ERROR_NONE) {
        return fail_with_secrets(operations, &owned, &secrets, result);
    }

    clear_startup_secrets(operations, &secrets);
    result = operations->set_indicator(
        operations->context,
        storage_degraded ? DEVICE_INDICATOR_DEGRADED
                         : DEVICE_INDICATOR_READY);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    return APP_ERROR_NONE;
}
