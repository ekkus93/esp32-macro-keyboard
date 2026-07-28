#!/usr/bin/env python3
"""Correct deterministic defects in generated Phase 16 finalization output."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(relative: str, old: str, new: str, description: str) -> None:
    path = ROOT / relative
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"expected one {description}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


replace_once(
    "tests/host/test_web_execution_route_policy.c",
    "static void test_ready(void) {\n"
    "    const macro_execution_status_t status = running_status();\n"
    "    const web_api_path_t path = matching_path();\n",
    "static void test_ready(void) {\n"
    "    const macro_execution_status_t status = running_status();\n"
    "    web_api_path_t path = matching_path();\n",
    "generated test_ready path declaration",
)

replace_once(
    "firmware/components/web_server/web_api_sets.c",
    '#include "app_uuid.h"\n#include "macro_model.h"\n',
    '#include "app_uuid.h"\n#include "macro_limits.h"\n#include "macro_model.h"\n',
    "generated set API macro_limits include insertion point",
)

print("Phase 16 generated output corrections applied")
