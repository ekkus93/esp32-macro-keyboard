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
import secrets
import shutil
import socket
import ssl
import subprocess
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
STATE_SCHEMA = 3
ESP_IDF_VERSION = "ESP-IDF v5.5.5"
FIRMWARE_COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")
BUILD_ID_PATTERN = re.compile(r"^[0-9a-f]{39}$")
ELF_SHA_OUTPUT_PATTERN = re.compile(r"ELF file SHA256:\s*([0-9A-Fa-f]{64})")
AUTH_LOGIN_PATH = "/api/v1/auth/login"
BLOB_COLLECTION_PATH = "/api/v1/blob"
DIAGNOSTICS_PATH = "/api/v1/diagnostics"
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


def resolve_manifest_app_image(manifest_path: Path, manifest: dict[str, Any]) -> Path:
    app_image = manifest.get("appImage")
    require(isinstance(app_image, str) and bool(app_image),
            "flash manifest appImage is missing")
    relative = Path(app_image)
    require(not relative.is_absolute() and ".." not in relative.parts,
            "flash manifest appImage must remain inside the build directory")
    build_directory = manifest_path.parent.resolve()
    resolved = (build_directory / relative).resolve()
    require(resolved.is_relative_to(build_directory),
            "flash manifest appImage escapes the build directory")
    require(resolved.is_file(), f"flash manifest application image is missing: {resolved}")
    return resolved


