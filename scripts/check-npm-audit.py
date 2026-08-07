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
SEVERITIES = ("info", "low", "moderate", "high", "critical", "total")


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
        raise ValueError("audit finding via field must be an array")
    for entry in via:
        if isinstance(entry, dict):
            source = entry.get("source")
            if isinstance(source, int):
                sources.add(source)
        elif not isinstance(entry, str):
            raise ValueError("audit finding via entry must be an object or package name")
    return sources


def finding_diagnostics(finding: dict[str, Any]) -> dict[str, Any]:
    """Return bounded fields needed to review unexpected npm audit findings."""
    return {
        "nodes": finding.get("nodes"),
        "range": finding.get("range"),
        "fixAvailable": finding.get("fixAvailable"),
        "via": finding.get("via"),
    }


def read_counts(report: dict[str, Any]) -> dict[str, int]:
    if "error" in report:
        raise ValueError(f"npm audit returned an error object: {report['error']!r}")
    metadata = report.get("metadata")
    if not isinstance(metadata, dict):
        raise ValueError("audit metadata must be an object")
    raw_counts = metadata.get("vulnerabilities")
    if not isinstance(raw_counts, dict):
        raise ValueError("audit metadata.vulnerabilities must be an object")
    counts: dict[str, int] = {}
    for severity in SEVERITIES:
        value = raw_counts.get(severity)
        if not isinstance(value, int) or isinstance(value, bool) or value < 0:
            raise ValueError(f"audit count {severity} must be a non-negative integer")
        counts[severity] = value
    return counts


def read_findings(report: dict[str, Any]) -> dict[str, dict[str, Any]]:
    raw_findings = report.get("vulnerabilities")
    if not isinstance(raw_findings, dict):
        raise ValueError("audit vulnerabilities must be an object")
    findings: dict[str, dict[str, Any]] = {}
    for name, finding in raw_findings.items():
        if not isinstance(name, str) or not isinstance(finding, dict):
            raise ValueError("every audit finding must be a named object")
        severity = finding.get("severity")
        if severity not in {"info", "low", "moderate", "high", "critical"}:
            raise ValueError(f"{name}: invalid or missing severity")
        findings[name] = finding
    return findings


def validate_policy(
    report: dict[str, Any], lockfile: dict[str, Any], today: dt.date
) -> str:
    counts = read_counts(report)
    findings = read_findings(report)

    severity_names = {
        severity: {
            name for name, finding in findings.items() if finding["severity"] == severity
        }
        for severity in ("info", "low", "moderate", "high", "critical")
    }
    for severity, names in severity_names.items():
        if counts[severity] != len(names):
            raise ValueError(
                f"npm audit {severity} count does not match finding set: "
                f"count={counts[severity]}, names={sorted(names)}"
            )
    if counts["total"] != len(findings):
        raise ValueError(
            "npm audit total count does not match finding set: "
            f"count={counts['total']}, findings={len(findings)}"
        )
    if counts["critical"] != 0:
        raise ValueError(
            f"npm audit contains {counts['critical']} critical finding(s)"
        )

    unexpected_non_high = sorted(
        name
        for severity in ("info", "low", "moderate")
        for name in severity_names[severity]
    )
    if unexpected_non_high:
        details = {
            name: finding_diagnostics(findings[name]) for name in unexpected_non_high
        }
        raise ValueError(
            "npm audit contains unreviewed non-high findings: "
            f"names={unexpected_non_high}, details={details!r}"
        )

    high_names = severity_names["high"]
    if not findings:
        return "npm audit policy: no findings"
    if today > ACCEPTANCE_EXPIRES:
        raise ValueError(
            f"npm audit acceptance expired on {ACCEPTANCE_EXPIRES.isoformat()}"
        )
    if high_names != ACCEPTED_HIGH_FINDINGS:
        unexpected = sorted(high_names - ACCEPTED_HIGH_FINDINGS)
        missing = sorted(ACCEPTED_HIGH_FINDINGS - high_names)
        details = {name: finding_diagnostics(findings[name]) for name in unexpected}
        raise ValueError(
            "npm audit high-finding set changed; "
            f"unexpected={unexpected}, missing={missing}, "
            f"unexpected_details={details!r}"
        )

    packages = lockfile.get("packages", {})
    if not isinstance(packages, dict):
        raise ValueError("package-lock packages must be an object")
    seen_sources: set[int] = set()
    for name in sorted(high_names):
        finding = findings[name]
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
