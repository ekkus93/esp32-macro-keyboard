#include "fake_gpio_backend.h"

#include <stdlib.h>
#include <string.h>

void fake_gpio_backend_reset(fake_gpio_backend_t *gpio)
{
    if (gpio == NULL) {
        abort();
    }
    memset(gpio, 0, sizeof(*gpio));
    fake_call_log_reset(&gpio->calls);
}

int fake_gpio_backend_configure(fake_gpio_backend_t *gpio, unsigned int pin, int mode)
{
    if (gpio == NULL || pin >= FAKE_GPIO_COUNT) {
        abort();
    }
    if (fake_call_log_record(&gpio->calls, "gpio_configure", pin, (uint64_t)(uint32_t)mode)) {
        return -1;
    }
    return gpio->configure_result;
}

int fake_gpio_backend_get(fake_gpio_backend_t *gpio, unsigned int pin)
{
    if (gpio == NULL || pin >= FAKE_GPIO_COUNT) {
        abort();
    }
    return fake_call_log_record(&gpio->calls, "gpio_get", pin, 0U)
               ? -1
               : gpio->levels[pin];
}

int fake_gpio_backend_set(fake_gpio_backend_t *gpio, unsigned int pin, int level)
{
    if (gpio == NULL || pin >= FAKE_GPIO_COUNT) {
        abort();
    }
    if (fake_call_log_record(&gpio->calls, "gpio_set", pin, (uint64_t)(uint32_t)level)) {
        return -1;
    }
    gpio->levels[pin] = level;
    return gpio->set_result;
}
