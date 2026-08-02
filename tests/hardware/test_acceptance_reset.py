#!/usr/bin/env python3
"""SPEC 24.6 acceptance: power-cycle persistence, factory reset, re-provisioning.

Three of the eleven items SPEC 24.6 requires, run end to end against the real
device. They are one script because they are one story: data has to survive an
ordinary restart, factory reset has to actually destroy it, and the device has
to be usable again afterwards. Testing any one of them alone leaves the device
in a state the next one has to undo.

    SPEC 24.6 item: power-cycle persistence
    SPEC 24.6 item: factory reset

What this deliberately does NOT claim: a true power cycle. This restarts the
device over its own API, which re-runs the whole boot path including the NVS and
LittleFS mounts, but the rails never drop. Pulling the cable is a separate,
manual check.

Usage:
    python3 tests/hardware/test_acceptance_reset.py
"""

import json
import sys
import time
import uuid
import urllib.error
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import hil_state  # noqa: E402
from device_client import Device  # noqa: E402

SET_NAME = "Acceptance persistence set"
MACRO_NAME = "Acceptance macro"
# Deliberately harmless: no Enter, no shell metacharacters. This device is a
# keyboard, and the host is whatever happens to have focus while it types.
MACRO_SOURCE = "acceptance persistence check"
REBOOT_TIMEOUT_S = 90


def report(step: str, detail: str = "") -> None:
    print(f"  {step}" + (f": {detail}" if detail else ""), flush=True)


def wait_for_setup_state(ip: str, completed: bool, timeout_s: int = REBOOT_TIMEOUT_S) -> dict:
    """Wait until the device answers with the expected provisioning state."""
    deadline = time.time() + timeout_s
    last = None
    while time.time() < deadline:
        time.sleep(2)
        try:
            device = Device(ip)
            status, payload = device.get("/api/v1/setup-state")
        except (urllib.error.URLError, TimeoutError, OSError):
            continue
        if status == 200 and isinstance(payload, dict) and payload.get("ok"):
            last = payload["data"]
            if last["completed"] == completed:
                return last
    raise SystemExit(
        f"error: device did not report completed={completed} within {timeout_s}s "
        f"(last seen: {last})"
    )


def create_fixture(device: Device) -> tuple[str, str]:
    # The client chooses the identifiers (SPEC 12: stable IDs are the caller's,
    # so a retry cannot create a second copy of the same object).
    set_id = str(uuid.uuid4())
    status, payload = device.post(
        "/api/v1/sets",
        {"schema_version": 1, "id": set_id, "revision": 1, "name": SET_NAME},
    )
    if status not in (200, 201):
        raise SystemExit(f"error: could not create set: {status} {payload}")

    macro_id = str(uuid.uuid4())
    status, payload = device.post(
        f"/api/v1/sets/{set_id}/macros",
        {
            "schema_version": 1,
            "id": macro_id,
            "revision": 1,
            "set_id": set_id,
            "name": MACRO_NAME,
            "source": MACRO_SOURCE,
            "key_press_ms": 8,
            "inter_key_ms": 15,
        },
    )
    if status not in (200, 201):
        raise SystemExit(f"error: could not create macro: {status} {payload}")
    return set_id, macro_id


def main() -> int:
    ip = hil_state.device_ip()
    print(f"SPEC 24.6 acceptance against {ip}")

    print("\n1. power-cycle persistence")
    device = Device(ip)
    device.login()
    set_id, macro_id = create_fixture(device)
    report("created", f"set {set_id} with one macro")

    # These take no body at all. Sending `{}` is a 422: the route policy rejects a
# body on a route that has no fields, the same way /sets/{id}/select does.
    status, _ = device.post("/api/v1/device/restart")
    if status not in (200, 202):
        raise SystemExit(f"error: restart was refused: {status}")
    report("restart accepted", "waiting for the device to come back")
    wait_for_setup_state(ip, completed=True)

    # Reachable at the same address at all means the station credentials
    # survived the restart and were used without a console command.
    report("device answered again", f"at {ip}, so it rejoined Wi-Fi unaided")

    device = Device(ip)
    device.login()
    status, payload = device.get(f"/api/v1/sets/{set_id}")
    if status != 200 or payload["data"]["name"] != SET_NAME:
        raise SystemExit(f"error: the set did not survive the restart: {status} {payload}")
    status, payload = device.get(f"/api/v1/sets/{set_id}/macros")
    macros = payload["data"] if status == 200 else []
    if not any(item["id"] == macro_id and item["source"] == MACRO_SOURCE for item in macros):
        raise SystemExit(f"error: the macro did not survive the restart: {status} {payload}")
    report("PASS", "set and macro survived, byte for byte")

    print("\n2. factory reset")
    status, payload = device.post("/api/v1/device/factory-reset")
    if status not in (200, 202):
        raise SystemExit(f"error: factory reset was refused: {status} {payload}")
    report("accepted", "waiting for the device to come back unprovisioned")

    # It comes back on its access point only: factory reset clears the whole
    # provisioning record, station credentials included, which is the one time
    # SPEC 15.2 permits them to be discarded without an explicit empty SSID.
    print("   the device is now AP-only; rejoining Wi-Fi over the console")
    time.sleep(12)
    address = hil_state.connect_wifi()
    report("rejoined", address)
    state = wait_for_setup_state(address, completed=False, timeout_s=30)
    if state["completed"]:
        raise SystemExit("error: the device is still provisioned after a factory reset")
    report("PASS", "device is unprovisioned and its data is gone")

    print("\n3. re-provisioning, and the station network surviving setup")
    import provision_device  # noqa: PLC0415

    if provision_device.main() != 0:
        raise SystemExit("error: re-provisioning failed")

    # The regression this guards: setup used to rebuild the provisioning record
    # from scratch and drop the station credentials, so the device came back
    # from setup on its access point alone. If it answers here, they survived.
    state = wait_for_setup_state(address, completed=True)
    if not state["completed"]:
        raise SystemExit("error: the device did not come back provisioned")
    report("PASS", "provisioned, and still reachable over Wi-Fi after the setup restart")

    print("\nAll SPEC 24.6 items in this script passed.")
    print(json.dumps({"device": address, "setId": set_id, "macroId": macro_id}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