def read_app_elf_sha256(app_image: Path) -> str:
    esptool = shutil.which("esptool.py")
    require(esptool is not None,
            "esptool.py is required to verify the flash manifest application image")
    try:
        result = subprocess.run(
            [esptool, "image_info", "--version", "2", str(app_image)],
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as error:
        raise EvidenceError(f"could not run esptool.py image_info: {error}") from error
    output = result.stdout + "\n" + result.stderr
    require(result.returncode == 0,
            f"esptool.py image_info failed with exit {result.returncode}: {output.strip()}")
    match = ELF_SHA_OUTPUT_PATTERN.search(output)
    require(match is not None,
            "esptool.py image_info did not report a full ELF file SHA256")
    return match.group(1).lower()


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


def owned_blobs(state: dict[str, Any]) -> list[dict[str, str]]:
    value = state.setdefault("ownedBlobs", [])
    require(isinstance(value, list), "evidence ownedBlobs must be a list")
    seen: set[str] = set()
    for item in value:
        require(isinstance(item, dict)
                and isinstance(item.get("id"), str)
                and isinstance(item.get("sha256"), str)
                and SHA256_PATTERN.fullmatch(item["sha256"]) is not None,
                "evidence ownedBlobs contains an invalid entry")
        require(item["id"] not in seen, f"duplicate collector-owned blob ID {item['id']}")
        seen.add(item["id"])
    return value


def journal_created_blob(state_path: Path, state: dict[str, Any], item: dict[str, str],
                         stage_key: str | None = None) -> None:
    current = owned_blobs(state)
    require(all(existing["id"] != item["id"] for existing in current),
            f"collector-owned blob ID {item['id']} was already recorded")
    current.append(dict(item))
    if stage_key is not None:
        stage = state.setdefault(stage_key, [])
        require(isinstance(stage, list), f"evidence {stage_key} must be a list")
        stage.append(dict(item))
    write_json(state_path, state)


def journal_deleted_blob(state_path: Path, state: dict[str, Any], blob_id: str) -> None:
    state["ownedBlobs"] = [item for item in owned_blobs(state) if item["id"] != blob_id]
    write_json(state_path, state)


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


def load_flash_manifest(path: Path) -> dict[str, str]:
    manifest = read_json(path)
    git_commit = manifest.get("gitCommit")
    app_image_sha256 = manifest.get("appImageSha256")
    app_elf_sha256 = manifest.get("appElfSha256")
    diagnostics_build_id = manifest.get("diagnosticsBuildId")
    esp_idf_version = manifest.get("espIdfVersion")
    require(isinstance(git_commit, str)
            and FIRMWARE_COMMIT_PATTERN.fullmatch(git_commit) is not None,
            "flash manifest gitCommit must be an exact 40-character SHA")
    require(manifest.get("gitDirty") is False,
            "flash manifest records a dirty build; rebuild from a clean checkout")
    require(manifest.get("buildType") == "production",
            "flash manifest is not a production build")
    require(esp_idf_version == ESP_IDF_VERSION,
            f"flash manifest must use {ESP_IDF_VERSION}, found {esp_idf_version!r}")
    require(isinstance(app_image_sha256, str)
            and SHA256_PATTERN.fullmatch(app_image_sha256) is not None,
            "flash manifest appImageSha256 is invalid")
    require(isinstance(app_elf_sha256, str)
            and SHA256_PATTERN.fullmatch(app_elf_sha256) is not None,
            "flash manifest appElfSha256 is invalid")
    require(isinstance(diagnostics_build_id, str)
            and BUILD_ID_PATTERN.fullmatch(diagnostics_build_id) is not None,
            "flash manifest diagnosticsBuildId is invalid")
    app_image = resolve_manifest_app_image(path, manifest)
    actual_image_sha256 = sha256_file(app_image)
    require(actual_image_sha256 == app_image_sha256,
            "flash manifest application-image SHA-256 does not match the actual image")
    actual_elf_sha256 = read_app_elf_sha256(app_image)
    require(actual_elf_sha256 == app_elf_sha256,
            "flash manifest ELF SHA-256 does not match esptool.py image_info")
    require(actual_elf_sha256.startswith(diagnostics_build_id),
            "flash manifest diagnosticsBuildId is not the verified ELF SHA prefix")
    return {
        "gitCommit": git_commit,
        "appImage": str(manifest["appImage"]),
        "appImageSha256": actual_image_sha256,
        "appElfSha256": actual_elf_sha256,
        "diagnosticsBuildId": diagnostics_build_id,
        "espIdfVersion": esp_idf_version,
        "flashManifestSha256": sha256_file(path),
    }


def verify_firmware_provenance(manifest: dict[str, str], diagnostics: dict[str, Any]) -> None:
    require(diagnostics["buildId"] == manifest["diagnosticsBuildId"],
            "board buildId does not match the exact application image in the flash manifest")


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
            "POST", AUTH_LOGIN_PATH, body, {"Content-Type": "application/json"}
        )
        require(status == 200, f"login failed with HTTP {status}: {response!r}")
        parse_success(response)
        require(any(True for _ in self.cookies), "login succeeded without setting a session cookie")

    def list_blobs(self) -> list[dict[str, Any]]:
        status, body, _ = self._request("GET", BLOB_COLLECTION_PATH)
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
            "POST", BLOB_COLLECTION_PATH, payload, {"Content-Type": "application/gzip"}
        )
        if 200 <= status < 300:
            return status, parse_success(body)
        try:
            parsed: Any = json.loads(body.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            parsed = {"raw": body.hex()}
        return status, parsed

    def load_blob(self, blob_id: str) -> bytes:
        status, body, _ = self._request("GET", f"{BLOB_COLLECTION_PATH}/{blob_id}")
        require(status == 200, f"blob {blob_id} load failed with HTTP {status}")
        return body

    def delete_blob(self, blob_id: str) -> None:
        status, body, _ = self._request("DELETE", f"{BLOB_COLLECTION_PATH}/{blob_id}")
        require(status in (200, 204),
                f"blob {blob_id} delete failed with HTTP {status}: {body!r}")

    def diagnostics(self) -> Any:
        status, body, _ = self._request("GET", DIAGNOSTICS_PATH)
        require(status == 200, f"diagnostics failed with HTTP {status}: {body!r}")
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


def parse_diagnostics(diagnostics: Any) -> dict[str, Any]:
    require(isinstance(diagnostics, dict), "diagnostics data must be an object")
    build_id = diagnostics.get("buildId")
    reset_reason = diagnostics.get("resetReason")
    uptime_ms = diagnostics.get("uptimeMs")
    blob_scan = diagnostics.get("blobScan")
    require(isinstance(build_id, str)
            and BUILD_ID_PATTERN.fullmatch(build_id) is not None,
            "diagnostics buildId must be a 39-character lowercase ELF SHA prefix")
    require(isinstance(reset_reason, str) and bool(reset_reason),
            "diagnostics resetReason is invalid")
    require(type(uptime_ms) is int and uptime_ms >= 0, "diagnostics uptimeMs is invalid")
    require(isinstance(blob_scan, dict), "diagnostics blobScan is invalid")
    temporary_count = blob_scan.get("temporaryFileCount")
    temporary_files = blob_scan.get("temporaryFiles")
    require(type(temporary_count) is int and temporary_count >= 0,
            "diagnostics temporaryFileCount is invalid")
    require(isinstance(temporary_files, list)
            and all(isinstance(value, str) for value in temporary_files),
            "diagnostics temporaryFiles is invalid")
    require(temporary_count == len(temporary_files),
            "diagnostics temporary-file count does not match its list")
    return {
        "buildId": build_id,
        "resetReason": reset_reason,
        "uptimeMs": uptime_ms,
        "temporaryFileCount": temporary_count,
        "temporaryFiles": temporary_files,
    }


def command_start(args: argparse.Namespace) -> None:
    require(not args.state.exists(), f"refusing to overwrite existing state {args.state}")
    manifest = load_flash_manifest(args.flash_manifest)
    api = DeviceApi(args.base_url, args.timeout)
    api.login(password_from_environment(args.password_env))
    initial_diagnostics = parse_diagnostics(api.diagnostics())
    verify_firmware_provenance(manifest, initial_diagnostics)
    baseline = snapshot(api)

    state: dict[str, Any] = {
        "schemaVersion": STATE_SCHEMA,
        "task": "V2-035",
        "createdAt": utc_now(),
        "baseUrl": args.base_url.rstrip("/"),
        "firmwareCommit": manifest["gitCommit"],
        "firmwareBuildId": manifest["diagnosticsBuildId"],
        "appImage": manifest["appImage"],
        "appImageSha256": manifest["appImageSha256"],
        "appElfSha256": manifest["appElfSha256"],
        "flashManifestSha256": manifest["flashManifestSha256"],
        "espIdfVersion": manifest["espIdfVersion"],
        "targetHardware": "ESP32-S3R8",
        "phase": "start_in_progress",
        "baseline": baseline,
        "sentinels": [],
        "ownedBlobs": [],
        "startCreated": [],
        "initialDiagnostics": initial_diagnostics,
        "scenarios": {},
    }
    write_json(args.state, state)

    created: list[dict[str, str]] = []
    for label in ("ordering-a", "ordering-b", "ordering-c"):
        payload = small_payload(f"{label}:{secrets.token_hex(16)}")
        status, data, item = create_journaled_blob(
            api, args.state, state, payload, "startCreated"
        )
        require(status == 201 and item is not None,
                f"blob creation returned HTTP {status}: {data!r}")
        created.append(item)

    numeric = [int(item["id"]) for item in created]
    require(numeric == sorted(numeric) and len(set(numeric)) == 3,
            f"created IDs are not strictly increasing: {numeric}")
    listed = blob_ids(api)
    listed_numeric = [int(value) for value in listed]
    require(listed_numeric == sorted(listed_numeric, reverse=True),
            f"blob list is not newest-first numeric order: {listed}")
    for item in created:
        require(sha256_bytes(api.load_blob(item["id"])) == item["sha256"],
                f"new blob {item['id']} did not round-trip byte-identically")

    removed = created[1]
    delete_owned_blob(api, state, args.state, removed)
    expected = dict(baseline)
    for item in (created[0], created[2]):
        expected[item["id"]] = item["sha256"]
    verify_snapshot(api, expected, exact_ids=True)

    state["sentinels"] = [created[0], created[2]]
    state.pop("startCreated", None)
    add_scenario(state, "numeric_ordering", {"listedIds": listed, "createdIds": [x["id"] for x in created]})
    add_scenario(state, "delete_preservation", {"deletedId": removed["id"], "preservedHashes": expected})
    state["phase"] = "awaiting_power_cycle"
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


def owned_snapshot(state: dict[str, Any]) -> dict[str, str]:
    return {item["id"]: item["sha256"] for item in owned_blobs(state)}


def expected_owned_snapshot(state: dict[str, Any]) -> dict[str, str]:
    expected = state.get("baseline")
    require(isinstance(expected, dict), "evidence baseline is invalid")
    result = dict(expected)
    result.update(owned_snapshot(state))
    return result


def pending_creation(state: dict[str, Any]) -> dict[str, Any] | None:
    value = state.get("pendingCreation")
    if value is None:
        return None
    require(isinstance(value, dict), "evidence pendingCreation must be an object")
    payload_sha256 = value.get("payloadSha256")
    before_hashes = value.get("beforeHashes")
    stage_key = value.get("stageKey")
    require(isinstance(payload_sha256, str)
            and SHA256_PATTERN.fullmatch(payload_sha256) is not None,
            "pending creation payload SHA-256 is invalid")
    require(isinstance(before_hashes, dict)
            and all(isinstance(blob_id, str)
                    and isinstance(blob_hash, str)
                    and SHA256_PATTERN.fullmatch(blob_hash) is not None
                    for blob_id, blob_hash in before_hashes.items()),
            "pending creation beforeHashes is invalid")
    require(stage_key is None or isinstance(stage_key, str),
            "pending creation stageKey is invalid")
    return value


def begin_pending_creation(api: DeviceApi, state_path: Path, state: dict[str, Any],
                           payload: bytes, stage_key: str | None) -> None:
    require(pending_creation(state) is None,
            "another blob creation is already pending recovery")
    before = snapshot(api)
    require(before == expected_owned_snapshot(state),
            "live blobs changed before the journaled creation request")
    state["pendingCreation"] = {
        "startedAt": utc_now(),
        "payloadSha256": sha256_bytes(payload),
        "beforeHashes": before,
        "stageKey": stage_key,
    }
    write_json(state_path, state)


def finish_pending_creation(state_path: Path, state: dict[str, Any],
                            item: dict[str, str]) -> None:
    pending = pending_creation(state)
    require(pending is not None, "no pending blob creation is available to finish")
    require(item["sha256"] == pending["payloadSha256"],
            "created blob hash does not match the pending creation intent")
    require(item["id"] not in pending["beforeHashes"],
            f"created blob ID {item['id']} already existed before the request")
    current = owned_blobs(state)
    require(all(existing["id"] != item["id"] for existing in current),
            f"collector-owned blob ID {item['id']} was already recorded")
    current.append(dict(item))
    stage_key = pending["stageKey"]
    if stage_key is not None:
        stage = state.setdefault(stage_key, [])
        require(isinstance(stage, list), f"evidence {stage_key} must be a list")
        stage.append(dict(item))
    state.pop("pendingCreation", None)
    write_json(state_path, state)


def clear_pending_creation_if_unchanged(api: DeviceApi, state_path: Path,
                                        state: dict[str, Any]) -> None:
    pending = pending_creation(state)
    require(pending is not None, "no pending blob creation is available to clear")
    require(snapshot(api) == pending["beforeHashes"],
            "failed blob creation changed the live blob set; run recover-cleanup")
    state.pop("pendingCreation", None)
    write_json(state_path, state)


def create_journaled_blob(api: DeviceApi, state_path: Path, state: dict[str, Any],
                          payload: bytes, stage_key: str) -> tuple[int, Any, dict[str, str] | None]:
    begin_pending_creation(api, state_path, state, payload, stage_key)
    status, data = api.create_blob(payload)
    if status != 201:
        clear_pending_creation_if_unchanged(api, state_path, state)
        return status, data, None
    require(isinstance(data, dict) and isinstance(data.get("id"), str),
            "successful blob creation returned an invalid response")
    item = {"id": data["id"], "sha256": sha256_bytes(payload)}
    require(sha256_bytes(api.load_blob(item["id"])) == item["sha256"],
            f"new blob {item['id']} did not round-trip byte-identically")
    finish_pending_creation(state_path, state, item)
    return status, data, item


def reconcile_pending_creation(api: DeviceApi, state_path: Path,
                               state: dict[str, Any]) -> dict[str, str] | None:
    pending = pending_creation(state)
    if pending is None:
        return None
    current = snapshot(api)
    before = pending["beforeHashes"]
    for blob_id, expected_hash in before.items():
        require(current.get(blob_id) == expected_hash,
                f"blob {blob_id} changed while a creation intent was pending")
    unknown = sorted(set(current) - set(before))
    require(len(unknown) <= 1,
            f"pending creation produced multiple unknown blob IDs: {unknown}")
    if not unknown:
        state.pop("pendingCreation", None)
        write_json(state_path, state)
        return None
    blob_id = unknown[0]
    require(current[blob_id] == pending["payloadSha256"],
            f"unknown blob {blob_id} does not match the pending creation hash")
    item = {"id": blob_id, "sha256": current[blob_id]}
    finish_pending_creation(state_path, state, item)
    return item


def delete_owned_blob(api: DeviceApi, state: dict[str, Any], state_path: Path,
                      item: dict[str, str]) -> None:
    current_ids = set(blob_ids(api))
    if item["id"] in current_ids:
        require(sha256_bytes(api.load_blob(item["id"])) == item["sha256"],
                f"collector-owned blob {item['id']} changed before cleanup")
        api.delete_blob(item["id"])
    journal_deleted_blob(state_path, state, item["id"])


def verify_recoverable_snapshot(api: DeviceApi, state: dict[str, Any]) -> None:
    baseline = state.get("baseline")
    require(isinstance(baseline, dict), "evidence baseline is invalid")
    collector = owned_snapshot(state)
    actual_ids = set(blob_ids(api))
    allowed_ids = set(baseline) | set(collector)
    unknown = actual_ids - allowed_ids
    require(not unknown,
            f"refusing recovery because unowned blob IDs appeared: {sorted(unknown)}")
    for blob_id, expected_hash in baseline.items():
        require(blob_id in actual_ids, f"baseline blob {blob_id} disappeared")
        require(sha256_bytes(api.load_blob(blob_id)) == expected_hash,
                f"baseline blob {blob_id} changed byte-for-byte")
    for blob_id, expected_hash in collector.items():
        if blob_id in actual_ids:
            require(sha256_bytes(api.load_blob(blob_id)) == expected_hash,
                    f"collector-owned blob {blob_id} changed byte-for-byte")


def command_recover_cleanup(args: argparse.Namespace) -> None:
    state = load_state(args.state)
    require(state.get("phase") != "complete", "complete evidence cannot be recovered or discarded")
    api = api_from_state(state, args)
    reconcile_pending_creation(api, args.state, state)
    verify_recoverable_snapshot(api, state)
    for item in list(reversed(owned_blobs(state))):
        delete_owned_blob(api, state, args.state, item)
    verify_snapshot(api, state["baseline"], exact_ids=True)
    state["phase"] = "recovered"
    state["recoveredAt"] = utc_now()
    write_json(args.state, state)
    try:
        args.state.unlink()
    except OSError as error:
        raise EvidenceError(f"cleanup succeeded but state file could not be removed: {error}") from error
    print("PASS: collector-owned blobs were removed and the original baseline was restored")


def command_verify_power_cycle(args: argparse.Namespace) -> None:
    state = load_state(args.state, "awaiting_power_cycle")
    api = api_from_state(state, args)
    expected = expected_live_snapshot(state)
    verify_snapshot(api, expected, exact_ids=True)
    diagnostics = parse_diagnostics(api.diagnostics())
    require(diagnostics["buildId"] == state["initialDiagnostics"]["buildId"],
            "firmware build changed across the power-cycle stage")
    require(diagnostics["resetReason"] == "power-on",
            f"expected a physical power-on reset, found {diagnostics["resetReason"]!r}")
    add_scenario(state, "power_cycle_persistence",
                 {"verifiedHashes": expected, "postBootDiagnostics": diagnostics})
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
    path = (parsed.path.rstrip("/") if parsed.path else "") + BLOB_COLLECTION_PATH
    connection.putrequest("POST", path, skip_host=True)
    connection.putheader("Host", parsed.netloc)
    connection.putheader("Content-Type", "application/gzip")
    connection.putheader("Content-Length", str(BLOB_MAX_BYTES))
    connection.putheader("Cookie", api.cookie_header(BLOB_COLLECTION_PATH))
    connection.putheader("Connection", "close")
    connection.endheaders()
    return connection


def command_arm_interrupted_upload(args: argparse.Namespace) -> None:
    state = load_state(args.state, "ready_for_interrupted_upload")
    api = api_from_state(state, args)
    before = snapshot(api)
    require(before == expected_live_snapshot(state), "live blobs changed before interruption test")
    before_diagnostics = parse_diagnostics(api.diagnostics())
    payload = exact_gzip_payload(f"power-cut-upload:{secrets.token_hex(16)}")
    state["phase"] = "interrupted_upload_in_progress"
    write_json(args.state, state)
    begin_pending_creation(api, args.state, state, payload, None)
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
        "beforeDiagnostics": before_diagnostics,
    }
    state["phase"] = "awaiting_interrupted_upload_reboot"
    write_json(args.state, state)
    print(f"Power loss observed after {sent} bytes; wait for reboot, then run verify-interrupted-upload.")


