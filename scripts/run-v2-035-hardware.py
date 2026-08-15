#!/usr/bin/env python3
"""Collect fail-closed V2-035 evidence from a physical ESP32-S3 board."""

from __future__ import annotations

import argparse
import http.cookiejar
import http.client
import json
import secrets
# shutil and subprocess are not called here any more -- they moved to
# v2_035_support/evidence.py -- but tests/scripts/test-v2-035-hardware.py
# patches MODULE.shutil.which and MODULE.subprocess.run through this module,
# so removing these imports would break that test with an AttributeError.
import shutil
import socket
import ssl
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import Any

# Both dependents load this file BY PATH -- scripts/run-h5-055-hardware.py and
# tests/scripts/test-v2-035-hardware.py both use
# importlib.util.spec_from_file_location -- and a module loaded that way has no
# package context, so a plain "from v2_035_support import ..." would not
# resolve. Put this script's own directory on sys.path first. The path of this
# file is fixed by roughly ten evidence documents under docs/implementation-v2/
# that cite it, so the entry point stays here and re-exports what the two
# dependents reach for.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from v2_035_support.core import (  # noqa: E402
    AUTH_LOGIN_PATH,
    BLOB_COLLECTION_PATH,
    BLOB_MAX_BYTES,
    BUILD_ID_PATTERN,
    DIAGNOSTICS_PATH,
    ELF_SHA_OUTPUT_PATTERN,
    ESP_IDF_VERSION,
    EvidenceError,
    FIRMWARE_COMMIT_PATTERN,
    LIVE_BUILD_ID_PATTERN,
    REQUIRED_SCENARIOS,
    SHA256_PATTERN,
    STATE_SCHEMA,
    USERDATA_BYTES,
    require,
)
from v2_035_support.payloads import (  # noqa: E402
    deterministic_bytes,
    exact_gzip_payload,
    gzip_member,
    small_payload,
)
from v2_035_support.evidence import (  # noqa: E402
    journal_created_blob,
    journal_deleted_blob,
    load_flash_manifest,
    load_state,
    owned_blobs,
    password_from_environment,
    read_app_elf_sha256,
    read_json,
    resolve_manifest_app_image,
    sha256_bytes,
    sha256_file,
    utc_now,
    verify_firmware_provenance,
    write_json,
)
from v2_035_support.device_api import (  # noqa: E402
    DeviceApi,
    add_scenario,
    blob_ids,
    parse_diagnostics,
    parse_success,
    snapshot,
    verify_snapshot,
)
from v2_035_support.blob_journal import (  # noqa: E402
    api_from_state,
    begin_pending_creation,
    clear_pending_creation_if_unchanged,
    create_journaled_blob,
    delete_owned_blob,
    expected_live_snapshot,
    expected_owned_snapshot,
    finish_pending_creation,
    owned_snapshot,
    pending_creation,
    reconcile_pending_creation,
    verify_recoverable_snapshot,
)

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
    require(diagnostics["resetReason"] == "power_on",
            f"expected a physical power_on reset, found {diagnostics['resetReason']!r}")
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
    require(diagnostics["resetReason"] == "power_on",
            f"expected power_on reset after interruption, found {diagnostics['resetReason']!r}")
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
