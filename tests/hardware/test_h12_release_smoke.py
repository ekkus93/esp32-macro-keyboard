#!/usr/bin/env python3
"""H12-122 destructive final hardware smoke for one exact production SHA.

This script intentionally mutates the reference device and ends by factory
resetting and reprovisioning it. It never flashes firmware itself. Before it
runs, flash the production image from the same clean checkout and generate the
flash manifest with ``scripts/generate-flash-manifest.sh``.

Covered here: exact build provenance, login, ordinary send, opaque snapshot
save/load/delete, password change, restart, factory reset, reprovision, and a
final proof that the same production firmware image is still running.

The confirmation-required send/cancel/HID-capture portion remains in
``test_send_confirmation.py`` because it needs the native USB capture endpoint.
Both scripts must pass for H12-122.
"""

from __future__ import annotations

import argparse
import gzip
import importlib.util
import json
import secrets
import string
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

HARDWARE_DIR = Path(__file__).resolve().parent
REPO_ROOT = HARDWARE_DIR.parent.parent
sys.path.insert(0, str(HARDWARE_DIR))

import hil_state  # noqa: E402
import provision_device  # noqa: E402
from device_client import Device  # noqa: E402

TERMINAL_SEND_STATES = {"completed", "cancelled", "timed_out", "failed"}
SEND_TIMEOUT_S = 30
RESTART_TIMEOUT_S = 90
ALPHABET = string.ascii_letters + string.digits


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"error: {message}")


def response_data(payload: Any) -> dict[str, Any]:
    require(isinstance(payload, dict), f"expected JSON object, got {payload!r}")
    return payload


def generated_secret(length: int = 24) -> str:
    return "".join(secrets.choice(ALPHABET) for _ in range(length))


def load_provenance_module():
    path = REPO_ROOT / "scripts" / "run-v2-035-hardware.py"
    spec = importlib.util.spec_from_file_location("v2_hardware_evidence", path)
    require(spec is not None and spec.loader is not None, f"could not load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def verify_production_provenance(
    *, firmware_sha: str, flash_manifest: Path, base_url: str, password: str
) -> tuple[str, dict[str, str]]:
    v2 = load_provenance_module()
    manifest = v2.load_flash_manifest(flash_manifest)
    require(manifest["gitCommit"] == firmware_sha, (
        f"flash manifest gitCommit {manifest['gitCommit']} != requested SHA {firmware_sha}"
    ))
    api = v2.DeviceApi(base_url, 20.0)
    api.login(password)
    diagnostics = v2.parse_diagnostics(api.diagnostics())
    v2.verify_firmware_provenance(manifest, diagnostics)
    return diagnostics["buildId"], manifest


def wait_for_send(device: Device, timeout_s: int = SEND_TIMEOUT_S) -> dict[str, Any]:
    deadline = time.monotonic() + timeout_s
    last: Any = None
    while time.monotonic() < deadline:
        status, payload = device.get("/api/v1/send")
        require(status == 200, f"GET /api/v1/send -> HTTP {status} {payload}")
        state = response_data(payload)
        last = state
        if state.get("state") in TERMINAL_SEND_STATES:
            return state
        time.sleep(0.1)
    raise SystemExit(f"error: send did not become terminal within {timeout_s}s: {last}")


