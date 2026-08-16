#!/usr/bin/env python3
"""Fail closed on the two SPEC_V2 §17 guards that had no dedicated check.

§17 requires checks that *prevent reintroduction* of five things. Three were
already guarded by `check-v2-phase2-architecture.py` and friends; these two were
covered only by ordinary feature tests, which is easier to regress unnoticed
than a checked-in guard (V2-142):

  1. **Automatic snapshot creation or deletion.** SPEC_V2 §10.5 and §14.3 make
     every snapshot mutation an explicit user action. The failure mode this
     guards is a mutation moving into a React effect, where it would run on
     render rather than on a click.

  2. **Mandatory standalone send-flow navigation.** SPEC_V2 §14.4 and
     UI_UX_SPEC_V2 §8 keep an ordinary send inline on the Macros page. The
     failure mode is a dedicated confirmation screen reappearing in the route
     union, or the send path navigating away.

Both checks are deliberately narrow: they look for the specific shape each
regression would take, so they stay quiet on ordinary refactoring.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

REPO_ROOT = Path(__file__).resolve().parents[1]
WEBAPP_SRC = Path("webapp/src")

# Every entry point that creates or destroys a stored snapshot.
SNAPSHOT_MUTATIONS = (
    "saveWorkingCopyAsSnapshot",
    "deleteSnapshot",
    "replaceSnapshotWithWorkingCopy",
)
ROUTING_FILE = Path("webapp/src/v2/routingV2.ts")
# A screen whose name suggests a standalone send/confirmation step.
FORBIDDEN_SCREEN = re.compile(r"confirm|sending|send-", re.IGNORECASE)


def fail(messages: list[str]) -> int:
    for message in messages:
        print(f"error: {message}", file=sys.stderr)
    return 1


def effect_ranges(text: str) -> list[tuple[int, int]]:
    """Character ranges covered by each `useEffect(` callback body."""
    ranges: list[tuple[int, int]] = []
    for match in re.finditer(r"\buseEffect\s*\(", text):
        index = match.end() - 1
        depth = 0
        for position in range(index, len(text)):
            character = text[position]
            if character == "(":
                depth += 1
            elif character == ")":
                depth -= 1
                if depth == 0:
                    ranges.append((match.start(), position))
                    break
    return ranges


def check_snapshot_mutations(root: Path) -> list[str]:
    problems: list[str] = []
    base = root / WEBAPP_SRC
    for path in sorted(base.rglob("*")):
        if path.suffix not in {".ts", ".tsx"} or not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        ranges = effect_ranges(text)
        if not ranges:
            continue
        for name in SNAPSHOT_MUTATIONS:
            for call in re.finditer(r"\b" + re.escape(name) + r"\s*\(", text):
                for start, end in ranges:
                    if start <= call.start() <= end:
                        line = text.count("\n", 0, call.start()) + 1
                        problems.append(
                            f"{path.relative_to(root)}:{line}: {name}() is called inside a "
                            "useEffect; snapshot creation and deletion must stay explicit "
                            "user actions (SPEC_V2 §10.5, §17)"
                        )
                        break
    return problems


def check_send_navigation(root: Path) -> list[str]:
    problems: list[str] = []
    routing = root / ROUTING_FILE
    if not routing.is_file():
        return [f"{ROUTING_FILE}: not found; cannot verify the v2 screen union"]
    text = routing.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"screensV2\s*=\s*\[(.*?)\]", text, re.DOTALL)
    if match is None:
        return [f"{ROUTING_FILE}: could not locate the screensV2 union"]
    screens = re.findall(r'"([^"]+)"', match.group(1))
    if not screens:
        return [f"{ROUTING_FILE}: screensV2 is empty"]
    for screen in screens:
        if FORBIDDEN_SCREEN.search(screen):
            problems.append(
                f"{ROUTING_FILE}: screen {screen!r} looks like a standalone send or "
                "confirmation step; an ordinary send stays inline on the Macros page "
                "(SPEC_V2 §14.4, §17)"
            )
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=REPO_ROOT)
    args = parser.parse_args()
    root = args.root.resolve()

    problems = check_snapshot_mutations(root) + check_send_navigation(root)
    if problems:
        return fail(problems)
    print("v2 snapshot-lifecycle and send-flow policy guards passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
