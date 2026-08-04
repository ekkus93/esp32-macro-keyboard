#!/usr/bin/env python3
"""Generate conservative source-level traceability for the authoritative v2 specs.

The report deliberately does not convert a citation into a coverage claim. A
source is marked referenced only when first-party tests or gate scripts use the
explicit syntax ``SPEC_V2 §<section>`` or ``UI_UX_SPEC_V2 §<section>``. Until
then, every requirement in that source remains visibly unmapped.
"""

from __future__ import annotations

import hashlib
import re
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOCUMENT = ROOT / "docs/SPEC_V2_TEST_TRACEABILITY.md"
NORMATIVE = re.compile(r"\b(?:MUST(?:\s+NOT)?|REQUIRED|SHOULD(?:\s+NOT)?)\b")


@dataclass(frozen=True)
class SourceSpec:
    label: str
    path: Path
    citation_prefix: str


SOURCES = (
    SourceSpec("Product / firmware specification", ROOT / "docs/SPEC_V2.md", "SPEC_V2"),
    SourceSpec(
        "React UI/UX specification",
        ROOT / "docs/UI_UX_SPEC_V2.md",
        "UI_UX_SPEC_V2",
    ),
)


def git_blob_sha(path: Path) -> str:
    content = path.read_bytes()
    header = f"blob {len(content)}\0".encode()
    return hashlib.sha1(header + content).hexdigest()


def has_normative_requirements(path: Path) -> bool:
    return any(
        NORMATIVE.search(line)
        for line in path.read_text(encoding="utf-8").splitlines()
    )


def citation_sources() -> list[Path]:
    roots = (
        ROOT / "tests",
        ROOT / "webapp/tests",
        ROOT / "scripts",
    )
    paths: set[Path] = set()
    for source_root in roots:
        if not source_root.exists():
            continue
        for path in source_root.rglob("*"):
            if not path.is_file() or path == Path(__file__).resolve():
                continue
            if path.suffix not in {
                ".c",
                ".h",
                ".inc",
                ".ts",
                ".tsx",
                ".mjs",
                ".py",
                ".sh",
            }:
                continue
            paths.add(path)
    return sorted(paths)


def explicit_citations(prefix: str) -> list[str]:
    pattern = re.compile(rf"\b{re.escape(prefix)}\s+§\s*(\d+(?:\.\d+)*)")
    references: set[str] = set()
    for path in citation_sources():
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for match in pattern.finditer(text):
            references.add(f"§{match.group(1)}")
    return sorted(
        references,
        key=lambda value: [int(part) for part in value[1:].split(".")],
    )


def render() -> str:
    rows: list[str] = []
    mapped_sources = 0
    for source in SOURCES:
        if not has_normative_requirements(source.path):
            raise ValueError(
                f"{source.path.relative_to(ROOT)} has no normative requirements"
            )
        citations = explicit_citations(source.citation_prefix)
        if citations:
            mapped_sources += 1
            status = "referenced, not proven"
            reference_text = ", ".join(citations)
        else:
            status = "**UNMAPPED**"
            reference_text = "—"
        rows.append(
            "| "
            + str(source.path.relative_to(ROOT))
            + f" | `{git_blob_sha(source.path)}` | present | {status} | {reference_text} |"
        )

    source_count = len(SOURCES)
    unmapped_sources = source_count - mapped_sources
    lines = [
        "# v2 specification traceability",
        "",
        "Generated from `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`, first-party",
        "tests, and gate scripts. Regenerate with",
        "`python3 scripts/generate-spec-traceability.py`. Do not edit by hand.",
        "",
        "## Interpretation",
        "",
        "This is a conservative source-level worklist. A source is marked",
        "`referenced, not proven` only when a first-party test or gate script uses an",
        "explicit citation such as `SPEC_V2 §13.7` or `UI_UX_SPEC_V2 §8.2`.",
        "A citation does not prove every requirement in that section. An unmapped",
        "source means no requirement in that source is being claimed covered by this",
        "matrix.",
        "",
        "The source fingerprints make the report fail closed when either authoritative",
        "specification changes. The generator also fails if either source contains zero",
        "normative requirement lines.",
        "",
        "## Totals",
        "",
        "| Metric | Count |",
        "| --- | ---: |",
        f"| Authoritative v2 sources | {source_count} |",
        f"| Sources containing normative requirements | {source_count} |",
        f"| Sources with explicit v2 citations | {mapped_sources} |",
        f"| Unmapped authoritative sources | {unmapped_sources} |",
        "",
        "## Source-level mapping",
        "",
        "| Source | Git blob SHA | Normative requirements | Status | Explicit section citations |",
        "| --- | --- | --- | --- | --- |",
        *rows,
        "",
        "## Next refinement",
        "",
        "Phase 2 and later work should add explicit v2 section citations while tests are",
        "rewritten against the authoritative specifications. This matrix must remain",
        "conservative: citations are references, never automatic coverage claims.",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    try:
        rendered = render()
    except (OSError, ValueError) as error:
        print(f"v2 traceability generation failed: {error}", file=sys.stderr)
        return 1

    if "--check" in sys.argv[1:]:
        if not DOCUMENT.exists() or DOCUMENT.read_text(encoding="utf-8") != rendered:
            print(
                f"{DOCUMENT.relative_to(ROOT)} is out of date; run "
                "python3 scripts/generate-spec-traceability.py",
                file=sys.stderr,
            )
            return 1
        print(f"{DOCUMENT.relative_to(ROOT)} is current")
        return 0

    if len(sys.argv) != 1:
        print("usage: generate-spec-traceability.py [--check]", file=sys.stderr)
        return 2
    DOCUMENT.write_text(rendered, encoding="utf-8")
    print(f"wrote {DOCUMENT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
