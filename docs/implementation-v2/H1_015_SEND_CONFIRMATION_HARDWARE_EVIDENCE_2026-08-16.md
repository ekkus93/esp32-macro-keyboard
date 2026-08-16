# H1-015 — End-to-end send-confirmation hardware evidence — 2026-08-16

## Status

**PASS.** All six H1-015 checkboxes and both Phase H1 exit-gate items are
satisfied by the run captured below, performed on the reference board.

## Exact conditions

| | |
| --- | --- |
| Firmware Git SHA (flashed) | `897038f2c3dce3bda142c6ec1968339b3d738dbd` |
| Firmware build id | `189918701cff8b00df3775cad4c8b773d9e2d5e` |
| ESP-IDF | v5.5.5, target esp32s3 |
| Board | ESP32-S3R8 (QFN56 rev v0.2, 8 MB octal PSRAM), MAC `9c:13:9e:a8:77:38` |
| Host | Linux 7.0.0-28-generic x86_64, glibc 2.39 |
| Device address | 192.168.88.111 (station mode on the bench LAN) |
| UART console | `/dev/ttyACM0` — the CH340 bridge, **not** native USB |
| HID capture | `/dev/hidraw6`, auto-resolved by VID/PID `303A:4001` |
| Setting under test | `requireSerialConfirmation` enabled by the harness, prior value restored on exit |

The flashed image is exactly what `HEAD` builds: `git diff --name-only`
between `897038f` and the recording commit touches no file under
`firmware/`, `webapp/`, `scripts/`, `tests/` or `contracts/`. Board
identity was confirmed independently at flash time from the boot-log ELF SHA and
from authenticated `/api/v1/diagnostics`.

## Command

```bash
python3 tests/hardware/test_send_confirmation.py \
  --firmware-sha 897038f2c3dce3bda142c6ec1968339b3d738dbd \
  --console /dev/ttyACM0
```

## Captured output

```text
SHA under test: 897038f2c3dce3bda142c6ec1968339b3d738dbd
host console: /dev/ttyACM0 (CH340)  device: 192.168.88.111  hidraw: auto-resolved
H1 real-send confirmation hardware acceptance
firmware_sha=897038f2c3dce3bda142c6ec1968339b3d738dbd
board=ESP32-S3R8
host=Linux-7.0.0-28-generic-x86_64-with-glibc2.39
console=/dev/ttyACM0
device_ip=192.168.88.111
PASS: settings update response contains settings
PASS: requireSerialConfirmation enabled

[1/3] confirmation gates HID output
PASS: POST /api/v1/send accepted (202)
PASS: accepted response is an object
PASS: accepted response reports awaiting_confirmation
PASS: GET remains awaiting before confirm
PASS: zero HID key-down reports before confirm
PASS: serial confirm transitions send to completed
PASS: confirmed send typed the exact expected text
PASS: confirmed send ended with an all-zero release report

[2/3] cancel before confirmation types nothing
PASS: POST /api/v1/send accepted (202)
PASS: accepted response is an object
PASS: accepted response reports awaiting_confirmation
PASS: zero HID key-down reports before cancellation
PASS: DELETE /api/v1/send accepted (202)
PASS: cancel-before-confirmation reaches cancelled
PASS: cancel-before-confirmation produced zero HID key-down reports
PASS: cancel-before-confirmation typed nothing

[3/3] real 60-second confirmation expiry types nothing
PASS: POST /api/v1/send accepted (202)
PASS: accepted response is an object
PASS: accepted response reports awaiting_confirmation
PASS: unconfirmed send reaches timed_out
PASS: hardware timeout was not shortened (60.4s observed)
PASS: hardware timeout remained bounded (60.4s observed)
PASS: timeout path produced zero HID key-down reports
PASS: timeout path typed nothing

H1 hardware acceptance: PASS
```

## What each checkbox rests on

- *Enable the confirmation setting* — harness enables `requireSerialConfirmation`
  and restores the prior value on exit.
- *Zero key-down before confirmation* — scenario 1, with the send held at
  `awaiting_confirmation` and the HID stream captured throughout.
- *Confirm and capture the expected reports* — scenario 1 typed the exact
  expected text and ended with an all-zero release report, so no key was left
  held.
- *Cancel before confirmation types nothing* — scenario 2.
- *Expiry/timeout types nothing* — scenario 3, and the harness asserts the
  timeout was **not** shortened: 60.4 s observed against the real 60 s bound.
- *Record SHA, board, host, settings, commands, output* — this document.

Phase H1 exit gate:

- *Real `POST /api/v1/send` honors physical/serial confirmation end-to-end* —
  proven above against the live HTTP API and the real UART `confirm`.
- *Host, sanitizer, browser, and hardware evidence committed* — host and browser
  coverage landed with H1-013/H1-014; `./scripts/run-tests.sh --sanitizers`
  passes 66/66 at this tree; this document is the hardware half.

## Harness defect found and fixed during this run

The first attempt failed in the capture harness, not the firmware:
`RuntimeError: HID reader for /dev/hidraw6 did not stop`.
`Capture._run()` blocked in `read(8)`, which parks forever when the device
sends nothing, so the reader never observed its stop flag and `__exit__`'s
join timed out — failing precisely the scenarios whose point is that the device
types nothing. Fixed in `3d2e5f3` by `select()`ing with a timeout before
reading and joining the thread before closing the handle. Scenario 1 had already
passed before the fix; all three pass after it.
