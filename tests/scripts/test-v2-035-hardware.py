#!/usr/bin/env python3
"""Regression tests for the physical V2-035 evidence collector."""

from __future__ import annotations

import gzip
import importlib.util
import json
import tempfile
from argparse import Namespace
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "scripts" / "run-v2-035-hardware.py"
SPEC = importlib.util.spec_from_file_location("v2_035_hardware", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def expect_failure(function, *args) -> None:
    try:
        function(*args)
    except MODULE.EvidenceError:
        return
    raise AssertionError("expected EvidenceError")


def complete_state() -> dict:
    scenarios = {
        name: {"status": "pass", "observedAt": "2026-08-06T00:00:00Z"}
        for name in MODULE.REQUIRED_SCENARIOS
    }
    return {
        "schemaVersion": MODULE.STATE_SCHEMA,
        "task": "V2-035",
        "phase": "ready_to_finalize",
        "scenarios": scenarios,
    }


def test_exact_gzip() -> None:
    first = MODULE.exact_gzip_payload("alpha")
    second = MODULE.exact_gzip_payload("alpha")
    third = MODULE.exact_gzip_payload("beta")
    assert len(first) == MODULE.BLOB_MAX_BYTES
    assert first == second
    assert first != third
    assert len(gzip.decompress(first)) == MODULE.BLOB_MAX_BYTES - 64


def test_recursive_values() -> None:
    value = {"storage": {"content": {"temporaryFiles": 0}}, "other": [{"temporaryFiles": 2}]}
    assert MODULE.recursive_values(value, "temporaryFiles") == [0, 2]


def test_complete_validation() -> None:
    state = complete_state()
    MODULE.validate_complete_state(state)
    del state["scenarios"][MODULE.REQUIRED_SCENARIOS[0]]
    expect_failure(MODULE.validate_complete_state, state)


def test_mount_failure_record() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        state_path = root / "state.json"
        original = b"A" * MODULE.USERDATA_BYTES
        corrupt = b"B" * MODULE.USERDATA_BYTES
        paths = {
            "backup": root / "backup.bin",
            "corrupt": root / "corrupt.bin",
            "post": root / "post.bin",
            "restored": root / "restored.bin",
        }
        paths["backup"].write_bytes(original)
        paths["corrupt"].write_bytes(corrupt)
        paths["post"].write_bytes(corrupt)
        paths["restored"].write_bytes(original)
        serial_log = root / "serial.log"
        serial_log.write_text("E storage: userdata mount failed: storage unavailable\n", encoding="utf-8")
        state_path.write_text(
            json.dumps(
                {
                    "schemaVersion": MODULE.STATE_SCHEMA,
                    "task": "V2-035",
                    "phase": "ready_for_mount_failure_record",
                    "scenarios": {},
                }
            ),
            encoding="utf-8",
        )
        args = Namespace(
            state=state_path,
            backup_image=paths["backup"],
            corrupt_image=paths["corrupt"],
            post_boot_image=paths["post"],
            restored_image=paths["restored"],
            serial_log=serial_log,
        )
        MODULE.command_record_mount_failure(args)
        recorded = json.loads(state_path.read_text(encoding="utf-8"))
        evidence = recorded["scenarios"]["mount_failure_no_format"]
        assert evidence["status"] == "pass"
        assert evidence["corruptSha256"] == evidence["postBootSha256"]
        assert evidence["backupSha256"] == evidence["restoredSha256"]

        paths["post"].write_bytes(b"C" * MODULE.USERDATA_BYTES)
        state = json.loads(state_path.read_text(encoding="utf-8"))
        state["phase"] = "ready_for_mount_failure_record"
        state_path.write_text(json.dumps(state), encoding="utf-8")
        expect_failure(MODULE.command_record_mount_failure, args)


def main() -> int:
    test_exact_gzip()
    test_recursive_values()
    test_complete_validation()
    test_mount_failure_record()
    print("PASS: V2-035 hardware evidence collector regression tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
