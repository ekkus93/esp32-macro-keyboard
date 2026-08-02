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

C_FUNCTION = re.compile(r"^(?:static )?void (\w*test_?\w+)\(")
# it("...") / test("...") / describe("..."), quoted with ' or " or a backtick.
VITEST_CASE = re.compile(r"""^\s*(?:it|test|describe)(?:\.\w+)?\(\s*['"`]([^'"`]+)""")
# `SPEC 24.1 item: every named key` -- attaches to one row of an expanded list.
ITEM_CITE = re.compile(r"SPEC\s*§?\s*(\d+(?:\.\d+)?)\s+item:\s*(.+)$")
# A section-level citation, which an item citation is NOT: without the lookahead,
# "SPEC 24.3 item: chords" also reads as a citation of all of section 24.3, and
# one covered item would mark the whole checklist referenced.
SECTION_CITE = re.compile(r"SPEC\s*§?\s*(\d+(?:\.\d+)?)(?!\s+item:)")

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
- **gate-enforced** — no test cites it, but a `scripts/check-*.sh` that runs on
  every `check-all.sh` does. Some prohibitions are properties of the tree, not
  behaviours of a function: "MUST NOT fetch remote resources" and "MUST NOT use
  warning suppression" cannot be unit-tested, and a script that fails the build
  is the stronger enforcement. Listing them as unmapped understated coverage;
  calling them tests would overstate it.
- **UNMAPPED** — nothing anywhere cites this section. Certainly not deliberately
  covered.

