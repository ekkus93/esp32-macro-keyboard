#ifndef DEVICE_CONTROLS_H
#define DEVICE_CONTROLS_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "subsystem_health.h"

typedef enum {
    DEVICE_INDICATOR_BOOTING = 0,
    DEVICE_INDICATOR_READY,
    DEVICE_INDICATOR_EXECUTING,
    DEVICE_INDICATOR_DEGRADED,
    DEVICE_INDICATOR_FATAL
} device_indicator_state_t;

typedef struct {
    app_error_code_t last_error;
    app_error_code_t cleanup_error;
    app_error_code_t last_confirmation_error;
    bool task_running;
    bool indicator_output_failed;
    bool confirmation_signal_failed;
    bool gpio_configuration_failed;
    bool task_start_failed;
    bool task_stop_failed;
} device_controls_health_t;

typedef struct {
    bool durably_accepted;
    bool recovery_required;
    app_error_code_t primary_error;
} device_controls_factory_reset_outcome_t;

app_error_code_t device_controls_init(void);
app_error_code_t device_controls_deinit(void);
void device_controls_set_indicator(device_indicator_state_t state);
app_error_code_t device_controls_wait_for_confirmation(unsigned int timeout_ms);

/* Gives the confirmation signal without a button press, so a device with no
 * confirm button wired to it is still fully usable. Backs the `confirm`
 * serial-console command; the physical button, when present, does exactly the
 * same thing. Returns APP_ERROR_CONFLICT when the controls task is not
 * running. */
app_error_code_t device_controls_signal_confirmation(void);
device_controls_health_t device_controls_get_health(void);

/* SPEC_V2.md §13.12 device actions. The destructive confirmation phrase and
 * (for factory reset) admin password are validated by the caller before any
 * of these run -- SPEC_V2.md §13.12: "The destructive operation MUST NOT
 * begin until the complete request, password, and confirmation phrase have
 * been validated." Each schedules an asynchronous reboot after
 * DEVICE_CONTROLS_RESTART_DELAY_MS so an HTTP caller's response has time to
 * flush before the connection drops; none of them block waiting for it. */
#define DEVICE_CONTROLS_RESTART_DELAY_MS 500U

/* No settings, credential, or repository change; schedules a reboot. */
app_error_code_t device_controls_restart(void);

/* Applies SPEC_V2.md §11.4 "reset settings", invalidates every session, and
 * schedules a reboot. Repository blobs and the AP/admin credentials are
 * untouched. */
app_error_code_t device_controls_reset_settings(void);

/* Applies SPEC_V2.md §11.4 "factory reset": erases device configuration,
 * credentials, and provisioning state; invalidates every session; deletes
 * every repository blob; and schedules a reboot into the unprovisioned
 * state. */
device_controls_factory_reset_outcome_t device_controls_factory_reset(void);

/* Derives a stable subsystem_health_state_t for Phase 19 diagnostics (FIX1
 * handoff §7.1) from the existing device_controls_health_t fields: FAILED if
 * any error or specific-failure flag is set (never HEALTHY while one is),
 * UNAVAILABLE if the controls task isn't running, HEALTHY otherwise. */
subsystem_health_state_t device_controls_health_derive_state(device_controls_health_t health);

/* Controls task stack high-water mark in words for Phase 19 diagnostics (FIX1
 * handoff §7.1), or 0 when the task isn't running. Not host-testable (reads
 * FreeRTOS state directly); the diagnostics aggregator reaches it through an
 * injected ops seam. */
size_t device_controls_stack_high_water_mark(void);

#endif
