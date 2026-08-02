#!/usr/bin/env python3
"""Send every macro in a set, in its stored order, and read back the wire.

    SPEC 24.6 item: a full set of macros sent in order against a harmless text target

The other typing test sends one macro at a time and checks each in isolation.
This one checks the property that only shows up across a whole set: that the
order the user arranged is the order the target computer receives (SPEC 7.1),
and that nothing leaks between runs -- no key still held from the previous
macro, no report arriving after a macro reported completion.

The "harmless text target" is this: every macro types lowercase letters and
spaces only. No Enter, no modifiers, no shell metacharacters. The device is a
keyboard and whatever has focus is the target, so a test that types into it
must be safe to run with a terminal focused.

Usage:
    python3 tests/hardware/test_full_set_in_order.py
"""

import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import hil_state  # noqa: E402
from device_client import Device  # noqa: E402
from hid_capture import Capture  # noqa: E402

SET_NAME = "Ordered acceptance set"
# Distinct, so a swapped pair is visible in the output rather than plausible.
MACROS = [
    ("first", "alpha "),
    ("second", "bravo "),
    ("third", "charlie "),
    ("fourth", "delta "),
    ("fifth", "echo"),
]
TERMINAL = ("completed", "cancelled", "failed", "timed_out")


def build_set(device):
    """Create a set whose macros are stored in a known order."""
    set_id = str(uuid.uuid4())
    status, payload = device.post(
        "/api/v1/sets",
        {"schema_version": 1, "id": set_id, "revision": 1, "name": SET_NAME},
    )
    if status not in (200, 201):
        raise SystemExit(f"error: could not create set: {status} {payload}")
    # Unlike /device/restart, which rejects a body, this route wants an empty
    # JSON object and a content type. The two are inconsistent; match reality.
    status, _ = device.post(f"/api/v1/sets/{set_id}/select", {})
    if status != 200:
        raise SystemExit(f"error: could not select the set: {status}")

    created = []
    for name, source in MACROS:
        macro_id = str(uuid.uuid4())
        status, payload = device.post(
            f"/api/v1/sets/{set_id}/macros",
            {
                "schema_version": 1,
                "id": macro_id,
                "revision": 1,
                "set_id": set_id,
                "name": name,
                "source": source,
                "key_press_ms": 5,
                "inter_key_ms": 5,
            },
        )
        if status not in (200, 201):
            raise SystemExit(f"error: could not create macro {name}: {status} {payload}")
        created.append((macro_id, name, source))
    return set_id, created


def run_macro(device, set_id, macro_id, timeout_s=20):
    status, payload = device.post(
        "/api/v1/executions",
        {"setId": set_id, "macroId": macro_id, "macroRevision": 1},
    )
    if status != 202:
        raise SystemExit(f"error: submit refused: {status} {payload}")
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        time.sleep(0.2)
        state = device.get("/api/v1/executions/current")[1]["data"]
        if state["state"] in TERMINAL:
            return state["state"]
    raise SystemExit("error: execution never reached a terminal state")


def main() -> int:
    device = Device()
    device.login()
    try:
        set_id, created = build_set(device)
        print(f"created set {set_id} with {len(created)} macros")

        # The order the device reports is the order the user arranged; send in
        # that order rather than the order they were created, so a repository
        # that reordered them would be caught here rather than assumed away.
        status, payload = device.get(f"/api/v1/sets/{set_id}/macros")
        if status != 200:
            raise SystemExit(f"error: could not list macros: {status} {payload}")
        stored = [item["id"] for item in payload["data"]]
        expected_order = [macro_id for macro_id, _, _ in created]
        if stored != expected_order:
            raise SystemExit(
                f"error: the set came back in a different order than it was stored\n"
                f"  stored:   {expected_order}\n  returned: {stored}"
            )
        print("stored order preserved by the API")

        sources = {macro_id: source for macro_id, _, source in created}
        expected_text = "".join(source for _, _, source in created)

        with Capture() as capture:
            for index, macro_id in enumerate(stored, 1):
                state = run_macro(device, set_id, macro_id)
                if state != "completed":
                    raise SystemExit(f"error: macro {index} ended {state}, not completed")
                print(f"  {index}/{len(stored)} typed {sources[macro_id]!r} ({state})")
                time.sleep(0.4)
            time.sleep(0.5)  # let trailing reports land

        typed = capture.typed_text()
        released = capture.ended_released()
        print(f"\n  expected: {expected_text!r}")
        print(f"  typed:    {typed!r}")
        print(f"  final report all-zero (no key left held): {released}")

        if typed != expected_text:
            raise SystemExit("error: the wire does not match the set's stored order")
        if not released:
            raise SystemExit("error: a key was still held after the last macro")
        print("\nPASS: the whole set typed in stored order, nothing left held")
        return 0
    finally:
        device.logout()


if __name__ == "__main__":
    raise SystemExit(main())
