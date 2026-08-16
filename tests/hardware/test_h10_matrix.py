#!/usr/bin/env python3
"""H10-103 — hardware matrix items not already covered by H1/H2/H3 or H12-122.

H12-122's smoke acceptance already covers HID text/release, cancel-produces-no-
keystroke, the confirmation-required flow, factory reset/recovery, blob
operations after reprovision, and reconnect. H1/H2/H3 cover confirmation
timeout, password change and reset interruption. This fills the rest:

  * USB HID identity — project-owned VID/PID and descriptor strings, read from
    the host's view of the enumerated device rather than from the build.
  * Chords — modifier combinations actually emitted as HID reports, which plain
    text macros never exercise.
  * AP survival after station failure — the SoftAP must stay up when the station
    network is unavailable, because losing both is an unrecoverable lockout.
  * Bounded reconnect — a failed station join must not retry unbounded.

Usage:
    python3 tests/hardware/test_h10_matrix.py --firmware-sha <exact-git-sha>
"""
from __future__ import annotations

import argparse
import os
import platform
import re
import subprocess
import sys
import time

import hid_capture
import hil_state
from device_client import Device

EXPECTED_VID = "303a"
EXPECTED_PID = "4001"

failures: list[str] = []


def check(condition: bool, message: str) -> bool:
    print(f"{'PASS' if condition else 'FAIL'}: {message}")
    if not condition:
        failures.append(message)
    return condition


def console(port: str, command: str, settle: float = 2.5, total: float = 40.0) -> str:
    """Send one console command and return what the device printed."""
    import serial

    ser = serial.Serial()
    ser.port = port
    ser.baudrate = 115200
    ser.timeout = 0.2
    ser.dsrdtr = False
    ser.rtscts = False
    ser.open()
    try:
        ser.dtr = False
        ser.rts = False
    except OSError:
        pass
    deadline = time.time() + 30.0
    buffer = b""
    poke = 0.0
    while time.time() < deadline:
        if time.time() >= poke:
            ser.write(b"\r\n")
            ser.flush()
            poke = time.time() + 2.0
        buffer += ser.read(4096)
        if b"keyboard>" in buffer:
            break
    # Opening the port can reset the board, and the REPL prompt appears while
    # app_core is still wiring subsystems. Wait for startup to finish or the
    # reply gets lost in boot output.
    if b"Returned from app_main()" not in buffer:
        settle_deadline = time.time() + 25.0
        while time.time() < settle_deadline:
            buffer += ser.read(4096)
            if b"Returned from app_main()" in buffer:
                break
    time.sleep(1.0)
    ser.reset_input_buffer()
    ser.write(command.encode() + b"\r\n")
    ser.flush()
    hard = time.time() + total
    end = time.time() + settle
    chunks: list[bytes] = []
    while time.time() < end and time.time() < hard:
        data = ser.read(4096)
        if data:
            chunks.append(data)
            end = time.time() + settle
    ser.close()
    return b"".join(chunks).decode("utf-8", "replace")


def usb_identity() -> None:
    print("\n[1/4] USB HID identity as the host sees it")
    listing = subprocess.run(["lsusb"], capture_output=True, text=True, timeout=30).stdout
    line = next((l for l in listing.splitlines()
                 if f"{EXPECTED_VID}:{EXPECTED_PID}" in l.lower()), "")
    check(bool(line), f"device enumerates with project-owned VID:PID {EXPECTED_VID}:{EXPECTED_PID}")
    if line:
        print(f"       {line.strip()}")

    node = hid_capture.resolve_hidraw()
    name = ""
    uevent = f"/sys/class/hidraw/{os.path.basename(node)}/device/uevent"
    try:
        with open(uevent, encoding="utf-8") as handle:
            for entry in handle:
                if entry.startswith("HID_NAME="):
                    name = entry.split("=", 1)[1].strip()
    except OSError:
        pass
    check(bool(name), f"HID descriptor exposes a product string ({name!r})")
    check("esp32" in name.lower() or "macro keyboard" in name.lower(),
          "product string is project-owned, not a vendor default")


