# H10-103 — hardware matrix refresh — 2026-08-16

## Status

**PASS** for every item this document covers. Two items are honestly recorded as
**not performed** rather than claimed — see "Hosts not available".

## How the matrix is covered

Most of H10-103 was revalidated earlier today by dedicated phases rather than
duplicated here. This document covers only what those did not.

| Matrix item | Where it was revalidated |
| --- | --- |
| Linux HID identity | **here**, §1 |
| Linux HID text / release | H1-015 (`typed the exact expected text`, all-zero release report) |
| Linux HID chords | **here**, §2 |
| Linux HID cancel | H1-015 (`cancel-before-confirmation produced zero HID key-down reports`) |
| confirmation required / confirm / cancel / timeout | H1-015, all three scenarios incl. the real 60.4 s expiry |
| password change | H2-024, incl. survival across reset and PBKDF2 timing |
| factory reset / recovery | H3-035, incl. interruption mid-cleanup and boot resuming the reset |
| blob add / list / load / delete | V2-035 collector (`numeric_ordering`, `delete_preservation`) |
| interrupted upload / power cycle | V2-035 collector, real power loss 57,344 bytes in |
| USB disconnect / reconnect | **partially** — see "Honest limits" |
| AP survival after station failure | **here**, §3 |
| bounded reconnect | **here**, §3 and §4 |

## Exact conditions

| | |
| --- | --- |
| Flashed build | `fd0ddf76cba91a438270d61a51e85df0e4a18418`, clean tree, `gitDirty: false` |
| Recording SHA | `cb9b1573795a244b47ebdaa2aa8ba66a12b173f5` — zero files differ under `firmware/`, `webapp/`, `scripts/`, `tests/` or `contracts/` |
| Board | ESP32-S3R8, MAC `9c:13:9e:a8:77:38` |
| Command | `python3 tests/hardware/test_h10_matrix.py --firmware-sha <sha> --console /dev/ttyACM0` |

## Captured output

```text
H10-103 hardware matrix, items not covered elsewhere
firmware_sha=cb9b1573795a244b47ebdaa2aa8ba66a12b173f5
board=ESP32-S3R8
host=Linux-7.0.0-28-generic-x86_64-with-glibc2.39
device_ip=192.168.88.111
console=/dev/ttyACM0

[1/4] USB HID identity as the host sees it
PASS: device enumerates with project-owned VID:PID 303a:4001
       Bus 003 Device 014: ID 303a:4001 ESP32 Macro Keyboard Project ESP32 Macro Keyboard
PASS: HID descriptor exposes a product string ('ESP32 Macro Keyboard Project ESP32 Macro Keyboard')
PASS: product string is project-owned, not a vendor default

[2/4] chords emit modifier-bearing HID reports
PASS: POST /api/v1/send accepted a chord (HTTP 202)
PASS: a report carried a non-zero modifier byte
PASS: the modifier byte sets left-control (0x01)
PASS: the chord carried an ordinary key alongside the modifier
PASS: the chord ended with an all-zero release report

[3/4] SoftAP survives a failed station join, and the retry is bounded
PASS: SoftAP is ready before the failed join
PASS: the failed station join is reported as a failure, not silently retried
PASS: the join attempt was bounded (21.2s, not indefinite)
PASS: SoftAP is STILL ready after the station join failed
       before: keyboard>  

[4/4] the device reconnects to the real network afterwards
PASS: station rejoined the real network after the failure
PASS: authenticated service is reachable again on the LAN

H10-103 matrix: PASS
```

## What the new checks establish

**USB HID identity** is read from the host's view of the enumerated device, not
from the build: VID:PID `303a:4001` and the product string
`ESP32 Macro Keyboard Project ESP32 Macro Keyboard`. `check-usb-identity.sh`
asserts the descriptors at build time; this asserts what actually enumerated.

**Chords** are the one HID path plain text never exercises. `{CTRL+L}`
produced a report with modifier byte `0x01` (left-control) *and* an ordinary
key in the same report, terminated by an all-zero release. A chord that emitted
the modifier and key separately, or left the modifier held, would fail these.

**AP survival after station failure** is the item with the worst failure mode:
losing both the station link and the SoftAP is an unrecoverable lockout. The
SoftAP was `ready` before a deliberately-wrong-passphrase join, the join failed
explicitly rather than hanging or retrying silently, it was **bounded at 21.2 s**,
and the SoftAP was still `ready` afterwards.

**Bounded reconnect** then rejoined the real network and the authenticated
service returned on the LAN.

## Honest limits

**USB disconnect/reconnect is only partially covered.** Re-enumeration is
exercised every time the board resets — flashing, EN-pin resets and the factory
reset all force the host to re-enumerate the HID device, and it returns each
time, which this session did dozens of times. A *cable* disconnect while a send
is in flight is not covered: it needs a hand on the connector, and the bench hub
has no per-port power switching to simulate it. Recorded as partial rather than
claimed.

## Hosts not available

Per the TODO's "optional unavailable hosts remain honestly recorded":

- **ChromeOS: not performed.** No ChromeOS host is attached to this bench. No
  result is claimed.
- **Windows: not performed.** No Windows host is attached to this bench. No
  result is claimed.

Neither is inferred from the Linux result. The USB HID class behaviour is
host-side and cannot be established from another operating system's observation.

## Harness defects found while writing this

Both mine, both would have produced a false failure against the firmware:

1. The send was posted as `{"macroSource": …}`; the contract is
   `{"source", "keyPressMs", "interKeyMs"}`. The device correctly answered
   `400 invalid_argument`.
2. The console helper sent its command at the first `keyboard>` prompt, which
   appears while `app_core` is still wiring subsystems, so the reply was lost
   in boot output and `wifi-status` looked like a failure. It now waits for
   `Returned from app_main()` — the same trap recorded in H3's evidence.
