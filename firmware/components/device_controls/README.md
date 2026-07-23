# Device Controls Component

This component owns physical input and status indication for the ESP32-S3 device.

Implemented foundations include debounced button handling, bounded confirmation
and cancellation signaling, and thread-safe status LED updates. Cancellation while
the executor is idle must not manufacture a fatal state.

Credential-reset and factory-reset gestures remain unimplemented and require
separate timing, confirmation, and hardware tests before they may be documented as
available. Normal short presses must never trigger a destructive reset.
