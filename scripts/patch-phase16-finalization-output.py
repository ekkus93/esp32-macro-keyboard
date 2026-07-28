#!/usr/bin/env python3
"""Correct deterministic defects in generated Phase 16 finalization output."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATH = ROOT / "tests/host/test_web_execution_route_policy.c"
text = PATH.read_text(encoding="utf-8")
old = (
    "static void test_ready(void) {\n"
    "    const macro_execution_status_t status = running_status();\n"
    "    const web_api_path_t path = matching_path();\n"
)
new = (
    "static void test_ready(void) {\n"
    "    const macro_execution_status_t status = running_status();\n"
    "    web_api_path_t path = matching_path();\n"
)
count = text.count(old)
if count != 1:
    raise SystemExit(f"expected one generated test_ready path declaration, found {count}")
PATH.write_text(text.replace(old, new, 1), encoding="utf-8")
print("Phase 16 generated test mutability corrected")
