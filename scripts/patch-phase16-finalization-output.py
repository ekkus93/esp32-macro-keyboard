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

set_api_path = ROOT / "firmware/components/web_server/web_api_sets.c"
set_api_text = set_api_path.read_text(encoding="utf-8")
include = '#include "macro_limits.h"\n'
if set_api_text.count(include) != 0:
    raise SystemExit("generated set API unexpectedly already includes macro_limits.h")
anchor = '#include "macro_model.h"\n'
anchor_count = set_api_text.count(anchor)
if anchor_count != 1:
    raise SystemExit(f"expected one generated set API macro_model include, found {anchor_count}")
set_api_path.write_text(set_api_text.replace(anchor, include + anchor, 1), encoding="utf-8")

print("Phase 16 generated output corrections applied")
