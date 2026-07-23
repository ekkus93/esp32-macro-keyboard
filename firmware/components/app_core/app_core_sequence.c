#include "app_core_sequence.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static bool operations_valid(const app_core_ops_t *operations)
{
    return operations != NULL && operations->nvs_init != NULL &&
           operations->storage_mount != NULL && operations->storage_recover != NULL &&
           operations->repository_init != NULL && operations->auth_init != NULL &&
           operations->usb_init != NULL && operations->executor_init != NULL &&
           operations->controls_init != NULL && operations->random_fill != NULL &&
           operations->password_create != NULL && operations->wifi_start != NULL &&
           operations->http_start != NULL && operations->http_stop != NULL &&
           operations->wifi_stop != NULL && operations->storage_unmount != NULL &&
           operations->set_indicator != NULL && operations->secure_zero != NULL &&
           operations->log_event != NULL;
}

static void log_stage(const app_core_ops_t *operations,
                      const char *stage,
                      app_error_code_t result)
{
    const app_core_log_event_t event = {
        .type = APP_CORE_LOG_STAGE,
        .stage = stage,
        .primary_error = result,
        .secondary_error = APP_ERROR_NONE,
        .ssid = NULL,
        .ap_passphrase = NULL,
        .web_password = NULL,
    };
    operations->log_event(operations->context, &event);
}

static void log_simple(const app_core_ops_t *operations,
                       app_core_log_type_t type,
                       app_error_code_t primary,
                       app_error_code_t secondary)
{
    const app_core_log_event_t event = {
        .type = type,
        .stage = NULL,
        .primary_error = primary,
        .secondary_error = secondary,
        .ssid = NULL,
        .ap_passphrase = NULL,
        .web_password = NULL,
    };
    operations->log_event(operations->context, &event);
}

static void log_credentials(const app_core_ops_t *operations,
                            const char *ssid,
                            const char *ap_passphrase,
                            const char *web_password)
{
    const app_core_log_event_t event = {
        .type = APP_CORE_LOG_DEVELOPMENT_CREDENTIALS,
        .stage = NULL,
        .primary_error = APP_ERROR_NONE,
        .secondary_error = APP_ERROR_NONE,
        .ssid = ssid,
        .ap_passphrase = ap_passphrase,
        .web_password = web_password,
    };
    operations->log_event(operations->context, &event);
}

