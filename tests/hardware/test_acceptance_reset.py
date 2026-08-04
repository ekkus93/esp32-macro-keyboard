#!/usr/bin/env python3
"""Hardware acceptance for restart, factory reset, and credential reset.

This test deliberately uses only retained Phase 2 state. It mutates the device
settings record, restarts the firmware, verifies persistence, then distinguishes
factory reset from credential reset by the provisioning revision history:

* factory reset erases the record, so setup creates revision 1;
* credential reset preserves the record, so clearing credentials and completing
  setup advance the prior revision by two.

The test uses a software restart rather than removing power. A true cable-pull
power-cycle remains a separate manual hardware check.
"""

import json
import sys
import time
import urllib.error
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import hil_state  # noqa: E402
import provision_device  # noqa: E402
from device_client import Device  # noqa: E402

REBOOT_TIMEOUT_S = 90


def report(step: str, detail: str = "") -> None:
    print(f"  {step}" + (f": {detail}" if detail else ""), flush=True)


def response_data(payload):
    if not isinstance(payload, dict):
        raise SystemExit(f"error: expected JSON object, got {payload!r}")
    return payload.get("data", payload)


def read_settings(device: Device) -> dict:
    status, payload = device.get("/api/v1/settings")
    if status != 200:
        raise SystemExit(f"error: could not read settings: HTTP {status} {payload}")
    settings = response_data(payload)
    required = {
        "schemaVersion",
        "revision",
        "requirePhysicalConfirmation",
        "alwaysSelectPackage",
    }
    if not required.issubset(settings):
        raise SystemExit(f"error: incomplete settings response: {payload}")
    return settings


def write_settings(device: Device, current: dict, *, always_select: bool) -> dict:
    status, payload = device.put(
        "/api/v1/settings",
        {
            "expectedRevision": current["revision"],
            "requirePhysicalConfirmation": current["requirePhysicalConfirmation"],
            "alwaysSelectPackage": always_select,
        },
    )
    if status != 200:
        raise SystemExit(f"error: could not update settings: HTTP {status} {payload}")
    return response_data(payload)


def post_expecting_reboot(device: Device, path: str) -> None:
    """POST a route whose response may be cut off by the requested restart."""
    try:
        status, payload = device.post(path)
    except (urllib.error.URLError, TimeoutError, OSError):
        return
    if status not in (200, 202):
        raise SystemExit(f"error: {path} was refused: HTTP {status} {payload}")


def console_command(command: str, seconds: int = 6) -> str:
    import serial  # noqa: PLC0415

    port = serial.Serial(hil_state.DEFAULT_CONSOLE, 115200, timeout=1)
    try:
        time.sleep(0.3)
        while port.in_waiting:
            port.read(port.in_waiting)
        port.write((command + "\n").encode())
        port.flush()
        deadline = time.time() + seconds
        data = b""
        while time.time() < deadline:
            chunk = port.read(4096)
            if chunk:
                data += chunk
    finally:
        port.close()
    return data.decode("utf-8", "replace")


def capture_boot_log(seconds: int = 20) -> str:
    import serial  # noqa: PLC0415

    port = serial.Serial(hil_state.DEFAULT_CONSOLE, 115200, timeout=1)
    try:
        port.setDTR(False)
        port.setRTS(True)
        time.sleep(0.2)
        port.setRTS(False)
        deadline = time.time() + seconds
        data = b""
        while time.time() < deadline:
            chunk = port.read(4096)
            if chunk:
                data += chunk
    finally:
        port.close()
    return data.decode("utf-8", "replace")


def rejoin_wifi(attempts: int = 4) -> str:
    last = None
    for attempt in range(1, attempts + 1):
        try:
            return hil_state.connect_wifi()
        except SystemExit as error:
            last = error
            report(f"join attempt {attempt} failed", "retrying")
            time.sleep(5)
    raise SystemExit(f"error: could not rejoin Wi-Fi after {attempts} attempts: {last}")


def wait_for_provisioning(
    ip: str, provisioned: bool, timeout_s: int = REBOOT_TIMEOUT_S
) -> None:
    deadline = time.time() + timeout_s
    last = None
    while time.time() < deadline:
        time.sleep(2)
        try:
            status, payload = Device(ip).get("/api/v1/setup-state")
        except (urllib.error.URLError, TimeoutError, OSError):
            continue
        last = (status, payload)
        if provisioned and status == 404:
            return
        if (
            not provisioned
            and status == 200
            and isinstance(payload, dict)
            and payload.get("ok")
            and not payload["data"]["completed"]
        ):
            return
    raise SystemExit(
        f"error: device did not reach provisioned={provisioned} within "
        f"{timeout_s}s (last seen: {last})"
    )


