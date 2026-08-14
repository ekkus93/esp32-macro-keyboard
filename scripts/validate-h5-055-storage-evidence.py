#!/usr/bin/env python3
"""Validate fresh physical storage evidence for H5-055."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

FIRMWARE_COMMIT_PATTERN = re.compile(r"^[0-9a-f]{40}$")
SHA256_PATTERN = re.compile(r"^[0-9a-f]{64}$")
ESP_IDF_VERSION = "ESP-IDF v5.5.5"
TARGET_HARDWARE = "ESP32-S3R8"
USERDATA_BYTES = 524_288
MIN_INTERRUPTED_UPLOAD_BYTES = 16_384
FORBIDDEN_FORMAT_MARKERS = (
    "formatting userdata",
    "formatted userdata",
    "esp_littlefs_format",
    "format succeeded",
)


class EvidenceError(RuntimeError):
    """Raised when physical evidence does not prove an H5-055 invariant."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise EvidenceError(message)


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def canonical_sha256(value: dict[str, Any]) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(encoded).hexdigest()


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
    temporary.replace(path)


def require_sha256(value: Any, label: str) -> str:
    require(
        isinstance(value, str) and SHA256_PATTERN.fullmatch(value) is not None,
        f"{label} must be a lowercase SHA-256",
    )
    return value


def require_hash_map(value: Any, label: str) -> dict[str, str]:
    require(isinstance(value, dict), f"{label} must be an object")
    result: dict[str, str] = {}
    for blob_id, digest in value.items():
        require(isinstance(blob_id, str) and blob_id.isdigit(), f"{label} has invalid blob ID")
        result[blob_id] = require_sha256(digest, f"{label}[{blob_id}]")
    return result


def require_pass_scenario(scenarios: dict[str, Any], name: str) -> dict[str, Any]:
    value = scenarios.get(name)
    require(
        isinstance(value, dict) and value.get("status") == "pass",
        f"scenario {name} is not passing",
    )
    return value


