# Hardware-in-the-loop checks

These scripts exercise the retained Phase 2 firmware on a real ESP32-S3. They
are not part of ordinary CI because they require a board, its UART console, a
Wi-Fi network, and in some cases a USB host capture setup.

## Current Phase 2 checks

### Provision a device

```bash
python3 tests/hardware/provision_device.py
```

The helper performs first-run setup, stores disposable bench credentials outside
the repository, and waits for the normal authenticated service to return.

### Network authentication and retired-route boundary

```bash
python3 tests/hardware/test_network_security.py
```

This verifies authenticated settings access, refusal of missing or forged
sessions, login throttling, and HTTP 404 responses for the removed package,
execution, repository, and restore routes.

### HTTP confirmation concurrency

```bash
python3 tests/hardware/test_httpd_concurrency.py
```

This enables physical confirmation, holds a device-restart request until it
times out, verifies unrelated status requests remain responsive, confirms a
second waiter is refused, and checks that the server releases the request
socket. The script restores the prior setting afterward.

### End-to-end send confirmation (H1)

```bash
python3 tests/hardware/test_send_confirmation.py --firmware-sha <exact-git-sha>
```

This enables `requireSerialConfirmation`, captures the native USB HID stream,
proves zero key-down reports before the UART `confirm` command, proves the
confirmed send types exactly once, then verifies cancel-before-confirmation and
the real 60-second expiry path both type nothing. The prior setting is restored
on exit. This is intentionally a real 60-second timeout test rather than a
test-only shortened substitute.

### H12-122 final exact-release acceptance

Build the production firmware, web assets/webfs image, and flash manifest from
the exact clean candidate SHA using the repository's documented release build
workflow. Then run one fail-closed acceptance command:

```bash
python3 scripts/run-h12-122-hardware.py \
  --flash-manifest firmware/build/flash-manifest.json \
  --firmware-sha <exact-40-character-git-sha> \
  --flash-port /dev/ttyACM0 \
  --console /dev/ttyACM1 \
  --output docs/implementation-v2/hardware/H12_122_<sha>.json
```

The H12 harness invokes `scripts/flash-release-manifest.py` itself *before* any
HTTP or HID smoke step. Do not separately flash a different image between that
operation and the acceptance run. The flasher refuses a dirty/development/wrong-
SHA manifest, a source checkout that is not the same clean SHA, lockfile drift,
missing or mutated flash artifacts, path traversal, a test-app image, or a
release set that omits `webfs.bin`. Every flashed file is SHA-256 checked.

The bounded smoke then verifies manifest/diagnostics provenance, login, active
HID send, confirmation-gated HID send, cancellation, snapshot save/load,
password change and session invalidation, software restart, factory reset, fresh
UART `setup-code` reprovisioning, snapshot erasure, and that the same production
build remains on the board at sign-off. Restart must be proven by diagnostics
reset reason plus an uptime discontinuity; the runner deliberately refuses UART
Wi-Fi recovery on restart or on the post-setup reboot because that would hide a
station-persistence failure. Factory reset must visibly enter unprovisioned mode
on the trusted UART before reprovisioning, and the pre-reset administrator
password must be rejected afterward. Evidence output paths are immutable: choose
a new filename for every physical attempt. The emitted JSON contains non-secret
evidence only.

For a non-writing provenance preflight, `scripts/flash-release-manifest.py` also
supports `--dry-run`; that does **not** count as the H12-122 flash or hardware
acceptance.

## Deferred device checks

H1 now has a dedicated real-send confirmation/HID acceptance harness above.
Other hardware checks remain task-specific; package CRUD, package ordering,
package backup/restore, and stored-macro execution tests were deleted in Phase 2
because those firmware-owned resources no longer exist.

## Bench state and dependencies

`hil_state.py` stores generated secrets and the last known device address in a
state directory outside the repository. Install `pyserial` for UART operations.
The exact UART and native USB connectors are board-specific; the ESP32-S3 native
USB port must remain dedicated to TinyUSB HID while the separate UART bridge
provides the console.

No hardware result should be recorded from these scripts without the board
model, firmware SHA, host OS, ports used, and captured command output.
