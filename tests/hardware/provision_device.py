#!/usr/bin/env python3
"""Take an unprovisioned device through first-run setup.

Needed on its own, and needed again by the acceptance items that deliberately
wipe the device: the specification (section 24.6) requires credential reset and factory reset to be
exercised, and both leave a device that cannot be used until it is set up again.

An unprovisioned device never starts the USB stack, so it does not enumerate as
a keyboard at all -- every HID assertion in this directory depends on this
having run.

Credentials are generated here rather than typed: they are disposable bench
secrets, and generating them means no real password is ever pasted into a
terminal, a script, or this repository. They are written to the state directory
outside the tree (see hil_state) and never printed.

Usage:
    python3 tests/hardware/provision_device.py [--ip ADDRESS]

The setup code comes from the manufacturing banner and must already be in the
state directory as setup_code.txt.
"""

import argparse
import json
import secrets
import string
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

import hil_state

CONSOLE = hil_state.DEFAULT_CONSOLE
# the specification (section 16.1): setup waits for a physical confirmation, bounded. The console
# `confirm` command stands in for the button this device deliberately does not
# have (SPEC 19), and the wait is generous enough to lose a race with it.
CONFIRMATION_TIMEOUT_S = 25
RESTART_TIMEOUT_S = 60

ALPHABET = string.ascii_letters + string.digits


def generated_secret(length: int = 24) -> str:
    return "".join(secrets.choice(ALPHABET) for _ in range(length))


def store(name: str, value: str) -> None:
    path = hil_state.state_dir() / name
    path.write_text(value, encoding="utf-8")
    path.chmod(0o600)


def request(ip: str, path: str, body: dict | None = None, timeout: int = 30) -> dict:
    data = None if body is None else json.dumps(body).encode()
    headers = {} if data is None else {"Content-Type": "application/json"}
    call = urllib.request.Request(
        f"http://{ip}{path}", data=data, headers=headers,
        method="GET" if data is None else "POST",
    )
    try:
        with urllib.request.urlopen(call, timeout=timeout) as response:  # noqa: S310
            return json.loads(response.read().decode())
    except urllib.error.HTTPError as error:
        return json.loads(error.read().decode())


def send_confirmation() -> None:
    """Give the confirmation the setup route is waiting on, over the UART console."""
    import serial  # noqa: PLC0415 - optional dependency, only needed on the bench

    port = serial.Serial(CONSOLE, 115200, timeout=1)
    try:
        time.sleep(0.2)
        port.write(b"confirm\n")
        port.flush()
    finally:
        port.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ip", default=None, help="device address (default: stored)")
    arguments = parser.parse_args()
    ip = arguments.ip or hil_state.device_ip()

    state = request(ip, "/api/v1/setup-state")
    if not state.get("ok"):
        print(f"error: could not read setup state: {state}", file=sys.stderr)
        return 1
    if state["data"]["completed"]:
        print(f"device at {ip} is already provisioned; nothing to do")
        return 0

    setup_code = (hil_state.state_dir() / "setup_code.txt").read_text(encoding="utf-8").strip()
    ap_passphrase = generated_secret()
    administrator_password = generated_secret()

    submission = {
        "setupCode": setup_code,
        "apSsid": state["data"]["apSsid"],
        "apPassphrase": ap_passphrase,
        "administratorPassword": administrator_password,
        # Off, so the HID acceptance runs are not gated on a console command for
        # every send. The setting itself is exercised by the host suite; leaving
        # it on here would only prove the harness can type `confirm`.
        "requirePhysicalConfirmation": False,
        "alwaysSelectSet": True,
    }

    # The credentials must be stored before the request, not after: if setup
    # succeeds and this script then dies, a device with unknown credentials is
    # a device that has to be erased and started over.
    store("ap_passphrase.txt", ap_passphrase)
    store("admin_password.txt", administrator_password)

    import threading  # noqa: PLC0415

    confirmation = threading.Timer(1.5, send_confirmation)
    confirmation.start()
    try:
        result = request(ip, "/api/v1/setup/credentials", submission,
                         timeout=CONFIRMATION_TIMEOUT_S + 10)
    finally:
        confirmation.cancel()

    if not result.get("ok"):
        print(f"error: setup rejected: {result}", file=sys.stderr)
        return 1
    print("setup accepted; restarting")

    request(ip, "/api/v1/setup/restart", {}, timeout=10)

    # A provisioned device removes the setup routes altogether, so the proof
    # that setup took is that /setup-state stops existing. Waiting for
    # completed=true here waits forever.
    deadline = time.time() + RESTART_TIMEOUT_S
    while time.time() < deadline:
        time.sleep(2)
        try:
            call = urllib.request.Request(f"http://{ip}/api/v1/setup-state")
            with urllib.request.urlopen(call, timeout=5):  # noqa: S310
                continue  # still serving setup: not finished restarting
        except urllib.error.HTTPError as error:
            if error.code == 404:
                print(f"device provisioned and back up at {ip}")
                print("credentials stored in", hil_state.state_dir())
                return 0
        except (urllib.error.URLError, TimeoutError, ConnectionError, OSError):
            continue
    print("error: device did not come back provisioned within "
          f"{RESTART_TIMEOUT_S}s", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    raise SystemExit(main())
