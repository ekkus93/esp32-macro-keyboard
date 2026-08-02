"""Shared state and device plumbing for the hardware-in-the-loop tests.

Everything the HIL tests need that is specific to *your* bench - Wi-Fi
credentials, the administrator password, the device's current IP, the created
fixture - lives in a state directory that is deliberately NOT committed:

    ~/.config/esp32-macro-keyboard/hil/   (outside the repo; override with HIL_STATE_DIR)
        wifi.json                   {"ssid": "...", "password": "..."}
        admin_password.txt          the device's administrator password
        device_ip.txt               written by connect_wifi()
        fixture.json                written by create_fixture.py

Nothing here reads or writes anything under version control, so a stray
`git add` cannot capture a credential.
"""

import json
import os
import re
import time
from pathlib import Path

DEFAULT_CONSOLE = "/dev/ttyACM1"
CONNECT_TIMEOUT_S = 25


def state_dir() -> Path:
    """Bench-specific state directory (created on first use)."""
    # Defaults OUTSIDE the repository. These files hold a Wi-Fi password, the
    # device administrator password, and manufacturing secrets; this is a public
    # repository, and .gitignore only stops `git add`. It does not stop
    # `git add -f`, an archive of the working tree, a backup tool, or an editor
    # that indexes the checkout. Keeping them out of the tree entirely is the
    # only version of this that cannot go wrong.
    override = os.environ.get("HIL_STATE_DIR")
    default = Path.home() / ".config" / "esp32-macro-keyboard" / "hil"
    path = Path(override) if override else default
    path.mkdir(parents=True, exist_ok=True)
    return path


def _read(name: str) -> str:
    path = state_dir() / name
    if not path.is_file():
        raise SystemExit(
            f"error: {path} not found.\n"
            f"See tests/hardware/README.md for the files the HIL tests expect."
        )
    return path.read_text(encoding="utf-8").strip()


def admin_password() -> str:
    return _read("admin_password.txt")


def device_ip() -> str:
    return _read("device_ip.txt")


def wifi_credentials() -> tuple[str, str]:
    data = json.loads(_read("wifi.json"))
    return data["ssid"], data["password"]


def fixture() -> dict:
    return json.loads(_read("fixture.json"))


def save_fixture(data: dict) -> None:
    (state_dir() / "fixture.json").write_text(json.dumps(data, indent=2), encoding="utf-8")


def connect_wifi(console: str = DEFAULT_CONSOLE) -> str:
    """Join the device to your Wi-Fi network over the UART serial console.

    Station mode is a development facility and is not persisted, so this has to
    run after every reboot or reflash. Returns the device's IP address and
    records it in the state directory. Credentials are never echoed.
    """
    import serial                                  # noqa: PLC0415 - optional dep

    ssid, password = wifi_credentials()
    port = serial.Serial(console, 115200, timeout=1)
    try:
        time.sleep(0.3)
        while port.in_waiting:
            port.read(port.in_waiting)
        port.write(f"wifi-connect {ssid} {password}\n".encode())
        deadline, buffer = time.time() + CONNECT_TIMEOUT_S, b""
        while time.time() < deadline:
            chunk = port.read(4096)
            if chunk:
                buffer += chunk
                if b"keyboard>" in buffer[-15:]:
                    break
    finally:
        port.close()

    match = re.search(rb"IP address:\s*(\d+\.\d+\.\d+\.\d+)", buffer)
    if match is None:
        redacted = (buffer.decode("utf-8", errors="replace")
                    .replace(password, "***").replace(ssid, "***"))
        raise SystemExit(f"error: device did not report an IP address.\n{redacted[-400:]}")
    address = match.group(1).decode()
    (state_dir() / "device_ip.txt").write_text(address, encoding="utf-8")
    return address
