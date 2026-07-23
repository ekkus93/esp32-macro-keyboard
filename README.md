# ESP32 Macro Keyboard

[![Host Tests](https://github.com/ekkus93/esp32-macro-keyboard/actions/workflows/host-tests.yml/badge.svg?branch=master)](https://github.com/ekkus93/esp32-macro-keyboard/actions/workflows/host-tests.yml)
[![Device Test Build](https://github.com/ekkus93/esp32-macro-keyboard/actions/workflows/device-tests-build.yml/badge.svg?branch=master)](https://github.com/ekkus93/esp32-macro-keyboard/actions/workflows/device-tests-build.yml)

ESP32-S3 firmware that enumerates as a native USB HID keyboard and serves a local,
mobile-first web application for managing and running explicit keyboard macros.

The authoritative design is in [`docs/SPEC.md`](docs/SPEC.md). The mandatory
implementation sequence is in [`docs/TODO.md`](docs/TODO.md).

## Repository status

The project is under active implementation. Hardware-dependent acceptance items
remain open until they are demonstrated on an ESP32-S3 and recorded in committed
test evidence.

Host tests run automatically for pushes to `master`, pull requests targeting
`master`, and tagged commits. Device-test firmware is linted and compiled for the
ESP32-S3 on the same events. Compiled assets and test logs are uploaded only for
tagged commits.

## Toolchain

- ESP-IDF `v5.5.5` exactly
- Target `esp32s3`
- Node.js `24.18.0`
- React, TypeScript, Tailwind CSS, and Vite

See [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md).

## Run the ESP32-S3 device tests

The device-test application uses ESP-IDF's Unity test menu. The current suite runs
on one ESP32-S3 and tests hardware-RNG UUID generation, UUID validation, macro
parsing and compilation, parser failure atomicity, and the authoritative firmware
limits.

Install and activate the pinned toolchain:

```bash
./scripts/install-esp-idf.sh
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
```

Build the test firmware:

```bash
bash ./scripts/build-device-tests.sh
```

Connect the ESP32-S3, replace the example serial port with the correct port for
your system, then flash and monitor it:

```bash
cd firmware/test_app
idf.py -B build -p /dev/ttyUSB0 flash monitor
```

Common Linux ports are `/dev/ttyUSB0` and `/dev/ttyACM0`. To leave the ESP-IDF
monitor, press `Ctrl+]`.

When the Unity test application is idle, press Enter to display the test menu.
Then enter one of these selectors:

```text
*                       Run every device test
[device]                Run every single-device test
[uuid]                  Run hardware-RNG and UUID tests
[macro_parser]          Run macro parser/compiler tests
[limits]                Run centralized-limit tests
```

A successful run ends with Unity reporting zero failures. Copy the complete serial
output when reporting a failure; do not omit warnings, resets, panic output, or the
first failing assertion.

These tests do not validate USB enumeration against a host, actual keyboard input,
Wi-Fi clients, browser workflows, physical buttons, or power interruption. Those
remain separate hardware-in-the-loop tests.
