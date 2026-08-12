#!/usr/bin/env python3
"""End-to-end H1 confirmation acceptance on a real ESP32-S3R8.

Requires an already-provisioned device reachable through tests/hardware/hil_state,
the native TinyUSB HID cable connected to the host, and the UART console on
HIL_CONSOLE (default /dev/ttyACM1).

The script deliberately waits for the real 60-second confirmation timeout; it
does not replace that hardware evidence with a shortened test-only timeout.
"""

from __future__ import annotations

import argparse
import os
import platform
import sys
import time

import hil_state
from device_client import Device
from hid_capture import Capture

TERMINAL = {"completed", "cancelled", "failed", "timed_out"}
POLL_SECONDS = 0.25


def response_data(payload):
    return payload.get("data", payload) if isinstance(payload, dict) else payload


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")
    print(f"PASS: {message}")


def write_serial_confirm(console: str) -> None:
    import serial  # noqa: PLC0415 - optional hardware dependency

    with serial.Serial(console, 115200, timeout=1) as port:
        time.sleep(0.2)
        while port.in_waiting:
            port.read(port.in_waiting)
        port.write(b"confirm\n")
        port.flush()


def get_send(device: Device) -> dict:
    status, payload = device.get("/api/v1/send")
    if status != 200:
        raise SystemExit(f"FAIL: GET /api/v1/send -> HTTP {status} {payload}")
    value = response_data(payload)
    if not isinstance(value, dict):
        raise SystemExit(f"FAIL: malformed send status: {payload}")
    return value


def wait_for_state(device: Device, wanted: set[str], timeout_s: float) -> dict:
    deadline = time.monotonic() + timeout_s
    last = None
    while time.monotonic() < deadline:
        last = get_send(device)
        if last.get("state") in wanted:
            return last
        time.sleep(POLL_SECONDS)
    raise SystemExit(f"FAIL: timed out waiting for {sorted(wanted)}; last={last}")


def post_send(device: Device, source: str) -> dict:
    status, payload = device.post(
        "/api/v1/send",
        {"source": source, "keyPressMs": 12, "interKeyMs": 18},
    )
    require(status == 202, f"POST /api/v1/send accepted ({status})")
    accepted = response_data(payload)
    require(isinstance(accepted, dict), "accepted response is an object")
    require(
        accepted.get("state") == "awaiting_confirmation",
        "accepted response reports awaiting_confirmation",
    )
    return accepted


def key_down_reports(capture: Capture) -> list[bytes]:
    return [report for _, report in capture.reports if report[0] != 0 or any(report[2:8])]


def test_confirm_then_type(device: Device, console: str) -> None:
    source = "h1-confirm-42"
    print("\n[1/3] confirmation gates HID output")
    with Capture() as capture:
        post_send(device, source)
        time.sleep(0.75)
        waiting = get_send(device)
        require(
            waiting.get("state") == "awaiting_confirmation",
            "GET remains awaiting before confirm",
        )
        require(len(key_down_reports(capture)) == 0, "zero HID key-down reports before confirm")

        write_serial_confirm(console)
        terminal = wait_for_state(device, TERMINAL, 10.0)
        require(
            terminal.get("state") == "completed",
            "serial confirm transitions send to completed",
        )
        time.sleep(0.25)

    require(capture.typed_text() == source, "confirmed send typed the exact expected text")
    require(capture.ended_released(), "confirmed send ended with an all-zero release report")


def test_cancel_before_confirm(device: Device) -> None:
    print("\n[2/3] cancel before confirmation types nothing")
    with Capture() as capture:
        post_send(device, "h1-cancel-must-not-type")
        time.sleep(0.5)
        require(
            len(key_down_reports(capture)) == 0,
            "zero HID key-down reports before cancellation",
        )
        status, payload = device.delete("/api/v1/send")
        require(status == 202, f"DELETE /api/v1/send accepted ({status})")
        terminal = wait_for_state(device, {"cancelled", "failed"}, 10.0)
        require(
            terminal.get("state") == "cancelled",
            "cancel-before-confirmation reaches cancelled",
        )
        time.sleep(0.25)

    require(
        len(key_down_reports(capture)) == 0,
        "cancel-before-confirmation produced zero HID key-down reports",
    )
    require(capture.typed_text() == "", "cancel-before-confirmation typed nothing")


def test_real_timeout(device: Device) -> None:
    print("\n[3/3] real 60-second confirmation expiry types nothing")
    start = time.monotonic()
    with Capture() as capture:
        post_send(device, "h1-timeout-must-not-type")
        terminal = wait_for_state(device, {"timed_out", "failed"}, 70.0)
        elapsed = time.monotonic() - start
        require(terminal.get("state") == "timed_out", "unconfirmed send reaches timed_out")
        require(
            elapsed >= 59.0,
            f"hardware timeout was not shortened ({elapsed:.1f}s observed)",
        )
        require(
            elapsed <= 70.0,
            f"hardware timeout remained bounded ({elapsed:.1f}s observed)",
        )
        time.sleep(0.25)

    require(
        len(key_down_reports(capture)) == 0,
        "timeout path produced zero HID key-down reports",
    )
    require(capture.typed_text() == "", "timeout path typed nothing")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--console", default=os.environ.get("HIL_CONSOLE", hil_state.DEFAULT_CONSOLE)
    )
    parser.add_argument(
        "--firmware-sha",
        default=os.environ.get("HIL_FIRMWARE_SHA"),
        help="Exact firmware Git SHA flashed to the board; required for auditable evidence.",
    )
    args = parser.parse_args()
    if not args.firmware_sha:
        raise SystemExit("error: pass --firmware-sha or set HIL_FIRMWARE_SHA")

    print("H1 real-send confirmation hardware acceptance")
    print(f"firmware_sha={args.firmware_sha}")
    print("board=ESP32-S3R8")
    print(f"host={platform.platform()}")
    print(f"console={args.console}")
    print(f"device_ip={hil_state.device_ip()}")

    device = Device()
    device.login()
    status, payload = device.get("/api/v1/settings")
    if status != 200:
        raise SystemExit(f"error: GET settings -> HTTP {status} {payload}")
    original = response_data(payload)
    original_required = bool(original["requireSerialConfirmation"])

    status, payload = device.put(
        "/api/v1/settings",
        {"requireSerialConfirmation": True},
    )
    if status != 200:
        raise SystemExit(f"error: enabling requireSerialConfirmation -> HTTP {status} {payload}")
    updated = response_data(payload)
    configured = updated.get("settings") if isinstance(updated, dict) else None
    require(isinstance(configured, dict), "settings update response contains settings")
    require(
        configured.get("requireSerialConfirmation") is True,
        "requireSerialConfirmation enabled",
    )

    try:
        test_confirm_then_type(device, args.console)
        test_cancel_before_confirm(device)
        test_real_timeout(device)
    finally:
        # Restore the owner setting even if one acceptance step fails.
        status, restore_payload = device.put(
            "/api/v1/settings",
            {"requireSerialConfirmation": original_required},
        )
        if status != 200:
            print(
                "WARNING: could not restore requireSerialConfirmation: "
                f"HTTP {status} {restore_payload}",
                file=sys.stderr,
            )
        device.logout()

    print("\nH1 hardware acceptance: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