def require_power_on_clean_diagnostics(value: Any, label: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{label} must be an object")
    require(value.get("resetReason") == "power_on", f"{label} is not a physical power-on reset")
    require(value.get("temporaryFileCount") == 0, f"{label} reports temporary files")
    require(value.get("temporaryFiles") == [], f"{label} reports temporary file names")
    return value


def expected_live_hashes(evidence: dict[str, Any]) -> dict[str, str]:
    result = require_hash_map(evidence.get("baseline"), "baseline")
    sentinels = evidence.get("sentinels")
    require(isinstance(sentinels, list) and bool(sentinels), "sentinels must be a non-empty list")
    for item in sentinels:
        require(isinstance(item, dict), "sentinel entry must be an object")
        blob_id = item.get("id")
        require(isinstance(blob_id, str) and blob_id.isdigit(), "sentinel blob ID is invalid")
        require(blob_id not in result, f"sentinel blob ID {blob_id} duplicates the baseline")
        result[blob_id] = require_sha256(item.get("sha256"), f"sentinel {blob_id} sha256")
    return result


def validate_source_self_hash(evidence: dict[str, Any]) -> str:
    recorded = require_sha256(evidence.get("evidenceSha256"), "evidenceSha256")
    unhashed = dict(evidence)
    unhashed.pop("evidenceSha256", None)
    require(recorded == canonical_sha256(unhashed), "evidenceSha256 does not match evidence contents")
    return recorded


def validate_numeric_list_behavior(scenarios: dict[str, Any]) -> None:
    ordering = require_pass_scenario(scenarios, "numeric_ordering")
    created = ordering.get("createdIds")
    listed = ordering.get("listedIds")
    require(
        isinstance(created, list)
        and len(created) >= 2
        and all(isinstance(value, str) and value.isdigit() for value in created),
        "numeric_ordering.createdIds is invalid",
    )
    require(
        isinstance(listed, list)
        and all(isinstance(value, str) and value.isdigit() for value in listed),
        "numeric_ordering.listedIds is invalid",
    )
    created_numbers = [int(value) for value in created]
    listed_numbers = [int(value) for value in listed]
    require(
        created_numbers == sorted(created_numbers) and len(set(created_numbers)) == len(created),
        "created blob IDs are not strictly increasing",
    )
    require(listed_numbers == sorted(listed_numbers, reverse=True), "blob list is not newest-first")
    require(set(created).issubset(set(listed)), "blob list omitted a newly created blob")


def validate_mount_failure(scenarios: dict[str, Any]) -> dict[str, Any]:
    mount = require_pass_scenario(scenarios, "mount_failure_no_format")
    require(mount.get("partitionBytes") == USERDATA_BYTES, "mount test used wrong userdata size")
    backup = require_sha256(mount.get("backupSha256"), "mount backupSha256")
    corrupt = require_sha256(mount.get("corruptSha256"), "mount corruptSha256")
    post_boot = require_sha256(mount.get("postBootSha256"), "mount postBootSha256")
    restored = require_sha256(mount.get("restoredSha256"), "mount restoredSha256")
    require(corrupt == post_boot, "failed mount mutated userdata; formatting cannot be excluded")
    require(backup == restored, "restored userdata is not byte-identical to the pre-test backup")
    require(backup != corrupt, "mount-failure corruption image did not differ from the backup")
    excerpt = mount.get("serialLogExcerpt")
    require(
        isinstance(excerpt, list) and all(isinstance(line, str) for line in excerpt),
        "mount serialLogExcerpt is invalid",
    )
    lowered = "\n".join(excerpt).lower()
    require("mount" in lowered and any(word in lowered for word in ("fail", "error", "unavailable")),
            "mount serial evidence does not show an explicit failure")
    require(
        not any(marker in lowered for marker in FORBIDDEN_FORMAT_MARKERS),
        "mount serial evidence contains a formatting marker",
    )
    require_sha256(mount.get("serialLogSha256"), "mount serialLogSha256")
    return mount


def validate_h5_055(evidence: dict[str, Any], expected_commit: str) -> dict[str, Any]:
    require(FIRMWARE_COMMIT_PATTERN.fullmatch(expected_commit) is not None,
            "expected firmware commit must be an exact lowercase 40-character SHA")
    require(evidence.get("schemaVersion") == 3, "source evidence schemaVersion must be 3")
    require(evidence.get("task") == "V2-035", "source evidence must come from V2-035 collector")
    require(evidence.get("phase") == "complete", "source evidence is not finalized")
    require(evidence.get("firmwareCommit") == expected_commit,
            "source evidence was not collected on the requested H5 candidate SHA")
    require(evidence.get("espIdfVersion") == ESP_IDF_VERSION,
            f"source evidence must use {ESP_IDF_VERSION}")
    require(evidence.get("targetHardware") == TARGET_HARDWARE,
            f"source evidence must target {TARGET_HARDWARE}")
    source_hash = validate_source_self_hash(evidence)
    cleanup = evidence.get("testBlobCleanup")
    require(isinstance(cleanup, dict) and cleanup.get("status") == "pass",
            "collector-created blobs were not proven cleaned up")

    scenarios = evidence.get("scenarios")
    require(isinstance(scenarios, dict), "source evidence is missing scenarios")
    expected_hashes = expected_live_hashes(evidence)

    power_cycle = require_pass_scenario(scenarios, "power_cycle_persistence")
    require_power_on_clean_diagnostics(power_cycle.get("postBootDiagnostics"),
                                       "power-cycle postBootDiagnostics")
    power_hashes = require_hash_map(power_cycle.get("verifiedHashes"),
                                    "power-cycle verifiedHashes")
    require(power_hashes == expected_hashes,
            "power-cycle byte-identity set does not match baseline plus sentinels")

    interrupted = require_pass_scenario(scenarios, "interrupted_upload_no_partial_final")
    reboot_cleanup = require_pass_scenario(scenarios, "reboot_temporary_cleanup")
    interrupted_state = evidence.get("interruptedUpload")
    require(isinstance(interrupted_state, dict), "interruptedUpload metadata is missing")
    bytes_sent = interrupted_state.get("bytesSentBeforeDisconnect")
    content_length = interrupted_state.get("contentLength")
    require(type(bytes_sent) is int and type(content_length) is int,
            "interrupted upload byte counters are invalid")
    require(MIN_INTERRUPTED_UPLOAD_BYTES <= bytes_sent < content_length,
            "upload was not interrupted after meaningful staging but before completion")
    require(interrupted.get("bytesSentBeforePowerLoss") == bytes_sent,
            "interrupted scenario byte count does not match interruption metadata")
    before_hashes = require_hash_map(interrupted_state.get("beforeHashes"),
                                     "interruptedUpload.beforeHashes")
    interrupted_hashes = require_hash_map(interrupted.get("preservedHashes"),
                                          "interrupted preservedHashes")
    cleanup_hashes = require_hash_map(reboot_cleanup.get("preservedHashes"),
                                      "reboot cleanup preservedHashes")
    require(before_hashes == expected_hashes,
            "interruption baseline does not match the expected live blob set")
    require(interrupted_hashes == before_hashes,
            "interrupted upload changed canonical blob bytes or IDs")
    require(cleanup_hashes == before_hashes,
            "reboot cleanup evidence does not preserve the pre-interruption blob set")
    require_power_on_clean_diagnostics(interrupted.get("postBootDiagnostics"),
                                       "interrupted postBootDiagnostics")
    require_power_on_clean_diagnostics(reboot_cleanup.get("postBootDiagnostics"),
                                       "reboot-cleanup postBootDiagnostics")

    validate_numeric_list_behavior(scenarios)
    deletion = require_pass_scenario(scenarios, "delete_preservation")
    delete_hashes = require_hash_map(deletion.get("preservedHashes"),
                                     "delete-preservation preservedHashes")
    require(delete_hashes == expected_hashes,
            "delete/list evidence does not match the byte-identical sentinel set")

    mount = validate_mount_failure(scenarios)

    return {
        "schemaVersion": 1,
        "task": "H5-055",
        "validatedAt": utc_now(),
        "firmwareCommit": expected_commit,
        "firmwareBuildId": evidence.get("firmwareBuildId"),
        "espIdfVersion": evidence.get("espIdfVersion"),
        "targetHardware": evidence.get("targetHardware"),
        "sourceTask": evidence.get("task"),
        "sourceEvidenceSha256": source_hash,
        "checks": {
            "interruptedUploadPowerCycle": "pass",
            "mountFailureNoFormat": "pass",
            "byteIdentityAndBlobList": "pass",
        },
        "observations": {
            "powerCycleVerifiedHashes": power_hashes,
            "interruptedBytesSentBeforePowerLoss": bytes_sent,
            "interruptedPreservedHashes": interrupted_hashes,
            "mountCorruptSha256": mount["corruptSha256"],
            "mountPostBootSha256": mount["postBootSha256"],
            "mountBackupSha256": mount["backupSha256"],
            "mountRestoredSha256": mount["restoredSha256"],
        },
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--expected-firmware-commit", required=True)
    parser.add_argument("--output", type=Path)
    return parser


def main() -> int:
    try:
        args = build_parser().parse_args()
        evidence = read_json(args.evidence)
        summary = validate_h5_055(evidence, args.expected_firmware_commit)
        if args.output is not None:
            write_json(args.output, summary)
            print(f"PASS: H5-055 summary written to {args.output}")
        else:
            print("PASS: H5-055 physical storage durability evidence is complete")
        return 0
    except EvidenceError as error:
        print(f"H5-055 FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
