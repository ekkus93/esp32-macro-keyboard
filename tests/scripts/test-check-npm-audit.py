#!/usr/bin/env python3
"""Regression tests for the reviewed npm audit exception policy."""

from __future__ import annotations

import copy
import datetime as dt
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

NAMES = {
    "@eslint/config-array",
    "@eslint/eslintrc",
    "brace-expansion",
    "eslint",
    "minimatch",
}


def accepted_report() -> dict[str, Any]:
    vulnerabilities: dict[str, Any] = {}
    for name in sorted(NAMES):
        via: list[object] = ["minimatch"]
        if name == "brace-expansion":
            via = [{"source": 1124334, "severity": "high"}]
        vulnerabilities[name] = {
            "severity": "high",
            "nodes": [f"node_modules/{name}"],
            "via": via,
        }
    return {
        "metadata": {
            "vulnerabilities": {
                "info": 0,
                "low": 0,
                "moderate": 0,
                "high": 5,
                "critical": 0,
                "total": 5,
            }
        },
        "vulnerabilities": vulnerabilities,
    }


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


def accepted_lockfile() -> dict[str, Any]:
    return {
        "packages": {
            f"node_modules/{name}": {"dev": True} for name in sorted(NAMES)
        }
    }


def expect_failure(report: dict[str, Any], lockfile: dict[str, Any]) -> None:
    try:
        MODULE.validate_policy(report, lockfile, dt.date(2026, 7, 29))
    except ValueError:
        return
    raise AssertionError("policy unexpectedly accepted invalid audit data")


def main() -> None:
    message = MODULE.validate_policy(
        accepted_report(), accepted_lockfile(), dt.date(2026, 7, 29)
    )
    assert "accepted five dev-only" in message
    assert (
        MODULE.validate_policy(
            clean_report(), accepted_lockfile(), dt.date(2026, 7, 29)
        )
        == "npm audit policy: no findings"
    )

    critical = accepted_report()
    critical["vulnerabilities"]["critical-package"] = {
        "severity": "critical",
        "nodes": ["node_modules/critical-package"],
        "via": [{"source": 999998}],
    }
    critical["metadata"]["vulnerabilities"]["critical"] = 1
    critical["metadata"]["vulnerabilities"]["total"] = 6
    expect_failure(critical, accepted_lockfile())

    unexpected = accepted_report()
    unexpected["vulnerabilities"]["new-package"] = {
        "severity": "high",
        "nodes": ["node_modules/new-package"],
        "via": [{"source": 999999}],
    }
    unexpected["metadata"]["vulnerabilities"]["high"] = 6
    unexpected["metadata"]["vulnerabilities"]["total"] = 6
    expect_failure(unexpected, accepted_lockfile())

    runtime_lock = accepted_lockfile()
    runtime_lock["packages"]["node_modules/eslint"] = {"dev": False}
    expect_failure(accepted_report(), runtime_lock)

    wrong_source = accepted_report()
    wrong_source["vulnerabilities"]["brace-expansion"]["via"] = [
        {"source": 999999, "severity": "high"}
    ]
    expect_failure(wrong_source, accepted_lockfile())

    network_error = {"error": {"code": "ENETUNREACH", "summary": "offline"}}
    expect_failure(network_error, accepted_lockfile())

    missing_metadata = {"vulnerabilities": {}}
    expect_failure(missing_metadata, accepted_lockfile())

    count_mismatch = accepted_report()
    count_mismatch["metadata"]["vulnerabilities"]["high"] = 4
    expect_failure(count_mismatch, accepted_lockfile())

    malformed_finding = accepted_report()
    malformed_finding["vulnerabilities"]["eslint"] = "not-an-object"
    expect_failure(malformed_finding, accepted_lockfile())

    invalid_via = copy.deepcopy(accepted_report())
    invalid_via["vulnerabilities"]["eslint"]["via"] = [False]
    expect_failure(invalid_via, accepted_lockfile())

    try:
        MODULE.validate_policy(
            accepted_report(), accepted_lockfile(), dt.date(2026, 10, 1)
        )
    except ValueError as error:
        assert "expired" in str(error)
    else:
        raise AssertionError("expired exception was accepted")

    print("npm audit policy tests passed")


if __name__ == "__main__":
    main()
