"""Crash-safe blob creation: the pending-creation journal and snapshots."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Any

from .core import SHA256_PATTERN, require
from .device_api import DeviceApi, blob_ids, snapshot
from .evidence import (
    journal_deleted_blob,
    owned_blobs,
    password_from_environment,
    sha256_bytes,
    utc_now,
    write_json,
)


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


