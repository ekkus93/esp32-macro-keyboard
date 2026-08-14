#!/usr/bin/env python3
"""Regression tests for the H5-055 physical collector wrapper."""

from __future__ import annotations

import importlib.util
import json
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "scripts" / "run-h5-055-hardware.py"
SPEC = importlib.util.spec_from_file_location("h5_055_hardware", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)
COLLECTOR = MODULE.COLLECTOR


class FakeApi:
    def __init__(self, blobs: dict[str, bytes], mode: str) -> None:
        self.blobs = dict(blobs)
        self.mode = mode
        self.create_calls = 0

    def list_blobs(self) -> list[dict]:
        return [
            {"id": blob_id, "sizeBytes": len(payload)}
            for blob_id, payload in sorted(
                self.blobs.items(), key=lambda item: int(item[0]), reverse=True
            )
        ]

    def load_blob(self, blob_id: str) -> bytes:
        return self.blobs[blob_id]

    def create_blob(self, payload: bytes):
        self.create_calls += 1
        if self.mode == "uncertain-match":
            self.blobs["2"] = payload
            return 503, {"error": {"code": "commit_uncertain", "message": "uncertain"}}
        if self.mode == "uncertain-none":
            return 503, {"error": {"code": "commit_uncertain", "message": "uncertain"}}
        if self.mode == "uncertain-ambiguous":
            self.blobs["2"] = payload
            self.blobs["3"] = b"concurrent"
            return 503, {"error": {"code": "commit_uncertain", "message": "uncertain"}}
        if self.mode == "storage-full":
            return 507, {"error": {"code": "storage_full", "message": "full"}}
        raise AssertionError(f"unknown fake mode {self.mode}")


def state_for(root: Path, baseline_payload: bytes) -> tuple[Path, dict]:
    state_path = root / "state.json"
    state = {
        "schemaVersion": COLLECTOR.STATE_SCHEMA,
        "task": "V2-035",
        "phase": "start_in_progress",
        "baseline": {"1": COLLECTOR.sha256_bytes(baseline_payload)},
        "ownedBlobs": [],
        "startCreated": [],
    }
    state_path.write_text(json.dumps(state), encoding="utf-8")
    return state_path, state


def expect_evidence_failure(function, *args) -> None:
    try:
        function(*args)
    except COLLECTOR.EvidenceError:
        return
    raise AssertionError("expected EvidenceError")


def test_uncertain_match_is_adopted_without_repost() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        baseline = b"baseline"
        payload = b"exact pending payload"
        state_path, state = state_for(root, baseline)
        api = FakeApi({"1": baseline}, "uncertain-match")

        status, data, item = MODULE.create_journaled_blob(
            api, state_path, state, payload, "startCreated"
        )

        assert api.create_calls == 1
        assert status == 201
        assert data == {
            "id": "2",
            "reconciledCommitUncertain": True,
            "originalHttpStatus": 503,
        }
        assert item == {"id": "2", "sha256": COLLECTOR.sha256_bytes(payload)}
        recorded = json.loads(state_path.read_text(encoding="utf-8"))
        assert "pendingCreation" not in recorded
        assert recorded["ownedBlobs"] == [item]
        assert recorded["startCreated"] == [item]


def test_uncertain_without_match_does_not_retry() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        baseline = b"baseline"
        state_path, state = state_for(root, baseline)
        api = FakeApi({"1": baseline}, "uncertain-none")

        status, data, item = MODULE.create_journaled_blob(
            api, state_path, state, b"pending", "startCreated"
        )

        assert api.create_calls == 1
        assert status == 503
        assert data["error"]["code"] == "commit_uncertain"
        assert item is None
        recorded = json.loads(state_path.read_text(encoding="utf-8"))
        assert "pendingCreation" not in recorded
        assert recorded["ownedBlobs"] == []


def test_uncertain_ambiguous_state_fails_closed_without_repost() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        baseline = b"baseline"
        state_path, state = state_for(root, baseline)
        api = FakeApi({"1": baseline}, "uncertain-ambiguous")

        expect_evidence_failure(
            MODULE.create_journaled_blob,
            api,
            state_path,
            state,
            b"pending",
            "startCreated",
        )

        assert api.create_calls == 1
        recorded = json.loads(state_path.read_text(encoding="utf-8"))
        assert "pendingCreation" in recorded
        assert recorded["ownedBlobs"] == []


def test_non_uncertain_failure_requires_unchanged_storage() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        baseline = b"baseline"
        state_path, state = state_for(root, baseline)
        api = FakeApi({"1": baseline}, "storage-full")

        status, data, item = MODULE.create_journaled_blob(
            api, state_path, state, b"pending", "startCreated"
        )

        assert api.create_calls == 1
        assert status == 507
        assert data["error"]["code"] == "storage_full"
        assert item is None
        recorded = json.loads(state_path.read_text(encoding="utf-8"))
        assert "pendingCreation" not in recorded


def main() -> int:
    test_uncertain_match_is_adopted_without_repost()
    test_uncertain_without_match_does_not_retry()
    test_uncertain_ambiguous_state_fails_closed_without_repost()
    test_non_uncertain_failure_requires_unchanged_storage()
    print("PASS: H5-055 hardware collector wrapper regression tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