def command_verify_interrupted_upload(args: argparse.Namespace) -> None:
    state = load_state(args.state, "awaiting_interrupted_upload_reboot")
    api = api_from_state(state, args)
    before = state["interruptedUpload"]["beforeHashes"]
    pending = pending_creation(state)
    require(pending is not None and pending["beforeHashes"] == before,
            "interrupted-upload creation intent is missing or does not match its baseline")
    verify_snapshot(api, before, exact_ids=True)
    diagnostics = parse_diagnostics(api.diagnostics())
    before_diagnostics = state["interruptedUpload"]["beforeDiagnostics"]
    require(diagnostics["buildId"] == before_diagnostics["buildId"],
            "firmware build changed across interrupted-upload reboot")
    require(diagnostics["resetReason"] == "power-on",
            f"expected power-on reset after interruption, found {diagnostics["resetReason"]!r}")
    require(diagnostics["temporaryFileCount"] == 0
            and diagnostics["temporaryFiles"] == [],
            f"reboot left temporary files: {diagnostics}")
    details = {
        "bytesSentBeforePowerLoss": state["interruptedUpload"]["bytesSentBeforeDisconnect"],
        "preservedHashes": before,
        "prePowerLossDiagnostics": before_diagnostics,
        "postBootDiagnostics": diagnostics,
    }
    add_scenario(state, "interrupted_upload_no_partial_final", details)
    add_scenario(state, "reboot_temporary_cleanup", details)
    state.pop("pendingCreation", None)
    state["phase"] = "ready_for_storage_full"
    write_json(args.state, state)
    print("PASS: interrupted upload produced no final blob and reboot cleanup reports zero temporary files")


