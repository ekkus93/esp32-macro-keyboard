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

### Restart and reset acceptance

```bash
python3 tests/hardware/test_acceptance_reset.py
```

This verifies retained settings across a software restart, factory-reset record
erasure, re-provisioning, saved station-network behavior, confirmation-gated
credential reset, and provisioning revision continuity. The script does not
claim a true power-removal cycle; unplug/replug testing remains manual.

## Deferred device checks

Direct macro text submission, HID typing capture, and in-flight cancellation
return with the V2 `/api/v1/send` endpoint in a later phase. Package CRUD,
package ordering, package backup/restore, and stored-macro execution tests were
deleted in Phase 2 because those firmware-owned resources no longer exist.

## Bench state and dependencies

`hil_state.py` stores generated secrets and the last known device address in a
state directory outside the repository. Install `pyserial` for UART operations.
The exact UART and native USB connectors are board-specific; the ESP32-S3 native
USB port must remain dedicated to TinyUSB HID while the separate UART bridge
provides the console.

No hardware result should be recorded from these scripts without the board
model, firmware SHA, host OS, ports used, and captured command output.
