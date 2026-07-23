#ifndef FAKE_GPIO_BACKEND_H
#define FAKE_GPIO_BACKEND_H

#include <stddef.h>

#include "fake_call_log.h"

#define FAKE_GPIO_COUNT 64U

typedef struct {
    int levels[FAKE_GPIO_COUNT];
    int configure_result;
    int set_result;
    fake_call_log_t calls;
} fake_gpio_backend_t;

void fake_gpio_backend_reset(fake_gpio_backend_t *gpio);
int fake_gpio_backend_configure(fake_gpio_backend_t *gpio, unsigned int pin, int mode);
int fake_gpio_backend_get(fake_gpio_backend_t *gpio, unsigned int pin);
int fake_gpio_backend_set(fake_gpio_backend_t *gpio, unsigned int pin, int level);

#endif
