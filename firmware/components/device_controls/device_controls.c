#include "device_controls.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "device_controls_logic.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define CONTROL_POLL_MS 20U
#define CONTROLS_TASK_PRIORITY 5U
#define CONTROLS_SHUTDOWN_WAIT_MS 2000U

static const char *const TAG = "device_controls";

static SemaphoreHandle_t confirmation_semaphore;
static SemaphoreHandle_t controls_stopped;
static TaskHandle_t controls_task_handle;
static portMUX_TYPE controls_lock = portMUX_INITIALIZER_UNLOCKED;
static volatile bool controls_stop_requested;
static device_indicator_state_t requested_indicator_state = DEVICE_INDICATOR_BOOTING;
static device_controls_engine_t engine;

static bool adapter_lock(void *context) {
    (void)context;
    portENTER_CRITICAL(&controls_lock);
    return true;
}

static bool adapter_unlock(void *context) {
    (void)context;
    portEXIT_CRITICAL(&controls_lock);
    return true;
}

static bool adapter_signal_confirmation(void *context) {
    (void)context;
    return confirmation_semaphore != NULL && xSemaphoreGive(confirmation_semaphore) == pdTRUE;
}

static bool adapter_signal_stopped(void *context) {
    (void)context;
    return controls_stopped != NULL && xSemaphoreGive(controls_stopped) == pdTRUE;
}

