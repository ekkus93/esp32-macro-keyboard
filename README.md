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
[`docs/IMPLEMENTATION_STATUS.md`](docs/IMPLEMENTATION_STATUS.md).

**Verified on real hardware (2026-08-02),** by the scripts in `tests/hardware/`
against an attached ESP32-S3:

- **typing**, read back from the kernel's `hidraw` node as USB HID reports — the
  bytes on the wire, not text captured from an editor: printable text arrives
  exactly, a chord sets the modifier bit concurrently with the usage code, and
  every run ends with an all-zero report so no key is left held;
- **a whole package sent in the order the user arranged it**, arriving in that order;
- **cancellation over both paths the specification requires** — the HTTP API and
  the console `cancel` command — during a delay and mid-typing, each reaching
  `cancelled` with the last keystroke 92–127 ms after the request;
- **power-cycle persistence, factory reset, credential reset, and
  re-provisioning**, including that a saved Wi-Fi network survives setup and a
  credential reset but not a factory reset, and that the device rejoins it
  unaided about 12 s after a reboot;
- **all three ways a package is applied** — whole-repository restore reporting
  per-package outcomes, import as a new package, and replacing an existing package's
  contents — against a real repository, on the device.

Still not verified on hardware: the on-device Unity test menu, repeated USB and
access-point reconnects, a firmware slot switch, and any host other than Linux.
Those are the open items in `docs/SPEC.md` §24.6.

## Known product limitation: unauthenticated serial console

Every build currently includes an interactive command console on UART0
(`wifi-connect`, `wifi-status`). It accepts commands with **no session or
physical-button confirmation** — possession of the board and access
to its UART port is the authorization.

This is deliberate. Reaching that port means holding the hardware, which
already allows reflashing the device outright, so authenticating it would add
friction without adding protection. The network surface is held to the
opposite standard: every Wi-Fi-reachable route requires a valid session, and
authentication is rate-limited. The session cookie is `HttpOnly` and
`SameSite=Strict`, which is what stops another site driving the device through
a browser (SPEC §16.2). See
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

## Build and test

Everything below runs from the repository root and needs no hardware. Run
scripts rather than the underlying tools directly — they pin versions and are
what CI calls.

```bash
./scripts/check-all.sh          # everything, in the order CI runs it (several minutes)
```

That is the gate a change has to pass. While iterating, use the narrower loops:

| Suite | Where the tests live | Run just this |
| --- | --- | --- |
| Host C — 52 `test_*.c` plus 20 `.inc` fragments | `tests/host/` | `./scripts/run-tests.sh` |
| Frontend — 17 vitest files, 118 tests, ~2s | `webapp/tests/` (**not** `webapp/src/`) | `npm --prefix webapp run test` |
| Browser (Playwright) | `webapp/tests/browser/` | `npm --prefix webapp run test:browser` |
| On-device Unity | `firmware/test_app/` | see the next section |
| Hardware-in-the-loop (Python) | `tests/hardware/` | needs the board attached |

```bash
./scripts/run-tests.sh storage            # one label: support parser storage executor auth
                                          # web startup usb controls wifi model
./scripts/run-tests.sh --sanitizers       # ASan + UBSan
./scripts/run-tests.sh --coverage         # gcovr; one mode and one label at most
./scripts/check-webapp.sh                 # full frontend chain: ci, typecheck, lint,
                                          # stylelint, test, build, local-assets
./scripts/check-firmware.sh               # ESP-IDF build plus clang-tidy
```

`check-webapp.sh` runs the whole frontend chain and is what to run before
committing; `npm --prefix webapp run test` is the inner loop.

The first frontend run needs dependencies installed, and Node must be exactly
the pinned version:

```bash
nvm use                                   # reads .nvmrc (24.18.0)
npm --prefix webapp ci
```

Firmware and device commands need the pinned ESP-IDF on `PATH` first:

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
```

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
idf.py -B build -p /dev/ttyACM0 flash monitor
```

To leave the ESP-IDF monitor, press `Ctrl+]`.

**The two USB connectors are not interchangeable.** Identify them by vendor ID
rather than by device path, because the numbering depends on the order things
were plugged in:

```bash
lsusb | grep -E '303a|1a86|10c4'
```

| Port | Enumerates as | Use it for |
| --- | --- | --- |
| Native USB (the ESP32-S3's own peripheral) | `303a:4001` running the app, `303a:1001` otherwise | flashing, `esptool`, HID validation, log output |
| USB-UART bridge (a separate CH340/CP210x chip) | `1a86:55d3` or `10c4:ea60` | **the interactive serial console** |

The production firmware packages `CONFIG_ESP_CONSOLE_UART_DEFAULT=y`, so the
interactive console (`wifi-connect`, `wifi-status`, `confirm`, `cancel`) reads
from UART0 on the bridge. USB-Serial-JTAG is the secondary console: it mirrors
log output but accepts no input, and commands sent to it are silently discarded.

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
