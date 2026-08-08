#!/usr/bin/env python3
"""Regression tests for the physical V2-035 evidence collector."""

from __future__ import annotations

import gzip
import importlib.util
import os
import subprocess
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
        "firmwareCommit": "0123456789abcdef0123456789abcdef01234567",
        "firmwareBuildId": "a" * 39,
        "appImageSha256": "b" * 64,
        "appElfSha256": "a" * 64,
        "flashManifestSha256": "c" * 64,
        "targetHardware": "ESP32-S3R8",
        "espIdfVersion": MODULE.ESP_IDF_VERSION,
        "ownedBlobs": [],
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



def test_complete_validation() -> None:
    state = complete_state()
    MODULE.validate_complete_state(state)
    del state["scenarios"][MODULE.REQUIRED_SCENARIOS[0]]
    expect_failure(MODULE.validate_complete_state, state)






def test_current_v2_routes() -> None:
    assert MODULE.AUTH_LOGIN_PATH == "/api/v1/auth/login"
    assert MODULE.BLOB_COLLECTION_PATH == "/api/v1/blob"
    assert MODULE.DIAGNOSTICS_PATH == "/api/v1/diagnostics"
    source = SCRIPT.read_text(encoding="utf-8")
    assert "/api/v1/blobs" not in source
    assert '"/api/v1/login"' not in source


def test_diagnostics_schema() -> None:
    assert MODULE.BUILD_ID_PATTERN.fullmatch("a" * 39) is not None
    parsed = MODULE.parse_diagnostics(
        {
            "buildId": "a" * 39,
            "resetReason": "power_on",
            "uptimeMs": 12,
            "blobScan": {"temporaryFileCount": 0, "temporaryFiles": []},
        }
    )
    assert parsed["temporaryFileCount"] == 0
    assert parsed["temporaryFiles"] == []

    expect_failure(
        MODULE.parse_diagnostics,
        {
            "buildId": "a" * 39,
            "resetReason": "power_on",
            "uptimeMs": 12,
            "blobScan": {"temporaryFileCount": 1, "temporaryFiles": []},
        },
    )


def test_flash_manifest_provenance() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        manifest_path = root / "flash-manifest.json"
        app_image = root / "esp32_macro_keyboard.bin"
        app_image.write_bytes(b"verified application image")
        image_sha256 = MODULE.sha256_file(app_image)
        manifest_path.write_text(
            json.dumps(
                {
                    "gitCommit": "0123456789abcdef0123456789abcdef01234567",
                    "gitDirty": False,
                    "buildType": "production",
                    "espIdfVersion": MODULE.ESP_IDF_VERSION,
                    "appImage": app_image.name,
                    "appImageSha256": image_sha256,
                    "appElfSha256": "a" * 64,
                    "diagnosticsBuildId": "a" * 39,
                }
            ),
            encoding="utf-8",
        )
        original_which = MODULE.shutil.which
        original_run = MODULE.subprocess.run
        MODULE.shutil.which = lambda name: "/fake/esptool.py" if name == "esptool.py" else None
        esptool_commands: list[list[str]] = []

        def fake_run(command, **kwargs):
            assert command == [
                "/fake/esptool.py",
                "image_info",
                "--version",
                "2",
                str(app_image),
            ]
            assert kwargs == {
                "check": False,
                "capture_output": True,
                "text": True,
            }
            esptool_commands.append(command)
            return subprocess.CompletedProcess(
                args=command,
                returncode=0,
                stdout=f"ELF file SHA256: {'a' * 64}\n",
                stderr="",
            )

        MODULE.subprocess.run = fake_run
        try:
            manifest = MODULE.load_flash_manifest(manifest_path)
            assert len(esptool_commands) == 1
            MODULE.verify_firmware_provenance(manifest, {"buildId": "a" * 39})
            expect_failure(
                MODULE.verify_firmware_provenance,
                manifest,
                {"buildId": "d" * 39},
            )

            dirty = json.loads(manifest_path.read_text(encoding="utf-8"))
            dirty["gitDirty"] = True
            manifest_path.write_text(json.dumps(dirty), encoding="utf-8")
            expect_failure(MODULE.load_flash_manifest, manifest_path)

            dirty["gitDirty"] = False
            dirty["espIdfVersion"] = "ESP-IDF v5.5.4"
            manifest_path.write_text(json.dumps(dirty), encoding="utf-8")
            expect_failure(MODULE.load_flash_manifest, manifest_path)

            dirty["espIdfVersion"] = MODULE.ESP_IDF_VERSION
            app_image.write_bytes(b"tampered application image")
            manifest_path.write_text(json.dumps(dirty), encoding="utf-8")
            expect_failure(MODULE.load_flash_manifest, manifest_path)
        finally:
            MODULE.shutil.which = original_which
            MODULE.subprocess.run = original_run