def chords(device: Device) -> None:
    print("\n[2/4] chords emit modifier-bearing HID reports")
    # {CTRL+L} is a single chord: one key-down carrying the control modifier,
    # then a release-all. Plain text macros never set a modifier byte, so this
    # is the only path that exercises it.
    with hid_capture.Capture() as capture:
        status, payload = device.post(
            "/api/v1/send",
            {"source": "{CTRL+L}", "keyPressMs": 12, "interKeyMs": 18},
        )
        if not check(status == 202, f"POST /api/v1/send accepted a chord (HTTP {status})"):
            print(f"       {payload}")
            return
        deadline = time.time() + 15.0
        while time.time() < deadline:
            state, body = device.get("/api/v1/send")
            data = body.get("data", body) if isinstance(body, dict) else {}
            if data.get("state") in {"completed", "failed", "cancelled", "timed_out"}:
                break
            time.sleep(0.3)
        time.sleep(0.5)

    reports = [data for _, data in capture.reports]
    modifier_reports = [r for r in reports if r[0] != 0]
    check(bool(modifier_reports), "a report carried a non-zero modifier byte")
    if modifier_reports:
        first = modifier_reports[0]
        # USB HID keyboard: byte 0 bit 0 is left-control.
        check(bool(first[0] & 0x01), f"the modifier byte sets left-control (0x{first[0]:02x})")
        check(any(b != 0 for b in first[2:]), "the chord carried an ordinary key alongside the modifier")
    check(reports and all(b == 0 for b in reports[-1]),
          "the chord ended with an all-zero release report")


def ap_survives_station_failure(port: str, device_ip: str) -> None:
    print("\n[3/4] SoftAP survives a failed station join, and the retry is bounded")
    before = console(port, "wifi-status")
    check("AP state: ready" in before, "SoftAP is ready before the failed join")

    started = time.monotonic()
    # Deliberately wrong passphrase for the real SSID: the join must fail rather
    # than hang, and must not take the AP down with it.
    ssid, _ = hil_state.wifi_credentials()
    output = console(port, f"wifi-connect {ssid} definitely-not-the-passphrase",
                     settle=4.0, total=90.0)
    elapsed = time.monotonic() - started
    check("connection failed" in output.lower() or "failed" in output.lower(),
          "the failed station join is reported as a failure, not silently retried")
    check(elapsed < 90.0, f"the join attempt was bounded ({elapsed:.1f}s, not indefinite)")

    after = console(port, "wifi-status")
    check("AP state: ready" in after, "SoftAP is STILL ready after the station join failed")
    print(f"       before: {before.strip().splitlines()[-2] if len(before.strip().splitlines())>1 else before.strip()[:60]}")


def bounded_reconnect(port: str, device_ip: str) -> None:
    print("\n[4/4] the device reconnects to the real network afterwards")
    ssid, password = hil_state.wifi_credentials()
    output = console(port, f"wifi-connect {ssid} {password}", settle=4.0, total=90.0)
    redacted = output.replace(password, "<redacted>")
    check("connected" in redacted.lower(), "station rejoined the real network after the failure")
    deadline = time.time() + 60.0
    reachable = False
    while time.time() < deadline:
        import urllib.error
        import urllib.request
        try:
            urllib.request.urlopen(
                urllib.request.Request(f"http://{device_ip}/api/v1/diagnostics"), timeout=3)
            reachable = True
            break
        except urllib.error.HTTPError as error:
            if error.code == 401:
                reachable = True
                break
        except OSError:
            time.sleep(3)
    check(reachable, "authenticated service is reachable again on the LAN")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware-sha", required=True)
    parser.add_argument("--console", default=os.environ.get("HIL_CONSOLE", "/dev/ttyACM0"))
    args = parser.parse_args()

    ip = hil_state.device_ip()
    print("H10-103 hardware matrix, items not covered elsewhere")
    print(f"firmware_sha={args.firmware_sha}")
    print("board=ESP32-S3R8")
    print(f"host={platform.platform()}")
    print(f"device_ip={ip}")
    print(f"console={args.console}")

    usb_identity()

    device = Device(ip)
    device.login()
    chords(device)

    ap_survives_station_failure(args.console, ip)
    bounded_reconnect(args.console, ip)

    print()
    if failures:
        print(f"H10-103 matrix: FAIL ({len(failures)} check(s))")
        for item in failures:
            print(f"  - {item}")
        return 1
    print("H10-103 matrix: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
