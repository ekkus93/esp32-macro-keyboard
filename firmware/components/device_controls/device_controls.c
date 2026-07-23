#include "device_controls.h"

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "macro_executor.h"
#include "sdkconfig.h"

#define CONTROL_POLL_MS 20U
#define DEBOUNCE_SAMPLES 3U

static SemaphoreHandle_t confirmation_semaphore;
static portMUX_TYPE indicator_lock = portMUX_INITIALIZER_UNLOCKED;
static device_indicator_state_t indicator_state = DEVICE_INDICATOR_BOOTING;

typedef struct {
    bool stable;
    bool candidate;
    uint8_t candidate_count;
} debounced_button_t;

static bool button_pressed(gpio_num_t gpio)
{
    return gpio_get_level(gpio) == CONFIG_APP_BUTTON_ACTIVE_LEVEL;
}

static bool update_button(debounced_button_t *button, bool sample)
{
    if (sample != button->candidate) {
        button->candidate = sample;
        button->candidate_count = 1U;
        return false;
    }
    if (button->candidate_count < DEBOUNCE_SAMPLES) {
        ++button->candidate_count;
    }
    if (button->candidate_count == DEBOUNCE_SAMPLES && button->stable != sample) {
        button->stable = sample;
        return sample;
    }
    return false;
}

static device_indicator_state_t get_indicator_state(void)
{
    portENTER_CRITICAL(&indicator_lock);
    const device_indicator_state_t state = indicator_state;
    portEXIT_CRITICAL(&indicator_lock);
    return state;
}

static bool indicator_on(device_indicator_state_t state, TickType_t now)
{
    const uint32_t elapsed_ms = (uint32_t)(now * portTICK_PERIOD_MS);
    switch (state) {
    case DEVICE_INDICATOR_READY:
        return true;
    case DEVICE_INDICATOR_BOOTING:
        return (elapsed_ms % 1000U) < 250U;
    case DEVICE_INDICATOR_EXECUTING:
        return (elapsed_ms % 200U) < 100U;
    case DEVICE_INDICATOR_DEGRADED:
        return (elapsed_ms % 2000U) < 250U ||
               ((elapsed_ms % 2000U) >= 500U && (elapsed_ms % 2000U) < 750U);
    case DEVICE_INDICATOR_FATAL:
        return (elapsed_ms % 500U) < 250U;
    default:
        return false;
    }
}

static void controls_task(void *context)
{
    (void)context;
    debounced_button_t confirm = {0};
    debounced_button_t cancel = {0};

    while (true) {
        if (update_button(&confirm,
                          button_pressed((gpio_num_t)CONFIG_APP_CONFIRM_BUTTON_GPIO))) {
            (void)xSemaphoreGive(confirmation_semaphore);
        }
        if (update_button(&cancel,
                          button_pressed((gpio_num_t)CONFIG_APP_CANCEL_BUTTON_GPIO))) {
            const app_error_code_t result = macro_executor_cancel();
            if (result != APP_ERROR_NONE && result != APP_ERROR_CONFLICT) {
                device_controls_set_indicator(DEVICE_INDICATOR_FATAL);
            }
        }

        const bool on = indicator_on(get_indicator_state(), xTaskGetTickCount());
        const int level = on ? CONFIG_APP_LED_ACTIVE_LEVEL : !CONFIG_APP_LED_ACTIVE_LEVEL;
        if (gpio_set_level((gpio_num_t)CONFIG_APP_STATUS_LED_GPIO, level) != ESP_OK) {
            portENTER_CRITICAL(&indicator_lock);
            indicator_state = DEVICE_INDICATOR_FATAL;
            portEXIT_CRITICAL(&indicator_lock);
        }
        vTaskDelay(pdMS_TO_TICKS(CONTROL_POLL_MS));
    }
}

app_error_code_t device_controls_init(void)
{
    if (CONFIG_APP_CONFIRM_BUTTON_GPIO == CONFIG_APP_CANCEL_BUTTON_GPIO ||
        CONFIG_APP_CONFIRM_BUTTON_GPIO == CONFIG_APP_STATUS_LED_GPIO ||
        CONFIG_APP_CANCEL_BUTTON_GPIO == CONFIG_APP_STATUS_LED_GPIO) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const uint64_t input_mask = (1ULL << (unsigned int)CONFIG_APP_CONFIRM_BUTTON_GPIO) |
                                (1ULL << (unsigned int)CONFIG_APP_CANCEL_BUTTON_GPIO);
    const gpio_config_t input = {
        .pin_bit_mask = input_mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = CONFIG_APP_BUTTON_ACTIVE_LEVEL == 0 ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en =
            CONFIG_APP_BUTTON_ACTIVE_LEVEL == 1 ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
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
