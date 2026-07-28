#!/usr/bin/env python3
from pathlib import Path

path = Path(__file__).with_name("apply-phase16-finalization.py")
text = path.read_text(encoding="utf-8")
old = '''replace_once(
    "firmware/components/storage/storage_repository_procedures.c",
    "static app_error_code_t procedure_list_locked(const app_uuid_t *set_id,\\n"
    "                                               storage_procedure_list_t *out_list) {\\n",
    "app_error_code_t storage_procedure_list_locked(const app_uuid_t *set_id,\\n"
    "                                                storage_procedure_list_t *out_list) {\\n",
)
'''
new = '''regex_once(
    "firmware/components/storage/storage_repository_procedures.c",
    r"static app_error_code_t procedure_list_locked\\(const app_uuid_t \\*set_id,\\s+"
    r"storage_procedure_list_t \\*out_list\\) \\{",
    "app_error_code_t storage_procedure_list_locked(const app_uuid_t *set_id,\\n"
    "                                                storage_procedure_list_t *out_list) {",
)
'''
count = text.count(old)
if count != 1:
    raise SystemExit(f"expected one finalization matcher, found {count}")
path.write_text(text.replace(old, new, 1), encoding="utf-8")
print("Phase 16 finalization matcher patched")
