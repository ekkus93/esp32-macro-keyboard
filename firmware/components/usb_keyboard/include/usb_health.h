#ifndef USB_HEALTH_H
#define USB_HEALTH_H

#include <stdbool.h>

#include "app_error.h"
#include "subsystem_health.h"

/* USB health for Phase 19 diagnostics (FIX1 handoff §7.1). Portable C with no
 * ESP-IDF dependency, so it is host-testable directly. Recorded from
 * app_core.c's existing usb_init/usb_deinit call sites. */
typedef struct {
    subsystem_health_state_t state;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} usb_health_t;

void usb_health_reset(void);
void usb_health_record_primary(app_error_code_t error);
void usb_health_record_cleanup(app_error_code_t cleanup_error, bool cleanup_incomplete);
usb_health_t usb_health_snapshot(void);

#endif