def test_interrupted_upload_request_headers() -> None:
    class FakeApi:
        base_url = "http://192.0.2.1:8080/base"
        timeout = 3.0

        @staticmethod
        def cookie_header(path: str) -> str:
            assert path == MODULE.BLOB_COLLECTION_PATH
            return "session=test"

    class FakeConnection:
        def __init__(self, host: str, port: int, timeout: float) -> None:
            self.host = host
            self.port = port
            self.timeout = timeout
            self.requests = []
            self.headers = []
            self.ended = False

        def putrequest(self, method: str, path: str, **kwargs) -> None:
            self.requests.append((method, path, kwargs))

        def putheader(self, name: str, value: str) -> None:
            self.headers.append((name, value))

        def endheaders(self) -> None:
            self.ended = True

    original = MODULE.http.client.HTTPConnection
    MODULE.http.client.HTTPConnection = FakeConnection
    try:
        connection = MODULE.open_upload_connection(FakeApi())
    finally:
        MODULE.http.client.HTTPConnection = original

    assert connection.requests == [
        ("POST", "/base/api/v1/blob", {"skip_host": True})
    ]
    host_headers = [
        value for name, value in connection.headers if name.lower() == "host"
    ]
    assert host_headers == ["192.0.2.1:8080"]
    assert ("Content-Length", str(MODULE.BLOB_MAX_BYTES)) in connection.headers
    assert connection.ended is True

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


class FakeRecoveryApi:
    def __init__(self, blobs: dict[str, bytes], fail_delete_once: str | None = None) -> None:
        self.blobs = dict(blobs)
        self.fail_delete_once = fail_delete_once

    def list_blobs(self) -> list[dict]:
        return [
            {"id": blob_id, "sizeBytes": len(self.blobs[blob_id])}
            for blob_id in sorted(self.blobs, key=int, reverse=True)
        ]

    def load_blob(self, blob_id: str) -> bytes:
        if blob_id not in self.blobs:
            raise MODULE.EvidenceError(f"missing blob {blob_id}")
        return self.blobs[blob_id]

    def delete_blob(self, blob_id: str) -> None:
        if self.fail_delete_once == blob_id:
            self.fail_delete_once = None
            raise MODULE.EvidenceError(f"injected delete failure for {blob_id}")
        self.blobs.pop(blob_id, None)


def test_recover_cleanup_uses_owned_journal() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        state_path = root / "state.json"
        baseline_payload = b"baseline"
        owned_payload = b"collector"
        state = {
            "schemaVersion": MODULE.STATE_SCHEMA,
            "task": "V2-035",
            "phase": "start_in_progress",
            "baseUrl": "http://device.test",
            "baseline": {"0000000001": MODULE.sha256_bytes(baseline_payload)},
            "ownedBlobs": [
                {"id": "0000000002", "sha256": MODULE.sha256_bytes(owned_payload)}
            ],
        }
        state_path.write_text(json.dumps(state), encoding="utf-8")
        fake = FakeRecoveryApi(
            {"0000000001": baseline_payload, "0000000002": owned_payload}
        )
        original_api_from_state = MODULE.api_from_state
        MODULE.api_from_state = lambda current, args: fake
        try:
            MODULE.command_recover_cleanup(
                Namespace(state=state_path, timeout=1.0, password_env="UNUSED")
            )
        finally:
            MODULE.api_from_state = original_api_from_state
        assert fake.blobs == {"0000000001": baseline_payload}
        assert not state_path.exists()


