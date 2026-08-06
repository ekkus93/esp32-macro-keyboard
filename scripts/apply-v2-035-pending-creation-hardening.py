#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/run-v2-035-hardware.py"
TEST = ROOT / "tests/scripts/test-v2-035-hardware.py"
RUNBOOK = ROOT / "docs/implementation-v2/V2_035_HARDWARE_EVIDENCE_HARNESS_2026-08-06.md"


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one match, found {count}: {old[:100]!r}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


def insert_before(path: Path, anchor: str, addition: str) -> None:
    replace_once(path, anchor, addition + anchor)


replace_once(
    SCRIPT,
    "import re\nimport shutil\n",
    "import re\nimport secrets\nimport shutil\n",
)

insert_before(
    SCRIPT,
    "def delete_owned_blob(api: DeviceApi, state: dict[str, Any], state_path: Path,\n",
    r'''def expected_owned_snapshot(state: dict[str, Any]) -> dict[str, str]:
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


''',
)

replace_once(
    SCRIPT,
    r'''    created: list[dict[str, str]] = []
    for label in ("ordering-a", "ordering-b", "ordering-c"):
        payload = small_payload(label)
        status, data = api.create_blob(payload)
        require(status == 201 and isinstance(data, dict) and isinstance(data.get("id"), str),
                f"blob creation returned HTTP {status}: {data!r}")
        item = {"id": data["id"], "sha256": sha256_bytes(payload)}
        created.append(item)
        journal_created_blob(args.state, state, item, "startCreated")
''',
    r'''    created: list[dict[str, str]] = []
    for label in ("ordering-a", "ordering-b", "ordering-c"):
        payload = small_payload(f"{label}:{secrets.token_hex(16)}")
        status, data, item = create_journaled_blob(
            api, args.state, state, payload, "startCreated"
        )
        require(status == 201 and item is not None,
                f"blob creation returned HTTP {status}: {data!r}")
        created.append(item)
''',
)

replace_once(
    SCRIPT,
    r'''def command_recover_cleanup(args: argparse.Namespace) -> None:
    state = load_state(args.state)
    require(state.get("phase") != "complete", "complete evidence cannot be recovered or discarded")
    api = api_from_state(state, args)
    verify_recoverable_snapshot(api, state)
''',
    r'''def command_recover_cleanup(args: argparse.Namespace) -> None:
    state = load_state(args.state)
    require(state.get("phase") != "complete", "complete evidence cannot be recovered or discarded")
    api = api_from_state(state, args)
    reconcile_pending_creation(api, args.state, state)
    verify_recoverable_snapshot(api, state)
''',
)

replace_once(
    SCRIPT,
    r'''    before_diagnostics = parse_diagnostics(api.diagnostics())
    payload = exact_gzip_payload("power-cut-upload")
    connection = open_upload_connection(api)
''',
    r'''    before_diagnostics = parse_diagnostics(api.diagnostics())
    payload = exact_gzip_payload(f"power-cut-upload:{secrets.token_hex(16)}")
    state["phase"] = "interrupted_upload_in_progress"
    write_json(args.state, state)
    begin_pending_creation(api, args.state, state, payload, None)
    connection = open_upload_connection(api)
''',
)

replace_once(
    SCRIPT,
    r'''    state = load_state(args.state, "awaiting_interrupted_upload_reboot")
    api = api_from_state(state, args)
    before = state["interruptedUpload"]["beforeHashes"]
    verify_snapshot(api, before, exact_ids=True)
''',
    r'''    state = load_state(args.state, "awaiting_interrupted_upload_reboot")
    api = api_from_state(state, args)
    before = state["interruptedUpload"]["beforeHashes"]
    pending = pending_creation(state)
    require(pending is not None and pending["beforeHashes"] == before,
            "interrupted-upload creation intent is missing or does not match its baseline")
    verify_snapshot(api, before, exact_ids=True)
''',
)

