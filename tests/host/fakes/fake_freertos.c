#include "fake_freertos.h"

#include <stdlib.h>
#include <string.h>

void fake_freertos_reset(fake_freertos_t *freertos)
{
    if (freertos == NULL) {
        abort();
    }
    memset(freertos, 0, sizeof(*freertos));
    fake_call_log_reset(&freertos->calls);
}

bool fake_freertos_lock(fake_freertos_t *freertos)
{
    if (freertos == NULL || freertos->locked) {
        abort();
    }
    if (fake_call_log_record(&freertos->calls, "lock", 0U, 0U)) {
        return false;
    }
    freertos->locked = true;
    return true;
}

bool fake_freertos_unlock(fake_freertos_t *freertos)
{
    if (freertos == NULL || !freertos->locked) {
        abort();
    }
    const bool fail = fake_call_log_record(&freertos->calls, "unlock", 0U, 0U);
    freertos->locked = false;
    return !fail;
}

bool fake_freertos_queue_send(fake_freertos_t *freertos,
                              const void *data,
                              size_t length)
{
    if (freertos == NULL || data == NULL || length == 0U ||
        length > sizeof(freertos->queued_bytes)) {
        abort();
    }
    if (fake_call_log_record(&freertos->calls, "queue_send", length, 0U) ||
        freertos->queue_full) {
        return false;
    }
    memcpy(freertos->queued_bytes, data, length);
    freertos->queued_length = length;
    freertos->queue_full = true;
    return true;
}

void fake_freertos_notify(fake_freertos_t *freertos)
{
    if (freertos == NULL) {
        abort();
    }
    (void)fake_call_log_record(&freertos->calls, "notify", 0U, 0U);
    freertos->notification_pending = true;
}

bool fake_freertos_wait(fake_freertos_t *freertos, uint32_t milliseconds)
{
    if (freertos == NULL || UINT32_MAX - freertos->elapsed_ms < milliseconds) {
        abort();
    }
    if (fake_call_log_record(&freertos->calls, "wait", milliseconds, 0U)) {
        return false;
    }
    freertos->elapsed_ms += milliseconds;
    const bool notified = freertos->notification_pending;
    freertos->notification_pending = false;
    return notified;
}
