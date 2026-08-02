#!/usr/bin/env python3
"""Regenerate docs/SPEC_TEST_TRACEABILITY.md from SPEC.md and the host tests.

Maps every normative statement (MUST / MUST NOT) in the specification to the
tests that cite its section. Deliberately reports "referenced" rather than
"covered": a citation means someone had the section in mind, not that the
sentence is tested. This is a worklist, not a coverage score.

Run with --check to verify the committed document is current without writing.
"""
import collections
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DOCUMENT = ROOT / "docs/SPEC_TEST_TRACEABILITY.md"

PREAMBLE = """# SPEC → test traceability

Generated from `docs/SPEC.md` and `tests/host/test_*.c`. Regenerate with
`scripts/generate-spec-traceability.py`. Do not edit by hand.

## Why this exists

The host suite has hundreds of test functions and passes. That number says
nothing about whether the specification is covered, because the tests were
written **after** the code they test, in the same pass — so they encode what the
implementation does rather than what the specification requires.

That is not hypothetical. `POST /api/v1/sets/{setId}/select` required an
`expectedRevision` field that the handler parsed and never used. Six tests
asserted that requirement because the handler had it. Not one asked what §12.3
actually says, which is nothing about a revision on selection. All six passed
for months. Hardware found it in a minute by sending `{}`.

Consensus among tests derived from the same source is worth nothing. This
document exists so a requirement can be checked against a test, in that
direction.

## How to read `Status`

- **referenced** — at least one test cites this section. That is a *weak* signal:
  it means someone had the section in mind, not that this particular sentence is
  covered. Verify before trusting it.
- **UNMAPPED** — no test anywhere cites this section. Certainly not deliberately
  covered.

Neither value is a coverage measurement. This is a worklist, not a score.
"""


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


def sources():
    """Every host-test source, including the .inc fragments.

    Several suites -- auth, the executor, web security, the web-server adapter --
    keep their test bodies in .inc files that a single test_*.c includes. Scanning
    only test_*.c made every citation in those fragments invisible, which would
    have reported covered sections as unmapped.
    """
    directory = ROOT / "tests/host"
    return sorted(directory.glob("test_*.c")) + sorted(directory.glob("*.inc"))


def citations():
    cites = collections.defaultdict(set)
    for path in sources():
        suite = path.stem[len("test_"):] if path.stem.startswith("test_") else path.stem
        function = None
        for line in path.read_text().splitlines():
            match = re.match(r"^(?:static )?void (\w*test_?\w+)\(", line)
            if match:
                function = match.group(1).removeprefix("test_")
            for cite in re.finditer(r"SPEC\s*§?\s*(\d+(?:\.\d+)?)", line):
                cites[cite.group(1)].add(f"{suite} → {function or '(file)'}")
    return cites


def table(rows, cites):
    lines = [
        "| Section | SPEC line | Requirement | Status | Test(s) referencing this section |",
        "| --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        referencing = sorted(cites.get(row["sec_num"], ()))
        status = "referenced" if referencing else "**UNMAPPED**"
        text = row["text"].replace("|", "\\|")
        lines.append(f"| §{row['sec_num']} | L{row['line']} | {text} | {status} | "
                     f"{'<br>'.join(referencing) if referencing else '—'} |")
    return "\n".join(lines)


def render(found, cites):
    prohibitions = [s for s in found if s["kind"] == "MUST NOT"]
    requirements = [s for s in found if s["kind"] == "MUST"]
    unmapped = [s for s in found if s["sec_num"] not in cites]
    unmapped_prohibitions = [s for s in prohibitions if s["sec_num"] not in cites]
    unmapped_requirements = [s for s in requirements if s["sec_num"] not in cites]
    return "\n".join([
        PREAMBLE,
        "## Totals",
        "",
        "| | Statements | Unmapped |",
        "| --- | --- | --- |",
        f"| MUST NOT | {len(prohibitions)} | {len(unmapped_prohibitions)} |",
        f"| MUST | {len(requirements)} | {len(unmapped_requirements)} |",
        f"| **Total** | **{len(found)}** | **{len(unmapped)}** |",
        "",
        "## Prohibitions (`MUST NOT`) — do these first",
        "",
        "A prohibition has no happy path, so nothing covers it by accident. These are the",
        "cheapest place to find real gaps.",
        "",
        table(prohibitions, cites),
        "",
        "## Requirements (`MUST`)",
        "",
        table(requirements, cites),
        "",
    ])


def main():
    found, cites = statements(), citations()
    rendered = render(found, cites)
    unmapped = len([s for s in found if s["sec_num"] not in cites])
    summary = f"{len(found)} normative statements, {unmapped} in sections no test cites"

    if "--check" in sys.argv[1:]:
        if not DOCUMENT.exists() or DOCUMENT.read_text() != rendered:
            print(f"{DOCUMENT.relative_to(ROOT)} is out of date; "
                  f"run scripts/generate-spec-traceability.py", file=sys.stderr)
            return 1
        print(f"{summary} (document current)")
        return 0

    DOCUMENT.write_text(rendered)
    print(f"{summary} -> {DOCUMENT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
