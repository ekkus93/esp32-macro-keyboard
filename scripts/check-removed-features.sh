#!/usr/bin/env bash
set -euo pipefail

# SPEC 1.1 lists what this revision removed and states it "MUST NOT be
# reintroduced without a deliberate amendment". SPEC 26 sharpens that: those
# items are not deferred, they are rejected, and "Deferred features MUST NOT be
# partially or silently enabled in version 0.1."
#
# A rejected feature is only rejected for as long as something checks. Seven
# phases of work removed procedures, steps, progress, global macros, quarantine,
# and the staging/trash/transaction directories; nothing prevented them coming
# back one plausible-looking commit at a time. This is that guard.

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

python3 - <<'PY'
import re
import sys
from pathlib import Path

# Identifier fragments that only appear if the feature itself is back. "progress"
# as a bare word is a legitimate execution concept, so only the removed compound
# forms are listed.
PATTERNS = [
    r"procedure_t|procedure_id|procedure_step|checkpoint_step|instruction_step",
    r"WEB_API_ROUTE_(SET_)?PROCEDURES?|WEB_API_ROUTE_PROCEDURE_PROGRESS",
    r"progress_resource|procedure_progress|progress_complete|progress_skip",
    r"global_macro|shared_macro|api/v1/global",
    r"quarantine",
    r"STORAGE_(STAGING|TRASH|TRANSACTIONS)|/staging|/trash|/transactions",
    r"transaction_(begin|commit|rollback|journal)",
]
SOURCES = [
    "firmware/components",
    "firmware/main",
    "firmware/test_app/main",
    "webapp/src",
    "tests/host",
]
SUFFIXES = {".c", ".h", ".inc", ".ts", ".tsx"}
SKIP_DIRS = {"build", "build-coverage", "node_modules", "managed_components"}

# A line that must legitimately name a removed feature is one that proves it is
# gone: a route that MUST now 404, a directory that MUST NOT exist. Those opt out
# explicitly, with a reason, so the exception is visible where it applies rather
# than encoded in a clever pattern here. Accepted on the line itself or within the
# few lines above it: clang-format decides where a wrapped call breaks, and the
# offending token often lands on a continuation line rather than the first one.
MARKER = "removed-feature-ok:"
MARKER_LOOKBACK = 3
COMMENT = re.compile(r"^\s*(\*|//|/\*)")

matcher = re.compile("|".join(f"({p})" for p in PATTERNS))
findings = []

for root in SOURCES:
    base = Path(root)
    if not base.is_dir():
        continue
    for path in sorted(base.rglob("*")):
        if path.suffix not in SUFFIXES or not path.is_file():
            continue
        if SKIP_DIRS.intersection(path.parts):
            continue
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        for number, line in enumerate(lines, 1):
            if COMMENT.match(line) or not matcher.search(line):
                continue
            window = lines[max(0, number - 1 - MARKER_LOOKBACK):number]
            if any(MARKER in candidate for candidate in window):
                continue
            findings.append(f"{path}:{number}:{line.strip()}")

if findings:
    print("error: a feature SPEC 1.1 removed has reappeared (SPEC 1.1, SPEC 26)", file=sys.stderr)
    for finding in findings:
        print(f"  {finding}", file=sys.stderr)
    print(f"       if the line proves the feature is gone, mark it: {MARKER} <reason>",
          file=sys.stderr)
    raise SystemExit(1)

print("removed features: none of the SPEC 1.1 rejections have reappeared")
PY
