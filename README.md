# ESP32 Macro Keyboard

[![Host Tests](https://github.com/ekkus93/esp32-macro-keyboard/actions/workflows/host-tests.yml/badge.svg?branch=master)](https://github.com/ekkus93/esp32-macro-keyboard/actions/workflows/host-tests.yml)

ESP32-S3 firmware that enumerates as a native USB HID keyboard and serves a local,
mobile-first web application for managing and running explicit keyboard macros.

The authoritative design is in [`docs/SPEC.md`](docs/SPEC.md). The mandatory
implementation sequence is in [`docs/TODO.md`](docs/TODO.md).

## Repository status

The project is under active implementation. Hardware-dependent acceptance items
remain open until they are demonstrated on an ESP32-S3 and recorded in committed
test evidence.

Host tests run automatically for pushes to `master`, pull requests targeting
`master`, and tagged commits. Compiled host-test assets and test logs are uploaded
only for tagged commits.

## Toolchain

- ESP-IDF `v5.5.5` exactly
- Target `esp32s3`
- Node.js `24.18.0`
- React, TypeScript, Tailwind CSS, and Vite

See [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md).
