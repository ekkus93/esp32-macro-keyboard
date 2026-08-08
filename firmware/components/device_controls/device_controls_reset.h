#ifndef DEVICE_CONTROLS_RESET_H
#define DEVICE_CONTROLS_RESET_H

#include <stdbool.h>
#include <stdint.h>

#include "app_error.h"

/* Host-testable orchestration behind the SPEC_V2.md §13.12 device actions
 * (restart, reset-settings, factory-reset). Each op is a single side effect;
 * the hardware adapter (device_controls.c) wires them to device_settings,
 * auth, storage_blob, and a delayed esp_restart(), while this module owns only
 * the ordering and failure-handling contract below -- exercised by host tests
 * against fakes, with no NVS/FreeRTOS/hardware involved.
 *
 * Ordering rationale:
 *   - restart: no state changes at all, just a scheduled reboot.
 *   - reset-settings: apply the settings change first; only invalidate
 *     sessions and schedule the reboot if that succeeded. Nothing destructive
 *     happens on a settings-write failure.
 *   - factory-reset: erase settings first (the step that must not be skipped
 *     for the operation to mean anything). Once that succeeds, invalidate
 *     sessions and delete blobs are both attempted even if one of them fails
 *     -- continuing past a non-critical failure rather than leaving the
 *     device in a stuck half-provisioned state -- and the reboot is always
 *     scheduled afterward so the erased settings actually take effect. The
 *     first error encountered (if any) is still returned to the caller. */
typedef struct {
    void *context;
    app_error_code_t (*reset_settings_noncredential)(void *context);
    app_error_code_t (*erase_all_settings)(void *context);
    app_error_code_t (*invalidate_all_sessions)(void *context);
    app_error_code_t (*delete_all_blobs)(void *context);
    void (*schedule_restart)(void *context, uint32_t delay_ms);
} device_controls_reset_ops_t;

bool device_controls_reset_ops_is_valid(const device_controls_reset_ops_t *operations);

/* SPEC_V2.md §13.12 "restart": no settings, credential, or repository change
 * -- just a scheduled reboot. */
app_error_code_t device_controls_reset_engine_restart(const device_controls_reset_ops_t *operations,
                                                      uint32_t delay_ms);

/* SPEC_V2.md §13.12/§11.4 "reset-settings": applies the non-credential reset,
 * invalidates every session, and schedules a reboot. Aborts before touching
 * sessions or scheduling a reboot if the settings write itself fails. */
app_error_code_t
device_controls_reset_engine_reset_settings(const device_controls_reset_ops_t *operations,
                                            uint32_t delay_ms);

/* SPEC_V2.md §13.12/§11.4 "factory-reset": erases configuration/credentials/
 * provisioning state, invalidates sessions, deletes every repository blob, and
 * schedules a reboot into the unprovisioned state. Aborts before touching
 * sessions, blobs, or scheduling a reboot if erasing settings itself fails. */
app_error_code_t
device_controls_reset_engine_factory_reset(const device_controls_reset_ops_t *operations,
                                           uint32_t delay_ms);

#endif
