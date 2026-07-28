#!/usr/bin/env python3
"""Normalize source spelling expected by the one-shot Phase 16 transform."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATH = ROOT / "firmware/components/storage/storage_repository_procedures.c"
text = PATH.read_text(encoding="utf-8")
pattern = re.compile(
    r"static app_error_code_t procedure_list_locked\(const app_uuid_t \*set_id,\n"
    r"[ \t]+storage_procedure_list_t \*out_list\) \{\n"
)
replacement = (
    "static app_error_code_t procedure_list_locked(const app_uuid_t *set_id,\n"
    "                                               storage_procedure_list_t *out_list) {\n"
)
updated, count = pattern.subn(replacement, text, count=1)
if count != 1:
    raise SystemExit(f"expected one procedure_list_locked declaration, found {count}")
PATH.write_text(updated, encoding="utf-8")
print("Phase 16 finalization input normalized")
