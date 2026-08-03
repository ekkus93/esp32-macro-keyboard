#!/usr/bin/env python3
"""Hardware typing tests: submit real macros and verify the exact USB HID
reports the device emits.

Evidence comes from the kernel's hidraw node, decoded as 8-byte boot-protocol
reports -- the bytes that went down the wire, not text scraped from an editor.
That is what makes it possible to assert things a screen capture cannot: that a
chord package the modifier bit concurrently with the usage code, and that the final
report is all-zero with no key left held.
"""

import sys
import time

import hil_state
from device_client import Device
from hid_capture import Capture



def submit_and_capture(device, macro, settle=6.0):
    """Run one macro, capturing HID reports for its whole execution."""
    with Capture() as capture:
        status, payload = device.post(
            "/api/v1/executions",
            {
                "packageId": macro["package_id"],
                "macroId": macro["id"],
                "macroRevision": macro["revision"],
            },
        )
        if status != 202:
            return None, f"submit failed: HTTP {status} {str(payload)[:160]}"
        deadline = time.time() + settle
        while time.time() < deadline:
            time.sleep(0.3)
            state = device.get("/api/v1/executions/current")[1]["data"]
            if state["state"] in ("completed", "cancelled", "failed", "timed_out"):
                time.sleep(0.4)   # let trailing reports arrive
                return capture, state
    return capture, {"state": "timeout-waiting"}


def main():
    fixture = hil_state.fixture()
    package_id = fixture["package_id"]
    device = Device()
    device.login()

    results = {}

    # --- 1. printable text -------------------------------------------------
    macro = dict(fixture["macros"]["text"], package_id=package_id)
    capture, state = submit_and_capture(device, macro)
    if capture is None:
        print(f"[FAIL] printable text: {state}")
        results["text"] = False
    else:
        typed = capture.typed_text()
        expected = macro["source"]
        ok = typed == expected
        results["text"] = ok and capture.ended_released()
        print(f"[{'PASS' if results['text'] else 'FAIL'}] printable text")
        print(f"        expected {expected!r}")
        print(f"        typed    {typed!r}")
        print(f"        final report all-zero (release-all): {capture.ended_released()}")
        print(f"        terminal state: {state.get('state')}")

    time.sleep(1)

    # --- 2. chord ----------------------------------------------------------
    macro = dict(fixture["macros"]["chord"], package_id=package_id)
    capture, state = submit_and_capture(device, macro)
    if capture is None:
        print(f"[FAIL] chord: {state}")
        results["chord"] = False
    else:
        events = capture.events()
        downs = [e for e in events if e[1] == "down"]
        ok = any("CTRL" in e[2] and e[2].endswith("a") for e in downs)
        results["chord"] = ok and capture.ended_released()
        print(f"\n[{'PASS' if results['chord'] else 'FAIL'}] chord {macro['source']}")
        for e in events:
            print(f"        {e[0]:>6}s  {e[1]:<12} {e[2]}")

    print("\n" + "=" * 58)
    for name, ok in results.items():
        print(f"  {name:<16} {'PASS' if ok else 'FAIL'}")
    device.logout()
    return 0 if all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
