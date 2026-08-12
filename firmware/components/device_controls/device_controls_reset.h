#ifndef DEVICE_CONTROLS_RESET_H
#define DEVICE_CONTROLS_RESET_H

#include <stdbool.h>
#include <stdint.h>

#include "app_error.h"

/* Host-testable orchestration behind the SPEC_V2.md §13.12 device actions
 * (restart, reset-settings, factory-reset). Each op is a single side effect;
 * the hardware adapter (device_controls.c) wires them to device_settings,
 * auth, storage_blob, factory_reset_state, and delayed esp_restart().
 *
 * Factory reset owns one additional H3 invariant: the durable PENDING marker is
 * committed before the first destructive effect. After that commit, any later
 * failure keeps the marker and still schedules reboot so boot cannot resume
 * ordinary setup/normal service with ambiguous reset state. The marker clears
 * only after settings, session, and blob cleanup all report success. */
typedef struct {
    void *context;
    app_error_code_t (*reset_settings_noncredential)(void *context);
    app_error_code_t (*mark_factory_reset_pending)(void *context);
    app_error_code_t (*erase_all_settings)(void *context);
    app_error_code_t (*invalidate_all_sessions)(void *context);
    app_error_code_t (*delete_all_blobs)(void *context);
    app_error_code_t (*clear_factory_reset_pending)(void *context);
    void (*schedule_restart)(void *context, uint32_t delay_ms);
} device_controls_reset_ops_t;

bool device_controls_reset_ops_is_valid(const device_controls_reset_ops_t *operations);

/* SPEC_V2.md §13.12 "restart": no settings, credential, repository, or reset
 * journal change -- just a scheduled reboot. */
app_error_code_t device_controls_reset_engine_restart(const device_controls_reset_ops_t *operations,
                                                      uint32_t delay_ms);

/* SPEC_V2.md §13.12/§11.4 "reset-settings": applies the non-credential reset,
 * invalidates every session, and schedules a reboot. The H3 factory-reset
 * journal is intentionally untouched. */
app_error_code_t
device_controls_reset_engine_reset_settings(const device_controls_reset_ops_t *operations,
                                            uint32_t delay_ms);

/* H3 factory-reset transaction boundary:
 *   1. durably mark PENDING,
 *   2. erase settings,
 *   3. invalidate sessions and delete repository blobs,
 *   4. clear PENDING only if every required destructive effect succeeded,
 *   5. reboot after every post-marker outcome.
 *
 * A marker-write failure aborts before any destructive effect. Any later
 * failure deliberately leaves PENDING durable so the next boot fails closed
 * into reset recovery rather than pretending reset completed. */
app_error_code_t
device_controls_reset_engine_factory_reset(const device_controls_reset_ops_t *operations,
                                           uint32_t delay_ms);

#endif