app_error_code_t app_core_map_nvs_result(app_core_nvs_result_t result)
{
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

static app_error_code_t cleanup_after_failure(const app_core_ops_t *operations,
                                              bool web_started,
                                              bool wifi_started,
                                              bool storage_mounted,
                                              app_error_code_t original)
{
    app_error_code_t cleanup_error = APP_ERROR_NONE;
    if (web_started) {
        const app_error_code_t result = operations->http_stop(operations->context);
        if (result != APP_ERROR_NONE && cleanup_error == APP_ERROR_NONE) {
            cleanup_error = result;
        }
    }
    if (wifi_started) {
        const app_error_code_t result = operations->wifi_stop(operations->context);
        if (result != APP_ERROR_NONE && cleanup_error == APP_ERROR_NONE) {
            cleanup_error = result;
        }
    }
    if (storage_mounted) {
        const app_error_code_t result = operations->storage_unmount(operations->context);
        if (result != APP_ERROR_NONE && cleanup_error == APP_ERROR_NONE) {
            cleanup_error = result;
        }
    }
    const app_error_code_t indicator_result =
        operations->set_indicator(operations->context, DEVICE_INDICATOR_FATAL);
    if (indicator_result != APP_ERROR_NONE && cleanup_error == APP_ERROR_NONE) {
        cleanup_error = indicator_result;
    }
    if (cleanup_error != APP_ERROR_NONE) {
        log_simple(operations, APP_CORE_LOG_CLEANUP_FAILED, original, cleanup_error);
    }
    return original;
}

static app_error_code_t generate_password(const app_core_ops_t *operations,
                                          char *output,
                                          size_t output_size)
{
    static const char alphabet[] =
        "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    if (output == NULL || output_size < APP_CORE_DEVELOPMENT_PASSWORD_BYTES + 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t random_bytes[APP_CORE_DEVELOPMENT_PASSWORD_BYTES] = {0};
    memset(output, 0, output_size);
    const app_error_code_t result = operations->random_fill(
        operations->context, random_bytes, sizeof(random_bytes));
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
                                                      char *ap_passphrase,
                                                      size_t ap_size,
                                                      char *web_password,
                                                      size_t web_size)
{
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

static void clear_credentials(const app_core_ops_t *operations,
                              char *ap_passphrase,
                              size_t ap_size,
                              char *web_password,
                              size_t web_size,
                              web_server_config_t *web_configuration)
{
    operations->secure_zero(operations->context, ap_passphrase, ap_size);
    operations->secure_zero(operations->context, web_password, web_size);
    operations->secure_zero(operations->context,
                            web_configuration,
                            sizeof(*web_configuration));
}

app_error_code_t app_core_sequence_start(const app_core_ops_t *operations,
                                         const app_core_policy_t *policy)
{
    if (!operations_valid(operations) || policy == NULL ||
        (policy->development_provisioning_enabled &&
         (policy->development_ssid == NULL || policy->development_ssid[0] == '\0'))) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    bool storage_mounted = false;
    bool wifi_started = false;
    bool web_started = false;
    bool storage_degraded = false;

    app_error_code_t result =
        operations->set_indicator(operations->context, DEVICE_INDICATOR_BOOTING);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }

    result = app_core_map_nvs_result(operations->nvs_init(operations->context));
    log_stage(operations, "nvs", result);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }

    result = operations->storage_mount(operations->context);
    log_stage(operations, "storage_mount", result);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }
    storage_mounted = true;

    result = operations->storage_recover(operations->context);
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        storage_degraded = true;
        log_simple(operations,
                   APP_CORE_LOG_STORAGE_DEGRADED,
                   APP_ERROR_STORAGE_CORRUPT,
                   APP_ERROR_NONE);
    } else {
        log_stage(operations, "storage_recovery", result);
        if (result != APP_ERROR_NONE) {
            return cleanup_after_failure(operations,
                                         web_started,
                                         wifi_started,
                                         storage_mounted,
                                         result);
        }
    }

    result = operations->repository_init(operations->context);
    log_stage(operations, "storage_repository", result);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }

    result = operations->auth_init(operations->context);
    log_stage(operations, "authentication", result);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }

    result = operations->usb_init(operations->context);
    log_stage(operations, "usb", result);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }

    result = operations->executor_init(operations->context);
    log_stage(operations, "executor", result);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }

    result = operations->controls_init(operations->context);
    log_stage(operations, "controls", result);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }

    if (!policy->development_provisioning_enabled) {
        log_simple(operations,
                   APP_CORE_LOG_PROVISIONING_REQUIRED,
                   APP_ERROR_AUTH_REQUIRED,
                   APP_ERROR_NONE);
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     APP_ERROR_AUTH_REQUIRED);
    }

    char ap_passphrase[APP_CORE_DEVELOPMENT_PASSWORD_BYTES + 1U] = {0};
    char web_password[APP_CORE_DEVELOPMENT_PASSWORD_BYTES + 1U] = {0};
    web_server_config_t web_configuration = {0};

    result = generate_distinct_credentials(operations,
                                           ap_passphrase,
                                           sizeof(ap_passphrase),
                                           web_password,
                                           sizeof(web_password));
    log_stage(operations, "credential_generation", result);
    if (result == APP_ERROR_NONE) {
        result = operations->password_create(operations->context,
                                             web_password,
                                             APP_CORE_DEVELOPMENT_PASSWORD_BYTES,
                                             &web_configuration.password_record);
        log_stage(operations, "password_record", result);
    }
    if (result != APP_ERROR_NONE) {
        clear_credentials(operations,
                          ap_passphrase,
                          sizeof(ap_passphrase),
                          web_password,
                          sizeof(web_password),
                          &web_configuration);
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }
    web_configuration.login_enabled = true;
    log_credentials(operations,
                    policy->development_ssid,
                    ap_passphrase,
                    web_password);

    result = operations->wifi_start(operations->context,
                                    policy->development_ssid,
                                    ap_passphrase);
    log_stage(operations, "wifi", result);
    if (result == APP_ERROR_NONE) {
        wifi_started = true;
        result = operations->http_start(operations->context, &web_configuration);
        log_stage(operations, "http", result);
        if (result == APP_ERROR_NONE) {
            web_started = true;
        }
    }

    clear_credentials(operations,
                      ap_passphrase,
                      sizeof(ap_passphrase),
                      web_password,
                      sizeof(web_password),
                      &web_configuration);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }

    result = operations->set_indicator(
        operations->context,
        storage_degraded ? DEVICE_INDICATOR_DEGRADED : DEVICE_INDICATOR_READY);
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(operations,
                                     web_started,
                                     wifi_started,
                                     storage_mounted,
                                     result);
    }
    return APP_ERROR_NONE;
}