None of these is a coverage measurement. This is a worklist, not a score.
"""


BULLET = re.compile(r"^\s*[-*]\s+(.*)$")
HEADING = re.compile(r"^#{2,3}\s+(\d+(?:\.\d+)?)\.?\s+(.*)$")


def collect_bullets(lines, start):
    """The bullet list introduced by a `... MUST:` line, and where it ends.

    Returns [(line_number, text)] with wrapped bullets joined, or [] when the
    line introduces something else -- a code block, a table, a paragraph.
    """
    cursor = start
    if cursor < len(lines) and not lines[cursor].strip():
        cursor += 1  # the blank line the specification puts before every list
    bullets = []
    while cursor < len(lines):
        match = BULLET.match(lines[cursor])
        if match is None:
            break
        number, text = cursor + 1, match.group(1).strip()
        cursor += 1
        # A wrapped bullet continues on an indented, non-bullet, non-empty line.
        while (cursor < len(lines) and lines[cursor].strip()
               and BULLET.match(lines[cursor]) is None
               and lines[cursor].startswith((" ", "\t"))):
            text += " " + lines[cursor].strip()
            cursor += 1
        bullets.append((number, text.rstrip(";.").strip()))
    return bullets, cursor


def statements():
    """Every normative statement, with `MUST:` lists expanded into their items.

    A line like "Tests MUST cover:" is not one requirement, it is a heading over
    a dozen of them. Counting it once made SPEC 24 look like six statements when
    it carries about seventy, and the same flattening applied to every other
    `MUST:` list in the document. Each bullet is now its own row, inheriting the
    kind and section of the line that introduced it.
    """
    lines = ROOT.joinpath("docs/SPEC.md").read_text().splitlines()
    section, found, index = "0. (preamble)", [], 0
    while index < len(lines):
        line = lines[index]
        heading = HEADING.match(line)
        if heading:
            section = f"{heading.group(1)} {heading.group(2).strip()}"
        if re.search(r"\bMUST\b", line):
            kind = "MUST NOT" if re.search(r"\bMUST NOT\b", line) else "MUST"
            entry = {"sec_num": section.split()[0], "kind": kind}
            if line.strip().endswith(":"):
                bullets, cursor = collect_bullets(lines, index + 1)
                if bullets:
                    for number, text in bullets:
                        found.append({**entry, "line": number, "text": text})
                    index = cursor
                    continue
            found.append({
                **entry,
                "line": index + 1,
                "text": line.strip().lstrip("-*0123456789. ").strip(),
            })
        index += 1
    return found


def sources():
    """Every test source: host C, the .inc fragments, and the frontend suite.

    Several C suites -- auth, the executor, web security, the web-server adapter
    -- keep their test bodies in .inc files that a single test_*.c includes, and
    the frontend's 17 vitest files live in webapp/tests/ rather than beside the
    code. Scanning only tests/host/test_*.c made citations in either invisible,
    which reports covered sections as unmapped. The specification covers the web
    application (SPEC 9) as much as the firmware, so its tests count.
    """
    host = ROOT / "tests/host"
    webapp = ROOT / "webapp/tests"
    hardware = ROOT / "tests/hardware"
    return (sorted(host.glob("test_*.c")) + sorted(host.glob("*.inc")) +
            sorted(webapp.glob("*.test.ts")) + sorted(webapp.glob("*.test.tsx")) +
            # Real-Chrome workflows. Some requirements -- responsive layout above
            # all -- are only observable in a browser that does layout.
            sorted((webapp / "browser").glob("*.mjs")) +
            # Hardware-in-the-loop. These only run with a board attached, so a
            # citation here means "proved on the bench", not "runs in CI".
            sorted(hardware.glob("test_*.py")))


def enforcers():
    """Gate scripts that cite a SPEC section.

    Not every prohibition is a unit test. "MUST NOT fetch remote resources" and
    "MUST NOT use warning suppression" are properties of the tree, enforced by
    scripts/check-*.sh on every run of check-all.sh. Counting those sections as
    unmapped understated real coverage; crediting them as tests would overstate
    it. They are listed separately and marked.
    """
    cites = collections.defaultdict(set)
    scripts = sorted((ROOT / "scripts").glob("*.sh")) + sorted((ROOT / "scripts").glob("*.py"))
    for path in scripts:
        # This file names sections in its own documentation and enforces none of
        # them. Scanning itself made §9, §12.3, and every §24.1 item read as
        # gate-enforced on the strength of a docstring.
        if path.resolve() == pathlib.Path(__file__).resolve():
            continue
        for line in path.read_text().splitlines():
            for cite in SECTION_CITE.finditer(line):
                cites[cite.group(1)].add(f"{path.name} (gate script)")
    return cites


def item_citations():
    """Citations that name one item of an expanded `MUST:` list.

    Section-level granularity is too coarse for a checklist: one test citing
    SPEC 24.1 would mark all thirteen parser items referenced, including the
    ones nothing tests. A test can instead write

        SPEC 24.1 item: every named key

    and it attaches to that row alone. Matching is substring, case-insensitive,
    so the citation need not repeat the specification's exact wording.
    """
    cites = collections.defaultdict(set)
    for path in sources():
        suite = path.stem[len("test_"):] if path.stem.startswith("test_") else path.stem
        suite = suite[: -len(".test")] if suite.endswith(".test") else suite
        function = None
        for line in path.read_text().splitlines():
            match = C_FUNCTION.match(line) or VITEST_CASE.match(line)
            if match:
                function = match.group(1).removeprefix("test_")
            for cite in ITEM_CITE.finditer(line):
                section, item = cite.group(1), cite.group(2).strip().rstrip("*/ ").strip()
                if item:
                    cites[(section, item.lower())].add(f"{suite} → {function or '(file)'}")
    return cites


def citations():
    cites = collections.defaultdict(set)
    for path in sources():
        suite = path.stem[len("test_"):] if path.stem.startswith("test_") else path.stem
        suite = suite[: -len(".test")] if suite.endswith(".test") else suite
        function = None
        for line in path.read_text().splitlines():
            match = C_FUNCTION.match(line) or VITEST_CASE.match(line)
            if match:
                function = match.group(1).removeprefix("test_")
            for cite in SECTION_CITE.finditer(line):
                cites[cite.group(1)].add(f"{suite} → {function or '(file)'}")
    return cites


def table(rows, cites, gates, items):
    lines = [
        "| Section | SPEC line | Requirement | Status | Referencing test / enforcer |",
        "| --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        referencing = sorted(matching_items(row, items) or cites.get(row["sec_num"], ()))
        enforcing = sorted(gates.get(row["sec_num"], ()))
        if referencing:
            status = "referenced"
        elif enforcing:
            status = "gate-enforced"
        else:
            status = "**UNMAPPED**"
        text = row["text"].replace("|", "\\|")
        both = referencing + enforcing
        lines.append(f"| §{row['sec_num']} | L{row['line']} | {text} | {status} | "
                     f"{'<br>'.join(both) if both else '—'} |")
    return "\n".join(lines)


def matching_items(row, items):
    """Item-level citations attaching to this row, if any.

    An item citation wins over a section-level one: naming the item is a
    stronger claim than naming the section, and mixing them would let the weaker
    signal mask which rows are actually unmapped.
    """
    text = row["text"].lower()
    found = set()
    for (section, item), tests in items.items():
        if section == row["sec_num"] and (item in text or text in item):
            found |= tests
    return found


def render(found, cites, gates, items):
    def covered(statement):
        return (bool(matching_items(statement, items))
                or statement["sec_num"] in cites or statement["sec_num"] in gates)

    prohibitions = [s for s in found if s["kind"] == "MUST NOT"]
    requirements = [s for s in found if s["kind"] == "MUST"]
    unmapped = [s for s in found if not covered(s)]
    unmapped_prohibitions = [s for s in prohibitions if not covered(s)]
    unmapped_requirements = [s for s in requirements if not covered(s)]
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
        table(prohibitions, cites, gates, items),
        "",
        "## Requirements (`MUST`)",
        "",
        table(requirements, cites, gates, items),
        "",
    ])


def main():
    found, cites, gates = statements(), citations(), enforcers()
    items = item_citations()
    rendered = render(found, cites, gates, items)
    unmapped = len([s for s in found
                    if not matching_items(s, items)
                    and s["sec_num"] not in cites and s["sec_num"] not in gates])
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
