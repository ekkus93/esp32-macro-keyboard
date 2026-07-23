#include "device_controls.h"

#include <stdbool.h>
#include <stdint.h>

#include "device_controls_logic.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "macro_executor.h"
#include "sdkconfig.h"

#define CONTROL_POLL_MS 20U

static SemaphoreHandle_t confirmation_semaphore;
static portMUX_TYPE indicator_lock = portMUX_INITIALIZER_UNLOCKED;
static device_indicator_state_t indicator_state = DEVICE_INDICATOR_BOOTING;

static bool button_pressed(gpio_num_t gpio)
{
    return device_controls_level_is_pressed(gpio_get_level(gpio),
                                            CONFIG_APP_BUTTON_ACTIVE_LEVEL);
}

static device_indicator_state_t get_indicator_state(void)
{
    portENTER_CRITICAL(&indicator_lock);
    const device_indicator_state_t state = indicator_state;
    portEXIT_CRITICAL(&indicator_lock);
    return state;
}

static void controls_task(void *context)
{
    (void)context;
    device_controls_debounce_t confirm = {0};
    device_controls_debounce_t cancel = {0};

    while (true) {
        if (device_controls_debounce_update(
                &confirm,
                button_pressed((gpio_num_t)CONFIG_APP_CONFIRM_BUTTON_GPIO))) {
            if (xSemaphoreGive(confirmation_semaphore) != pdTRUE) {
                device_controls_set_indicator(DEVICE_INDICATOR_FATAL);
            }
        }
        if (device_controls_debounce_update(
                &cancel,
                button_pressed((gpio_num_t)CONFIG_APP_CANCEL_BUTTON_GPIO))) {
            const app_error_code_t result = macro_executor_cancel();
            if (result != APP_ERROR_NONE && result != APP_ERROR_NOT_FOUND) {
                device_controls_set_indicator(DEVICE_INDICATOR_FATAL);
            }
        }

        const uint64_t elapsed =
            (uint64_t)xTaskGetTickCount() * (uint64_t)portTICK_PERIOD_MS;
        const bool on = device_controls_indicator_on(get_indicator_state(),
                                                     (uint32_t)elapsed);
        const int level = on ? CONFIG_APP_LED_ACTIVE_LEVEL
                             : !CONFIG_APP_LED_ACTIVE_LEVEL;
        if (gpio_set_level((gpio_num_t)CONFIG_APP_STATUS_LED_GPIO, level) != ESP_OK) {
            portENTER_CRITICAL(&indicator_lock);
            indicator_state = DEVICE_INDICATOR_FATAL;
            portEXIT_CRITICAL(&indicator_lock);
        }
        vTaskDelay(pdMS_TO_TICKS(CONTROL_POLL_MS));
    }
}

static bool valid_gpio_number(int gpio)
{
    return gpio >= 0 && gpio < (int)GPIO_NUM_MAX;
}

app_error_code_t device_controls_init(void)
{
    if (!valid_gpio_number(CONFIG_APP_CONFIRM_BUTTON_GPIO) ||
        !valid_gpio_number(CONFIG_APP_CANCEL_BUTTON_GPIO) ||
        !valid_gpio_number(CONFIG_APP_STATUS_LED_GPIO) ||
        (CONFIG_APP_BUTTON_ACTIVE_LEVEL != 0 &&
         CONFIG_APP_BUTTON_ACTIVE_LEVEL != 1) ||
        (CONFIG_APP_LED_ACTIVE_LEVEL != 0 && CONFIG_APP_LED_ACTIVE_LEVEL != 1) ||
        CONFIG_APP_CONFIRM_BUTTON_GPIO == CONFIG_APP_CANCEL_BUTTON_GPIO ||
        CONFIG_APP_CONFIRM_BUTTON_GPIO == CONFIG_APP_STATUS_LED_GPIO ||
        CONFIG_APP_CANCEL_BUTTON_GPIO == CONFIG_APP_STATUS_LED_GPIO) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const uint64_t input_mask =
        (1ULL << (unsigned int)CONFIG_APP_CONFIRM_BUTTON_GPIO) |
        (1ULL << (unsigned int)CONFIG_APP_CANCEL_BUTTON_GPIO);
    const gpio_config_t input = {
        .pin_bit_mask = input_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = CONFIG_APP_BUTTON_ACTIVE_LEVEL == 0 ? GPIO_PULLUP_ENABLE
                                                          : GPIO_PULLUP_DISABLE,
        .pull_down_en = CONFIG_APP_BUTTON_ACTIVE_LEVEL == 1
                            ? GPIO_PULLDOWN_ENABLE
                            : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    const gpio_config_t output = {
        .pin_bit_mask = 1ULL << (unsigned int)CONFIG_APP_STATUS_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    if (gpio_config(&input) != ESP_OK || gpio_config(&output) != ESP_OK) {
        return APP_ERROR_INTERNAL;
    }
    confirmation_semaphore = xSemaphoreCreateBinary();
    if (confirmation_semaphore == NULL) {
        return APP_ERROR_INTERNAL;
    }
    if (xTaskCreate(controls_task, "controls", 2048U, NULL, 5U, NULL) != pdPASS) {
        vSemaphoreDelete(confirmation_semaphore);
        confirmation_semaphore = NULL;
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

void device_controls_set_indicator(device_indicator_state_t state)
{
    portENTER_CRITICAL(&indicator_lock);
    indicator_state = state;
    portEXIT_CRITICAL(&indicator_lock);
}

app_error_code_t device_controls_wait_for_confirmation(unsigned int timeout_ms)
{
    if (confirmation_semaphore == NULL || timeout_ms == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return xSemaphoreTake(confirmation_semaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE
               ? APP_ERROR_NONE
               : APP_ERROR_TIMEOUT;
}
