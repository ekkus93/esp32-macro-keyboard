#!/usr/bin/env python3
"""Regression tests for the reviewed npm audit exception policy."""

from __future__ import annotations

import datetime as dt
import importlib.util
from pathlib import Path

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


def accepted_report() -> dict[str, object]:
    vulnerabilities: dict[str, object] = {}
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


def accepted_lockfile() -> dict[str, object]:
    return {
        "packages": {
            f"node_modules/{name}": {"dev": True} for name in sorted(NAMES)
        }
    }


def expect_failure(report: dict[str, object], lockfile: dict[str, object]) -> None:
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

    critical = accepted_report()
    critical["metadata"]["vulnerabilities"]["critical"] = 1  # type: ignore[index]
    expect_failure(critical, accepted_lockfile())

    unexpected = accepted_report()
    unexpected["vulnerabilities"]["new-package"] = {  # type: ignore[index]
        "severity": "high",
        "nodes": ["node_modules/new-package"],
        "via": [{"source": 999999}],
    }
    expect_failure(unexpected, accepted_lockfile())

    runtime_lock = accepted_lockfile()
    runtime_lock["packages"]["node_modules/eslint"] = {"dev": False}  # type: ignore[index]
    expect_failure(accepted_report(), runtime_lock)

    wrong_source = accepted_report()
    wrong_source["vulnerabilities"]["brace-expansion"]["via"] = [  # type: ignore[index]
        {"source": 999999, "severity": "high"}
    ]
    expect_failure(wrong_source, accepted_lockfile())

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
