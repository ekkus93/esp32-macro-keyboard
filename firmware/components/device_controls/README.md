# Device Controls Component

This component owns status indication and the confirmation signal for the
ESP32-S3 device.

**There are no buttons.** The confirm and cancel buttons this component used to
poll were specified for hardware no board here exposes - cancel defaulted to
GPIO4, which a bare devkit does not break out - and requiring them made the
device unusable. Their two functions are now serial-console commands, `confirm`
and `cancel` (see `components/serial_console`). Credential-reset and
factory-reset gestures were never implemented and are not planned; a reset is a
network request like any other.

What remains: thread-safe status LED updates, and a confirmation signal that
`device_controls_signal_confirmation()` gives on behalf of the `confirm`
command. Physical confirmation is off by default, so nothing on the device
requires it at all.

## Device actions

This component also owns the three SPEC_V2.md §13.12 device actions —
`device_controls_restart()`, `device_controls_reset_settings()`, and
`device_controls_factory_reset()` — as a network request like any other; there
is still no physical reset gesture. Each validates nothing itself (the caller,
e.g. the future HTTP handler, checks the confirmation phrase and, for factory
reset, the admin password *before* calling in, per SPEC_V2.md §13.12: "The
destructive operation MUST NOT begin until the complete request, password, and
confirmation phrase have been validated") and then:

- **restart** only schedules a reboot;
- **reset-settings** applies SPEC_V2.md §11.4 (device name, confirmation
  requirement, send mode, retention target, source-preview setting, and
  last-selected package reset to defaults; station Wi-Fi credentials removed;
  AP credentials, admin password, provisioning state, and repository blobs
  preserved), then invalidates every session, then reboots;
- **factory-reset** erases device configuration, credentials, and
  provisioning state, invalidates every session, deletes every repository
  blob, then reboots into the unprovisioned state.

The ordering and continue-past-non-critical-failure contract is documented in
`device_controls_reset.h` and exercised by
`tests/host/test_device_controls_reset.c` against fakes, independent of NVS,
FreeRTOS, and `esp_restart()`. The reboot itself is deferred by
`DEVICE_CONTROLS_RESTART_DELAY_MS` via a one-shot `esp_timer` so an HTTP
caller's "accepted" response has time to leave the socket before the
connection drops.
