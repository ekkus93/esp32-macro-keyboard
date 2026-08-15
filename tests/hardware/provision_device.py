#!/usr/bin/env python3
"""Take a current-v2 unprovisioned device through first-run setup.

The helper obtains the fresh eight-digit one-time code only by explicitly
running ``setup-code`` on the physical UART console. It never prints or persists
that code. Disposable AP/admin credentials are stored outside the repository by
``hil_state`` so a successful setup cannot strand the bench without its new
administrator password.

Usage:
    python3 tests/hardware/provision_device.py [--ip ADDRESS] [--console PORT]
"""

from __future__ import annotations

import argparse
import json
import os
import re
import secrets
import string
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

import hil_state

RESTART_TIMEOUT_S = 75
SERIAL_TIMEOUT_S = 6
ALPHABET = string.ascii_letters + string.digits
SETUP_CODE_RE = re.compile(rb"setup code:\s*([0-9]{8})(?:\r?\n|$)")


def generated_secret(length: int = 24) -> str:
    return "".join(secrets.choice(ALPHABET) for _ in range(length))


def generated_ap_ssid() -> str:
    suffix = "".join(secrets.choice(string.ascii_uppercase + string.digits) for _ in range(8))
    return f"Macro-HIL-{suffix}"


def store(name: str, value: str) -> None:
    path = hil_state.state_dir() / name
    path.write_text(value, encoding="utf-8")
    path.chmod(0o600)


def request_json(
    ip: str, method: str, path: str, body: dict | None = None, timeout: int = 30
) -> tuple[int, object]:
    data = None if body is None else json.dumps(body, separators=(",", ":")).encode()
    headers = {} if data is None else {"Content-Type": "application/json"}
    call = urllib.request.Request(
        f"http://{ip}{path}", data=data, headers=headers, method=method
    )
    try:
        with urllib.request.urlopen(call, timeout=timeout) as response:  # noqa: S310
            raw = response.read()
            status = response.status
    except urllib.error.HTTPError as error:
        raw = error.read()
        status = error.code
    try:
        return status, json.loads(raw.decode())
    except (UnicodeDecodeError, json.JSONDecodeError):
        return status, raw.decode("utf-8", "replace")


def console_command(command: str, console: str, seconds: int = SERIAL_TIMEOUT_S) -> bytes:
    import serial  # noqa: PLC0415 - optional hardware dependency

    with serial.Serial(console, 115200, timeout=0.25) as port:
        time.sleep(0.25)
        while port.in_waiting:
            port.read(port.in_waiting)
        port.write((command + "\n").encode())
        port.flush()
        deadline = time.monotonic() + seconds
        data = bytearray()
        while time.monotonic() < deadline:
            chunk = port.read(4096)
            if chunk:
                data.extend(chunk)
                if b"keyboard>" in data[-64:]:
                    break
    return bytes(data)


def read_setup_code(console: str) -> str:
    output = console_command("setup-code", console)
    match = SETUP_CODE_RE.search(output)
    if match is None:
        raise SystemExit(
            "error: physical UART did not return a current eight-digit setup code; "
            "confirm the device is unprovisioned and the production image is running"
        )
    return match.group(1).decode("ascii")


def require_setup_code_unavailable(console: str) -> None:
    output = console_command("setup-code", console)
    if SETUP_CODE_RE.search(output) is not None:
        raise SystemExit("error: setup-code remained available after provisioning")
    if b"setup code unavailable" not in output:
        raise SystemExit(
            "error: could not prove setup-code retirement after provisioning"
        )


def setup_state(ip: str) -> tuple[int, object]:
    return request_json(ip, "GET", "/api/v1/setup", timeout=8)


def resolve_unprovisioned_address(requested_ip: str | None, console: str) -> str:
    candidates: list[str] = []
    if requested_ip:
        candidates.append(requested_ip)
    else:
        try:
            candidates.append(hil_state.device_ip())
        except SystemExit:
            pass

    for candidate in candidates:
        try:
            status, _ = setup_state(candidate)
        except (urllib.error.URLError, TimeoutError, ConnectionError, OSError):
            continue
        if status in (200, 404):
            return candidate

    return hil_state.connect_wifi(console)


