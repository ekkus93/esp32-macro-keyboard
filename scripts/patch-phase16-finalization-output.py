#!/usr/bin/env python3
"""Correct deterministic defects in generated Phase 16 finalization output."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(relative: str, old: str, new: str, description: str) -> None:
    path = ROOT / relative
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"expected one {description}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


def regex_once(relative: str, pattern: str, replacement: str, description: str) -> None:
    path = ROOT / relative
    text = path.read_text(encoding="utf-8")
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.MULTILINE)
    if count != 1:
        raise SystemExit(f"expected one {description}, found {count}")
    path.write_text(updated, encoding="utf-8")


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

replace_once(
    "firmware/components/web_server/web_api_json.c",
    '#include "macro_limits.h"\n',
    '#include "macro_limits.h"\n#include "macro_model.h"\n',
    "request JSON macro-model include anchor",
)

operations_file = "firmware/components/storage/storage_repository_set_operations.c"
regex_once(
    operations_file,
    r"static app_error_code_t write_duplicate_order\(const char \*staging, const char \*filename,\s+"
    r"const app_uuid_t \*ids, size_t count,\s+size_t maximum_count\) \{",
    "static app_error_code_t write_duplicate_order(const char *staging, const char *filename,\n"
    "                                               size_t maximum_count, const app_uuid_t *ids,\n"
    "                                               size_t count) {",
    "generated duplicate-order helper signature",
)
regex_once(
    operations_file,
    r'result = write_duplicate_order\(staging, "macro-order\.json", ordered_ids,\s+'
    r"macros->count,\s+APP_MACROS_PER_SET_MAX\);",
    'result = write_duplicate_order(staging, "macro-order.json", APP_MACROS_PER_SET_MAX,\n'
    "                                       ordered_ids, macros->count);",
    "generated macro-order helper call",
)
regex_once(
    operations_file,
    r'result = write_duplicate_order\(staging, "procedure-order\.json", ordered_ids,\s+'
    r"procedures->count, APP_PROCEDURES_PER_SET_MAX\);",
    'result = write_duplicate_order(staging, "procedure-order.json",\n'
    "                                       APP_PROCEDURES_PER_SET_MAX, ordered_ids,\n"
    "                                       procedures->count);",
    "generated procedure-order helper call",
)

print("Phase 16 generated output corrections applied")
