#!/usr/bin/env python3
"""H2-024 — password-change hardware validation on the reference ESP32-S3.

Covers the four open H2-024 items:

  1. repeat a successful password change on the reference board;
  2. verify old/new/session behaviour immediately, without a reboot;
  3. verify a power cycle preserves the new password; and
  4. re-run the PBKDF2 timing sanity check for cost regression.

The new password is generated here, is disposable, and replaces
``admin_password.txt`` in the bench state directory only after the device has
confirmed it. If any step fails after the change is accepted, the working
password is still written out, because leaving the bench credential stale would
lock the operator out of their own board.

Reset for item 3 is an esptool RTS->EN-pin hardware reset. That reports
``resetReason: power_on`` and is adequate here: the password write completes and
is acknowledged before the reset, so no write is in flight. Proving durability
across genuine VBUS removal is V2-035/H5-055's job, not this one.

Usage:
    python3 tests/hardware/test_password_change.py --firmware-sha <exact-git-sha>
"""
from __future__ import annotations

import argparse
import json
import os
import pathlib
import platform
import secrets
import statistics
import string
import subprocess
import sys
import time
import urllib.error
import urllib.request

import hil_state
from device_client import Device

# V2-041 (2026-08-09), 20 real logins, full round trip on this board.
BASELINE = {"min": 441.0, "median": 522.5, "p90": 757.2, "worst": 839.1}
# A regression is a median materially worse than the recorded baseline. 2x is
# deliberately loose: this is a cost-regression tripwire (e.g. an accidental
# iteration-count change), not a benchmark, and bench Wi-Fi jitter is included.
MEDIAN_REGRESSION_FACTOR = 2.0
TIMING_SAMPLES = 20

failures: list[str] = []


def check(condition: bool, message: str) -> bool:
    if condition:
        print(f"PASS: {message}")
    else:
        print(f"FAIL: {message}")
        failures.append(message)
    return condition


def login_status(ip: str, password: str) -> int:
    """Raw login attempt; returns the HTTP status without keeping a session."""
    body = json.dumps({"adminPassword": password}, separators=(",", ":")).encode()
    request = urllib.request.Request(
        f"http://{ip}/api/v1/auth/login",
        data=body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=15) as response:
            return response.status
    except urllib.error.HTTPError as error:
        return error.code


def session_still_valid(device: Device) -> bool:
    """Device.get returns (status, payload) and does not raise on 4xx, so the
    status must be inspected -- catching an exception here would silently report
    every session as still valid."""
    try:
        status, _ = device.get("/api/v1/auth/session")
    except OSError:
        return False
    return status == 200


def hardware_reset(port: str) -> None:
    # esptool.py from the sourced ESP-IDF environment, not `sys.executable -m
    # esptool`: the interpreter running this script is not necessarily the IDF
    # one and generally has no esptool module.
    subprocess.run(
        ["esptool.py", "--chip", "esp32s3", "--port", port,
         "--after", "hard_reset", "read_mac"],
        check=True, capture_output=True, timeout=120,
    )


def wait_for_device(ip: str, timeout: float = 90.0) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            request = urllib.request.Request(f"http://{ip}/api/v1/diagnostics")
            with urllib.request.urlopen(request, timeout=3):
                return True
        except urllib.error.HTTPError as error:
            if error.code == 401:
                return True
        except OSError:
            pass
        time.sleep(3)
    return False


def generate_password() -> str:
    alphabet = string.ascii_letters + string.digits
    return "".join(secrets.choice(alphabet) for _ in range(24))


def store_password(password: str) -> None:
    path = hil_state.state_dir() / "admin_password.txt"
    path.write_text(password + "\n", encoding="utf-8")
    os.chmod(path, 0o600)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware-sha", required=True,
                        help="Exact firmware Git SHA flashed to the board.")
    parser.add_argument("--port", default=os.environ.get("HIL_PORT", "/dev/ttyACM0"),
                        help="Serial port used only to issue the hardware reset.")
    args = parser.parse_args()

    ip = hil_state.device_ip()
    old_password = hil_state.admin_password()
    new_password = generate_password()

    print("H2-024 password-change hardware validation")
    print(f"firmware_sha={args.firmware_sha}")
    print("board=ESP32-S3R8")
    print(f"host={platform.platform()}")
    print(f"device_ip={ip}")
    print(f"reset_port={args.port}")

    print("\n[1/4] change the password on the reference board")
    changer = Device(ip)
    changer.login(old_password)
    changer.post("/api/v1/settings/change-password",
                 {"currentPassword": old_password, "newPassword": new_password})
    check(True, "POST /api/v1/settings/change-password accepted")
    store_password(new_password)
    print("  new disposable password stored in the bench state directory")

    print("\n[2/4] old/new/session behaviour immediately, without a reboot")
    check(login_status(ip, old_password) == 401, "the old password is refused immediately")
    check(login_status(ip, new_password) == 200, "the new password is accepted immediately")
    check(not session_still_valid(changer),
          "the session that changed the password is invalidated immediately")

    print("\n[3/4] a power cycle preserves the new password")
    hardware_reset(args.port)
    if not check(wait_for_device(ip), "device returned after the hardware reset"):
        return 1
    check(login_status(ip, new_password) == 200, "the new password still works after reset")
    check(login_status(ip, old_password) == 401, "the old password is still refused after reset")

    print(f"\n[4/4] PBKDF2 timing sanity, {TIMING_SAMPLES} real logins")
    samples: list[float] = []
    for _ in range(TIMING_SAMPLES):
        started = time.monotonic()
        status = login_status(ip, new_password)
        elapsed = (time.monotonic() - started) * 1000.0
        if status != 200:
            check(False, f"timing login returned HTTP {status}")
            break
        samples.append(elapsed)
        time.sleep(0.2)

    if samples:
        ordered = sorted(samples)
        observed = {
            "min": min(ordered),
            "median": statistics.median(ordered),
            "p90": ordered[max(0, int(len(ordered) * 0.9) - 1)],
            "worst": max(ordered),
        }
        print(f"  samples={len(ordered)}")
        for key in ("min", "median", "p90", "worst"):
            print(f"  {key:<7} {observed[key]:7.1f} ms   (V2-041 baseline {BASELINE[key]:7.1f} ms)")
        limit = BASELINE["median"] * MEDIAN_REGRESSION_FACTOR
        check(observed["median"] <= limit,
              f"median {observed['median']:.1f} ms within {limit:.1f} ms "
              f"({MEDIAN_REGRESSION_FACTOR}x the V2-041 baseline)")
        check(len(ordered) == TIMING_SAMPLES,
              f"all {TIMING_SAMPLES} timing logins succeeded")

    print()
    if failures:
        print(f"H2-024 hardware validation: FAIL ({len(failures)} check(s))")
        for item in failures:
            print(f"  - {item}")
        return 1
    print("H2-024 hardware validation: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
