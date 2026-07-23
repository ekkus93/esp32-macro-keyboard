# ESP32 Macro Keyboard

ESP32-S3 firmware that enumerates as a native USB HID keyboard and serves a local,
mobile-first web application for managing and running explicit keyboard macros.

The authoritative design is in [`docs/SPEC.md`](docs/SPEC.md). The mandatory
implementation sequence is in [`docs/TODO.md`](docs/TODO.md).

## Repository status

The project is under active implementation. Hardware-dependent acceptance items
remain open until they are demonstrated on an ESP32-S3 and recorded in committed
test evidence.

## Toolchain

- ESP-IDF `v5.5.5` exactly
- Target `esp32s3`
- Node.js `24.18.0`
- React, TypeScript, Tailwind CSS, and Vite

See [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md).
