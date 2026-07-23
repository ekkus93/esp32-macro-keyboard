#include "wifi_ap_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

bool wifi_ap_ops_is_valid(const wifi_ap_ops_t *operations)
{
    return operations != NULL && operations->status_get != NULL &&
           operations->status_set != NULL && operations->netif_init != NULL &&
           operations->event_loop_create != NULL &&
           operations->netif_create != NULL && operations->wifi_init != NULL &&
           operations->handler_register != NULL &&
           operations->set_mode_ap != NULL && operations->set_config != NULL &&
           operations->wifi_start != NULL && operations->wifi_stop != NULL &&
           operations->handler_unregister != NULL &&
           operations->wifi_deinit != NULL && operations->netif_destroy != NULL;
}

static void publish_status(wifi_ap_engine_t *engine,
                           wifi_ap_state_t state,
                           size_t clients,
                           app_error_code_t error,
                           app_error_code_t cleanup_error)
{
    const wifi_ap_status_t status = {
        .state = state,
        .client_count = clients,
        .last_error = error,
        .cleanup_error = cleanup_error,
    };
    engine->operations.status_set(engine->operations.context, &status);
}

static bool bounded_length(const char *text, size_t maximum, size_t *out_length)
{
    if (text == NULL || out_length == NULL) {
        return false;
    }
    for (size_t index = 0U; index <= maximum; ++index) {
        if (text[index] == '\0') {
            *out_length = index;
            return true;
        }
    }
    return false;
}

