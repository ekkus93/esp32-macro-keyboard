#!/usr/bin/env python3
"""Regression tests for the H5-055 physical durability evidence validator."""

from __future__ import annotations

import importlib.util
import json
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "scripts" / "validate-h5-055-hardware-evidence.py"
EVIDENCE = REPO_ROOT / "docs" / "hardware-evidence" / "V2_035_STORAGE_ESP32S3R8_2026-08-10.json"
SPEC = importlib.util.spec_from_file_location("h5_055_hardware_evidence", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def load_evidence() -> dict:
    return json.loads(EVIDENCE.read_text(encoding="utf-8"))


def reseal(evidence: dict) -> None:
    unhashed = dict(evidence)
    unhashed.pop("evidenceSha256", None)
    evidence["evidenceSha256"] = MODULE.COLLECTOR.sha256_bytes(
        json.dumps(unhashed, sort_keys=True, separators=(",", ":")).encode()
    )


def expect_failure(evidence: dict, expected_commit: str) -> None:
    try:
        MODULE.validate_h5_055(evidence, expected_commit)
    except MODULE.COLLECTOR.EvidenceError:
        return
    raise AssertionError("expected H5-055 evidence validation failure")


def test_complete_physical_shape_is_accepted_when_commit_matches() -> None:
    evidence = load_evidence()
    MODULE.validate_h5_055(evidence, evidence["firmwareCommit"])


def test_historical_evidence_cannot_satisfy_a_new_firmware_commit() -> None:
    evidence = load_evidence()
    expect_failure(evidence, "f" * 40)


def test_mount_failure_must_leave_corrupt_partition_byte_identical() -> None:
    evidence = load_evidence()
    evidence["scenarios"]["mount_failure_no_format"]["postBootSha256"] = "d" * 64
    reseal(evidence)
    expect_failure(evidence, evidence["firmwareCommit"])


def test_interrupted_upload_must_remove_temporary_files_after_power_cycle() -> None:
    evidence = load_evidence()
    cleanup = evidence["scenarios"]["reboot_temporary_cleanup"]["postBootDiagnostics"]
    cleanup["temporaryFileCount"] = 1
    cleanup["temporaryFiles"] = ["00000000000000000007.gz.tmp"]
    reseal(evidence)
    expect_failure(evidence, evidence["firmwareCommit"])


def test_interrupted_upload_must_preserve_exact_sentinel_bytes() -> None:
    evidence = load_evidence()
    interrupted = evidence["scenarios"]["interrupted_upload_no_partial_final"]["preservedHashes"]
    first_blob = next(iter(interrupted))
    interrupted[first_blob] = "e" * 64
    reseal(evidence)
    expect_failure(evidence, evidence["firmwareCommit"])


def test_blob_list_must_remain_newest_first_numeric_order() -> None:
    evidence = load_evidence()
    evidence["scenarios"]["numeric_ordering"]["listedIds"] = ["1", "2", "3"]
    reseal(evidence)
    expect_failure(evidence, evidence["firmwareCommit"])


def test_interrupted_transfer_must_be_partial_and_substantial() -> None:
    evidence = load_evidence()
    evidence["interruptedUpload"]["bytesSentBeforeDisconnect"] = 4096
    evidence["scenarios"]["interrupted_upload_no_partial_final"]["bytesSentBeforePowerLoss"] = 4096
    evidence["scenarios"]["reboot_temporary_cleanup"]["bytesSentBeforePowerLoss"] = 4096
    reseal(evidence)
    expect_failure(evidence, evidence["firmwareCommit"])


def main() -> int:
    test_complete_physical_shape_is_accepted_when_commit_matches()
    test_historical_evidence_cannot_satisfy_a_new_firmware_commit()
    test_mount_failure_must_leave_corrupt_partition_byte_identical()
    test_interrupted_upload_must_remove_temporary_files_after_power_cycle()
    test_interrupted_upload_must_preserve_exact_sentinel_bytes()
    test_blob_list_must_remain_newest_first_numeric_order()
    test_interrupted_transfer_must_be_partial_and_substantial()
    print("PASS: H5-055 hardware evidence validator regression tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
