#!/usr/bin/env python3
"""Collect fail-closed V2-035 evidence from a physical ESP32-S3 board."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import http.cookiejar
import http.client
import json
import os
import re
import socket
import ssl
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import zlib
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

BLOB_MAX_BYTES = 131_072
USERDATA_BYTES = 524_288
STATE_SCHEMA = 1
FIRMWARE_COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")
REQUIRED_SCENARIOS = (
    "power_cycle_persistence",
    "numeric_ordering",
    "delete_preservation",
    "interrupted_upload_no_partial_final",
    "reboot_temporary_cleanup",
    "storage_full_507_preservation",
    "mount_failure_no_format",
)


class EvidenceError(RuntimeError):
    """Raised when a hardware observation fails a required invariant."""


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65_536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def deterministic_bytes(label: str, length: int) -> bytes:
    output = bytearray()
    counter = 0
    while len(output) < length:
        output.extend(hashlib.sha256(f"{label}:{counter}".encode()).digest())
        counter += 1
    return bytes(output[:length])


def gzip_member(payload: bytes, extra_length: int = 0) -> bytes:
    if extra_length < 0 or extra_length > 65_535:
        raise EvidenceError("gzip extra field length is out of range")
    flags = 0x04 if extra_length else 0x00
    header = bytearray(b"\x1f\x8b\x08")
    header.extend(bytes((flags, 0, 0, 0, 0, 0, 255)))
    if extra_length:
        header.extend(extra_length.to_bytes(2, "little"))
        header.extend(b"V" * extra_length)
    compressor = zlib.compressobj(level=0, wbits=-15)
    deflated = compressor.compress(payload) + compressor.flush()
    trailer = zlib.crc32(payload).to_bytes(4, "little")
    trailer += (len(payload) & 0xFFFFFFFF).to_bytes(4, "little")
    return bytes(header) + deflated + trailer


def exact_gzip_payload(label: str, target_length: int = BLOB_MAX_BYTES) -> bytes:
    if target_length < 128:
        raise EvidenceError("target gzip length is too small")
    first = gzip_member(deterministic_bytes(label, target_length - 64))
    remainder = target_length - len(first)
    require(remainder >= 25, "target leaves too little room for a padding gzip member")
    padding = gzip_member(b"", extra_length=remainder - 25)
    candidate = first + padding
    require(len(candidate) == target_length, "exact-size gzip construction drifted")
    gzip.decompress(candidate)
    return candidate


def small_payload(label: str) -> bytes:
    return gzip.compress(deterministic_bytes(label, 4096), compresslevel=6, mtime=0)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise EvidenceError(message)


def read_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise EvidenceError(f"could not read {path}: {error}") from error
    require(isinstance(value, dict), f"{path} must contain a JSON object")
    return value


def write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(temporary, path)


def load_state(path: Path, expected_phase: str | None = None) -> dict[str, Any]:
    state = read_json(path)
    require(state.get("schemaVersion") == STATE_SCHEMA, "unsupported evidence state schema")
    if expected_phase is not None:
        require(state.get("phase") == expected_phase,
                f"expected phase {expected_phase!r}, found {state.get('phase')!r}")
    return state


def password_from_environment(name: str) -> str:
    password = os.environ.get(name, "")
    require(bool(password), f"set {name} instead of placing the password on the command line")
    return password


def parse_success(body: bytes) -> Any:
    try:
        envelope = json.loads(body.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as error:
        raise EvidenceError(f"device returned invalid JSON: {error}") from error
    require(isinstance(envelope, dict), "device response must be a JSON object")
    require(envelope.get("ok") is True, f"device returned a failed envelope: {envelope!r}")
    require("data" in envelope, "device success envelope is missing data")
    return envelope["data"]


class DeviceApi:
    def __init__(self, base_url: str, timeout: float = 15.0) -> None:
        parsed = urllib.parse.urlparse(base_url.rstrip("/"))
        require(parsed.scheme in ("http", "https"), "base URL must use http or https")
        require(bool(parsed.hostname), "base URL must include a host")
        self.base_url = base_url.rstrip("/")
        self.timeout = timeout
        self.cookies = http.cookiejar.CookieJar()
        self.opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(self.cookies))

    def _request(self, method: str, path: str, body: bytes | None = None,
                 headers: dict[str, str] | None = None) -> tuple[int, bytes, Any]:
        request = urllib.request.Request(
            self.base_url + path,
            data=body,
            headers=headers or {},
            method=method,
        )
        try:
            with self.opener.open(request, timeout=self.timeout) as response:
                return response.status, response.read(), response.headers
        except urllib.error.HTTPError as error:
            return error.code, error.read(), error.headers
        except OSError as error:
            raise EvidenceError(f"device request failed: {method} {path}: {error}") from error

    def login(self, password: str) -> None:
        body = json.dumps({"password": password}, separators=(",", ":")).encode()
        status, response, _ = self._request(
            "POST", "/api/v1/login", body, {"Content-Type": "application/json"}
        )
        require(status == 200, f"login failed with HTTP {status}: {response!r}")
        parse_success(response)
        require(any(True for _ in self.cookies), "login succeeded without setting a session cookie")

    def list_blobs(self) -> list[dict[str, Any]]:
        status, body, _ = self._request("GET", "/api/v1/blobs")
        require(status == 200, f"blob list failed with HTTP {status}: {body!r}")
        data = parse_success(body)
        require(isinstance(data, dict) and isinstance(data.get("blobs"), list),
                "blob list response has the wrong schema")
        entries = data["blobs"]
        for entry in entries:
            require(isinstance(entry, dict) and isinstance(entry.get("id"), str),
                    "blob list contains an invalid entry")
        return entries

    def create_blob(self, payload: bytes) -> tuple[int, Any]:
        status, body, _ = self._request(
            "POST", "/api/v1/blobs", payload, {"Content-Type": "application/gzip"}
        )
        if 200 <= status < 300:
            return status, parse_success(body)
        try:
            parsed: Any = json.loads(body.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            parsed = {"raw": body.hex()}
        return status, parsed

    def load_blob(self, blob_id: str) -> bytes:
        status, body, _ = self._request("GET", f"/api/v1/blobs/{blob_id}")
        require(status == 200, f"blob {blob_id} load failed with HTTP {status}")
        return body

    def delete_blob(self, blob_id: str) -> None:
        status, body, _ = self._request("DELETE", f"/api/v1/blobs/{blob_id}")
        require(status in (200, 204),
                f"blob {blob_id} delete failed with HTTP {status}: {body!r}")

    def status(self) -> Any:
        status, body, _ = self._request("GET", "/api/v1/status")
        require(status == 200, f"status failed with HTTP {status}: {body!r}")
        return parse_success(body)

    def cookie_header(self, path: str) -> str:
        request = urllib.request.Request(self.base_url + path)
        self.cookies.add_cookie_header(request)
        cookie = request.get_header("Cookie")
        require(bool(cookie), "authenticated request has no session cookie")
        return str(cookie)


def blob_ids(api: DeviceApi) -> list[str]:
    return [entry["id"] for entry in api.list_blobs()]


def snapshot(api: DeviceApi) -> dict[str, str]:
    result: dict[str, str] = {}
    for blob_id in blob_ids(api):
        require(blob_id not in result, f"duplicate blob ID {blob_id}")
        result[blob_id] = sha256_bytes(api.load_blob(blob_id))
    return result


def verify_snapshot(api: DeviceApi, expected: dict[str, str], exact_ids: bool = False) -> None:
    actual_ids = blob_ids(api)
    if exact_ids:
        require(set(actual_ids) == set(expected),
                f"blob ID set changed: expected {sorted(expected)}, found {actual_ids}")
    for blob_id, expected_hash in expected.items():
        actual_hash = sha256_bytes(api.load_blob(blob_id))
        require(actual_hash == expected_hash, f"blob {blob_id} changed byte-for-byte")


def add_scenario(state: dict[str, Any], name: str, details: dict[str, Any]) -> None:
    scenarios = state.setdefault("scenarios", {})
    require(name in REQUIRED_SCENARIOS, f"unknown V2-035 scenario {name}")
    scenarios[name] = {"status": "pass", "observedAt": utc_now(), **details}


def recursive_values(value: Any, key: str) -> list[Any]:
    found: list[Any] = []
    if isinstance(value, dict):
        for child_key, child in value.items():
            if child_key == key:
                found.append(child)
            found.extend(recursive_values(child, key))
    elif isinstance(value, list):
        for child in value:
            found.extend(recursive_values(child, key))
    return found


def command_start(args: argparse.Namespace) -> None:
    require(not args.state.exists(), f"refusing to overwrite existing state {args.state}")
    api = DeviceApi(args.base_url, args.timeout)
    api.login(password_from_environment(args.password_env))
    firmware_commit = args.firmware_sha.strip().lower()
    require(
        FIRMWARE_COMMIT_PATTERN.fullmatch(firmware_commit) is not None,
        "firmware SHA must be exactly 40 hexadecimal characters",
    )
    baseline = snapshot(api)

    created: list[dict[str, str]] = []
    for label in ("ordering-a", "ordering-b", "ordering-c"):
        payload = small_payload(label)
        status, data = api.create_blob(payload)
        require(status == 201 and isinstance(data, dict) and isinstance(data.get("id"), str),
                f"blob creation returned HTTP {status}: {data!r}")
        created.append({"id": data["id"], "sha256": sha256_bytes(payload)})

    numeric = [int(item["id"]) for item in created]
    require(numeric == sorted(numeric) and len(set(numeric)) == 3,
            f"created IDs are not strictly increasing: {numeric}")
    listed = blob_ids(api)
    require([int(value) for value in listed] == sorted(int(value) for value in listed),
            f"blob list is not in numeric order: {listed}")
    for item in created:
        require(sha256_bytes(api.load_blob(item["id"])) == item["sha256"],
                f"new blob {item['id']} did not round-trip byte-identically")

    removed = created[1]
    api.delete_blob(removed["id"])
    expected = dict(baseline)
    for item in (created[0], created[2]):
        expected[item["id"]] = item["sha256"]
    verify_snapshot(api, expected, exact_ids=True)

    state: dict[str, Any] = {
        "schemaVersion": STATE_SCHEMA,
        "task": "V2-035",
        "createdAt": utc_now(),
        "baseUrl": args.base_url.rstrip("/"),
        "firmwareCommit": firmware_commit,
        "targetHardware": "ESP32-S3R8",
        "phase": "awaiting_power_cycle",
        "baseline": baseline,
        "sentinels": [created[0], created[2]],
        "scenarios": {},
    }
    add_scenario(state, "numeric_ordering", {"listedIds": listed, "createdIds": [x["id"] for x in created]})
    add_scenario(state, "delete_preservation", {"deletedId": removed["id"], "preservedHashes": expected})
    write_json(args.state, state)
    print(f"PASS: ordering and deletion evidence written to {args.state}")
    print("Physically remove power from the ESP32-S3, restore power, wait for Wi-Fi, then run verify-power-cycle.")


def api_from_state(state: dict[str, Any], args: argparse.Namespace) -> DeviceApi:
    api = DeviceApi(str(state["baseUrl"]), args.timeout)
    api.login(password_from_environment(args.password_env))
    return api


def expected_live_snapshot(state: dict[str, Any]) -> dict[str, str]:
    expected = dict(state["baseline"])
    for item in state["sentinels"]:
        expected[item["id"]] = item["sha256"]
    return expected


def command_verify_power_cycle(args: argparse.Namespace) -> None:
    state = load_state(args.state, "awaiting_power_cycle")
    api = api_from_state(state, args)
    expected = expected_live_snapshot(state)
    verify_snapshot(api, expected, exact_ids=True)
    add_scenario(state, "power_cycle_persistence", {"verifiedHashes": expected})
    state["phase"] = "ready_for_interrupted_upload"
    write_json(args.state, state)
    print("PASS: all pre-power-cycle blobs reloaded byte-identically")
    print("Next run arm-interrupted-upload and cut board power only after the displayed CUT POWER NOW banner.")


def open_upload_connection(api: DeviceApi) -> http.client.HTTPConnection:
    parsed = urllib.parse.urlparse(api.base_url)
    port = parsed.port or (443 if parsed.scheme == "https" else 80)
    connection_type = http.client.HTTPSConnection if parsed.scheme == "https" else http.client.HTTPConnection
    context = ssl.create_default_context() if parsed.scheme == "https" else None
    if context is None:
        connection = connection_type(parsed.hostname, port, timeout=api.timeout)
    else:
        connection = connection_type(parsed.hostname, port, timeout=api.timeout, context=context)
    path = (parsed.path.rstrip("/") if parsed.path else "") + "/api/v1/blobs"
    connection.putrequest("POST", path, skip_host=True)
    connection.putheader("Host", parsed.netloc)
    connection.putheader("Content-Type", "application/gzip")
    connection.putheader("Content-Length", str(BLOB_MAX_BYTES))
    connection.putheader("Cookie", api.cookie_header("/api/v1/blobs"))
    connection.putheader("Connection", "close")
    connection.endheaders()
    return connection


def command_arm_interrupted_upload(args: argparse.Namespace) -> None:
    state = load_state(args.state, "ready_for_interrupted_upload")
    api = api_from_state(state, args)
    before = snapshot(api)
    require(before == expected_live_snapshot(state), "live blobs changed before interruption test")
    payload = exact_gzip_payload("power-cut-upload")
    connection = open_upload_connection(api)
    sent = 0
    disconnected = False
    try:
        while sent < len(payload):
            end = min(sent + 4096, len(payload))
            connection.send(payload[sent:end])
            sent = end
            if sent == 16_384:
                print("\n=== CUT POWER NOW: remove ESP32-S3 power, then restore it ===\n", flush=True)
            time.sleep(args.chunk_delay)
        response = connection.getresponse()
        body = response.read()
        raise EvidenceError(
            f"upload completed with HTTP {response.status}; no power interruption was observed: {body!r}"
        )
    except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError, socket.timeout, OSError):
        disconnected = True
    finally:
        connection.close()
    require(disconnected and sent >= 16_384,
            "connection ended before enough data was written to establish a temporary upload")
    state["interruptedUpload"] = {
        "attemptedAt": utc_now(),
        "bytesSentBeforeDisconnect": sent,
        "contentLength": len(payload),
        "beforeHashes": before,
    }
    state["phase"] = "awaiting_interrupted_upload_reboot"
    write_json(args.state, state)
    print(f"Power loss observed after {sent} bytes; wait for reboot, then run verify-interrupted-upload.")


def command_verify_interrupted_upload(args: argparse.Namespace) -> None:
    state = load_state(args.state, "awaiting_interrupted_upload_reboot")
    api = api_from_state(state, args)
    before = state["interruptedUpload"]["beforeHashes"]
    verify_snapshot(api, before, exact_ids=True)
    status = api.status()
    temporary_values = recursive_values(status, "temporaryFiles")
    scan_failed_values = recursive_values(status, "scanFailed")
    require(temporary_values and all(value == 0 for value in temporary_values),
            f"diagnostics did not prove temporary-file cleanup: {temporary_values}")
    require(not scan_failed_values or all(value is False for value in scan_failed_values),
            f"diagnostics reported a failed storage scan: {scan_failed_values}")
    details = {
        "bytesSentBeforePowerLoss": state["interruptedUpload"]["bytesSentBeforeDisconnect"],
        "preservedHashes": before,
        "temporaryFiles": temporary_values,
        "scanFailed": scan_failed_values,
    }
    add_scenario(state, "interrupted_upload_no_partial_final", details)
    add_scenario(state, "reboot_temporary_cleanup", details)
    state["phase"] = "ready_for_storage_full"
    write_json(args.state, state)
    print("PASS: interrupted upload produced no final blob and reboot cleanup reports zero temporary files")


def command_fill_storage(args: argparse.Namespace) -> None:
    state = load_state(args.state, "ready_for_storage_full")
    api = api_from_state(state, args)
    original = snapshot(api)
    require(original == expected_live_snapshot(state), "live blobs changed before storage-full test")
    payload = exact_gzip_payload("storage-full")
    fill_created: list[dict[str, str]] = []
    failure: Any = None
    for attempt in range(1, 9):
        status, data = api.create_blob(payload)
        if status == 201:
            require(isinstance(data, dict) and isinstance(data.get("id"), str),
                    "successful fill upload returned an invalid response")
            fill_created.append({"id": data["id"], "sha256": sha256_bytes(payload)})
            continue
        require(status == 507, f"within-limit storage exhaustion returned HTTP {status}: {data!r}")
        failure = {"attempt": attempt, "httpStatus": status, "response": data}
        break
    require(failure is not None, "storage never returned HTTP 507 within eight maximum-size uploads")
    preserved = dict(original)
    for item in fill_created:
        preserved[item["id"]] = item["sha256"]
    verify_snapshot(api, preserved, exact_ids=True)
    for item in reversed(fill_created):
        api.delete_blob(item["id"])
    verify_snapshot(api, original, exact_ids=True)
    add_scenario(
        state,
        "storage_full_507_preservation",
        {
            "maximumUploadBytes": len(payload),
            "committedBefore507": fill_created,
            "failure": failure,
            "preservedHashes": preserved,
            "cleanupVerified": True,
        },
    )
    state["phase"] = "ready_for_mount_failure_record"
    write_json(args.state, state)
    print("PASS: HTTP 507 observed and every committed final blob remained byte-identical")
    print("Run the documented backed-up userdata corruption procedure, then record-mount-failure.")


def require_image(path: Path, label: str) -> str:
    try:
        size = path.stat().st_size
    except OSError as error:
        raise EvidenceError(f"could not stat {label} image {path}: {error}") from error
    require(size == USERDATA_BYTES, f"{label} image must be exactly {USERDATA_BYTES} bytes, found {size}")
    return sha256_file(path)


def relevant_log_lines(text: str) -> list[str]:
    lines = []
    for line in text.splitlines():
        lowered = line.lower()
        if any(token in lowered for token in ("littlefs", "userdata", "storage", "mount")):
            lines.append(line[:500])
    return lines[-100:]


def command_record_mount_failure(args: argparse.Namespace) -> None:
    state = load_state(args.state, "ready_for_mount_failure_record")
    backup_hash = require_image(args.backup_image, "backup")
    corrupt_hash = require_image(args.corrupt_image, "corrupt")
    post_hash = require_image(args.post_boot_image, "post-boot")
    restored_hash = require_image(args.restored_image, "restored")
    require(corrupt_hash == post_hash,
            "userdata changed after the failed mount; formatting or another mutation may have occurred")
    require(backup_hash == restored_hash, "restored userdata does not match the pre-test backup")
    require(backup_hash != corrupt_hash, "corrupt image is identical to the original userdata image")
    try:
        log_text = args.serial_log.read_text(encoding="utf-8", errors="replace")
    except OSError as error:
        raise EvidenceError(f"could not read serial log {args.serial_log}: {error}") from error
    lowered = log_text.lower()
    require("mount" in lowered and any(token in lowered for token in ("fail", "unavailable", "error")),
            "serial log does not contain an explicit mount failure")
    forbidden = ("formatting userdata", "formatted userdata", "esp_littlefs_format", "format succeeded")
    require(not any(token in lowered for token in forbidden),
            "serial log contains an explicit formatting marker")
    excerpt = relevant_log_lines(log_text)
    require(bool(excerpt), "serial log has no storage-related evidence lines")
    add_scenario(
        state,
        "mount_failure_no_format",
        {
            "partitionBytes": USERDATA_BYTES,
            "backupSha256": backup_hash,
            "corruptSha256": corrupt_hash,
            "postBootSha256": post_hash,
            "restoredSha256": restored_hash,
            "serialLogSha256": sha256_file(args.serial_log),
            "serialLogExcerpt": excerpt,
        },
    )
    state["phase"] = "ready_to_finalize"
    write_json(args.state, state)
    print("PASS: failed mount left the corrupt userdata partition byte-identical and backup restoration matched")


def validate_complete_state(state: dict[str, Any]) -> None:
    firmware_commit = state.get("firmwareCommit")
    require(
        isinstance(firmware_commit, str)
        and FIRMWARE_COMMIT_PATTERN.fullmatch(firmware_commit) is not None,
        "evidence is not bound to an exact firmware commit",
    )
    require(
        state.get("targetHardware") == "ESP32-S3R8",
        "evidence target hardware is not ESP32-S3R8",
    )
    scenarios = state.get("scenarios")
    require(isinstance(scenarios, dict), "evidence is missing scenarios")
    for name in REQUIRED_SCENARIOS:
        value = scenarios.get(name)
        require(isinstance(value, dict) and value.get("status") == "pass",
                f"scenario {name} is not supported by passing physical evidence")
    require(state.get("phase") in ("ready_to_finalize", "complete"),
            f"evidence phase is incomplete: {state.get('phase')!r}")


def command_finalize(args: argparse.Namespace) -> None:
    state = load_state(args.state, "ready_to_finalize")
    validate_complete_state(state)
    api = api_from_state(state, args)
    expected = expected_live_snapshot(state)
    verify_snapshot(api, expected, exact_ids=True)
    for item in reversed(state["sentinels"]):
        api.delete_blob(item["id"])
    verify_snapshot(api, state["baseline"], exact_ids=True)
    state["phase"] = "complete"
    state["completedAt"] = utc_now()
    state["testBlobCleanup"] = {"status": "pass", "verifiedAt": utc_now()}
    validate_complete_state(state)
    write_json(args.state, state)
    output = dict(state)
    output["evidenceSha256"] = sha256_bytes(
        json.dumps(state, sort_keys=True, separators=(",", ":")).encode()
    )
    write_json(args.output, output)
    print(f"PASS: complete V2-035 evidence written to {args.output}")


def command_validate(args: argparse.Namespace) -> None:
    state = read_json(args.evidence)
    validate_complete_state(state)
    require(state.get("testBlobCleanup", {}).get("status") == "pass",
            "test-created blob cleanup is not verified")
    print(f"PASS: {args.evidence} contains all seven required physical scenarios")


def add_online_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--state", type=Path, required=True)
    parser.add_argument("--password-env", default="V2_035_PASSWORD")
    parser.add_argument("--timeout", type=float, default=15.0)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    start = subparsers.add_parser("start", help="run ordering/delete tests and stage persistence")
    start.add_argument("--base-url", required=True)
    start.add_argument("--firmware-sha", required=True)
    add_online_arguments(start)
    start.set_defaults(function=command_start)

    verify_power = subparsers.add_parser("verify-power-cycle")
    add_online_arguments(verify_power)
    verify_power.set_defaults(function=command_verify_power_cycle)

    interrupt = subparsers.add_parser("arm-interrupted-upload")
    add_online_arguments(interrupt)
    interrupt.add_argument("--chunk-delay", type=float, default=1.0)
    interrupt.set_defaults(function=command_arm_interrupted_upload)

    verify_interrupt = subparsers.add_parser("verify-interrupted-upload")
    add_online_arguments(verify_interrupt)
    verify_interrupt.set_defaults(function=command_verify_interrupted_upload)

    fill = subparsers.add_parser("fill-storage")
    add_online_arguments(fill)
    fill.set_defaults(function=command_fill_storage)

    mount = subparsers.add_parser("record-mount-failure")
    mount.add_argument("--state", type=Path, required=True)
    mount.add_argument("--backup-image", type=Path, required=True)
    mount.add_argument("--corrupt-image", type=Path, required=True)
    mount.add_argument("--post-boot-image", type=Path, required=True)
    mount.add_argument("--restored-image", type=Path, required=True)
    mount.add_argument("--serial-log", type=Path, required=True)
    mount.set_defaults(function=command_record_mount_failure)

    finalize = subparsers.add_parser("finalize")
    add_online_arguments(finalize)
    finalize.add_argument("--output", type=Path, required=True)
    finalize.set_defaults(function=command_finalize)

    validate = subparsers.add_parser("validate")
    validate.add_argument("--evidence", type=Path, required=True)
    validate.set_defaults(function=command_validate)
    return parser


def main() -> int:
    try:
        args = build_parser().parse_args()
        args.function(args)
        return 0
    except EvidenceError as error:
        print(f"V2-035 FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
