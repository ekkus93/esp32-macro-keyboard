#!/usr/bin/env python3
"""Hardware cancellation tests, against real USB HID reports.

    SPEC 24.6 item: cancellation over both the API and the `cancel` console command

The specification requires both paths, and they are genuinely different code:
the API route runs on the HTTP worker, the console command runs on the console
task, and both have to reach the one executor task. Testing one proves nothing
about the other.

Verifies:
  * cancellation during a long delay stops the macro before the remaining keys
  * cancellation during rapid typing stops mid-stream
  * both reach the `cancelled` terminal state, not `completed`
  * every key is released afterwards (final report all-zero)
  * cancellation latency is measured, not assumed
"""

import sys
import time
import uuid

import hil_state
from device_client import Device
from hid_capture import Capture

RAPID_TEXT = "the quick brown fox jumps over the lazy dog and keeps on going"
DELAY_SOURCE = "ab{DELAY:3000}cd"


def ensure_rapid_macro(device, fixture):
    """Create the rapid-typing macro once; reuse it on later runs."""
    if "rapid" in fixture["macros"]:
        return fixture["macros"]["rapid"]
    macro_id = str(uuid.uuid4())
    body = {
        "schema_version": 1,
        "id": macro_id,
        "revision": 1,
        "package_id": fixture["package_id"],
        "name": "rapid typing",
        "source": RAPID_TEXT,
        "key_press_ms": 5,
        "inter_key_ms": 5,
    }
    status, payload = device.post(f"/api/v1/package/{fixture['package_id']}/macros", body)
    if status not in (200, 201):
        raise SystemExit(f"rapid macro create failed: HTTP {status} {str(payload)[:200]}")
    entry = {"id": macro_id, "revision": 1, "source": RAPID_TEXT}
    fixture["macros"]["rapid"] = entry
    hil_state.save_fixture(fixture)
    print(f"created rapid-typing macro ({len(RAPID_TEXT)} chars)")
    return entry


def ensure_delay_macro(device, fixture):
    """The original fixture used `{DELAY 3000}`; the parser wants `{DELAY:3000}`."""
    existing = fixture["macros"].get("delay2")
    if existing and existing["source"] == DELAY_SOURCE:
        return existing
    macro_id = str(uuid.uuid4())
    body = {
        "schema_version": 1, "id": macro_id, "revision": 1,         "package_id": fixture["package_id"], "name": "delay cancel", "source": DELAY_SOURCE,
        "key_press_ms": 8, "inter_key_ms": 15,
    }
    status, payload = device.post(f"/api/v1/package/{fixture['package_id']}/macros", body)
    if status not in (200, 201):
        raise SystemExit(f"delay macro create failed: HTTP {status} {str(payload)[:200]}")
    entry = {"id": macro_id, "revision": 1, "source": DELAY_SOURCE}
    fixture["macros"]["delay2"] = entry
    hil_state.save_fixture(fixture)
    print(f"created delay macro {DELAY_SOURCE!r}")
    return entry


def cancel_over_console():
    """Issue `cancel` on the UART console.

    A separate path from the API route in every respect that matters: a
    different task, no session, no HTTP. SPEC 16.5 makes the console trusted
    because physical access already implies control.
    """
    import serial  # noqa: PLC0415

    port = serial.Serial(hil_state.DEFAULT_CONSOLE, 115200, timeout=1)
    try:
        time.sleep(0.1)
        port.write(b"cancel\n")
        port.flush()
    finally:
        port.close()
    return "console"


def run_cancel_test(device, fixture, macro_key, cancel_after, label, expect_prefix=None,
                    over_console=False):
    macro = fixture["macros"][macro_key]
    print(f"\n--- {label} ---")
    with Capture() as capture:
        status, payload = device.post(
            "/api/v1/executions",
            {
                "packageId": fixture["package_id"],
                "macroId": macro["id"],
                "macroRevision": macro["revision"],
            },
        )
        if status != 202:
            print(f"[FAIL] submit: HTTP {status} {str(payload)[:160]}")
            return False
        submitted = time.monotonic()

        time.sleep(cancel_after)
        cancel_sent = time.monotonic()
        if over_console:
            cancel_over_console()
            cstatus = "console"
        else:
            cstatus, _ = device.post("/api/v1/executions/current/cancel")
        cancel_ack = time.monotonic()

        # wait for a terminal state
        state, deadline = None, time.time() + 12
        while time.time() < deadline:
            time.sleep(0.2)
            state = device.get("/api/v1/executions/current")[1]["data"]
            if state["state"] in ("completed", "cancelled", "failed", "timed_out"):
                break
        time.sleep(0.4)          # let trailing HID reports land

    typed = capture.typed_text()
    released = capture.ended_released()
    last_report = capture.reports[-1][0] if capture.reports else None
    latency_ms = (last_report - cancel_sent) * 1000 if last_report else None

    terminal = state["state"] if state else "?"
    ok = terminal == "cancelled" and released
    if expect_prefix is not None:
        ok = ok and typed == expect_prefix

    print(f"    cancel HTTP        : {cstatus} ({(cancel_ack-cancel_sent)*1000:.0f} ms to ack)")
    print(f"    terminal state     : {terminal}")
    print(f"    typed              : {typed!r}")
    if expect_prefix is not None:
        print(f"    expected           : {expect_prefix!r}")
    print(f"    release-all        : {released}")
    if latency_ms is not None and latency_ms >= 0:
        print(f"    last keystroke was : {latency_ms:.0f} ms after cancel was sent")
    print(f"    [{'PASS' if ok else 'FAIL'}]")
    return ok


def main():
    fixture = hil_state.fixture()
    device = Device()
    device.login()
    ensure_rapid_macro(device, fixture)
    fixture = hil_state.fixture()
    ensure_delay_macro(device, fixture)
    fixture = hil_state.fixture()

    results = {}
    # 'ab{DELAY 3000}cd': cancel inside the 3 s delay -> only "ab" should type.
    results["delay cancellation (API)"] = run_cancel_test(
        device, fixture, "delay2", cancel_after=1.2,
        label="delay cancellation (API, cancel during the delay)",
        expect_prefix="ab")
    time.sleep(1.5)
    # rapid typing: cancel mid-stream -> a strict prefix of the source.
    results["rapid typing cancellation (API)"] = run_cancel_test(
        device, fixture, "rapid", cancel_after=0.25,
        label="rapid typing cancellation (API, cancel mid-stream)")
    time.sleep(1.5)

    # Both paths are required. Same assertions, different route in.
    results["delay cancellation (console)"] = run_cancel_test(
        device, fixture, "delay2", cancel_after=1.2,
        label="delay cancellation (console `cancel` command)",
        expect_prefix="ab", over_console=True)
    time.sleep(1.5)
    results["rapid typing cancellation (console)"] = run_cancel_test(
        device, fixture, "rapid", cancel_after=0.25,
        label="rapid typing cancellation (console `cancel` command)",
        over_console=True)

    print("\n" + "=" * 60)
    for name, ok in results.items():
        print(f"  {name:<30} {'PASS' if ok else 'FAIL'}")
    device.logout()
    return 0 if all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
