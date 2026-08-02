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

Host tests, AddressSanitizer/UBSan, native coverage, and the frontend suite run
automatically for pushes to `master`, pull requests targeting `master`, and tagged
commits. Device-test firmware is linted and compiled for the ESP32-S3 on the same
events. Compiled assets and test logs are uploaded only for tagged commits.

Per-capability validation state — host-tested, sanitizer-tested, coverage-gated,
frontend-tested, device-build-tested, device-executed, and HIL-verified — is tracked
in [`docs/UNIT_TESTS1_PROGRESS.md`](docs/UNIT_TESTS1_PROGRESS.md) and
[`docs/IMPLEMENTATION_STATUS.md`](docs/IMPLEMENTATION_STATUS.md). Exactly one
capability has been device-executed so far (production firmware's heap and
task-stack high-water marks, read from a real ESP32-S3's serial console); no
capability is currently claimed HIL-verified, and the on-device Unity test
menu itself has not yet been run on hardware — see
[`docs/IMPLEMENTATION_STATUS.md`](docs/IMPLEMENTATION_STATUS.md) for exactly
what that one device-executed result covers.

## Known product limitation: unauthenticated serial console

Every build currently includes an interactive command console on UART0
(`wifi-connect`, `wifi-status`). It accepts commands with **no session, CSRF
token, or physical-button confirmation** — possession of the board and access
to its UART port is the authorization.

This is deliberate. Reaching that port means holding the hardware, which
already allows reflashing the device outright, so authenticating it would add
friction without adding protection. The network surface is held to the
opposite standard: every Wi-Fi-reachable route requires a valid session, and
mutations additionally require a matching CSRF token and accepted
`Host`/`Origin`, with rate-limited authentication. See
[`docs/SPEC.md`](docs/SPEC.md) §16.5.

Before any release to third parties this console must be excluded from the
shipped image, because a shipped device's physical surface belongs to its
user rather than its developer.

## Toolchain

- ESP-IDF `v5.5.5` exactly
- Target `esp32s3`
- Node.js `24.18.0`
- React, TypeScript, Tailwind CSS, and Vite

See [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md).

## Run the ESP32-S3 device tests

The device-test application uses ESP-IDF's Unity test menu. The current suite runs
on one ESP32-S3 and tests hardware-RNG UUID generation, UUID validation, macro
parsing and compilation, parser failure atomicity, the authoritative firmware
limits, authentication adapters, the macro executor's idle/USB-not-ready behavior,
and USB keyboard state initialization. The firmware is device-build-tested (it
compiles for the ESP32-S3 in CI); running it on hardware and reviewing serial
output is separate, not-yet-completed work.

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
[auth]                  Run authentication adapter tests
[executor]              Run macro executor state tests
[usb]                   Run USB keyboard state tests
```

A successful run ends with Unity reporting zero failures. Copy the complete serial
output when reporting a failure; do not omit warnings, resets, panic output, or the
first failing assertion.

These tests do not validate USB enumeration against a host, actual keyboard input,
Wi-Fi clients, browser workflows, physical buttons, or power interruption. Those
remain separate hardware-in-the-loop tests.
