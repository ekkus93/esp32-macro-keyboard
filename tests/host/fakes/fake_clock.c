#include "fake_clock.h"

#include <stdlib.h>

void fake_clock_reset(fake_clock_t *clock)
{
    if (clock == NULL) {
        abort();
    }
    clock->now_us = 0U;
    fake_call_log_reset(&clock->calls);
}

void fake_clock_set_us(fake_clock_t *clock, uint64_t now_us)
{
    if (clock == NULL) {
        abort();
    }
    if (!fake_call_log_record(&clock->calls, "clock_set", now_us, 0U)) {
        clock->now_us = now_us;
    }
}

void fake_clock_advance_us(fake_clock_t *clock, uint64_t delta_us)
{
    if (clock == NULL || UINT64_MAX - clock->now_us < delta_us) {
        abort();
    }
    if (!fake_call_log_record(&clock->calls, "clock_advance", delta_us, 0U)) {
        clock->now_us += delta_us;
    }
}

uint64_t fake_clock_now_us(fake_clock_t *clock)
{
    if (clock == NULL) {
        abort();
    }
    return fake_call_log_record(&clock->calls, "clock_now_us", clock->now_us, 0U)
               ? UINT64_MAX
               : clock->now_us;
}

uint32_t fake_clock_now_ms(fake_clock_t *clock)
{
    if (clock == NULL) {
        abort();
    }
    const uint32_t now_ms = (uint32_t)(clock->now_us / UINT64_C(1000));
    return fake_call_log_record(&clock->calls, "clock_now_ms", now_ms, 0U)
               ? UINT32_MAX
               : now_ms;
}
