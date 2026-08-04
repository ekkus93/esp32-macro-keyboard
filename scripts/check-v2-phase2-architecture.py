#!/usr/bin/env python3
"""Fail closed if the retired firmware-owned repository architecture returns."""

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]

FORBIDDEN_PATHS = [
    "firmware/components/storage/include/storage_repository.h",
    "firmware/components/storage/include/storage_package.h",
    "firmware/components/storage/include/storage_object_json.h",
    "firmware/components/storage/storage_repository_json.c",
    "firmware/components/storage/storage_repository_packages.c",
    "firmware/components/storage/storage_repository_macros.c",
    "firmware/components/storage/storage_package.c",
    "firmware/components/web_server/web_api_packages.c",
    "firmware/components/web_server/web_api_macros.c",
    "firmware/components/web_server/web_api_execution.c",
    "firmware/components/web_server/web_api_admin_boundary.c",
    "firmware/components/web_server/web_execution_submit.c",
    "tests/hardware/create_fixture.py",
    "tests/hardware/test_backup_restore.py",
    "tests/hardware/test_full_package_in_order.py",
    "tests/hardware/test_cancellation.py",
    "tests/hardware/test_typing.py",
    "tests/scripts/test-storage-package.sh",
]

FORBIDDEN_SOURCE = re.compile(
    r"storage_repository|storage_package|macro_package_t|activePackageId|"
    r"WEB_API_ROUTE_(SETS?|SET_|EXECUTIONS?|BACKUP|RESTORE)|"
    r'\"/api/v1/(?:package|executions|repository|restore)(?:/|\")'
)

findings: list[str] = []
for relative in FORBIDDEN_PATHS:
    if ROOT.joinpath(relative).exists():
        findings.append(f"retired path exists: {relative}")

for trigger in sorted(ROOT.joinpath(".github").glob("phase18-*trigger*.txt")):
    findings.append(f"obsolete CI trigger exists: {trigger.relative_to(ROOT)}")

for base in (ROOT / "firmware/components", ROOT / "firmware/main"):
    if not base.is_dir():
        continue
    for path in sorted(base.rglob("*")):
        if path.suffix not in {".c", ".h"} or not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for number, line in enumerate(text.splitlines(), 1):
            if FORBIDDEN_SOURCE.search(line):
                findings.append(f"{path.relative_to(ROOT)}:{number}:{line.strip()}")

if findings:
    print("error: retired v1 repository ownership is present", file=sys.stderr)
    for finding in findings:
        print(f"  {finding}", file=sys.stderr)
    raise SystemExit(1)

print("phase 2 architecture: no firmware-owned package or macro repository")
