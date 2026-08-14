#!/usr/bin/env python3
"""Run the V2-035 physical collector with H5 commit-uncertain reconciliation."""

from __future__ import annotations

import importlib.util
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[1]
COLLECTOR_PATH = REPO_ROOT / "scripts" / "run-v2-035-hardware.py"
COLLECTOR_SPEC = importlib.util.spec_from_file_location("v2_035_hardware", COLLECTOR_PATH)
if COLLECTOR_SPEC is None or COLLECTOR_SPEC.loader is None:
    raise RuntimeError(f"could not load hardware collector from {COLLECTOR_PATH}")
COLLECTOR = importlib.util.module_from_spec(COLLECTOR_SPEC)
COLLECTOR_SPEC.loader.exec_module(COLLECTOR)


def response_error_code(data: Any) -> str | None:
    if not isinstance(data, dict):
        return None
    error = data.get("error")
    if not isinstance(error, dict):
        return None
    code = error.get("code")
    return code if isinstance(code, str) else None


def finish_successful_creation(
    api: Any,
    state_path: Path,
    state: dict[str, Any],
    payload: bytes,
    data: Any,
) -> dict[str, str]:
    COLLECTOR.require(
        isinstance(data, dict) and isinstance(data.get("id"), str),
        "successful blob creation returned an invalid response",
    )
    item = {"id": data["id"], "sha256": COLLECTOR.sha256_bytes(payload)}
    COLLECTOR.require(
        COLLECTOR.sha256_bytes(api.load_blob(item["id"])) == item["sha256"],
        f"new blob {item['id']} did not round-trip byte-identically",
    )
    COLLECTOR.finish_pending_creation(state_path, state, item)
    return item


def create_journaled_blob(
    api: Any,
    state_path: Path,
    state: dict[str, Any],
    payload: bytes,
    stage_key: str,
) -> tuple[int, Any, dict[str, str] | None]:
    """Create once, reconciling commit uncertainty without an automatic retry."""
    COLLECTOR.begin_pending_creation(api, state_path, state, payload, stage_key)
    status, data = api.create_blob(payload)
    if status == 201:
        item = finish_successful_creation(api, state_path, state, payload, data)
        return status, data, item

    if status == 503 and response_error_code(data) == "commit_uncertain":
        item = COLLECTOR.reconcile_pending_creation(api, state_path, state)
        if item is None:
            return status, data, None
        return (
            201,
            {
                "id": item["id"],
                "reconciledCommitUncertain": True,
                "originalHttpStatus": status,
            },
            item,
        )

    COLLECTOR.clear_pending_creation_if_unchanged(api, state_path, state)
    return status, data, None


# The V2 collector's command functions resolve this global from their module at
# call time. Replacing only this seam preserves the proven hardware state
# machine while updating create behavior for the H5 commit-certainty contract.
COLLECTOR.create_journaled_blob = create_journaled_blob


if __name__ == "__main__":
    raise SystemExit(COLLECTOR.main())
