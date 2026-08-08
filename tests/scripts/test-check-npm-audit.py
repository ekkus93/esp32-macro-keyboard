#!/usr/bin/env python3
"""Regression tests for the strict-zero npm audit policy."""

from __future__ import annotations

import importlib.util
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
MODULE_PATH = REPO_ROOT / "scripts" / "check-npm-audit.py"
SPEC = importlib.util.spec_from_file_location("check_npm_audit", MODULE_PATH)
if SPEC is None or SPEC.loader is None:
    raise SystemExit("could not load check-npm-audit.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def clean_report() -> dict[str, Any]:
    return {
        "metadata": {
            "vulnerabilities": {
                "info": 0,
                "low": 0,
                "moderate": 0,
                "high": 0,
                "critical": 0,
                "total": 0,
            }
        },
        "vulnerabilities": {},
    }


def report_with_finding(severity: str) -> dict[str, Any]:
    report = clean_report()
    report["metadata"]["vulnerabilities"][severity] = 1
    report["metadata"]["vulnerabilities"]["total"] = 1
    report["vulnerabilities"]["example-package"] = {
        "severity": severity,
        "nodes": ["node_modules/example-package"],
        "range": "<1.0.0",
        "fixAvailable": True,
        "via": [{"source": 999999}],
    }
    return report


def expect_failure(report: dict[str, Any]) -> None:
    try:
        MODULE.validate_policy(report)
    except ValueError:
        return
    raise AssertionError("policy unexpectedly accepted invalid audit data")


def main() -> None:
    assert MODULE.validate_policy(clean_report()) == "npm audit policy: no findings"
    for severity in ("info", "low", "moderate", "high", "critical"):
        expect_failure(report_with_finding(severity))

    network_error = {"error": {"code": "ENETUNREACH", "summary": "offline"}}
    expect_failure(network_error)
    expect_failure({"vulnerabilities": {}})

    count_mismatch = clean_report()
    count_mismatch["metadata"]["vulnerabilities"]["high"] = 1
    expect_failure(count_mismatch)

    total_mismatch = clean_report()
    total_mismatch["metadata"]["vulnerabilities"]["total"] = 1
    expect_failure(total_mismatch)

    malformed_finding = clean_report()
    malformed_finding["vulnerabilities"]["example-package"] = "not-an-object"
    malformed_finding["metadata"]["vulnerabilities"]["total"] = 1
    expect_failure(malformed_finding)

    invalid_severity = clean_report()
    invalid_severity["vulnerabilities"]["example-package"] = {
        "severity": "severe"
    }
    invalid_severity["metadata"]["vulnerabilities"]["total"] = 1
    expect_failure(invalid_severity)

    print("npm audit policy tests passed")


if __name__ == "__main__":
    main()
