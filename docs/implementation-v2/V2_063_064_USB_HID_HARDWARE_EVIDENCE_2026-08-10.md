# V2-063/V2-064 — USB HID hardware evidence, collected on physical ESP32-S3R8

**Date:** 2026-08-10
**Task:** V2-063's last item (real-device cancellation latency) and all of
V2-064 (Phase 6 — Macro compiler, executor, USB HID, and send lifecycle)
**Board:** ESP32-S3 (QFN56, chip revision v0.2), 8MB embedded PSRAM, MAC `9c:13:9e:a8:77:38`
**Firmware commit:** `7f322c1129daa5002dc2c7f8d3b48cae4926d947` (unchanged from
the V2-035 session — no firmware code changed for this track, only
`tests/hardware/` tooling)
**Diagnostics buildId:** `b75802e07` (confirmed matching before every test run)

Continues the same real-Wi-Fi station-mode approach as
`docs/implementation-v2/V2_041_HARDWARE_LOGIN_FIX_2026-08-09.md` and
`docs/implementation-v2/V2_035_STORAGE_HARDWARE_EVIDENCE_2026-08-10.md`.

## 1. `tests/hardware/` had the same class of contract drift as the V2-035 harness

`device_client.py`'s `login()` sent `{"password": ...}`; the real contract
requires `{"adminPassword": ...}` — the identical bug found and fixed in
`scripts/run-v2-035-hardware.py` the same session, independently reintroduced
here because the two tools don't share code. Fixed in `device_client.py`
(used by every script in the directory) and in
`test_network_security.py`'s deliberately-wrong-password test (which would
otherwise have gotten `400 invalid login request` instead of `401 invalid
credentials` on every attempt, never exercising the rate limiter it's meant
to test). `provision_device.py` and `test_acceptance_reset.py` have their own
separate, pre-existing `{"ok":true,"data":...}` v1-envelope assumptions
(same root cause as the V2-035 harness's `parse_success()` bug) — out of
scope here since neither was needed for this track (the device was already
provisioned from prior sessions), left as a known gap for whoever next uses
those two scripts.

A one-time local `sudo bash scripts/install-hid-udev-rule.sh` was also
required to read `/dev/hidraw*` as a non-root user. The rule itself computed
correctly (`GROUP=plugdev MODE=0640`, confirmed via `udevadm test`) but had
no effect on the already-enumerated device node until a fresh unplug/replug
forced real re-enumeration — `udevadm trigger` alone does not retroactively
fix permissions on a node udev already created.

## 2. USB identity (V2-064, item 1)

```text
$ lsusb -v -d 303a:4001
  idVendor           0x303a
  idProduct          0x4001
  iManufacturer           1 ESP32 Macro Keyboard Project
  iProduct                2 ESP32 Macro Keyboard
  iSerial                 3 ESP32S3-MACRO-01
```

Matches `SPEC_V2.md` §7.1 exactly (VID:PID, manufacturer, product, and serial
strings).

## 3. Real HID report capture (V2-064, items 2-3)

Built on `tests/hardware/hid_capture.py` (already correct — resolves the
right `/dev/hidraw*` node by VID:PID via sysfs, not a hardcoded path; no
JSON/envelope involved, so unaffected by the contract-drift bugs above).

Sent `"abcXYZ123!@#"` via `POST /api/v1/send` while capturing real raw HID
reports: decoded text matched the source exactly, 25 reports captured, final
report all-zero. Repeated post-reconnect (§7) with `"post-reconnect-ok"`:
same exact match.

## 4. Chord: modifier and usage set concurrently (V2-064, item 4)

Sent `"{CTRL+SHIFT+T}"`. Raw captured reports:

```text
t=+0.000  modifier=0x03 usage_bytes=[23, 0, 0, 0, 0, 0]
t=+0.016  modifier=0x00 usage_bytes=[0, 0, 0, 0, 0, 0]
t=+0.024  modifier=0x00 usage_bytes=[0, 0, 0, 0, 0, 0]
```

`0x03` = CTRL (bit 0) + SHIFT (bit 1), set in the *same* report as usage `23`
(0x17, HID usage for `t`) — proving the chord is encoded as one report with
both the modifier bitmap and the key usage concurrently, not as sequential
modifier-then-key reports. Followed immediately by an all-zero release.

## 5. Invalid source types nothing (V2-064, item 6)

Sent `"hello{NOTAREALDIRECTIVE}world"`:

```json
{"error":{"code":"macro_parse_error","message":"unknown key directive","field":"source","byteOffset":5,"line":1,"column":6}}
```

`HTTP 422`, and **zero** HID reports captured — confirming complete-before-execute:
not even the leading `"hello"` was typed before the parser hit the invalid
directive.

## 6. Cancellation during typing and delay, with real latency (V2-063 last item, V2-064 item 7)

**During typing:** sent 60 repeated `"a"` characters (`keyPressMs:20,
interKeyMs:30`, ~3s total), issued `DELETE /api/v1/send` after 500ms.
Result: `state:"cancelled"`, `error:"execution_cancelled"`, only 13 of 60
characters typed, final report all-zero.

Real-device last-keystroke latency (client-side cancel request to the last
observed non-zero HID report, i.e. including real HTTP/Wi-Fi round-trip time,
not just the firmware-internal cancellation slice): **93.5 ms**. This is a
different, and more representative, measurement than a pure host-side bound
would give — the firmware's own `CANCELLATION_SLICE_MS` (10 ms, from
`macro_executor_engine.c`, see
`docs/implementation-v2/V2_063_EXECUTOR_CANCELLATION_RESPONSIVENESS_2026-08-08.md`)
bounds how long the *executor* can take to notice cancellation once the
request lands; this number also includes the real network path to deliver
that request to the device.

