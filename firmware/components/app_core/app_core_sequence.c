#include "app_core_sequence.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_core_ops.h"
#include "app_error.h"
#include "app_operation_result.h"
#include "device_controls.h"
#include "web_server.h"

static bool operations_valid(const app_core_ops_t *operations) {
    return operations != NULL && operations->nvs_init != NULL &&
           operations->storage_mount != NULL && operations->storage_recover != NULL &&
           operations->repository_init != NULL && operations->auth_init != NULL &&
           operations->usb_init != NULL && operations->executor_init != NULL &&
           operations->controls_init != NULL && operations->random_fill != NULL &&
           operations->password_create != NULL && operations->wifi_start != NULL &&
           operations->http_start != NULL && operations->http_stop != NULL &&
           operations->wifi_stop != NULL && operations->storage_unmount != NULL &&
           operations->repository_deinit != NULL && operations->auth_deinit != NULL &&
           operations->usb_deinit != NULL && operations->executor_deinit != NULL &&
           operations->controls_deinit != NULL && operations->nvs_deinit != NULL &&
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
        .ssid = NULL,
        .ap_passphrase = NULL,
        .web_password = NULL,
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
        .ssid = NULL,
        .ap_passphrase = NULL,
        .web_password = NULL,
    };
    operations->log_event(operations->context, &event);
}

