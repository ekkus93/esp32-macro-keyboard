#!/usr/bin/env python3
"""Provision an unprovisioned device through the current v2 one-shot setup API.

The stable bootstrap SoftAP credential comes from the manufacturing label. The
one-time setup code does not: SPEC_V2 requires a fresh eight-digit decimal code
on every unprovisioned boot, disclosed only on the trusted UART0 console.

This helper captures that code in memory, joins the bench station network over
UART, submits exactly one ``POST /api/v1/setup``, and verifies the restarted
normal service with the newly generated administrator password. The setup code
is never written to disk or printed.
"""

from __future__ import annotations

import argparse
import json
import re
import secrets
import string
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import hil_state  # noqa: E402
from device_client import Device  # noqa: E402

CONSOLE = hil_state.DEFAULT_CONSOLE
BOOT_CAPTURE_TIMEOUT_S = 30
RESTART_TIMEOUT_S = 90
ALPHABET = string.ascii_letters + string.digits
SETUP_CODE_PATTERN = re.compile(rb"setup code:\s*([0-9]{8})")


def generated_secret(length: int = 24) -> str:
    return "".join(secrets.choice(ALPHABET) for _ in range(length))


def store(name: str, value: str) -> None:
    path = hil_state.state_dir() / name
    path.write_text(value, encoding="utf-8")
    path.chmod(0o600)


def capture_setup_code(console: str) -> str:
    """Reset an unprovisioned board and capture its current UART-only code."""
    import serial  # noqa: PLC0415

    port = serial.Serial(console, 115200, timeout=1)
    try:
        while port.in_waiting:
            port.read(port.in_waiting)
        # EN/reset through the devkit UART bridge. Keep DTR deasserted so the
        # boot mode strap is not intentionally driven into the ROM downloader.
        port.setDTR(False)
        port.setRTS(True)
        time.sleep(0.2)
        port.setRTS(False)

        deadline = time.time() + BOOT_CAPTURE_TIMEOUT_S
        buffer = b""
        while time.time() < deadline:
            chunk = port.read(4096)
            if chunk:
                buffer += chunk
                match = SETUP_CODE_PATTERN.search(buffer)
                if match is not None:
                    return match.group(1).decode("ascii")
    finally:
        port.close()

    raise SystemExit(
        "error: current eight-digit setup code was not observed on the UART console; "
        "confirm the device is unprovisioned and the production firmware is flashed"
    )


def request_json(
    ip: str,
    method: str,
    path: str,
    body: dict | None = None,
    timeout: int = 30,
) -> tuple[int, dict | str]:
    data = None if body is None else json.dumps(body, separators=(",", ":")).encode()
    headers = {} if data is None else {"Content-Type": "application/json"}
    request = urllib.request.Request(
        f"http://{ip}{path}", data=data, headers=headers, method=method
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:  # noqa: S310
            raw = response.read().decode()
            status = response.status
    except urllib.error.HTTPError as error:
        status = error.code
        raw = error.read().decode()
    try:
        return status, json.loads(raw)
    except json.JSONDecodeError:
        return status, raw


def wait_for_normal_service(console: str, timeout_s: int = RESTART_TIMEOUT_S) -> str:
    deadline = time.time() + timeout_s
    last_error: BaseException | None = None
    while time.time() < deadline:
        time.sleep(3)
        try:
            address = hil_state.connect_wifi(console)
            device = Device(address)
            device.login()
            status, payload = device.get("/api/v1/status")
            device.logout()
            if status == 200 and isinstance(payload, dict) and payload.get("provisioned") is True:
                return address
        except (SystemExit, urllib.error.URLError, TimeoutError, OSError) as error:
            last_error = error
    raise SystemExit(
        f"error: normal authenticated service did not return within {timeout_s}s"
        + (f" ({last_error})" if last_error is not None else "")
    )


def provision(
    *,
    console: str = CONSOLE,
    device_name: str = "ESP32 Macro Keyboard",
    ap_ssid: str = "ESP32 Macro Keyboard",
    require_serial_confirmation: bool = False,
) -> str:
    """Provision the currently unprovisioned board and return its normal-mode IP."""
    setup_code = capture_setup_code(console)
    address = hil_state.connect_wifi(console)

    status, state = request_json(address, "GET", "/api/v1/setup")
    if status != 200 or not isinstance(state, dict):
        raise SystemExit(f"error: GET /api/v1/setup -> HTTP {status} {state}")
    if state.get("provisioned") is not False or not isinstance(state.get("deviceName"), str):
        raise SystemExit(f"error: invalid unprovisioned setup state: {state}")

    ap_passphrase = generated_secret()
    administrator_password = generated_secret()
    submission = {
        "setupCode": setup_code,
        "deviceName": device_name,
        "apSsid": ap_ssid,
        "apPassphrase": ap_passphrase,
        "adminPassword": administrator_password,
        "requireSerialConfirmation": require_serial_confirmation,
    }

    # Persist only credentials needed after reboot, outside the repository. The
    # ephemeral setup code intentionally remains memory-only.
    store("ap_passphrase.txt", ap_passphrase)
    store("admin_password.txt", administrator_password)
    store("ap_ssid.txt", ap_ssid)

    accepted = False
    try:
        status, result = request_json(address, "POST", "/api/v1/setup", submission, timeout=35)
        if status == 202 and isinstance(result, dict) and result.get("accepted") is True:
            accepted = True
        elif status not in (200, 202):
            raise SystemExit(f"error: POST /api/v1/setup -> HTTP {status} {result}")
    except (urllib.error.URLError, TimeoutError, ConnectionError, OSError):
        # The accepted contract says connectionWillClose=true. Do not call this
        # success yet; the authenticated post-restart verification below is the
        # fail-closed authority.
        pass

    address = wait_for_normal_service(console)
    setup_status, _ = request_json(address, "GET", "/api/v1/setup")
    if setup_status != 404:
        raise SystemExit(
            f"error: setup route remained available after provisioning (HTTP {setup_status})"
        )

    print("v2 setup verified after restart")
    print(f"device_ip={address}")
    if accepted:
        print("setup response was received before restart")
    else:
        print("setup response connection closed; post-restart verification proved commit")
    print(f"credentials stored in {hil_state.state_dir()}")
    return address


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--console", default=CONSOLE, help=f"UART console (default: {CONSOLE})"
    )
    parser.add_argument(
        "--device-name", default="ESP32 Macro Keyboard", help="configured device name"
    )
    parser.add_argument(
        "--ap-ssid", default="ESP32 Macro Keyboard", help="normal-mode access-point SSID"
    )
    parser.add_argument(
        "--require-serial-confirmation",
        action="store_true",
        help="require UART confirm for later macro sends",
    )
    arguments = parser.parse_args()
    provision(
        console=arguments.console,
        device_name=arguments.device_name,
        ap_ssid=arguments.ap_ssid,
        require_serial_confirmation=arguments.require_serial_confirmation,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
