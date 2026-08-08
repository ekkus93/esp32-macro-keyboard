#!/usr/bin/env python3
"""Fail closed unless npm audit reports zero vulnerabilities."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

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


def validate_policy(report: dict[str, Any]) -> str:
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
    if findings:
        details = {
            name: {
                "severity": finding["severity"],
                "range": finding.get("range"),
                "fixAvailable": finding.get("fixAvailable"),
            }
            for name, finding in sorted(findings.items())
        }
        raise ValueError(f"npm audit contains vulnerability findings: {details!r}")
    return "npm audit policy: no findings"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, help="Read npm audit JSON from this path")
    args = parser.parse_args()
    try:
        report = load_json(args.input) if args.input is not None else run_audit()
        print(validate_policy(report))
    except (OSError, RuntimeError, ValueError, json.JSONDecodeError) as error:
        print(f"npm audit policy failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