static app_error_code_t make_configuration(const char *ssid,
                                           const char *passphrase,
                                           wifi_ap_runtime_config_t *configuration)
{
    if (configuration == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    size_t ssid_length = 0U;
    size_t passphrase_length = 0U;
    if (!bounded_length(ssid, WIFI_AP_SSID_MAX_BYTES, &ssid_length) ||
        ssid_length == 0U ||
        !bounded_length(passphrase,
                        WIFI_AP_PASSPHRASE_MAX_BYTES,
                        &passphrase_length) ||
        passphrase_length < WIFI_AP_PASSPHRASE_MIN_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    memset(configuration, 0, sizeof(*configuration));
    memcpy(configuration->ssid, ssid, ssid_length);
    configuration->ssid_length = ssid_length;
    memcpy(configuration->passphrase, passphrase, passphrase_length);
    configuration->channel = WIFI_AP_DEFAULT_CHANNEL;
    configuration->maximum_clients = WIFI_AP_MAX_CLIENTS;
    configuration->wpa2_wpa3_psk = true;
    configuration->pmf_required = true;
    return APP_ERROR_NONE;
}

static app_error_code_t cleanup_resources(wifi_ap_engine_t *engine)
{
    app_error_code_t cleanup_error = APP_ERROR_NONE;

    if (engine->wifi_started) {
        cleanup_error = engine->operations.wifi_stop(engine->operations.context);
        if (cleanup_error != APP_ERROR_NONE) {
            return cleanup_error;
        }
        engine->wifi_started = false;
    }
    if (engine->handler_registered) {
        cleanup_error =
            engine->operations.handler_unregister(engine->operations.context);
        if (cleanup_error != APP_ERROR_NONE) {
            return cleanup_error;
        }
        engine->handler_registered = false;
    }
    if (engine->wifi_initialized) {
        cleanup_error = engine->operations.wifi_deinit(engine->operations.context);
        if (cleanup_error != APP_ERROR_NONE) {
            return cleanup_error;
        }
        engine->wifi_initialized = false;
    }
    if (engine->netif_created) {
        cleanup_error = engine->operations.netif_destroy(engine->operations.context);
        if (cleanup_error != APP_ERROR_NONE) {
            return cleanup_error;
        }
        engine->netif_created = false;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t fail_start(wifi_ap_engine_t *engine,
                                   app_error_code_t original)
{
    const app_error_code_t cleanup_error = cleanup_resources(engine);
    publish_status(engine, WIFI_AP_ERROR, 0U, original, cleanup_error);
    return original;
}

app_error_code_t wifi_ap_engine_init(wifi_ap_engine_t *engine,
                                     const wifi_ap_ops_t *operations)
{
    if (engine == NULL || !wifi_ap_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(engine, 0, sizeof(*engine));
    engine->operations = *operations;
    engine->initialized = true;
    publish_status(engine,
                   WIFI_AP_STOPPED,
                   0U,
                   APP_ERROR_NONE,
                   APP_ERROR_NONE);
    return APP_ERROR_NONE;
}

app_error_code_t wifi_ap_engine_start(wifi_ap_engine_t *engine,
                                      const char *ssid,
                                      const char *passphrase)
{
    if (engine == NULL || !engine->initialized) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const wifi_ap_status_t current =
        engine->operations.status_get(engine->operations.context);
    if (current.state != WIFI_AP_STOPPED || engine->netif_created ||
        engine->wifi_initialized || engine->handler_registered ||
        engine->wifi_started) {
        return APP_ERROR_CONFLICT;
    }

    wifi_ap_runtime_config_t configuration;
    const app_error_code_t configuration_result =
        make_configuration(ssid, passphrase, &configuration);
    if (configuration_result != APP_ERROR_NONE) {
        return configuration_result;
    }

    publish_status(engine,
                   WIFI_AP_STARTING,
                   0U,
                   APP_ERROR_NONE,
                   APP_ERROR_NONE);

    app_error_code_t result =
        engine->operations.netif_init(engine->operations.context);
    if (result != APP_ERROR_NONE) {
        return fail_start(engine, result);
    }

    const wifi_ap_event_loop_result_t event_result =
        engine->operations.event_loop_create(engine->operations.context);
    if (event_result == WIFI_AP_EVENT_LOOP_FAILED) {
        return fail_start(engine, APP_ERROR_INTERNAL);
    }

    result = engine->operations.netif_create(engine->operations.context);
    if (result != APP_ERROR_NONE) {
        return fail_start(engine, result);
    }
    engine->netif_created = true;

    result = engine->operations.wifi_init(engine->operations.context);
    if (result != APP_ERROR_NONE) {
        return fail_start(engine, result);
    }
    engine->wifi_initialized = true;

    result = engine->operations.handler_register(engine->operations.context);
    if (result != APP_ERROR_NONE) {
        return fail_start(engine, result);
    }
    engine->handler_registered = true;

    result = engine->operations.set_mode_ap(engine->operations.context);
    if (result != APP_ERROR_NONE) {
        return fail_start(engine, result);
    }
    result = engine->operations.set_config(engine->operations.context,
                                           &configuration);
    if (result != APP_ERROR_NONE) {
        return fail_start(engine, result);
    }

    engine->wifi_started = true;
    result = engine->operations.wifi_start(engine->operations.context);
    if (result != APP_ERROR_NONE) {
        return fail_start(engine, result);
    }
    return APP_ERROR_NONE;
}

app_error_code_t wifi_ap_engine_stop(wifi_ap_engine_t *engine)
{
    if (engine == NULL || !engine->initialized) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const wifi_ap_status_t current =
        engine->operations.status_get(engine->operations.context);
    if (current.state == WIFI_AP_STOPPED && !engine->netif_created &&
        !engine->wifi_initialized && !engine->handler_registered &&
        !engine->wifi_started) {
        return APP_ERROR_NONE;
    }

    const app_error_code_t cleanup_error = cleanup_resources(engine);
    if (cleanup_error == APP_ERROR_NONE) {
        publish_status(engine,
                       WIFI_AP_STOPPED,
                       0U,
                       APP_ERROR_NONE,
                       APP_ERROR_NONE);
        return APP_ERROR_NONE;
    }
    const app_error_code_t original = current.last_error != APP_ERROR_NONE
                                          ? current.last_error
                                          : cleanup_error;
    publish_status(engine, WIFI_AP_ERROR, 0U, original, cleanup_error);
    return cleanup_error;
}

void wifi_ap_engine_handle_event(wifi_ap_engine_t *engine, wifi_ap_event_t event)
{
    if (engine == NULL || !engine->initialized) {
        return;
    }
    wifi_ap_status_t current =
        engine->operations.status_get(engine->operations.context);
    switch (event) {
    case WIFI_AP_EVENT_STARTED:
        if (current.state == WIFI_AP_STARTING) {
            current.state = WIFI_AP_READY;
            current.last_error = APP_ERROR_NONE;
            current.cleanup_error = APP_ERROR_NONE;
        }
        break;
    case WIFI_AP_EVENT_STOPPED:
        current.state = WIFI_AP_STOPPED;
        current.client_count = 0U;
        break;
    case WIFI_AP_EVENT_CLIENT_CONNECTED:
        if ((current.state == WIFI_AP_STARTING || current.state == WIFI_AP_READY) &&
            current.client_count < WIFI_AP_MAX_CLIENTS) {
            ++current.client_count;
        }
        break;
    case WIFI_AP_EVENT_CLIENT_DISCONNECTED:
        if ((current.state == WIFI_AP_STARTING || current.state == WIFI_AP_READY) &&
            current.client_count > 0U) {
            --current.client_count;
        }
        break;
    case WIFI_AP_EVENT_UNKNOWN:
    default:
        return;
    }
    engine->operations.status_set(engine->operations.context, &current);
}
