#!/usr/bin/env python3
"""Validate H5-055 storage durability evidence from a physical ESP32-S3R8."""

from __future__ import annotations

import argparse
import importlib.util
import json
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[1]
COLLECTOR_PATH = REPO_ROOT / "scripts" / "run-v2-035-hardware.py"
COLLECTOR_SPEC = importlib.util.spec_from_file_location("v2_035_hardware", COLLECTOR_PATH)
if COLLECTOR_SPEC is None or COLLECTOR_SPEC.loader is None:
    raise RuntimeError(f"could not load hardware collector from {COLLECTOR_PATH}")
COLLECTOR = importlib.util.module_from_spec(COLLECTOR_SPEC)
COLLECTOR_SPEC.loader.exec_module(COLLECTOR)

MIN_INTERRUPTED_UPLOAD_BYTES = 16_384
H5_REQUIRED_SCENARIOS = (
    "power_cycle_persistence",
    "interrupted_upload_no_partial_final",
    "reboot_temporary_cleanup",
    "mount_failure_no_format",
    "numeric_ordering",
    "delete_preservation",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise COLLECTOR.EvidenceError(message)


def require_hash_map(value: Any, label: str) -> dict[str, str]:
    require(isinstance(value, dict), f"{label} must be an object")
    result: dict[str, str] = {}
    for blob_id, digest in value.items():
        require(isinstance(blob_id, str) and bool(blob_id), f"{label} contains an invalid blob ID")
        require(
            isinstance(digest, str) and COLLECTOR.SHA256_PATTERN.fullmatch(digest) is not None,
            f"{label} contains an invalid SHA-256",
        )
        result[blob_id] = digest
    return result


def validate_self_hash(evidence: dict[str, Any]) -> None:
    evidence_sha256 = evidence.get("evidenceSha256")
    require(
        isinstance(evidence_sha256, str)
        and COLLECTOR.SHA256_PATTERN.fullmatch(evidence_sha256) is not None,
        "evidenceSha256 is missing or invalid",
    )
    unhashed = dict(evidence)
    unhashed.pop("evidenceSha256", None)
    expected = COLLECTOR.sha256_bytes(
        json.dumps(unhashed, sort_keys=True, separators=(",", ":")).encode()
    )
    require(evidence_sha256 == expected, "evidenceSha256 does not match the evidence contents")


def validate_h5_055(evidence: dict[str, Any], expected_firmware_commit: str) -> None:
    require(
        COLLECTOR.FIRMWARE_COMMIT_PATTERN.fullmatch(expected_firmware_commit) is not None,
        "expected firmware commit must be an exact 40-character SHA",
    )
    require(evidence.get("phase") == "complete", "H5-055 evidence is not finalized")
    validate_self_hash(evidence)
    COLLECTOR.validate_complete_state(evidence)
    require(
        evidence.get("testBlobCleanup", {}).get("status") == "pass",
        "test-created blob cleanup is not verified",
    )
    require(
        evidence.get("firmwareCommit") == expected_firmware_commit,
        "hardware evidence firmware commit does not match the requested post-H5 commit",
    )

    scenarios = evidence.get("scenarios")
    require(isinstance(scenarios, dict), "H5-055 evidence is missing scenarios")
    for name in H5_REQUIRED_SCENARIOS:
        scenario = scenarios.get(name)
        require(
            isinstance(scenario, dict) and scenario.get("status") == "pass",
            f"H5-055 scenario {name} is not passing",
        )

    power_cycle = scenarios["power_cycle_persistence"]
    power_diagnostics = power_cycle.get("postBootDiagnostics")
    require(isinstance(power_diagnostics, dict), "power-cycle evidence is missing diagnostics")
    require(
        power_diagnostics.get("resetReason") == "power_on",
        "power-cycle evidence does not prove a real power-on reset",
    )
    power_hashes = require_hash_map(power_cycle.get("verifiedHashes"), "power-cycle verifiedHashes")
    require(bool(power_hashes), "power-cycle evidence contains no sentinel blob hashes")

    interrupted = scenarios["interrupted_upload_no_partial_final"]
    bytes_sent = interrupted.get("bytesSentBeforePowerLoss")
    require(
        type(bytes_sent) is int and bytes_sent >= MIN_INTERRUPTED_UPLOAD_BYTES,
        "interrupted upload did not send enough bytes before physical power loss",
    )
    interrupted_diagnostics = interrupted.get("postBootDiagnostics")
    require(isinstance(interrupted_diagnostics, dict), "interrupted-upload evidence is missing diagnostics")
    require(
        interrupted_diagnostics.get("resetReason") == "power_on",
        "interrupted-upload recovery does not prove a physical power-on reset",
    )
    interrupted_hashes = require_hash_map(
        interrupted.get("preservedHashes"), "interrupted-upload preservedHashes"
    )
    require(
        interrupted_hashes == power_hashes,
        "interrupted-upload recovery did not preserve the same sentinel blobs byte-for-byte",
    )

    interrupted_attempt = evidence.get("interruptedUpload")
    require(isinstance(interrupted_attempt, dict), "evidence is missing interruptedUpload metadata")
    content_length = interrupted_attempt.get("contentLength")
    recorded_sent = interrupted_attempt.get("bytesSentBeforeDisconnect")
    require(
        content_length == COLLECTOR.BLOB_MAX_BYTES,
        "interrupted upload did not use the maximum-size blob payload",
    )
    require(
        recorded_sent == bytes_sent and type(recorded_sent) is int and recorded_sent < content_length,
        "interrupted upload metadata does not prove a partial transfer",
    )

    temporary_cleanup = scenarios["reboot_temporary_cleanup"]
    cleanup_diagnostics = temporary_cleanup.get("postBootDiagnostics")
    require(isinstance(cleanup_diagnostics, dict), "temporary-cleanup evidence is missing diagnostics")
    require(
        cleanup_diagnostics.get("temporaryFileCount") == 0
        and cleanup_diagnostics.get("temporaryFiles") == [],
        "temporary upload files remain after the interrupted-upload reboot",
    )
    cleanup_hashes = require_hash_map(
        temporary_cleanup.get("preservedHashes"), "temporary-cleanup preservedHashes"
    )
    require(
        cleanup_hashes == power_hashes,
        "temporary-file cleanup changed the sentinel blob set or bytes",
    )

    mount_failure = scenarios["mount_failure_no_format"]
    require(
        mount_failure.get("partitionBytes") == COLLECTOR.USERDATA_BYTES,
        "mount-failure evidence does not cover the full userdata partition",
    )
    backup_hash = mount_failure.get("backupSha256")
    corrupt_hash = mount_failure.get("corruptSha256")
    post_boot_hash = mount_failure.get("postBootSha256")
    restored_hash = mount_failure.get("restoredSha256")
    for label, digest in (
        ("backupSha256", backup_hash),
        ("corruptSha256", corrupt_hash),
        ("postBootSha256", post_boot_hash),
        ("restoredSha256", restored_hash),
    ):
        require(
            isinstance(digest, str) and COLLECTOR.SHA256_PATTERN.fullmatch(digest) is not None,
            f"mount-failure {label} is invalid",
        )
    require(
        corrupt_hash == post_boot_hash,
        "failed mount rewrote the deliberately corrupt userdata partition",
    )
    require(
        backup_hash == restored_hash,
        "userdata partition restoration is not byte-identical to the backup",
    )
    serial_excerpt = mount_failure.get("serialLogExcerpt")
    require(
        isinstance(serial_excerpt, list)
        and bool(serial_excerpt)
        and all(isinstance(line, str) for line in serial_excerpt),
        "mount-failure evidence is missing the serial-log excerpt",
    )

    ordering = scenarios["numeric_ordering"]
    created_ids = ordering.get("createdIds")
    listed_ids = ordering.get("listedIds")
    require(
        isinstance(created_ids, list)
        and len(created_ids) >= 3
        and all(isinstance(blob_id, str) and blob_id.isdigit() for blob_id in created_ids),
        "numeric-ordering evidence has invalid created IDs",
    )
    require(
        isinstance(listed_ids, list)
        and all(isinstance(blob_id, str) and blob_id.isdigit() for blob_id in listed_ids),
        "numeric-ordering evidence has invalid listed IDs",
    )
    created_numeric = [int(blob_id) for blob_id in created_ids]
    listed_numeric = [int(blob_id) for blob_id in listed_ids]
    require(
        created_numeric == sorted(created_numeric) and len(set(created_numeric)) == len(created_numeric),
        "created blob IDs are not strictly increasing",
    )
    require(
        listed_numeric == sorted(listed_numeric, reverse=True),
        "blob list is not newest-first numeric order",
    )

    deletion = scenarios["delete_preservation"]
    delete_hashes = require_hash_map(deletion.get("preservedHashes"), "delete preservedHashes")
    require(
        delete_hashes == power_hashes,
        "delete/power-cycle evidence disagrees about the preserved sentinel blob bytes",
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--expected-firmware-commit", required=True)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    try:
        evidence = COLLECTOR.read_json(args.evidence)
        validate_h5_055(evidence, args.expected_firmware_commit)
    except COLLECTOR.EvidenceError as error:
        print(f"H5-055 FAIL: {error}", file=sys.stderr)
        return 1
    print(
        "PASS: H5-055 physical durability evidence is complete for "
        f"{args.expected_firmware_commit}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
