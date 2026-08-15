#!/usr/bin/env python3
"""Run H12-122 final exact-release hardware smoke acceptance.

The harness invokes ``flash-release-manifest.py`` itself before any HTTP/HID
checks, so the flash operation and the observed device provenance are one
fail-closed acceptance sequence. The resulting JSON intentionally contains no
passwords, setup codes, session cookies, AP passphrases, Wi-Fi credentials,
or macro source text.
"""

from __future__ import annotations

import argparse
import gzip
import hashlib
import importlib.util
import json
import os
import platform
import re
import secrets
import string
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any, TypedDict

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
HARDWARE_DIR = REPO_ROOT / "tests" / "hardware"
sys.path.insert(0, str(HARDWARE_DIR))

import hil_state  # noqa: E402
import provision_device  # noqa: E402
from device_client import Device  # noqa: E402
from hid_capture import Capture  # noqa: E402

FLASH_SCRIPT = SCRIPT_DIR / "flash-release-manifest.py"
spec = importlib.util.spec_from_file_location("flash_release_manifest", FLASH_SCRIPT)
flash_manifest = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(flash_manifest)

TERMINAL_SEND_STATES = {"completed", "cancelled", "failed", "timed_out"}
POLL_SECONDS = 0.25
RESTART_TIMEOUT_S = 75
RESTART_MIN_UPTIME_DROP_MS = 2000
SHA40_RE = re.compile(r"^[0-9a-f]{40}$")
SECRET_EVIDENCE_KEYS = {
    "password",
    "passphrase",
    "setupcode",
    "sessiontoken",
    "cookie",
    "source",
    "wifipassword",
}
ALPHABET = string.ascii_letters + string.digits


class DiagnosticsObservation(TypedDict):
    buildId: str
    firmwareVersion: str
    resetReason: str
    uptimeMs: int


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")
    print(f"PASS: {message}", flush=True)


def response_data(payload: object) -> object:
    return payload.get("data", payload) if isinstance(payload, dict) else payload


def manifest_sha256(path: Path) -> str:
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    return digest


def validate_diagnostics(
    manifest: dict[str, Any], diagnostics: object
) -> DiagnosticsObservation:
    value = response_data(diagnostics)
    if not isinstance(value, dict):
        raise ValueError("diagnostics response is not an object")
    build_id = value.get("buildId")
    firmware_version = value.get("firmwareVersion")
    reset_reason = value.get("resetReason")
    uptime_ms = value.get("uptimeMs")
    expected = manifest.get("diagnosticsBuildId")
    if not isinstance(build_id, str) or not re.fullmatch(r"[0-9a-f]{39}", build_id):
        raise ValueError(
            "diagnostics buildId is not the expected 39-character lowercase ELF-SHA prefix"
        )
    if not isinstance(expected, str) or build_id != expected:
        raise ValueError("on-device buildId does not exactly match flash-manifest ELF provenance")
    if not isinstance(firmware_version, str) or not firmware_version:
        raise ValueError("diagnostics firmwareVersion is missing")
    if not isinstance(reset_reason, str) or not reset_reason:
        raise ValueError("diagnostics resetReason is missing")
    if isinstance(uptime_ms, bool) or not isinstance(uptime_ms, int) or uptime_ms < 0:
        raise ValueError("diagnostics uptimeMs is not a non-negative integer")
    return {
        "buildId": build_id,
        "firmwareVersion": firmware_version,
        "resetReason": reset_reason,
        "uptimeMs": uptime_ms,
    }


def validate_restart_observation(
    before: DiagnosticsObservation,
    after: DiagnosticsObservation,
    elapsed_ms: int,
) -> None:
    if elapsed_ms < 0:
        raise ValueError("restart observation elapsed time is invalid")
    if after["resetReason"] != "software":
        raise ValueError(
            f"restart diagnostics resetReason is {after['resetReason']!r}, not 'software'"
        )
    continuous_uptime = before["uptimeMs"] + elapsed_ms
    uptime_drop = continuous_uptime - after["uptimeMs"]
    if uptime_drop < RESTART_MIN_UPTIME_DROP_MS:
        raise ValueError(
            "restart did not produce a large enough uptime discontinuity to prove a reboot"
        )


