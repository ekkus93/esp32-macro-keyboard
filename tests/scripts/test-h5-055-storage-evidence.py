#!/usr/bin/env python3
"""Regression tests for the H5-055 physical-storage evidence validator."""

from __future__ import annotations

import importlib.util
import json
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "scripts" / "validate-h5-055-storage-evidence.py"
SPEC = importlib.util.spec_from_file_location("h5_055_storage_evidence", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

COMMIT = "0123456789abcdef0123456789abcdef01234567"
HASH_A = "a" * 64
HASH_B = "b" * 64
HASH_C = "c" * 64
HASH_D = "d" * 64


def expect_failure(function, *args) -> None:
    try:
        function(*args)
    except MODULE.EvidenceError:
        return
    raise AssertionError("expected EvidenceError")


def valid_evidence() -> dict:
    expected = {"1": HASH_A, "3": HASH_C}
    evidence = {
        "schemaVersion": 3,
        "task": "V2-035",
        "phase": "complete",
        "firmwareCommit": COMMIT,
        "firmwareBuildId": "e" * 39,
        "espIdfVersion": MODULE.ESP_IDF_VERSION,
        "targetHardware": MODULE.TARGET_HARDWARE,
        "baseline": {},
        "sentinels": [
            {"id": "1", "sha256": HASH_A},
            {"id": "3", "sha256": HASH_C},
        ],
        "ownedBlobs": [],
        "interruptedUpload": {
            "bytesSentBeforeDisconnect": 98_304,
            "contentLength": 131_072,
            "beforeHashes": expected,
        },
        "scenarios": {
            "numeric_ordering": {
                "status": "pass",
                "createdIds": ["1", "2", "3"],
                "listedIds": ["3", "2", "1"],
            },
            "delete_preservation": {
                "status": "pass",
                "preservedHashes": expected,
            },
            "power_cycle_persistence": {
                "status": "pass",
                "verifiedHashes": expected,
                "postBootDiagnostics": {
                    "resetReason": "power_on",
                    "temporaryFileCount": 0,
                    "temporaryFiles": [],
                },
            },
            "interrupted_upload_no_partial_final": {
                "status": "pass",
                "bytesSentBeforePowerLoss": 98_304,
                "preservedHashes": expected,
                "postBootDiagnostics": {
                    "resetReason": "power_on",
                    "temporaryFileCount": 0,
                    "temporaryFiles": [],
                },
            },
            "reboot_temporary_cleanup": {
                "status": "pass",
                "preservedHashes": expected,
                "postBootDiagnostics": {
                    "resetReason": "power_on",
                    "temporaryFileCount": 0,
                    "temporaryFiles": [],
                },
            },
            "mount_failure_no_format": {
                "status": "pass",
                "partitionBytes": MODULE.USERDATA_BYTES,
                "backupSha256": HASH_B,
                "corruptSha256": HASH_D,
                "postBootSha256": HASH_D,
                "restoredSha256": HASH_B,
                "serialLogSha256": HASH_C,
                "serialLogExcerpt": [
                    "E esp_littlefs: mount failed (-84)",
                    "E app_core: storage_mount failed: storage_unavailable",
                ],
            },
        },
        "testBlobCleanup": {"status": "pass"},
    }
    evidence["evidenceSha256"] = MODULE.canonical_sha256(evidence)
    return evidence


def resign(evidence: dict) -> None:
    evidence.pop("evidenceSha256", None)
    evidence["evidenceSha256"] = MODULE.canonical_sha256(evidence)


def test_valid_evidence() -> None:
    summary = MODULE.validate_h5_055(valid_evidence(), COMMIT)
    assert summary["task"] == "H5-055"
    assert summary["firmwareCommit"] == COMMIT
    assert summary["checks"] == {
        "interruptedUploadPowerCycle": "pass",
        "mountFailureNoFormat": "pass",
        "byteIdentityAndBlobList": "pass",
    }
    assert summary["observations"]["interruptedBytesSentBeforePowerLoss"] == 98_304


def test_rejects_stale_candidate() -> None:
    expect_failure(MODULE.validate_h5_055, valid_evidence(), "f" * 40)


def test_rejects_tampered_self_hash() -> None:
    evidence = valid_evidence()
    evidence["scenarios"]["power_cycle_persistence"]["verifiedHashes"]["1"] = HASH_B
    expect_failure(MODULE.validate_h5_055, evidence, COMMIT)


def test_rejects_mount_mutation_even_when_resigned() -> None:
    evidence = valid_evidence()
    evidence["scenarios"]["mount_failure_no_format"]["postBootSha256"] = HASH_A
    resign(evidence)
    expect_failure(MODULE.validate_h5_055, evidence, COMMIT)


def test_rejects_interrupted_blob_set_drift() -> None:
    evidence = valid_evidence()
    evidence["scenarios"]["interrupted_upload_no_partial_final"]["preservedHashes"] = {
        "1": HASH_A,
        "4": HASH_D,
    }
    resign(evidence)
    expect_failure(MODULE.validate_h5_055, evidence, COMMIT)


def test_rejects_non_numeric_list_order() -> None:
    evidence = valid_evidence()
    evidence["scenarios"]["numeric_ordering"]["listedIds"] = ["1", "2", "3"]
    resign(evidence)
    expect_failure(MODULE.validate_h5_055, evidence, COMMIT)


def test_writes_h5_summary() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        evidence_path = root / "v2.json"
        summary_path = root / "h5.json"
        evidence = valid_evidence()
        evidence_path.write_text(json.dumps(evidence), encoding="utf-8")
        summary = MODULE.validate_h5_055(MODULE.read_json(evidence_path), COMMIT)
        MODULE.write_json(summary_path, summary)
        recorded = json.loads(summary_path.read_text(encoding="utf-8"))
        assert recorded["sourceEvidenceSha256"] == evidence["evidenceSha256"]
        assert recorded["checks"]["mountFailureNoFormat"] == "pass"


def main() -> int:
    test_valid_evidence()
    test_rejects_stale_candidate()
    test_rejects_tampered_self_hash()
    test_rejects_mount_mutation_even_when_resigned()
    test_rejects_interrupted_blob_set_drift()
    test_rejects_non_numeric_list_order()
    test_writes_h5_summary()
    print("PASS: H5-055 storage evidence validator regression tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