def test_pending_creation_recovery_adopts_exact_hash() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        state_path = root / "state.json"
        baseline_payload = b"baseline"
        pending_payload = b"unguessable pending payload"
        baseline = {"0000000001": MODULE.sha256_bytes(baseline_payload)}
        state = {
            "schemaVersion": MODULE.STATE_SCHEMA,
            "task": "V2-035",
            "phase": "start_in_progress",
            "baseUrl": "http://device.test",
            "baseline": baseline,
            "ownedBlobs": [],
            "startCreated": [],
            "pendingCreation": {
                "startedAt": "2026-08-06T00:00:00Z",
                "payloadSha256": MODULE.sha256_bytes(pending_payload),
                "beforeHashes": baseline,
                "stageKey": "startCreated",
            },
        }
        state_path.write_text(json.dumps(state), encoding="utf-8")
        fake = FakeRecoveryApi(
            {"0000000001": baseline_payload, "0000000002": pending_payload}
        )
        adopted = MODULE.reconcile_pending_creation(fake, state_path, state)
        assert adopted == {
            "id": "0000000002",
            "sha256": MODULE.sha256_bytes(pending_payload),
        }
        recorded = json.loads(state_path.read_text(encoding="utf-8"))
        assert "pendingCreation" not in recorded
        assert recorded["ownedBlobs"] == [adopted]
        assert recorded["startCreated"] == [adopted]

        MODULE.verify_recoverable_snapshot(fake, recorded)


def test_pending_creation_rejects_ambiguous_unknown_blobs() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        state_path = root / "state.json"
        baseline_payload = b"baseline"
        pending_payload = b"pending"
        baseline = {"0000000001": MODULE.sha256_bytes(baseline_payload)}
        state = {
            "schemaVersion": MODULE.STATE_SCHEMA,
            "task": "V2-035",
            "phase": "start_in_progress",
            "baseline": baseline,
            "ownedBlobs": [],
            "pendingCreation": {
                "startedAt": "2026-08-06T00:00:00Z",
                "payloadSha256": MODULE.sha256_bytes(pending_payload),
                "beforeHashes": baseline,
                "stageKey": "startCreated",
            },
        }
        state_path.write_text(json.dumps(state), encoding="utf-8")
        fake = FakeRecoveryApi(
            {
                "0000000001": baseline_payload,
                "0000000002": pending_payload,
                "0000000003": b"concurrent unowned blob",
            }
        )
        expect_failure(
            MODULE.reconcile_pending_creation, fake, state_path, state
        )
        recorded = json.loads(state_path.read_text(encoding="utf-8"))
        assert recorded["pendingCreation"] == state["pendingCreation"]
        assert recorded["ownedBlobs"] == []


def test_journaled_create_closes_response_window() -> None:
    class CreatingApi(FakeRecoveryApi):
        def create_blob(self, payload: bytes):
            blob_id = f"{max(map(int, self.blobs), default=0) + 1:010d}"
            self.blobs[blob_id] = payload
            return 201, {"id": blob_id}

    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        state_path = root / "state.json"
        baseline_payload = b"baseline"
        state = {
            "schemaVersion": MODULE.STATE_SCHEMA,
            "task": "V2-035",
            "phase": "start_in_progress",
            "baseline": {"0000000001": MODULE.sha256_bytes(baseline_payload)},
            "ownedBlobs": [],
            "startCreated": [],
        }
        state_path.write_text(json.dumps(state), encoding="utf-8")
        fake = CreatingApi({"0000000001": baseline_payload})
        payload = b"journaled payload"
        status, data, item = MODULE.create_journaled_blob(
            fake, state_path, state, payload, "startCreated"
        )
        assert status == 201
        assert data == {"id": "0000000002"}
        assert item == {
            "id": "0000000002",
            "sha256": MODULE.sha256_bytes(payload),
        }
        recorded = json.loads(state_path.read_text(encoding="utf-8"))
        assert "pendingCreation" not in recorded
        assert recorded["ownedBlobs"] == [item]
        assert recorded["startCreated"] == [item]


class DeleteThenFailApi(FakeRecoveryApi):
    def __init__(self, blobs: dict[str, bytes], fail_after_delete_once: str) -> None:
        super().__init__(blobs)
        self.fail_after_delete_once = fail_after_delete_once

    def delete_blob(self, blob_id: str) -> None:
        self.blobs.pop(blob_id, None)
        if self.fail_after_delete_once == blob_id:
            self.fail_after_delete_once = ""
            raise MODULE.EvidenceError(
                f"injected host crash after device delete for {blob_id}"
            )