def raw_request(
    device: Device,
    method: str,
    path: str,
    *,
    body: bytes | None = None,
    content_type: str | None = None,
    timeout: int = 30,
) -> tuple[int, bytes]:
    headers = {}
    if device.cookie:
        headers["Cookie"] = device.cookie
    if content_type is not None:
        headers["Content-Type"] = content_type
    request = urllib.request.Request(
        f"{device.base}{path}", data=body, headers=headers, method=method
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:  # noqa: S310
            return response.status, response.read()
    except urllib.error.HTTPError as error:
        return error.code, error.read()


def json_from_bytes(raw: bytes) -> dict[str, Any]:
    try:
        value = json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise SystemExit(f"error: invalid JSON response: {error}") from error
    require(isinstance(value, dict), f"expected JSON object, got {value!r}")
    return value


def snapshot_smoke(device: Device) -> None:
    repository = {
        "format": "esp32-macro-keyboard-repository",
        "schemaVersion": 1,
        "packages": [],
    }
    payload = gzip.compress(
        json.dumps(repository, separators=(",", ":")).encode("utf-8"), mtime=0
    )
    status, raw = raw_request(
        device,
        "POST",
        "/api/v1/blob",
        body=payload,
        content_type="application/gzip",
    )
    require(status == 201, f"snapshot create -> HTTP {status} {raw!r}")
    created = json_from_bytes(raw)
    blob_id = created.get("id")
    require(isinstance(blob_id, str) and blob_id.isdigit(), f"invalid created blob: {created}")

    try:
        status, loaded = raw_request(device, "GET", f"/api/v1/blob/{blob_id}")
        require(status == 200, f"snapshot load -> HTTP {status}")
        require(loaded == payload, "snapshot load bytes differ from the saved snapshot")
    finally:
        status, raw = raw_request(device, "DELETE", f"/api/v1/blob/{blob_id}")
        require(status == 204, f"snapshot cleanup -> HTTP {status} {raw!r}")


def post_action_expecting_reboot(device: Device, path: str, body: dict | None = None) -> None:
    try:
        status, payload = device.post(path, body)
    except (urllib.error.URLError, TimeoutError, ConnectionError, OSError):
        # Accepted device actions explicitly close the connection. The
        # post-reboot state check is the authority, so no PASS is inferred here.
        return
    require(status == 202, f"{path} -> HTTP {status} {payload}")
    data = response_data(payload)
    require(data.get("accepted") is True, f"{path} did not report accepted=true: {data}")
    require(
        data.get("connectionWillClose") is True,
        f"{path} did not report connectionWillClose=true: {data}",
    )


def reconnect_normal(console: str, password: str, timeout_s: int = RESTART_TIMEOUT_S) -> tuple[str, Device]:
    deadline = time.time() + timeout_s
    last: BaseException | None = None
    while time.time() < deadline:
        time.sleep(3)
        try:
            address = hil_state.connect_wifi(console)
            device = Device(address)
            device.login(password)
            status, payload = device.get("/api/v1/status")
            require(status == 200, f"status after restart -> HTTP {status} {payload}")
            return address, device
        except (SystemExit, urllib.error.URLError, TimeoutError, OSError) as error:
            last = error
    raise SystemExit(
        f"error: normal service did not return within {timeout_s}s"
        + (f" ({last})" if last is not None else "")
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware-sha", required=True, help="exact 40-character Git SHA")
    parser.add_argument("--flash-manifest", type=Path, required=True)
    parser.add_argument(
        "--console",
        default=hil_state.DEFAULT_CONSOLE,
        help=f"UART console (default: {hil_state.DEFAULT_CONSOLE})",
    )
    parser.add_argument(
        "--allow-destructive",
        action="store_true",
        help="required acknowledgement: password/restart/factory-reset/reprovision are destructive",
    )
    args = parser.parse_args()
    require(args.allow_destructive, "pass --allow-destructive to run the final H12 hardware smoke")
    require(
        len(args.firmware_sha) == 40 and all(c in "0123456789abcdef" for c in args.firmware_sha),
        "--firmware-sha must be a lowercase 40-character SHA",
    )

    address = hil_state.device_ip()
    original_password = hil_state.admin_password()
    build_id, manifest = verify_production_provenance(
        firmware_sha=args.firmware_sha,
        flash_manifest=args.flash_manifest,
        base_url=f"http://{address}",
        password=original_password,
    )
    print(f"exact production provenance: PASS ({args.firmware_sha}, build {build_id})")

    device = Device(address)
    device.login(original_password)

    status, payload = device.put("/api/v1/settings", {"requireSerialConfirmation": False})
    require(status == 200, f"disable send confirmation -> HTTP {status} {payload}")
    settings_result = response_data(payload)
    settings = settings_result.get("settings")
    require(
        isinstance(settings, dict) and settings.get("requireSerialConfirmation") is False,
        f"confirmation setting did not become false: {payload}",
    )

    status, payload = device.post(
        "/api/v1/send", {"source": "H12{ENTER}", "keyPressMs": 8, "interKeyMs": 15}
    )
    require(status == 202, f"ordinary send -> HTTP {status} {payload}")
    terminal = wait_for_send(device)
    require(terminal.get("state") == "completed", f"ordinary send ended as {terminal}")
    print("login + ordinary active send: PASS")

    snapshot_smoke(device)
    print("snapshot save/load/delete exact-byte round trip: PASS")

    new_password = generated_secret()
    status, payload = device.post(
        "/api/v1/settings/change-password",
        {"currentPassword": original_password, "newPassword": new_password},
    )
    require(status == 204, f"password change -> HTTP {status} {payload}")

    status, _ = device.get("/api/v1/status")
    require(status == 401, f"old session remained valid after password change (HTTP {status})")
    replacement = Device(address)
    replacement.login(new_password)
    provision_device.store("admin_password.txt", new_password)
    device = replacement
    print("password change + old-session invalidation + new login: PASS")

    post_action_expecting_reboot(device, "/api/v1/device/restart")
    address, device = reconnect_normal(args.console, new_password)
    status, diagnostics = device.get("/api/v1/diagnostics")
    require(status == 200, f"diagnostics after restart -> HTTP {status} {diagnostics}")
    require(
        isinstance(diagnostics, dict) and diagnostics.get("buildId") == build_id,
        f"firmware build changed across restart: {diagnostics}",
    )
    print("software restart + exact build continuity: PASS")

    post_action_expecting_reboot(
        device,
        "/api/v1/device/factory-reset",
        {"adminPassword": new_password, "confirmation": "FACTORY RESET"},
    )
    # The destructive reset removes credentials and blobs. Provisioning helper
    # performs a hardware reset while capturing the new per-boot UART setup code,
    # joins the saved bench network, and proves normal-mode login after setup.
    time.sleep(8)
    address = provision_device.provision(
        console=args.console,
        device_name="ESP32 Macro Keyboard",
        ap_ssid="ESP32 Macro Keyboard",
        require_serial_confirmation=False,
    )
    final_password = hil_state.admin_password()
    final_device = Device(address)
    final_device.login(final_password)
    try:
        status, final_diagnostics = final_device.get("/api/v1/diagnostics")
        require(status == 200, f"final diagnostics -> HTTP {status} {final_diagnostics}")
        require(
            isinstance(final_diagnostics, dict) and final_diagnostics.get("buildId") == build_id,
            f"factory reset/reprovision no longer runs the flashed production build: {final_diagnostics}",
        )
        setup_status, _ = final_device.get("/api/v1/setup")
        require(setup_status == 404, f"setup route remained active after final reprovision: {setup_status}")
    finally:
        final_device.logout()

    print("factory reset + random-code reprovision + production-image continuity: PASS")
    print("H12 release smoke: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
