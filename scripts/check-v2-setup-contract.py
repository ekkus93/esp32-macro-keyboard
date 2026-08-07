#!/usr/bin/env python3
"""Fail closed if the reviewed V2 setup examples drift or regain retired fields."""

from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent.parent
EXAMPLES = ROOT / "contracts/v2/api/examples.json"


def require_object(value: Any, context: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError(f"{context} must be an object")
    return value


def require_exact_keys(value: dict[str, Any], expected: set[str], context: str) -> None:
    actual = set(value)
    if actual != expected:
        raise ValueError(
            f"{context} fields differ: expected {sorted(expected)}, got {sorted(actual)}"
        )


def validate(document: Any) -> None:
    root = require_object(document, "examples root")
    state = require_object(root.get("setupState"), "setupState")
    request = require_object(root.get("setupRequest"), "setupRequest")
    accepted = require_object(root.get("setupAccepted"), "setupAccepted")

    require_exact_keys(state, {"provisioned", "deviceName"}, "setupState")
    if state["provisioned"] is not False or not isinstance(state["deviceName"], str):
        raise ValueError("setupState must contain provisioned=false and a string deviceName")

    required_request = {
        "setupCode",
        "deviceName",
        "apSsid",
        "apPassphrase",
        "adminPassword",
        "requireSerialConfirmation",
    }
    require_exact_keys(request, required_request, "setupRequest")
    code = request["setupCode"]
    if not isinstance(code, str) or len(code) != 8 or not code.isascii() or not code.isdigit():
        raise ValueError("setupRequest.setupCode must be exactly eight ASCII decimal digits")
    for name in ("deviceName", "apSsid", "apPassphrase", "adminPassword"):
        if not isinstance(request[name], str):
            raise ValueError(f"setupRequest.{name} must be a string")
    if not isinstance(request["requireSerialConfirmation"], bool):
        raise ValueError("setupRequest.requireSerialConfirmation must be boolean")

    retired = {"administratorPassword", "requirePhysicalConfirmation", "alwaysSelectPackage"}
    leaked = sorted(retired.intersection(request))
    if leaked:
        raise ValueError(f"retired setup fields reintroduced: {leaked}")

    require_exact_keys(
        accepted,
        {"accepted", "restartRequired", "connectionWillClose", "reprovisioningRequired"},
        "setupAccepted",
    )
    expected_accepted = {
        "accepted": True,
        "restartRequired": True,
        "connectionWillClose": True,
        "reprovisioningRequired": False,
    }
    if accepted != expected_accepted:
        raise ValueError("setupAccepted differs from the exact V2 response")


def main() -> int:
    try:
        validate(json.loads(EXAMPLES.read_text(encoding="utf-8")))
    except (OSError, json.JSONDecodeError, ValueError) as error:
        print(f"v2 setup contract example check failed: {error}", file=sys.stderr)
        return 1
    print("v2 setup contract examples are exact")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