def test_finalize_resumes_after_device_delete_before_journal() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        state_path = root / "state.json"
        output_path = root / "evidence.json"
        baseline_payload = b"baseline"
        first_payload = b"first"
        second_payload = b"second"
        first = {"id": "0000000002", "sha256": MODULE.sha256_bytes(first_payload)}
        second = {"id": "0000000003", "sha256": MODULE.sha256_bytes(second_payload)}
        state = complete_state()
        state.update(
            {
                "baseUrl": "http://device.test",
                "baseline": {"0000000001": MODULE.sha256_bytes(baseline_payload)},
                "sentinels": [first, second],
                "ownedBlobs": [first, second],
            }
        )
        state_path.write_text(json.dumps(state), encoding="utf-8")
        fake = DeleteThenFailApi(
            {
                "0000000001": baseline_payload,
                "0000000002": first_payload,
                "0000000003": second_payload,
            },
            fail_after_delete_once="0000000003",
        )
        args = Namespace(
            state=state_path,
            output=output_path,
            timeout=1.0,
            password_env="UNUSED",
        )
        original_api_from_state = MODULE.api_from_state
        MODULE.api_from_state = lambda current, current_args: fake
        try:
            expect_failure(MODULE.command_finalize, args)
            partial = json.loads(state_path.read_text(encoding="utf-8"))
            assert partial["phase"] == "finalize_in_progress"
            assert partial["ownedBlobs"] == [first, second]
            assert "0000000003" not in fake.blobs
            MODULE.command_finalize(args)
        finally:
            MODULE.api_from_state = original_api_from_state
        complete = json.loads(state_path.read_text(encoding="utf-8"))
        assert complete["phase"] == "complete"
        assert complete["ownedBlobs"] == []
        assert fake.blobs == {"0000000001": baseline_payload}
        MODULE.command_validate(Namespace(evidence=output_path))


def test_finalize_resumes_partial_cleanup() -> None:
    with tempfile.TemporaryDirectory() as temporary_directory:
        root = Path(temporary_directory)
        state_path = root / "state.json"
        output_path = root / "evidence.json"
        baseline_payload = b"baseline"
        first_payload = b"first"
        second_payload = b"second"
        first = {"id": "0000000002", "sha256": MODULE.sha256_bytes(first_payload)}
        second = {"id": "0000000003", "sha256": MODULE.sha256_bytes(second_payload)}
        state = complete_state()
        state.update(
            {
                "baseUrl": "http://device.test",
                "baseline": {"0000000001": MODULE.sha256_bytes(baseline_payload)},
                "sentinels": [first, second],
                "ownedBlobs": [first, second],
            }
        )
        state_path.write_text(json.dumps(state), encoding="utf-8")
        fake = FakeRecoveryApi(
            {
                "0000000001": baseline_payload,
                "0000000002": first_payload,
                "0000000003": second_payload,
            },
            fail_delete_once="0000000002",
        )
        args = Namespace(
            state=state_path,
            output=output_path,
            timeout=1.0,
            password_env="UNUSED",
        )
        original_api_from_state = MODULE.api_from_state
        MODULE.api_from_state = lambda current, current_args: fake
        try:
            expect_failure(MODULE.command_finalize, args)
            partial = json.loads(state_path.read_text(encoding="utf-8"))
            assert partial["phase"] == "finalize_in_progress"
            assert partial["ownedBlobs"] == [first]
            MODULE.command_finalize(args)
        finally:
            MODULE.api_from_state = original_api_from_state
        complete = json.loads(state_path.read_text(encoding="utf-8"))
        assert complete["phase"] == "complete"
        assert complete["ownedBlobs"] == []
        assert fake.blobs == {"0000000001": baseline_payload}
        MODULE.command_validate(Namespace(evidence=output_path))
        evidence = json.loads(output_path.read_text(encoding="utf-8"))
        evidence["phase"] = "ready_to_finalize"
        unhashed = dict(evidence)
        unhashed.pop("evidenceSha256", None)
        evidence["evidenceSha256"] = MODULE.sha256_bytes(
            json.dumps(unhashed, sort_keys=True, separators=(",", ":")).encode()
        )
        output_path.write_text(json.dumps(evidence), encoding="utf-8")
        expect_failure(
            MODULE.command_validate, Namespace(evidence=output_path)
        )


def main() -> int:
    test_exact_gzip()
    test_complete_validation()
    test_current_v2_routes()
    test_diagnostics_schema()
    test_flash_manifest_provenance()
    test_interrupted_upload_request_headers()
    test_mount_failure_record()
    test_recover_cleanup_uses_owned_journal()
    test_pending_creation_recovery_adopts_exact_hash()
    test_pending_creation_rejects_ambiguous_unknown_blobs()
    test_journaled_create_closes_response_window()
    test_finalize_resumes_after_device_delete_before_journal()
    test_finalize_resumes_partial_cleanup()
    print("PASS: V2-035 hardware evidence collector regression tests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
