#include "fake_usb_backend.h"

#include <stdlib.h>
#include <string.h>

void fake_usb_backend_reset(fake_usb_backend_t *usb)
{
    if (usb == NULL) {
        abort();
    }
    memset(usb, 0, sizeof(*usb));
    fake_call_log_reset(&usb->calls);
}

int fake_usb_backend_state(fake_usb_backend_t *usb)
{
    if (usb == NULL) {
        abort();
    }
    (void)fake_call_log_record(&usb->calls,
                               "usb_state",
                               (uint64_t)(uint32_t)usb->state,
                               0U);
    return usb->state;
}

int fake_usb_backend_press(fake_usb_backend_t *usb, uint8_t modifiers, uint8_t usage)
{
    if (usb == NULL) {
        abort();
    }
    if (fake_call_log_record(&usb->calls, "usb_press", modifiers, usage)) {
        return -1;
    }
    usb->last_modifiers = modifiers;
    usb->last_usage = usage;
    ++usb->press_count;
    return usb->press_result;
}

int fake_usb_backend_release_all(fake_usb_backend_t *usb)
{
    if (usb == NULL) {
        abort();
    }
    if (fake_call_log_record(&usb->calls, "usb_release_all", 0U, 0U)) {
        return -1;
    }
    ++usb->release_count;
    return usb->release_result;
}
