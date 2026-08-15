# Hardware-in-the-loop checks

These scripts exercise the retained Phase 2 firmware on a real ESP32-S3. They
are not part of ordinary CI because they require a board, its UART console, a
Wi-Fi network, and in some cases a USB host capture setup.

## Current Phase 2 checks

### Provision a device

```bash
python3 tests/hardware/provision_device.py
```

The helper resets an unprovisioned board while capturing the fresh eight-digit
setup code from UART0, keeps that code in memory only, submits the current one-shot
`POST /api/v1/setup`, stores disposable normal-mode bench credentials outside the
repository, and verifies the authenticated service after restart.

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

### H12 final release smoke

After building/flashing a clean production image and generating
`firmware/build/flash-manifest.json`, run:

```bash
python3 tests/hardware/test_h12_release_smoke.py \
  --firmware-sha "$(git rev-parse HEAD)" \
  --flash-manifest firmware/build/flash-manifest.json \
  --allow-destructive
```

This fail-closed destructive smoke binds the live board to the exact clean
production manifest and exercises login, ordinary send, snapshot save/load/delete,
password change, software restart, factory reset, random UART setup-code
reprovisioning, and final production-image continuity. The separate
`test_send_confirmation.py` remains required for confirmation-required send,
cancel, timeout, and native USB HID capture.

`test_acceptance_reset.py` is intentionally retired because its old v1-shaped
setup and revisioned-settings calls no longer describe the current v2 API.

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