def command_fill_storage(args: argparse.Namespace) -> None:
    state = load_state(args.state, "ready_for_storage_full")
    api = api_from_state(state, args)
    original = snapshot(api)
    require(original == expected_live_snapshot(state), "live blobs changed before storage-full test")
    state["phase"] = "storage_full_in_progress"
    state["fillCreated"] = []
    write_json(args.state, state)

    fill_created: list[dict[str, str]] = []
    failure: Any = None
    for attempt in range(1, 9):
        payload = exact_gzip_payload(
            f"storage-full:{attempt}:{secrets.token_hex(16)}"
        )
        status, data, item = create_journaled_blob(
            api, args.state, state, payload, "fillCreated"
        )
        if status == 201:
            require(item is not None, "successful fill upload was not journaled")
            fill_created.append(item)
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
        delete_owned_blob(api, state, args.state, item)
    verify_snapshot(api, original, exact_ids=True)
    add_scenario(
        state,
        "storage_full_507_preservation",
        {
            "maximumUploadBytes": BLOB_MAX_BYTES,
            "committedBefore507": fill_created,
            "failure": failure,
            "preservedHashes": preserved,
            "cleanupVerified": True,
        },
    )
    state.pop("fillCreated", None)
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
    require(state.get("schemaVersion") == STATE_SCHEMA,
            "evidence schemaVersion is invalid")
    require(state.get("task") == "V2-035",
            "evidence task is not V2-035")
    require(state.get("espIdfVersion") == ESP_IDF_VERSION,
            f"evidence ESP-IDF version is not {ESP_IDF_VERSION}")
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
    firmware_build_id = state.get("firmwareBuildId")
    app_image_sha256 = state.get("appImageSha256")
    app_elf_sha256 = state.get("appElfSha256")
    manifest_sha256 = state.get("flashManifestSha256")
    require(isinstance(firmware_build_id, str)
            and BUILD_ID_PATTERN.fullmatch(firmware_build_id) is not None,
            "evidence is not bound to the board-visible firmware build ID")
    for label, value in (("app image", app_image_sha256),
                         ("app ELF", app_elf_sha256),
                         ("flash manifest", manifest_sha256)):
        require(isinstance(value, str) and SHA256_PATTERN.fullmatch(value) is not None,
                f"evidence {label} SHA-256 is invalid")
    require(app_elf_sha256.startswith(firmware_build_id),
            "evidence firmware build ID is not the recorded ELF SHA prefix")
    scenarios = state.get("scenarios")
    require(isinstance(scenarios, dict), "evidence is missing scenarios")
    for name in REQUIRED_SCENARIOS:
        value = scenarios.get(name)
        require(isinstance(value, dict) and value.get("status") == "pass",
                f"scenario {name} is not supported by passing physical evidence")
    require(state.get("phase") in ("ready_to_finalize", "finalize_in_progress", "complete"),
            f"evidence phase is incomplete: {state.get('phase')!r}")
    require(state.get("pendingCreation") is None,
            "evidence still contains an unresolved creation intent")
    recorded_owned = state.get("ownedBlobs")
    require(isinstance(recorded_owned, list), "evidence is missing the owned-blob journal")
    if state.get("phase") == "complete":
        require(recorded_owned == [], "complete evidence still owns test blobs")