static app_error_code_t adapter_write_indicator(void *context, bool enabled) {
    (void)context;
    const int level = enabled ? CONFIG_APP_LED_ACTIVE_LEVEL : !CONFIG_APP_LED_ACTIVE_LEVEL;
    return gpio_set_level((gpio_num_t)CONFIG_APP_STATUS_LED_GPIO, (uint32_t)level) == ESP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

static void adapter_request_stop(void *context) {
    (void)context;
    portENTER_CRITICAL(&controls_lock);
    controls_stop_requested = true;
    portEXIT_CRITICAL(&controls_lock);
}

static void adapter_clear_stop_request(void *context) {
    (void)context;
    portENTER_CRITICAL(&controls_lock);
    controls_stop_requested = false;
    portEXIT_CRITICAL(&controls_lock);
}

static app_error_code_t adapter_wait_stopped(void *context, uint32_t timeout_ms) {
    (void)context;
    if (controls_stopped == NULL || timeout_ms == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return xSemaphoreTake(controls_stopped, pdMS_TO_TICKS(timeout_ms)) == pdTRUE
               ? APP_ERROR_NONE
               : APP_ERROR_TIMEOUT;
}

static app_error_code_t adapter_set_safe_output(void *context) {
    (void)context;
    /* The documented shutdown-safe state is an inactive status LED. The pin is
     * driven inactive before all three controls pins are reset to their default
     * high-impedance GPIO state. */
    const int inactive_level = !CONFIG_APP_LED_ACTIVE_LEVEL;
    return gpio_set_level((gpio_num_t)CONFIG_APP_STATUS_LED_GPIO, (uint32_t)inactive_level) ==
                   ESP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

static gpio_num_t pin_number(device_controls_pin_t pin) {
    (void)pin;
    return (gpio_num_t)CONFIG_APP_STATUS_LED_GPIO;
}

static app_error_code_t adapter_reset_pin(void *context, device_controls_pin_t pin) {
    (void)context;
    if (pin != DEVICE_CONTROLS_PIN_STATUS) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return gpio_reset_pin(pin_number(pin)) == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static void adapter_delete_confirmation_signal(void *context) {
    (void)context;
    if (confirmation_semaphore != NULL) {
        vSemaphoreDelete(confirmation_semaphore);
        confirmation_semaphore = NULL;
    }
}

static void adapter_delete_stopped_signal(void *context) {
    (void)context;
    if (controls_stopped != NULL) {
        vSemaphoreDelete(controls_stopped);
        controls_stopped = NULL;
    }
}

static device_controls_ops_t controls_operations(void) {
    return (device_controls_ops_t){
        .context = NULL,
        .lock = adapter_lock,
        .unlock = adapter_unlock,
        .signal_confirmation = adapter_signal_confirmation,
        .signal_stopped = adapter_signal_stopped,
        .write_indicator = adapter_write_indicator,
        .request_stop = adapter_request_stop,
        .clear_stop_request = adapter_clear_stop_request,
        .wait_stopped = adapter_wait_stopped,
        .set_safe_output = adapter_set_safe_output,
        .reset_pin = adapter_reset_pin,
        .delete_confirmation_signal = adapter_delete_confirmation_signal,
        .delete_stopped_signal = adapter_delete_stopped_signal,
    };
}

static bool stop_requested(void) {
    portENTER_CRITICAL(&controls_lock);
    const bool requested = controls_stop_requested;
    portEXIT_CRITICAL(&controls_lock);
    return requested;
}

static void log_controls_error(const char *operation, app_error_code_t error) {
    ESP_LOGE(TAG, "%s failed: %s", operation, app_error_code_string(error));
}

static void controls_task(void *context) {
    (void)context;
    while (true) {
        const uint64_t elapsed = (uint64_t)xTaskGetTickCount() * (uint64_t)portTICK_PERIOD_MS;
        const device_controls_poll_result_t result =
            device_controls_engine_poll(&engine, (device_controls_poll_input_t){
                                                     .stop_requested = stop_requested(),
                                                     .elapsed_ms = (uint32_t)elapsed,
                                                 });
        if (result.error != APP_ERROR_NONE) {
            log_controls_error("controls poll", result.error);
        }
        if (!result.continue_running) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(CONTROL_POLL_MS));
    }
    vTaskDelete(NULL);
}

static bool valid_gpio_number(int gpio) {
    return gpio >= 0 && gpio < GPIO_NUM_MAX;
}

static bool configuration_valid(void) {
    return valid_gpio_number(CONFIG_APP_STATUS_LED_GPIO) &&
           (CONFIG_APP_LED_ACTIVE_LEVEL == 0 || CONFIG_APP_LED_ACTIVE_LEVEL == 1);
}

static app_error_code_t acquire_resource(device_controls_resource_t resource) {
    return device_controls_engine_acquire_resource(&engine, resource);
}

static app_error_code_t record_init_failure(app_error_code_t primary,
                                            device_controls_failure_t failure) {
    const app_error_code_t record =
        device_controls_engine_record_failure(&engine, primary, failure, false);
    if (record != primary) {
        log_controls_error("record controls health", record);
    }
    const app_error_code_t cleanup =
        device_controls_engine_deinit(&engine, CONTROLS_SHUTDOWN_WAIT_MS);
    if (cleanup != APP_ERROR_NONE) {
        log_controls_error("controls init cleanup", cleanup);
    }
    return primary;
}

static app_error_code_t configure_output_pin(void) {
    app_error_code_t result = acquire_resource(DEVICE_CONTROLS_RESOURCE_STATUS_PIN);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const gpio_config_t output = {
        .pin_bit_mask = 1ULL << (unsigned int)CONFIG_APP_STATUS_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    return gpio_config(&output) == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t create_signals(void) {
    confirmation_semaphore = xSemaphoreCreateBinary();
    if (confirmation_semaphore == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = acquire_resource(DEVICE_CONTROLS_RESOURCE_CONFIRM_SIGNAL);
    if (result != APP_ERROR_NONE) {
        vSemaphoreDelete(confirmation_semaphore);
        confirmation_semaphore = NULL;
        return result;
    }

    controls_stopped = xSemaphoreCreateBinary();
    if (controls_stopped == NULL) {
        return APP_ERROR_INTERNAL;
    }
    result = acquire_resource(DEVICE_CONTROLS_RESOURCE_STOPPED_SIGNAL);
    if (result != APP_ERROR_NONE) {
        vSemaphoreDelete(controls_stopped);
        controls_stopped = NULL;
    }
    return result;
}

static device_indicator_state_t pending_indicator(void) {
    portENTER_CRITICAL(&controls_lock);
    const device_indicator_state_t state = requested_indicator_state;
    portEXIT_CRITICAL(&controls_lock);
    return state;
}

app_error_code_t device_controls_init(void) {
    if (controls_task_handle != NULL || confirmation_semaphore != NULL ||
        controls_stopped != NULL || engine.initialized ||
        device_controls_engine_owns_resources(&engine)) {
        return APP_ERROR_CONFLICT;
    }

    const device_controls_ops_t operations = controls_operations();
    app_error_code_t result = device_controls_engine_init(&engine, &operations);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!configuration_valid()) {
        return record_init_failure(APP_ERROR_INVALID_ARGUMENT,
                                   DEVICE_CONTROLS_FAILURE_GPIO_CONFIGURATION);
    }
    result = configure_output_pin();
    if (result != APP_ERROR_NONE) {
        return record_init_failure(result, DEVICE_CONTROLS_FAILURE_GPIO_CONFIGURATION);
    }
    result = adapter_set_safe_output(NULL);
    if (result != APP_ERROR_NONE) {
        return record_init_failure(result, DEVICE_CONTROLS_FAILURE_INDICATOR);
    }
    result = create_signals();
    if (result != APP_ERROR_NONE) {
        return record_init_failure(result, DEVICE_CONTROLS_FAILURE_GENERIC);
    }
    result = device_controls_engine_set_indicator(&engine, pending_indicator());
    if (result != APP_ERROR_NONE) {
        return record_init_failure(result, DEVICE_CONTROLS_FAILURE_GENERIC);
    }

    if (xTaskCreate(controls_task, "controls", 2048U, NULL, CONTROLS_TASK_PRIORITY,
                    &controls_task_handle) != pdPASS) {
        controls_task_handle = NULL;
        return record_init_failure(APP_ERROR_INTERNAL, DEVICE_CONTROLS_FAILURE_TASK_START);
    }
    result = device_controls_engine_task_started(&engine);
    if (result != APP_ERROR_NONE) {
        return record_init_failure(result, DEVICE_CONTROLS_FAILURE_TASK_START);
    }
    return APP_ERROR_NONE;
}

app_error_code_t device_controls_deinit(void) {
    if (!engine.initialized && !device_controls_engine_owns_resources(&engine) &&
        controls_task_handle == NULL && confirmation_semaphore == NULL &&
        controls_stopped == NULL) {
        return APP_ERROR_NONE;
    }
    const app_error_code_t result =
        device_controls_engine_deinit(&engine, CONTROLS_SHUTDOWN_WAIT_MS);
    if (device_controls_engine_task_stop_confirmed(&engine)) {
        controls_task_handle = NULL;
    }
    if (result != APP_ERROR_NONE) {
        log_controls_error("controls deinit", result);
    }
    return result;
}

void device_controls_set_indicator(device_indicator_state_t state) {
    if (state < DEVICE_INDICATOR_BOOTING || state > DEVICE_INDICATOR_FATAL) {
        ESP_LOGE(TAG, "invalid indicator state: %d", (int)state);
        return;
    }
    portENTER_CRITICAL(&controls_lock);
    requested_indicator_state = state;
    portEXIT_CRITICAL(&controls_lock);
    if (engine.initialized) {
        const app_error_code_t result = device_controls_engine_set_indicator(&engine, state);
        if (result != APP_ERROR_NONE) {
            log_controls_error("set indicator", result);
        }
    }
}

app_error_code_t device_controls_wait_for_confirmation(unsigned int timeout_ms) {
    if (confirmation_semaphore == NULL || timeout_ms == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const device_controls_health_t health = device_controls_engine_get_health(&engine);
    if (!health.task_running || stop_requested()) {
        return APP_ERROR_CONFLICT;
    }
    return xSemaphoreTake(confirmation_semaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE
               ? APP_ERROR_NONE
               : APP_ERROR_TIMEOUT;
}

app_error_code_t device_controls_signal_confirmation(void) {
    if (confirmation_semaphore == NULL) {
        return APP_ERROR_CONFLICT;
    }
    const device_controls_health_t health = device_controls_engine_get_health(&engine);
    if (!health.task_running || stop_requested()) {
        return APP_ERROR_CONFLICT;
    }
    return xSemaphoreGive(confirmation_semaphore) == pdTRUE ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

device_controls_health_t device_controls_get_health(void) {
    return device_controls_engine_get_health(&engine);
}

size_t device_controls_stack_high_water_mark(void) {
    return controls_task_handle != NULL ? (size_t)uxTaskGetStackHighWaterMark(controls_task_handle)
                                        : 0U;
}