def wait_for_provisioned(
    ip: str, console: str, *, allow_uart_reconnect: bool
) -> str:
    deadline = time.monotonic() + RESTART_TIMEOUT_S
    while time.monotonic() < deadline:
        time.sleep(2)
        try:
            status, _ = setup_state(ip)
        except (urllib.error.URLError, TimeoutError, ConnectionError, OSError):
            continue
        if status == 404:
            return ip

    if not allow_uart_reconnect:
        raise SystemExit(
            "error: device did not return on the persisted station connection after setup; "
            "UART Wi-Fi recovery is forbidden for this acceptance path"
        )

    # General bench provisioning may recover a changed DHCP address or an
    # intentionally replaced network through the trusted UART. Exact H12
    # acceptance disables this fallback so setup must preserve the station
    # configuration that was written before provisioning.
    refreshed = hil_state.connect_wifi(console)
    status, payload = setup_state(refreshed)
    if status != 404:
        raise SystemExit(
            "error: device did not return in provisioned mode after setup "
            f"(HTTP {status}: {payload})"
        )
    return refreshed


def provision(
    ip: str | None = None,
    console: str | None = None,
    *,
    require_unprovisioned: bool = False,
    allow_post_setup_uart_reconnect: bool = True,
) -> str:
    console = console or os.environ.get("HIL_CONSOLE", hil_state.DEFAULT_CONSOLE)
    address = resolve_unprovisioned_address(ip, console)
    status, state = setup_state(address)
    if status == 404:
        if require_unprovisioned:
            raise SystemExit(
                "error: expected an unprovisioned device, but setup is already retired"
            )
        print(f"device at {address} is already provisioned; nothing to do")
        return address
    if status != 200 or not isinstance(state, dict) or state.get("provisioned") is not False:
        raise SystemExit(f"error: unexpected GET /api/v1/setup response: HTTP {status} {state}")
    device_name = state.get("deviceName")
    if not isinstance(device_name, str) or not device_name:
        raise SystemExit("error: setup state did not contain a non-empty deviceName")

    setup_code = read_setup_code(console)
    ap_passphrase = generated_secret()
    administrator_password = generated_secret()
    ap_ssid = generated_ap_ssid()

    # Persist the replacement credentials before the transactional submit. If
    # the HTTP response is lost after durable commit, the operator still owns
    # the password needed after the automatic restart.
    store("ap_ssid.txt", ap_ssid)
    store("ap_passphrase.txt", ap_passphrase)
    store("admin_password.txt", administrator_password)

    submission = {
        "setupCode": setup_code,
        "deviceName": device_name,
        "apSsid": ap_ssid,
        "apPassphrase": ap_passphrase,
        "adminPassword": administrator_password,
        "requireSerialConfirmation": False,
    }
    try:
        status, result = request_json(
            address, "POST", "/api/v1/setup", submission, timeout=35
        )
    finally:
        # Do not retain another Python reference after the request is serialized.
        submission["setupCode"] = ""
        setup_code = ""

    if status != 202 or not isinstance(result, dict) or result.get("accepted") is not True:
        raise SystemExit(f"error: setup rejected: HTTP {status} {result}")
    if result.get("restartRequired") is not True or result.get("connectionWillClose") is not True:
        raise SystemExit(f"error: malformed setup acceptance response: {result}")

    print("setup accepted; waiting for automatic restart")
    address = wait_for_provisioned(
        address, console, allow_uart_reconnect=allow_post_setup_uart_reconnect
    )
    require_setup_code_unavailable(console)
    print(f"device provisioned and back up at {address}")
    print("credentials stored in", hil_state.state_dir())
    return address


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ip", default=None, help="device address (default: stored/UART-discovered)")
    parser.add_argument(
        "--console", default=os.environ.get("HIL_CONSOLE", hil_state.DEFAULT_CONSOLE)
    )
    arguments = parser.parse_args()
    provision(arguments.ip, arguments.console)
    return 0


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    raise SystemExit(main())
