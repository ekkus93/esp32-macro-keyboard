#ifndef FAKE_FREERTOS_H
#define FAKE_FREERTOS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fake_call_log.h"

typedef struct {
    bool locked;
    bool queue_full;
    bool notification_pending;
    uint32_t elapsed_ms;
    uint8_t queued_bytes[1024U];
    size_t queued_length;
    fake_call_log_t calls;
} fake_freertos_t;

void fake_freertos_reset(fake_freertos_t *freertos);
bool fake_freertos_lock(fake_freertos_t *freertos);
bool fake_freertos_unlock(fake_freertos_t *freertos);
bool fake_freertos_queue_send(fake_freertos_t *freertos,
                              const void *data,
                              size_t length);
void fake_freertos_notify(fake_freertos_t *freertos);
bool fake_freertos_wait(fake_freertos_t *freertos, uint32_t milliseconds);

#endif