def ensure_provisioned(ip: str) -> None:
    status, _ = Device(ip).get("/api/v1/setup-state")
    if status == 404:
        return
    report("device is unprovisioned", "running setup first")
    if provision_device.main() != 0:
        raise SystemExit("error: could not provision the device")
    wait_for_provisioning(ip, provisioned=True)


def reconnect_after_restart(ip: str, *, provisioned: bool) -> str:
    try:
        wait_for_provisioning(ip, provisioned=provisioned, timeout_s=60)
        return ip
    except SystemExit:
        report("device did not rejoin unaided", "rejoining over the console")
        address = rejoin_wifi()
        wait_for_provisioning(address, provisioned=provisioned, timeout_s=60)
        return address


def main() -> int:
    address = hil_state.device_ip()
    print(f"hardware reset acceptance against {address}")
    ensure_provisioned(address)

    print("\n1. restart persistence")
    device = Device(address)
    device.login()
    original = read_settings(device)
    marker = write_settings(
        device, original, always_select=not original["alwaysSelectPackage"]
    )
    report(
        "settings marker committed",
        f"revision {marker['revision']}, alwaysSelectPackage="
        f"{marker['alwaysSelectPackage']}",
    )
    post_expecting_reboot(device, "/api/v1/device/restart")
    address = reconnect_after_restart(address, provisioned=True)

    device = Device(address)
    device.login()
    persisted = read_settings(device)
    if persisted != marker:
        raise SystemExit(
            f"error: settings changed across restart: before={marker}, after={persisted}"
        )
    report("PASS", "settings revision and values survived the restart")

    print("\n2. factory reset")
    post_expecting_reboot(device, "/api/v1/device/factory-reset")
    time.sleep(12)
    address = rejoin_wifi()
    wait_for_provisioning(address, provisioned=False, timeout_s=45)
    report("device is unprovisioned", "the provisioning record was erased")

    if provision_device.main() != 0:
        raise SystemExit("error: re-provisioning after factory reset failed")
    wait_for_provisioning(address, provisioned=True)
    device = Device(address)
    device.login()
    after_factory = read_settings(device)
    if after_factory["revision"] != 1:
        raise SystemExit(
            "error: factory reset did not restart provisioning revision history: "
            f"{after_factory}"
        )
    report("PASS", "setup recreated a fresh revision-1 provisioning record")

    print("\n3. saved station network survives setup")
    log = capture_boot_log()
    entered_station_mode = "mode : sta" in log
    joined = "joined saved Wi-Fi network" in log
    if not entered_station_mode:
        raise SystemExit(
            "error: the device did not attempt its saved station network after setup:\n"
            + log[-600:]
        )
    report(
        "PASS",
        "the device attempted the saved network"
        + (" and joined it" if joined else " (association failed this boot)"),
    )
    address = reconnect_after_restart(address, provisioned=True)

    print("\n4. credential reset")
    device = Device(address)
    device.login()
    before_credentials = read_settings(device)

    refusal = console_command("credential-reset")
    if "usage:" not in refusal:
        raise SystemExit(f"error: bare credential-reset was not refused:\n{refusal}")
    report("bare command refused", "confirmation word required")

    output = console_command("credential-reset confirm", seconds=8)
    if "credentials cleared" not in output:
        raise SystemExit(f"error: credential reset did not report success:\n{output}")
    address = reconnect_after_restart(address, provisioned=False)
    report("device is unprovisioned", "saved network remained available")

    if provision_device.main() != 0:
        raise SystemExit("error: re-provisioning after credential reset failed")
    wait_for_provisioning(address, provisioned=True)
    device = Device(address)
    device.login()
    after_credentials = read_settings(device)
    expected_revision = before_credentials["revision"] + 2
    if after_credentials["revision"] != expected_revision:
        raise SystemExit(
            "error: credential reset did not preserve provisioning revision history: "
            f"before={before_credentials['revision']}, after="
            f"{after_credentials['revision']}, expected={expected_revision}"
        )
    report(
        "PASS",
        "credential clear and setup advanced the retained record by two revisions",
    )

    print("\nAll retained reset acceptance checks passed.")
    print(
        json.dumps(
            {
                "device": address,
                "restartRevision": marker["revision"],
                "factoryResetRevision": after_factory["revision"],
                "credentialResetRevision": after_credentials["revision"],
            },
            indent=2,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
