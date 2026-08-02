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
