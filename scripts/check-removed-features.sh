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
    # SPEC 1.1 rejects "any field, screen, or code path specific to Chromebooks,
    # ChromeOS, Debian, or any other particular target machine". SPEC 4 puts it
    # positively: no host detection, no behaviour conditional on the target. The
    # device types what it is told to type and does not know what is listening.
    #
    # Test fixtures count. A suite whose sample data is "Lab Chromebook
    # workflow" quietly reinstates the assumption the revision removed, and is
    # where a target-specific field would first look reasonable.
    r"chromebook|chrome ?os|\bdebian\b",
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
    # The frontend suite lives here rather than beside the code, and was outside
    # this check until its fixtures turned out to be named after a target
    # machine -- exactly what SPEC 1.1 rejects.
    "webapp/tests",
]
SUFFIXES = {".c", ".h", ".inc", ".ts", ".tsx", ".js", ".mjs"}
SKIP_DIRS = {"build", "build-coverage", "node_modules", "managed_components"}

# A line that must legitimately name a removed feature is one that proves it is
# gone: a route that MUST now 404, a directory that MUST NOT exist. Those opt out
# explicitly, with a reason, so the exception is visible where it applies rather
# than encoded in a clever pattern here. Accepted on the line itself or within the
# few lines above it: clang-format decides where a wrapped call breaks, and the
# offending token often lands on a continuation line rather than the first one.
MARKER = "removed-feature-ok:"
MARKER_LOOKBACK = 3
# A table of rejected inputs is a block of legitimate mentions, and marking each
# line of it would bury the data in annotations. These bracket the whole run.
BLOCK_OPEN = "removed-feature-ok-begin:"
BLOCK_CLOSE = "removed-feature-ok-end:"
COMMENT = re.compile(r"^\s*(\*|//|/\*)")
# A wrapped /* ... */ comment continues on lines with no marker of their own, so
# matching only the first line still flagged prose. Tracked as a block instead.
BLOCK_COMMENT_OPEN = re.compile(r"/\*(?!.*\*/)")
BLOCK_COMMENT_CLOSE = re.compile(r"\*/")

# Case-insensitive throughout: a removed feature reintroduced as `Quarantine`
# or `Chromebook` is the same feature.
matcher = re.compile("|".join(f"({p})" for p in PATTERNS), re.IGNORECASE)
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
        in_block = False
        in_comment = False
        for number, line in enumerate(lines, 1):
            if BLOCK_OPEN in line:
                in_block = True
            if BLOCK_CLOSE in line:
                in_block = False
                continue
            was_comment = in_comment
            if in_comment and BLOCK_COMMENT_CLOSE.search(line):
                in_comment = False
            elif not in_comment and BLOCK_COMMENT_OPEN.search(line):
                in_comment = True
            if (in_block or was_comment or in_comment
                    or COMMENT.match(line) or not matcher.search(line)):
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
