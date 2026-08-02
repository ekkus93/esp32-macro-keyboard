#!/usr/bin/env python3
"""Regenerate docs/SPEC_TEST_TRACEABILITY.md from SPEC.md and the host tests.

Maps every normative statement (MUST / MUST NOT) in the specification to the
tests that cite its section. Deliberately reports "referenced" rather than
"covered": a citation means someone had the section in mind, not that the
sentence is tested. This is a worklist, not a coverage score.
"""
import collections
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent


def statements():
    section, found = "0. (preamble)", []
    for number, line in enumerate(ROOT.joinpath("docs/SPEC.md").read_text().splitlines(), 1):
        heading = re.match(r"^#{2,3}\s+(\d+(?:\.\d+)?)\.?\s+(.*)$", line)
        if heading:
            section = f"{heading.group(1)} {heading.group(2).strip()}"
        if re.search(r"\bMUST\b", line):
            found.append({
                "line": number,
                "sec_num": section.split()[0],
                "kind": "MUST NOT" if re.search(r"\bMUST NOT\b", line) else "MUST",
                "text": line.strip().lstrip("-*0123456789. ").strip(),
            })
    return found


def citations():
    cites = collections.defaultdict(set)
    for path in sorted(ROOT.joinpath("tests/host").glob("test_*.c")):
        function = None
        for line in path.read_text().splitlines():
            match = re.match(r"^static void (test_\w+)\(", line)
            if match:
                function = match.group(1)
            for cite in re.finditer(r"SPEC\s*§?\s*(\d+(?:\.\d+)?)", line):
                cites[cite.group(1)].add(f"{path.name}:{function or '(file)'}")
    return cites


def main():
    found, cites = statements(), citations()
    unmapped = [s for s in found if s["sec_num"] not in cites]
    print(f"{len(found)} normative statements, {len(unmapped)} in sections no test cites")
    return 0


if __name__ == "__main__":
    sys.exit(main())
