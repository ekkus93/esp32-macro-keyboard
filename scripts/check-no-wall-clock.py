#!/usr/bin/env python3
"""Fail closed if firmware acquires, requires, or reports a wall-clock time.

SPEC_V2 §5.4: "The device has no trusted wall clock, RTC synchronization, or
SNTP service. Firmware MUST NOT create, require, or report wall-clock
timestamps."

The V2-156 final acceptance audit (2026-08-16) found this requirement satisfied
by construction but completely unguarded: no gate prevented a future
`time(NULL)` or SNTP client from being added. This is that guard.

Monotonic time is explicitly allowed and is what the firmware actually uses --
`esp_timer_get_time()`, `xTaskGetTickCount()`, and `clock_gettime()` with
`CLOCK_MONOTONIC`. Only calendar/epoch time is forbidden.

Comments and string literals are stripped before scanning: the tree contains a
prose "one at a time (rather than ...)" comment that a naive `\\btime\\s*\\(`
pattern matches, and a guard that cries wolf on English gets disabled.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
PRODUCTION_ROOTS = (Path("firmware/components"), Path("firmware/main"))
SOURCE_SUFFIXES = {".c", ".h"}

# Calendar/epoch-time entry points. Monotonic sources are deliberately absent.
FORBIDDEN_CALLS = (
    "time",
    "gettimeofday",
    "settimeofday",
    "localtime",
    "localtime_r",
    "gmtime",
    "gmtime_r",
    "mktime",
    "strftime",
    "ctime",
    "asctime",
    "difftime",
    "adjtime",
)
CALL = re.compile(r"(?<![\w.>-])(" + "|".join(FORBIDDEN_CALLS) + r")\s*\(")
# SNTP in any spelling: sntp_*, esp_sntp_*, esp_netif_sntp_*.
SNTP = re.compile(r"\b(?:esp_netif_|esp_)?sntp_[a-z_]+\s*\(", re.IGNORECASE)
# clock_gettime is allowed only against a monotonic clock.
CLOCK_GETTIME = re.compile(r"\bclock_gettime\s*\(\s*([A-Z_]+)")
ALLOWED_CLOCKS = {"CLOCK_MONOTONIC", "CLOCK_MONOTONIC_RAW", "CLOCK_BOOTTIME"}

BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.DOTALL)
LINE_COMMENT = re.compile(r"//[^\n]*")
STRING_LITERAL = re.compile(r'"(?:\\.|[^"\\])*"')
CHAR_LITERAL = re.compile(r"'(?:\\.|[^'\\])*'")


def strip_noncode(text: str) -> str:
    """Blank out comments and literals, preserving line numbering."""

    def blank(match: re.Match[str]) -> str:
        return re.sub(r"[^\n]", " ", match.group(0))

    text = BLOCK_COMMENT.sub(blank, text)
    text = LINE_COMMENT.sub(blank, text)
    text = STRING_LITERAL.sub(blank, text)
    return CHAR_LITERAL.sub(blank, text)


def source_files(root: Path) -> list[Path]:
    found: list[Path] = []
    for production_root in PRODUCTION_ROOTS:
        base = root / production_root
        if not base.is_dir():
            continue
        found.extend(
            path
            for path in sorted(base.rglob("*"))
            if path.suffix in SOURCE_SUFFIXES and path.is_file()
        )
    return found


def line_number(text: str, index: int) -> int:
    return text.count("\n", 0, index) + 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=REPO_ROOT)
    args = parser.parse_args()
    root = args.root.resolve()

    failures: list[str] = []
    scanned = 0
    for path in source_files(root):
        scanned += 1
        code = strip_noncode(path.read_text(encoding="utf-8", errors="replace"))
        relative = path.relative_to(root)
        for match in CALL.finditer(code):
            failures.append(
                f"{relative}:{line_number(code, match.start())}: "
                f"wall-clock call {match.group(1)}() is forbidden by SPEC_V2 §5.4"
            )
        for match in SNTP.finditer(code):
            failures.append(
                f"{relative}:{line_number(code, match.start())}: "
                "SNTP is forbidden by SPEC_V2 §5.4"
            )
        for match in CLOCK_GETTIME.finditer(code):
            clock = match.group(1)
            if clock not in ALLOWED_CLOCKS:
                failures.append(
                    f"{relative}:{line_number(code, match.start())}: "
                    f"clock_gettime({clock}) is not a monotonic clock (SPEC_V2 §5.4)"
                )

    if failures:
        for failure in failures:
            print(f"error: {failure}", file=sys.stderr)
        print(
            "error: firmware must not create, require, or report wall-clock timestamps",
            file=sys.stderr,
        )
        return 1

    print(f"no wall-clock usage in firmware ({scanned} source files scanned)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