def command_finalize(args: argparse.Namespace) -> None:
    state = load_state(args.state)
    require(state.get("phase") in ("ready_to_finalize", "finalize_in_progress"),
            f"cannot finalize from phase {state.get('phase')!r}")
    api = api_from_state(state, args)
    if state["phase"] == "ready_to_finalize":
        validate_complete_state(state)
        verify_snapshot(api, expected_live_snapshot(state), exact_ids=True)
        state["phase"] = "finalize_in_progress"
        write_json(args.state, state)
    else:
        # A host crash can occur after the device has deleted a collector-owned
        # blob but before the local ownership journal is updated.  Treat an
        # already-missing collector-owned blob as the intended delete having
        # reached the device, while still failing closed on any baseline change
        # or unowned blob.
        reconcile_pending_creation(api, args.state, state)
        verify_recoverable_snapshot(api, state)

    for item in list(reversed(owned_blobs(state))):
        delete_owned_blob(api, state, args.state, item)
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
    require(state.get("phase") == "complete",
            f"evidence is not finalized: {state.get('phase')!r}")
    evidence_sha256 = state.get("evidenceSha256")
    require(isinstance(evidence_sha256, str)
            and SHA256_PATTERN.fullmatch(evidence_sha256) is not None,
            "evidenceSha256 is missing or invalid")
    unhashed = dict(state)
    unhashed.pop("evidenceSha256", None)
    expected_sha256 = sha256_bytes(
        json.dumps(unhashed, sort_keys=True, separators=(",", ":")).encode()
    )
    require(evidence_sha256 == expected_sha256,
            "evidenceSha256 does not match the finalized evidence contents")
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
    start.add_argument("--flash-manifest", type=Path, required=True)
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

    recover = subparsers.add_parser(
        "recover-cleanup",
        help="remove journaled collector blobs after a failed or interrupted stage",
    )
    add_online_arguments(recover)
    recover.set_defaults(function=command_recover_cleanup)

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