replace_once(
    SCRIPT,
    r'''    add_scenario(state, "interrupted_upload_no_partial_final", details)
    add_scenario(state, "reboot_temporary_cleanup", details)
    state["phase"] = "ready_for_storage_full"
''',
    r'''    add_scenario(state, "interrupted_upload_no_partial_final", details)
    add_scenario(state, "reboot_temporary_cleanup", details)
    state.pop("pendingCreation", None)
    state["phase"] = "ready_for_storage_full"
''',
)

replace_once(
    SCRIPT,
    r'''    payload = exact_gzip_payload("storage-full")
    state["phase"] = "storage_full_in_progress"
''',
    r'''    state["phase"] = "storage_full_in_progress"
''',
)

replace_once(
    SCRIPT,
    r'''    for attempt in range(1, 9):
        status, data = api.create_blob(payload)
        if status == 201:
            require(isinstance(data, dict) and isinstance(data.get("id"), str),
                    "successful fill upload returned an invalid response")
            item = {"id": data["id"], "sha256": sha256_bytes(payload)}
            fill_created.append(item)
            journal_created_blob(args.state, state, item, "fillCreated")
            continue
''',
    r'''    for attempt in range(1, 9):
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
''',
)

replace_once(
    SCRIPT,
    r'''            "maximumUploadBytes": len(payload),
''',
    r'''            "maximumUploadBytes": BLOB_MAX_BYTES,
''',
)

replace_once(
    SCRIPT,
    r'''    require(state.get("phase") in ("ready_to_finalize", "finalize_in_progress", "complete"),
            f"evidence phase is incomplete: {state.get('phase')!r}")
    recorded_owned = state.get("ownedBlobs")
''',
    r'''    require(state.get("phase") in ("ready_to_finalize", "finalize_in_progress", "complete"),
            f"evidence phase is incomplete: {state.get('phase')!r}")
    require(state.get("pendingCreation") is None,
            "evidence still contains an unresolved creation intent")
    recorded_owned = state.get("ownedBlobs")
''',
)

insert_before(
    TEST,
    "def test_finalize_resumes_partial_cleanup() -> None:\n",
    r'''def test_pending_creation_recovery_adopts_exact_hash() -> None:
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


''',
)

replace_once(
    TEST,
    r'''    test_recover_cleanup_uses_owned_journal()
    test_finalize_resumes_partial_cleanup()
''',
    r'''    test_recover_cleanup_uses_owned_journal()
    test_pending_creation_recovery_adopts_exact_hash()
    test_pending_creation_rejects_ambiguous_unknown_blobs()
    test_journaled_create_closes_response_window()
    test_finalize_resumes_partial_cleanup()
''',
)

replace_once(
    RUNBOOK,
    r'''The collector writes its state before the first mutation and journals every
collector-owned blob immediately after creation. If `start`, `fill-storage`, or
`finalize` fails or the host process is interrupted, do not delete IDs by hand.
''',
    r'''The collector writes its state before the first mutation and persists a
hash-bound creation intent before every upload request. After a successful
response it converts that intent into a collector-owned blob journal entry. If
`start`, `arm-interrupted-upload`, `fill-storage`, or `finalize` fails or the
host process is interrupted, do not delete IDs by hand.
''',
)

replace_once(
    RUNBOOK,
    r'''Recovery verifies every baseline hash, refuses to touch unowned IDs, verifies
each surviving collector-owned blob before deletion, tolerates an owned blob
that was already deleted immediately before a host crash, restores the exact
pre-test blob set, and only then removes the local state file. After recovery,
restart V2-035 from Stage 1 with a newly generated state path.
''',
    r'''Recovery verifies every baseline hash and first reconciles any pending
creation. It adopts at most one new blob only when its bytes match the
pre-request SHA-256 exactly; multiple new IDs or a hash mismatch fail closed.
It then refuses to touch unowned IDs, verifies each surviving collector-owned
blob before deletion, tolerates an owned blob that was already deleted
immediately before a host crash, restores the exact pre-test blob set, and only
then removes the local state file. After recovery, restart V2-035 from Stage 1
with a newly generated state path.
''',
)

print("V2-035 pending-creation hardening applied")
