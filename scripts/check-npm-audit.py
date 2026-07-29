#!/usr/bin/env python3
"""Fail closed on npm audit findings outside the reviewed dev-only exception."""

from __future__ import annotations

import argparse
import datetime as dt
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

ACCEPTED_HIGH_FINDINGS = {
    "@eslint/config-array",
    "@eslint/eslintrc",
    "brace-expansion",
    "eslint",
    "minimatch",
}
ACCEPTED_ADVISORY_SOURCES = {1124334}
ACCEPTANCE_EXPIRES = dt.date(2026, 9, 30)


def load_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path}: expected a JSON object")
    return value


def run_audit() -> dict[str, Any]:
    completed = subprocess.run(
        ["npm", "audit", "--json"],
        check=False,
        capture_output=True,
        text=True,
    )
    if not completed.stdout.strip():
        if completed.stderr:
            print(completed.stderr, file=sys.stderr)
        raise RuntimeError("npm audit produced no JSON report")
    value = json.loads(completed.stdout)
    if not isinstance(value, dict):
        raise ValueError("npm audit did not return a JSON object")
    return value


def advisory_sources(finding: dict[str, Any]) -> set[int]:
    sources: set[int] = set()
    via = finding.get("via", [])
    if not isinstance(via, list):
        return sources
    for entry in via:
        if isinstance(entry, dict):
            source = entry.get("source")
            if isinstance(source, int):
                sources.add(source)
    return sources


def validate_policy(
    report: dict[str, Any], lockfile: dict[str, Any], today: dt.date
) -> str:
    metadata = report.get("metadata", {})
    counts = metadata.get("vulnerabilities", {}) if isinstance(metadata, dict) else {}
    if not isinstance(counts, dict):
        raise ValueError("audit metadata.vulnerabilities must be an object")
    critical = int(counts.get("critical", 0))
    if critical != 0:
        raise ValueError(f"npm audit contains {critical} critical finding(s)")

    findings = report.get("vulnerabilities", {})
    if not isinstance(findings, dict):
        raise ValueError("audit vulnerabilities must be an object")
    high_names = {
        name
        for name, finding in findings.items()
        if isinstance(name, str)
        and isinstance(finding, dict)
        and finding.get("severity") == "high"
    }
    unexpected_non_high = sorted(
        name
        for name, finding in findings.items()
        if isinstance(name, str)
        and isinstance(finding, dict)
        and finding.get("severity") not in {"high"}
    )
    if unexpected_non_high:
        raise ValueError(
            "npm audit contains unreviewed non-high findings: "
            + ", ".join(unexpected_non_high)
        )
    if not high_names:
        return "npm audit policy: no findings"
    if today > ACCEPTANCE_EXPIRES:
        raise ValueError(
            f"npm audit acceptance expired on {ACCEPTANCE_EXPIRES.isoformat()}"
        )
    if high_names != ACCEPTED_HIGH_FINDINGS:
        unexpected = sorted(high_names - ACCEPTED_HIGH_FINDINGS)
        missing = sorted(ACCEPTED_HIGH_FINDINGS - high_names)
        raise ValueError(
            f"npm audit high-finding set changed; unexpected={unexpected}, missing={missing}"
        )

    packages = lockfile.get("packages", {})
    if not isinstance(packages, dict):
        raise ValueError("package-lock packages must be an object")
    seen_sources: set[int] = set()
    for name in sorted(high_names):
        finding = findings[name]
        if not isinstance(finding, dict):
            raise ValueError(f"{name}: finding must be an object")
        nodes = finding.get("nodes", [])
        if not isinstance(nodes, list) or not nodes:
            raise ValueError(f"{name}: finding has no installed dependency nodes")
        for node in nodes:
            if not isinstance(node, str):
                raise ValueError(f"{name}: invalid dependency node")
            package = packages.get(node)
            if not isinstance(package, dict) or package.get("dev") is not True:
                raise ValueError(f"{name}: finding is not dev-only at {node}")
        seen_sources.update(advisory_sources(finding))

    if seen_sources != ACCEPTED_ADVISORY_SOURCES:
        raise ValueError(
            "npm audit advisory source set changed; "
            f"expected={sorted(ACCEPTED_ADVISORY_SOURCES)}, "
            f"actual={sorted(seen_sources)}"
        )
    return (
        "npm audit policy: accepted five dev-only ESLint toolchain findings "
        f"through {ACCEPTANCE_EXPIRES.isoformat()}; advisory sources "
        f"{sorted(seen_sources)}"
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, help="Read npm audit JSON from this path")
    parser.add_argument(
        "--lockfile", type=Path, default=Path("package-lock.json")
    )
    args = parser.parse_args()
    try:
        report = load_json(args.input) if args.input is not None else run_audit()
        lockfile = load_json(args.lockfile)
        print(validate_policy(report, lockfile, dt.date.today()))
    except (OSError, RuntimeError, ValueError, json.JSONDecodeError) as error:
        print(f"npm audit policy failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
