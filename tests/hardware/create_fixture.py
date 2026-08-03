#!/usr/bin/env python3
"""Create the macro package + macros used by the HID typing tests."""

import sys
import uuid

import hil_state
from device_client import Device



def v4():
    return str(uuid.uuid4())


def main():
    d = Device()
    d.login()

    package_id = v4()
    # SPEC 12.1: a package is a name and its ordered macros. description,
    # manufacturer, model, board, keyboard_layout and sort_order were removed in
    # the 2026-08-02 revision and the device rejects them as unknown fields.
    macro_package = {
        "schema_version": 1,
        "id": package_id,
        "revision": 1,
        "name": "HIL Typing Tests",
    }
    status, payload = d.post("/api/v1/package", macro_package)
    if status not in (200, 201):
        raise SystemExit(f"package create failed: HTTP {status} {payload}")
    print(f"created package {package_id}")

    status, payload = d.post(f"/api/v1/package/{package_id}/select", {})
    print(f"select package: HTTP {status}")

    # Deliberately harmless text - no shell metacharacters, no newline/Enter,
    # so nothing can execute if it lands in a terminal.
    macros = {
        "text": {"name": "plain text", "source": "hello world"},
        "chord": {"name": "chord", "source": "{CTRL+A}"},
        "delay": {"name": "delay", "source": "ab{DELAY:3000}cd"},
    }
    created = {}
    for key, spec in macros.items():
        macro_id = v4()
        # SPEC 12.2: no scope discriminator and no favorite flag. package_id stays
        # as the envelope field the API still carries.
        body = {
            "schema_version": 1,
            "id": macro_id,
            "revision": 1,
            "package_id": package_id,
            "name": spec["name"],
            "source": spec["source"],
            "key_press_ms": 8,
            "inter_key_ms": 15,
        }
        status, payload = d.post(f"/api/v1/package/{package_id}/macros", body)
        if status not in (200, 201):
            print(f"  macro {key!r} failed: HTTP {status} {str(payload)[:200]}")
            continue
        created[key] = {"id": macro_id, "revision": 1, "source": spec["source"]}
        print(f"  created macro {key!r} ({spec['source']!r})")

    hil_state.save_fixture({"package_id": package_id, "macros": created})
    print(f"\nfixture saved ({len(created)}/{len(macros)} macros)")
    d.logout()
    return 0 if len(created) == len(macros) else 1


if __name__ == "__main__":
    sys.exit(main())