static void log_credentials(const app_core_ops_t *operations, const char *ssid,
                            const char *ap_passphrase, const char *web_password) {
    const app_core_log_event_t event = {
        .type = APP_CORE_LOG_DEVELOPMENT_CREDENTIALS,
        .stage = NULL,
        .primary_error = APP_ERROR_NONE,
        .cleanup_error = APP_ERROR_NONE,
        .cleanup_incomplete = false,
        .operation_id = 0U,
        .ssid = ssid,
        .ap_passphrase = ap_passphrase,
        .web_password = web_password,
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

/* Tracks which startup stages actually completed so failure cleanup reverses
 * exactly what was acquired, rather than inferring ownership from a final return
 * code (a partially initialized subsystem must still be torn down). */
typedef struct {
    bool nvs_initialized;
    bool storage_mounted;
    bool repository_initialized;
    bool auth_initialized;
    bool usb_initialized;
    bool executor_initialized;
    bool controls_initialized;
    bool wifi_started;
    bool http_started;
} app_core_owned_t;

/* Attempt one reverse-teardown step. When should_run is set, invoke the teardown,
 * record any cleanup error, and clear the owned flag only if the teardown actually
 * succeeded (so residual ownership stays visible when it fails). */
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

/* Reverse every completed (or still-owned) startup stage in the opposite order it
 * was acquired. The primary error is preserved, the first cleanup error is
 * captured separately, and cleanup_incomplete records whether any teardown step
 * failed. Every remaining stage is attempted even after an earlier one fails.
 * http/wifi may retain resources after a failed start, so their teardown also
 * consults the residual-ownership queries. */
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
    teardown_stage(operations, &result, owned->repository_initialized,
                   &owned->repository_initialized, operations->repository_deinit);
    teardown_stage(operations, &result,
                   owned->storage_mounted || operations->storage_owns_mount(operations->context),
                   &owned->storage_mounted, operations->storage_unmount);
    teardown_stage(operations, &result, owned->nvs_initialized, &owned->nvs_initialized,
                   operations->nvs_deinit);

    const app_error_code_t indicator =
        operations->set_indicator(operations->context, DEVICE_INDICATOR_FATAL);
    app_operation_record_cleanup(&result, indicator);
    return result;
}

/* Run exhaustive cleanup for a failed startup, emit a structured cleanup event
 * when any teardown step failed, and return the preserved primary error. */
static app_error_code_t finish_failure(const app_core_ops_t *operations, app_core_owned_t *owned,
                                       app_error_code_t primary_error) {
    const app_operation_result_t result = cleanup_after_failure(operations, owned, primary_error);
    if (result.cleanup_incomplete) {
        log_simple(operations, APP_CORE_LOG_CLEANUP_FAILED, result.primary_error,
                   result.cleanup_error, result.cleanup_incomplete);
    }
    return result.primary_error;
}

static app_error_code_t generate_password(const app_core_ops_t *operations, char *output,
                                          size_t output_size) {
    static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    if (output == NULL || output_size < APP_CORE_DEVELOPMENT_PASSWORD_BYTES + 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t random_bytes[APP_CORE_DEVELOPMENT_PASSWORD_BYTES] = {0};
    memset(output, 0, output_size);
    const app_error_code_t result =
        operations->random_fill(operations->context, random_bytes, sizeof(random_bytes));
    if (result == APP_ERROR_NONE) {
        for (size_t index = 0U; index < sizeof(random_bytes); ++index) {
            output[index] = alphabet[random_bytes[index] % (sizeof(alphabet) - 1U)];
        }
        output[APP_CORE_DEVELOPMENT_PASSWORD_BYTES] = '\0';
    }
    operations->secure_zero(operations->context, random_bytes, sizeof(random_bytes));
    if (result != APP_ERROR_NONE) {
        operations->secure_zero(operations->context, output, output_size);
    }
    return result;
}

static app_error_code_t generate_distinct_credentials(const app_core_ops_t *operations,
                                                      char *ap_passphrase, size_t ap_size,
                                                      char *web_password, size_t web_size) {
    app_error_code_t result = generate_password(operations, ap_passphrase, ap_size);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    for (size_t attempt = 0U; attempt < APP_CORE_CREDENTIAL_RETRY_LIMIT; ++attempt) {
        result = generate_password(operations, web_password, web_size);
        if (result != APP_ERROR_NONE) {
            return result;
        }
        if (strcmp(ap_passphrase, web_password) != 0) {
            return APP_ERROR_NONE;
        }
        operations->secure_zero(operations->context, web_password, web_size);
    }
    return APP_ERROR_INTERNAL;
}

static void clear_credentials(const app_core_ops_t *operations, char *ap_passphrase, size_t ap_size,
                              char *web_password, size_t web_size,
                              web_server_config_t *web_configuration) {
    operations->secure_zero(operations->context, ap_passphrase, ap_size);
    operations->secure_zero(operations->context, web_password, web_size);
    operations->secure_zero(operations->context, web_configuration, sizeof(*web_configuration));
}

app_error_code_t app_core_sequence_start(const app_core_ops_t *operations,
                                         const app_core_policy_t *policy) {
    if (!operations_valid(operations) || policy == NULL ||
        (policy->development_provisioning_enabled &&
         (policy->development_ssid == NULL || policy->development_ssid[0] == '\0'))) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_core_owned_t owned = {0};
    bool storage_degraded = false;

    app_error_code_t result =
        operations->set_indicator(operations->context, DEVICE_INDICATOR_BOOTING);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }

    result = app_core_map_nvs_result(operations->nvs_init(operations->context));
    log_stage(operations, "nvs", result);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    owned.nvs_initialized = true;

    result = operations->storage_mount(operations->context);
    log_stage(operations, "storage_mount", result);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    owned.storage_mounted = true;

    result = operations->storage_recover(operations->context);
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        storage_degraded = true;
        log_simple(operations, APP_CORE_LOG_STORAGE_DEGRADED, APP_ERROR_STORAGE_CORRUPT,
                   APP_ERROR_NONE, false);
    } else {
        log_stage(operations, "storage_recovery", result);
        if (result != APP_ERROR_NONE) {
            return finish_failure(operations, &owned, result);
        }
    }

    result = operations->repository_init(operations->context);
    log_stage(operations, "storage_repository", result);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    owned.repository_initialized = true;

    result = operations->auth_init(operations->context);
    log_stage(operations, "authentication", result);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    owned.auth_initialized = true;

    /* Provisioning is validated after authentication and BEFORE any
     * normal-operation subsystem is initialized. A device that is not provisioned
     * for production must stop cleanly here rather than initialize USB, the
     * executor, and controls and only then refuse to run (FIX1 §4.5). */
    if (!policy->development_provisioning_enabled) {
        log_simple(operations, APP_CORE_LOG_PROVISIONING_REQUIRED, APP_ERROR_AUTH_REQUIRED,
                   APP_ERROR_NONE, false);
        return finish_failure(operations, &owned, APP_ERROR_AUTH_REQUIRED);
    }

    result = operations->usb_init(operations->context);
    log_stage(operations, "usb", result);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    owned.usb_initialized = true;

    result = operations->executor_init(operations->context);
    log_stage(operations, "executor", result);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    owned.executor_initialized = true;

    result = operations->controls_init(operations->context);
    log_stage(operations, "controls", result);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    owned.controls_initialized = true;

    char ap_passphrase[APP_CORE_DEVELOPMENT_PASSWORD_BYTES + 1U] = {0};
    char web_password[APP_CORE_DEVELOPMENT_PASSWORD_BYTES + 1U] = {0};
    web_server_config_t web_configuration = {0};

    result = generate_distinct_credentials(operations, ap_passphrase, sizeof(ap_passphrase),
                                           web_password, sizeof(web_password));
    log_stage(operations, "credential_generation", result);
    if (result == APP_ERROR_NONE) {
        result = operations->password_create(operations->context, web_password,
                                             APP_CORE_DEVELOPMENT_PASSWORD_BYTES,
                                             &web_configuration.password_record);
        log_stage(operations, "password_record", result);
    }
    if (result != APP_ERROR_NONE) {
        clear_credentials(operations, ap_passphrase, sizeof(ap_passphrase), web_password,
                          sizeof(web_password), &web_configuration);
        return finish_failure(operations, &owned, result);
    }
    web_configuration.login_enabled = true;
    log_credentials(operations, policy->development_ssid, ap_passphrase, web_password);

    result = operations->wifi_start(operations->context, policy->development_ssid, ap_passphrase);
    log_stage(operations, "wifi", result);
    if (result == APP_ERROR_NONE) {
        owned.wifi_started = true;
        result = operations->http_start(operations->context, &web_configuration);
        log_stage(operations, "http", result);
        if (result == APP_ERROR_NONE) {
            owned.http_started = true;
        }
    }

    clear_credentials(operations, ap_passphrase, sizeof(ap_passphrase), web_password,
                      sizeof(web_password), &web_configuration);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }

    result = operations->set_indicator(
        operations->context, storage_degraded ? DEVICE_INDICATOR_DEGRADED : DEVICE_INDICATOR_READY);
    if (result != APP_ERROR_NONE) {
        return finish_failure(operations, &owned, result);
    }
    return APP_ERROR_NONE;
}