def ensure_evidence_has_no_secret_keys(value: object, path: str = "$") -> None:
    if isinstance(value, dict):
        for key, child in value.items():
            normalized = re.sub(r"[^a-z0-9]", "", str(key).lower())
            if any(secret in normalized for secret in SECRET_EVIDENCE_KEYS):
                raise ValueError(f"secret-bearing evidence key forbidden at {path}.{key}")
            ensure_evidence_has_no_secret_keys(child, f"{path}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            ensure_evidence_has_no_secret_keys(child, f"{path}[{index}]")


def get_json(device: Device, path: str, expected: int = 200) -> object:
    status, payload = device.get(path)
    require(status == expected, f"GET {path} -> HTTP {expected}")
    return payload


def get_send(device: Device) -> dict[str, Any]:
    value = response_data(get_json(device, "/api/v1/send"))
    if not isinstance(value, dict):
        raise SystemExit("FAIL: malformed GET /api/v1/send response")
    return value


def wait_for_send(device: Device, wanted: set[str], timeout_s: float) -> dict[str, Any]:
    deadline = time.monotonic() + timeout_s
    last: dict[str, Any] | None = None
    while time.monotonic() < deadline:
        last = get_send(device)
        if last.get("state") in wanted:
            return last
        time.sleep(POLL_SECONDS)
    raise SystemExit(f"FAIL: send did not reach {sorted(wanted)}; last={last}")


def key_down_reports(capture: Capture) -> list[bytes]:
    return [report for _, report in capture.reports if report[0] != 0 or any(report[2:8])]


def set_confirmation(device: Device, required: bool) -> None:
    status, payload = device.put(
        "/api/v1/settings", {"requireSerialConfirmation": required}
    )
    require(status == 200, f"set requireSerialConfirmation={required}")
    value = response_data(payload)
    settings = value.get("settings") if isinstance(value, dict) else None
    require(
        isinstance(settings, dict) and settings.get("requireSerialConfirmation") is required,
        "settings response reflects confirmation preference",
    )


def post_send(device: Device, source: str, expected_initial: set[str]) -> dict[str, Any]:
    status, payload = device.post(
        "/api/v1/send", {"source": source, "keyPressMs": 12, "interKeyMs": 18}
    )
    require(status == 202, "POST /api/v1/send accepted")
    accepted = response_data(payload)
    if not isinstance(accepted, dict):
        raise SystemExit("FAIL: malformed send acceptance response")
    require(accepted.get("state") in expected_initial, "send initial state is expected")
    return accepted


def serial_command(command: str, console: str, seconds: float = 3.0) -> bytes:
    import serial  # noqa: PLC0415 - hardware-only dependency

    with serial.Serial(console, 115200, timeout=0.25) as port:
        time.sleep(0.2)
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


def active_send_smoke(device: Device) -> None:
    set_confirmation(device, False)
    source = "h12-active-42"
    with Capture() as capture:
        post_send(device, source, {"running", "completed"})
        terminal = wait_for_send(device, TERMINAL_SEND_STATES, 10.0)
        require(terminal.get("state") == "completed", "active send completed")
        time.sleep(0.25)
    require(capture.typed_text() == source, "active send typed exact expected text")
    require(capture.ended_released(), "active send ended with all keys released")


def confirmation_send_smoke(device: Device, console: str) -> None:
    set_confirmation(device, True)
    source = "h12-confirm-42"
    with Capture() as capture:
        post_send(device, source, {"awaiting_confirmation"})
        time.sleep(0.5)
        require(not key_down_reports(capture), "confirmation send typed nothing before confirm")
        serial_command("confirm", console)
        terminal = wait_for_send(device, TERMINAL_SEND_STATES, 10.0)
        require(terminal.get("state") == "completed", "confirmation send completed after UART confirm")
        time.sleep(0.25)
    require(capture.typed_text() == source, "confirmation send typed exact expected text")
    require(capture.ended_released(), "confirmation send ended with all keys released")


def cancel_send_smoke(device: Device) -> None:
    set_confirmation(device, False)
    with Capture() as capture:
        post_send(device, "{DELAY:10000}h12-cancel-must-not-type", {"running"})
        status, _ = device.delete("/api/v1/send")
        require(status == 202, "DELETE /api/v1/send accepted during active delay")
        terminal = wait_for_send(device, {"cancelled", "failed"}, 10.0)
        require(terminal.get("state") == "cancelled", "active send reached cancelled")
        time.sleep(0.25)
    require(not key_down_reports(capture), "cancelled delayed send produced no key-down report")
    require(capture.ended_released(), "cancel path ended with all keys released")


def raw_request(
    device: Device,
    method: str,
    path: str,
    *,
    body: bytes | None = None,
    content_type: str | None = None,
    timeout: int = 45,
) -> tuple[int, bytes, dict[str, str]]:
    headers: dict[str, str] = {}
    if device.cookie:
        headers["Cookie"] = device.cookie
    if content_type:
        headers["Content-Type"] = content_type
    request = urllib.request.Request(
        f"{device.base}{path}", data=body, headers=headers, method=method
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:  # noqa: S310
            return response.status, response.read(), dict(response.headers.items())
    except urllib.error.HTTPError as error:
        return error.code, error.read(), dict(error.headers.items())


def snapshot_smoke(device: Device) -> tuple[str, int]:
    repository = json.dumps(
        {"format": "esp32-macro-keyboard", "schemaVersion": 1, "packages": []},
        separators=(",", ":"),
        sort_keys=True,
    ).encode()
    compressed = gzip.compress(repository, compresslevel=9, mtime=0)
    status, body, _ = raw_request(
        device,
        "POST",
        "/api/v1/blob",
        body=compressed,
        content_type="application/gzip",
    )
    require(status == 201, "snapshot save returned 201")
    try:
        created_payload = json.loads(body.decode())
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise SystemExit(f"FAIL: malformed blob-create response: {error}") from error
    created = response_data(created_payload)
    blob_id = created.get("id") if isinstance(created, dict) else None
    require(isinstance(blob_id, str) and blob_id.isdigit(), "snapshot save returned canonical blob ID")

    status, loaded, headers = raw_request(device, "GET", f"/api/v1/blob/{blob_id}")
    require(status == 200, "snapshot load returned 200")
    require(headers.get("Content-Type", "").startswith("application/gzip"), "snapshot load is gzip")
    require(loaded == compressed, "snapshot load is byte-identical to saved gzip")
    return blob_id, len(compressed)


def generated_secret(length: int = 24) -> str:
    return "".join(secrets.choice(ALPHABET) for _ in range(length))


def change_password_smoke(device: Device) -> None:
    old_password = hil_state.admin_password()
    new_password = generated_secret()
    state = hil_state.state_dir()
    active_path = state / "admin_password.txt"
    pending_path = state / "admin_password.pending.txt"
    pending_path.write_text(new_password, encoding="utf-8")
    pending_path.chmod(0o600)

    status, payload = device.post(
        "/api/v1/settings/change-password",
        {"currentPassword": old_password, "newPassword": new_password},
        raw=True,
    )
    if status != 204:
        pending_path.unlink(missing_ok=True)
    require(status == 204, "password change returned 204")
    require(payload == "", "password change returned an empty success body")

    status, _ = device.get("/api/v1/settings")
    require(status == 401, "password change invalidated the active session")

    old_client = Device(device.ip)
    status, _ = old_client.post("/api/v1/auth/login", {"adminPassword": old_password})
    require(status == 401, "old password rejected after password change")
    fresh_client = Device(device.ip)
    status, _ = fresh_client.post("/api/v1/auth/login", {"adminPassword": new_password})
    require(status == 200, "new password accepted after password change")
    fresh_client.logout()

    # The pending file is written before the mutating request. If the HTTP
    # response is lost or the harness dies after durable commit, the new
    # credential remains recoverable outside the repository instead of being
    # generated only in volatile process memory. Promote it only after the new
    # password is proven usable.
    os.replace(pending_path, active_path)
    active_path.chmod(0o600)


def wait_for_authenticated_service(
    address: str, console: str, *, allow_uart_reconnect: bool
) -> str:
    deadline = time.monotonic() + RESTART_TIMEOUT_S
    while time.monotonic() < deadline:
        time.sleep(2)
        try:
            status, _ = Device(address).get("/api/v1/status")
        except (urllib.error.URLError, TimeoutError, ConnectionError, OSError):
            continue
        if status in (200, 401):
            return address
    if not allow_uart_reconnect:
        raise SystemExit(
            "FAIL: authenticated service did not return on the persisted station connection; "
            "UART Wi-Fi recovery is forbidden for restart acceptance"
        )
    refreshed = hil_state.connect_wifi(console)
    status, _ = Device(refreshed).get("/api/v1/status")
    require(status in (200, 401), "authenticated service returned after UART Wi-Fi reconnect")
    return refreshed


def restart_smoke(device: Device) -> tuple[str, float]:
    requested_at = time.monotonic()
    status, payload = device.post("/api/v1/device/restart")
    require(status == 202, "device restart returned 202")
    value = response_data(payload)
    require(isinstance(value, dict) and value.get("accepted") is True, "restart was accepted")
    address = wait_for_authenticated_service(
        device.ip, "", allow_uart_reconnect=False
    )
    return address, requested_at


def wait_for_unprovisioned_uart(console: str) -> None:
    import serial  # noqa: PLC0415 - hardware-only dependency

    deadline = time.monotonic() + RESTART_TIMEOUT_S
    while time.monotonic() < deadline:
        try:
            output = serial_command("setup-code", console, seconds=2.0)
        except (serial.SerialException, OSError):
            time.sleep(0.5)
            continue
        if provision_device.SETUP_CODE_RE.search(output) is not None:
            require(True, "factory reset entered unprovisioned setup mode on UART")
            return
        time.sleep(0.5)
    raise SystemExit(
        "FAIL: factory reset never exposed a fresh setup code on the trusted UART console"
    )


def factory_reset_and_reprovision(device: Device, console: str) -> str:
    password = hil_state.admin_password()
    status, payload = device.post(
        "/api/v1/device/factory-reset",
        {"adminPassword": password, "confirmation": "FACTORY RESET"},
    )
    require(status == 202, "factory reset returned 202")
    value = response_data(payload)
    require(
        isinstance(value, dict)
        and value.get("accepted") is True
        and value.get("reprovisioningRequired") is True,
        "factory reset explicitly requires reprovisioning",
    )
    wait_for_unprovisioned_uart(console)
    address = hil_state.connect_wifi(console)
    address = provision_device.provision(
        address,
        console,
        require_unprovisioned=True,
        allow_post_setup_uart_reconnect=False,
    )
    old_client = Device(address)
    old_status, _ = old_client.post(
        "/api/v1/auth/login", {"adminPassword": password}
    )
    require(old_status == 401, "pre-reset administrator password rejected after reprovision")
    return address


def diagnostics_smoke(
    device: Device, manifest: dict[str, Any]
) -> DiagnosticsObservation:
    payload = get_json(device, "/api/v1/diagnostics")
    try:
        observed = validate_diagnostics(manifest, payload)
    except ValueError as error:
        raise SystemExit(f"FAIL: {error}") from error
    require(True, "on-device diagnostics match manifest ELF provenance")
    return observed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flash-manifest", type=Path, required=True)
    parser.add_argument("--firmware-sha", required=True)
    parser.add_argument("--flash-port", required=True)
    parser.add_argument("--flash-baud", type=int, default=460800)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument(
        "--console", default=os.environ.get("HIL_CONSOLE", hil_state.DEFAULT_CONSOLE)
    )
    parser.add_argument("--board", default="ESP32-S3R8")
    args = parser.parse_args()
    if not SHA40_RE.fullmatch(args.firmware_sha):
        raise SystemExit("error: --firmware-sha must be an exact 40-character lowercase SHA")
    if args.flash_baud <= 0:
        raise SystemExit("error: --flash-baud must be positive")
    if Path(args.flash_port).resolve() == Path(args.console).resolve():
        raise SystemExit("error: --flash-port and --console must identify distinct devices")
    if args.output.exists():
        raise SystemExit(
            "error: --output already exists; H12 evidence is immutable, choose a new path"
        )

    manifest, _ = flash_manifest.load_manifest(args.flash_manifest, args.firmware_sha)
    flash_manifest.validate_source_checkout(manifest, REPO_ROOT, args.firmware_sha)
    manifest_digest = manifest_sha256(args.flash_manifest)
    print("H12-122 final hardware acceptance")
    print(f"firmware_sha={args.firmware_sha}")
    print(f"board={args.board}")
    print(f"host={platform.platform()}")
    print(f"flash_port={args.flash_port}")
    print(f"console={args.console}")
    require(True, "release manifest is clean, production, exact-SHA, fully hashed, and includes webfs")

    started = time.time()
    steps: list[dict[str, object]] = []
    try:
        subprocess.run(
            [
                sys.executable,
                str(FLASH_SCRIPT),
                "--flash-manifest",
                str(args.flash_manifest),
                "--firmware-sha",
                args.firmware_sha,
                "--port",
                args.flash_port,
                "--baud",
                str(args.flash_baud),
            ],
            check=True,
        )
    except subprocess.CalledProcessError as error:
        raise SystemExit(f"FAIL: exact release flashing failed: {error}") from error
    require(
        manifest_sha256(args.flash_manifest) == manifest_digest,
        "release manifest remained byte-identical through flashing",
    )
    steps.append({"name": "releaseFlash", "result": "passed"})

    address = wait_for_authenticated_service(
        hil_state.device_ip(), args.console, allow_uart_reconnect=True
    )
    device = Device(address)
    device.login()
    steps.append({"name": "login", "result": "passed"})
    initial_diag = diagnostics_smoke(device, manifest)
    steps.append({"name": "initialProvenance", "result": "passed"})

    active_send_smoke(device)
    steps.append({"name": "activeSend", "result": "passed"})
    confirmation_send_smoke(device, args.console)
    steps.append({"name": "confirmationSend", "result": "passed"})
    cancel_send_smoke(device)
    steps.append({"name": "cancel", "result": "passed"})

    blob_id, blob_size = snapshot_smoke(device)
    steps.append({"name": "snapshotSaveLoad", "result": "passed", "bytes": blob_size})

    change_password_smoke(device)
    steps.append({"name": "passwordChange", "result": "passed"})

    device = Device(device.ip)
    device.login()
    pre_restart_diag = diagnostics_smoke(device, manifest)
    address, restart_requested_at = restart_smoke(device)
    device = Device(address)
    device.login()
    restart_diag = diagnostics_smoke(device, manifest)
    restart_elapsed_ms = int((time.monotonic() - restart_requested_at) * 1000)
    try:
        validate_restart_observation(
            pre_restart_diag, restart_diag, restart_elapsed_ms
        )
    except ValueError as error:
        raise SystemExit(f"FAIL: {error}") from error
    require(True, "software restart is proven by reset reason and uptime discontinuity")
    steps.append(
        {
            "name": "restart",
            "result": "passed",
            "resetReason": restart_diag["resetReason"],
            "preRestartUptimeMs": pre_restart_diag["uptimeMs"],
            "postRestartUptimeMs": restart_diag["uptimeMs"],
            "elapsedMs": restart_elapsed_ms,
        }
    )

    address = factory_reset_and_reprovision(device, args.console)
    device = Device(address)
    device.login()
    final_diag = diagnostics_smoke(device, manifest)
    steps.append({"name": "factoryResetReprovision", "result": "passed"})

    status, payload = device.get("/api/v1/blob")
    require(status == 200, "blob list available after reprovision")
    blob_list = response_data(payload)
    blobs = blob_list.get("blobs") if isinstance(blob_list, dict) else None
    require(isinstance(blobs, list), "post-reset blob list has expected shape")
    require(
        all(not isinstance(entry, dict) or entry.get("id") != blob_id for entry in blobs),
        "factory reset erased the H12 snapshot",
    )
    steps.append({"name": "factoryResetErasedSnapshot", "result": "passed"})

    require(
        initial_diag["buildId"] == restart_diag["buildId"] == final_diag["buildId"],
        "same production build remains flashed through restart/reset/reprovision",
    )
    steps.append({"name": "productionImageAtSignoff", "result": "passed"})

    evidence = {
        "schemaVersion": 1,
        "task": "H12-122",
        "firmwareSha": args.firmware_sha,
        "board": args.board,
        "host": platform.platform(),
        "flashDevice": args.flash_port,
        "flashBaud": args.flash_baud,
        "consoleDevice": args.console,
        "buildType": manifest["buildType"],
        "espIdfVersion": manifest["espIdfVersion"],
        "manifestSha256": manifest_digest,
        "manifestBuildId": manifest["diagnosticsBuildId"],
        "observedBuildId": final_diag["buildId"],
        "firmwareVersion": final_diag["firmwareVersion"],
        "startedUtc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(started)),
        "completedUtc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "steps": steps,
        "result": "passed",
    }
    ensure_evidence_has_no_secret_keys(evidence)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"H12-122 hardware acceptance: PASS ({args.output})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
