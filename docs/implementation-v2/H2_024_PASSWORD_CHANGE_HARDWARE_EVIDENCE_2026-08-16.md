# H2-024 — password-change hardware validation — 2026-08-16

## Status

**PASS.** All four H2-024 items and the Phase H2 exit gate are satisfied.

## Exact conditions

| | |
| --- | --- |
| Firmware Git SHA (flashed) | `897038f2c3dce3bda142c6ec1968339b3d738dbd` |
| Firmware build id | `189918701cff8b00df3775cad4c8b773d9e2d5e` |
| ESP-IDF | v5.5.5, target esp32s3 |
| Board | ESP32-S3R8, MAC `9c:13:9e:a8:77:38` |
| Host | Linux 7.0.0-28-generic x86_64, glibc 2.39 |
| Device address | 192.168.88.111 (station mode on the bench LAN) |
| PBKDF2 iterations | 5,500 (`AUTH_PBKDF2_ITERATIONS`) |
| Reset method | esptool RTS→EN-pin hardware reset (`resetReason: power_on`) |

## Command

```bash
python3 tests/hardware/test_password_change.py \
  --firmware-sha 897038f2c3dce3bda142c6ec1968339b3d738dbd \
  --port /dev/ttyACM0
```

## Captured output

```text
H2-024 password-change hardware validation
firmware_sha=897038f2c3dce3bda142c6ec1968339b3d738dbd
board=ESP32-S3R8
host=Linux-7.0.0-28-generic-x86_64-with-glibc2.39
device_ip=192.168.88.111
reset_port=/dev/ttyACM0

[1/4] change the password on the reference board
PASS: POST /api/v1/settings/change-password accepted
  new disposable password stored in the bench state directory

[2/4] old/new/session behaviour immediately, without a reboot
PASS: the old password is refused immediately
PASS: the new password is accepted immediately
PASS: the session that changed the password is invalidated immediately

[3/4] a power cycle preserves the new password
PASS: device returned after the hardware reset
PASS: the new password still works after reset
PASS: the old password is still refused after reset

[4/4] PBKDF2 timing sanity, 20 real logins
  samples=20
  min       442.9 ms   (V2-041 baseline   441.0 ms)
  median    522.9 ms   (V2-041 baseline   522.5 ms)
  p90       635.2 ms   (V2-041 baseline   757.2 ms)
  worst     807.8 ms   (V2-041 baseline   839.1 ms)
PASS: median 522.9 ms within 1045.0 ms (2.0x the V2-041 baseline)
PASS: all 20 timing logins succeeded

H2-024 hardware validation: PASS
```

## PBKDF2 timing against the recorded baseline

The baseline is V2-041's 2026-08-09 measurement on this same board: 20 real
logins, full round trip including network, PBKDF2 derivation and response.

| | This run | V2-041 baseline | Δ |
| --- | ---: | ---: | ---: |
| min | 442.9 ms | 441.0 ms | +1.9 |
| median | 522.9 ms | 522.5 ms | **+0.4** |
| p90 | 635.2 ms | 757.2 ms | −122.0 |
| worst | 807.8 ms | 839.1 ms | −31.3 |

A median 0.4 ms from a baseline taken a week earlier, with p90 and worst both
lower, is a clear absence of cost regression. The harness gate is deliberately
loose (2× baseline median): it exists to catch an accidental iteration-count or
algorithm change, not to benchmark, and bench Wi-Fi jitter is inside every
sample.

## What each item rests on

- *Repeat a successful password change* — step 1; the route returned success and
  the new disposable credential was stored in the bench state directory.
- *Old/new/session behaviour immediately, without a reboot* — step 2: the old
  password is refused, the new one accepted, and the very session that performed
  the change is invalidated, all without restarting the device.
- *A power cycle preserves the new password* — step 3, across an EN-pin hardware
  reset reporting `power_on`. Adequate here because the password write is
  acknowledged before the reset, so nothing is in flight; durability across true
  VBUS removal is V2-035/H5-055's concern, not this item's.
- *PBKDF2 timing sanity* — step 4, table above.

Exit gate: host and failure-injection coverage landed with H2-022/H2-023;
`./scripts/run-tests.sh --sanitizers` passes 66/66 at this tree; this document
is the hardware half.

## Two harness defects found and fixed while writing this

Both were in the new test, not the firmware, and both would have produced a
false result:

1. `session_still_valid()` used `try/except` around `Device.get()`. That
   client **returns** `(status, payload)` and does not raise on 4xx, so the
   helper reported every session as still valid and the run showed a spurious
   `FAIL: the session that changed the password is invalidated immediately`.
   Now it inspects the status. Worth noting because the failure mode was a
   *false negative* — it accused the firmware of a bug it did not have.
2. The reset shelled out to `sys.executable -m esptool`. The interpreter
   running the script is not the ESP-IDF one and has no esptool module. Now
   invokes `esptool.py` from the sourced IDF environment.
