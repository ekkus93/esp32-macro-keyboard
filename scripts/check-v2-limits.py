#!/usr/bin/env python3
"""Verify that the C and TypeScript v2 limits mirror the shared JSON contract."""

from __future__ import annotations

import difflib
import json
import re
import sys
from pathlib import Path
from typing import Any

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
CONTRACT_PATH = REPOSITORY_ROOT / "contracts/v2/limits.json"
C_HEADER_PATH = (
    REPOSITORY_ROOT
    / "firmware/components/app_contracts_v2/include/app_limits_v2.h"
)
TYPESCRIPT_PATH = REPOSITORY_ROOT / "webapp/src/v2/limits.ts"

EXPECTED_KEYS = (
    "packageNameMaxBytes",
    "macroNameMaxBytes",
    "macroSourceMaxBytes",
    "compiledActionsMax",
    "delayDirectiveMaxMs",
    "keyPressMaxMs",
    "interKeyMaxMs",
    "estimatedDurationMaxMs",
    "executorAbsoluteDeadlineMs",
    "jsonBodyMaxBytes",
    "blobMaxBytes",
    "activeSessionsMax",
    "sessionIdleLifetimeSeconds",
    "sessionAbsoluteLifetimeSeconds",
    "serialConfirmationTimeoutSeconds",
    "adminPasswordMinBytes",
    "adminPasswordMaxBytes",
    "snapshotRetentionTargetMax",
)


def load_contract() -> dict[str, int]:
    value: Any = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError("v2 limits contract must be a JSON object")
    if tuple(value) != EXPECTED_KEYS:
        raise ValueError(
            "v2 limits contract keys or order differ from the reviewed contract"
        )

    result: dict[str, int] = {}
    for key, item in value.items():
        if (
            not isinstance(key, str)
            or not isinstance(item, int)
            or isinstance(item, bool)
            or item <= 0
        ):
            raise ValueError(f"{key!r}: v2 limit must be a positive integer")
        result[key] = item
    return result


def upper_snake(name: str) -> str:
    return re.sub(r"(?<!^)(?=[A-Z])", "_", name).upper()


def render_c_header(limits: dict[str, int]) -> str:
    lines = [
        "#pragma once",
        "",
        "#include <stdint.h>",
        "",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "",
    ]
    lines.extend(
        f"#define APP_V2_{upper_snake(name)} UINT32_C({value})"
        for name, value in limits.items()
    )
    lines.extend(["", "#ifdef __cplusplus", "}", "#endif", ""])
    return "\n".join(lines)


def typescript_integer(value: int) -> str:
    return f"{value:_}" if value >= 10_000 else str(value)


def render_typescript(limits: dict[str, int]) -> str:
    lines = ["export const v2Limits = {"]
    lines.extend(
        f"  {name}: {typescript_integer(value)}," for name, value in limits.items()
    )
    lines.extend(["} as const;", "", "export type V2Limits = typeof v2Limits;", ""])
    return "\n".join(lines)


def verify(path: Path, expected: str) -> bool:
    actual = path.read_text(encoding="utf-8")
    if actual == expected:
        return True
    relative_path = path.relative_to(REPOSITORY_ROOT)
    contract_path = CONTRACT_PATH.relative_to(REPOSITORY_ROOT)
    print(
        f"error: {relative_path} is not generated from {contract_path}",
        file=sys.stderr,
    )
    for line in difflib.unified_diff(
        actual.splitlines(),
        expected.splitlines(),
        fromfile="committed",
        tofile="expected",
        lineterm="",
    ):
        print(line, file=sys.stderr)
    return False


def main() -> int:
    try:
        limits = load_contract()
        c_ok = verify(C_HEADER_PATH, render_c_header(limits))
        typescript_ok = verify(TYPESCRIPT_PATH, render_typescript(limits))
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    if not c_ok or not typescript_ok:
        return 1
    print("v2 limits contract matches C and TypeScript mirrors")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