**During a delay:** sent `"a{DELAY:5000}b"`, cancelled ~500ms in (during the
delay, before `b` would have started). Result: `state:"cancelled"`,
typed text was exactly `"a"` — `b` never executed — final report all-zero.
No additional release event was observable after the cancel request in this
case, correctly: only key/chord actions ever hold a key, and none was held
during the bare delay when cancellation landed.

## 7. Disconnect and reconnect (V2-064, item 8)

A real physical native-USB unplug/replug (not a software reset), captured
live by polling both `lsusb` and `GET /api/v1/status`'s `usb.state`
concurrently:

```text
native=yes bridge=yes usb.state=ready        (baseline)
native=no  bridge=yes usb.state=suspended    (native cable pulled)
native=yes bridge=yes usb.state=ready        (native cable restored)
```

Full functional recovery confirmed afterward, not just the status field: a
fresh `/dev/hidraw*` node was created by the re-enumeration (picked up
correctly by `hid_capture.py`'s VID:PID resolution, no hardcoded path to go
stale), and a real send (`"post-reconnect-ok"`) typed correctly with an
exact match and a clean all-zero termination (§3).

Getting a genuine native-only disconnect required identifying which of the
board's two USB cables is actually native versus the UART bridge by directly
observing `lsusb` through several rounds of trial — cable labeling was not
obvious from the physical setup, and manual identification (the intended
cable, described as "the real USB," was repeatedly actually the UART bridge
cable) took three attempts before landing on the correct one.

## 8. What this report does not claim

- Terminal-path all-zero verification was performed for **completed** and
  **cancelled** paths across four distinct real captures (text, chord,
  cancel-during-typing, cancel-during-delay). The **failed** and
  **timed_out** terminal paths were not independently re-verified on
  hardware in this pass — inducing a genuine mid-execution USB-level failure
  without conflating it with a full power cycle isn't achievable with this
  rig. Their all-zero-release behavior relies on the same shared
  `release_all()` code path already covered by host tests (see
  `docs/implementation-v2/V2_060_061_062_COMPILER_EXECUTOR_RELEASE_ALL_2026-08-08.md`).
- `provision_device.py` and `test_acceptance_reset.py`'s v1-envelope bugs
  (§1) are not fixed here; neither script was used or needed for this track.
- This closes V2-064 and V2-063's last item specifically. It does not touch
  Phase 15's larger on-device/HIL matrices.

## 9. Commands run

```bash
sudo bash scripts/install-hid-udev-rule.sh   # one-time, then a fresh replug
lsusb -v -d 303a:4001

cd tests/hardware
python3 -c "import device_client; ..."       # login/status dry-run
python3 -c "
import device_client, hid_capture
with device_client.Device() as dev:
    with hid_capture.Capture() as cap:
        dev.post('/api/v1/send', {'source': '...', 'keyPressMs': 8, 'interKeyMs': 15})
        # poll GET /api/v1/send until a terminal state
    cap.typed_text(); cap.ended_released()
"
# repeated with: printable text, a chord, an invalid directive,
# cancel-during-typing (DELETE /api/v1/send), cancel-during-delay,
# and again after a real native-USB unplug/replug.
```
