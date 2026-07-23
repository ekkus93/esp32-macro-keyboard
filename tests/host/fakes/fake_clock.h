#ifndef FAKE_CLOCK_H
#define FAKE_CLOCK_H

#include <stdint.h>

#include "fake_call_log.h"

typedef struct {
    uint64_t now_us;
    fake_call_log_t calls;
} fake_clock_t;

void fake_clock_reset(fake_clock_t *clock);
void fake_clock_set_us(fake_clock_t *clock, uint64_t now_us);
void fake_clock_advance_us(fake_clock_t *clock, uint64_t delta_us);
uint64_t fake_clock_now_us(fake_clock_t *clock);
uint32_t fake_clock_now_ms(fake_clock_t *clock);

#endif
