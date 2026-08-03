#!/usr/bin/env python3
"""Verify the firmware setup-route policy mirrors the reviewed JSON contract."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

REPOSITORY_ROOT = Path(__file__).resolve().parent.parent
CONTRACT_PATH = REPOSITORY_ROOT / "contracts/v2/api/setup-route-policy.json"
HEADER_PATH = (
    REPOSITORY_ROOT
    / "firmware/components/app_contracts_v2/include/setup_route_policy_v2.h"
)

STRING_MACROS = {
    "APP_V2_SETUP_ROUTE_PATH": "/api/v1/setup",
    "APP_V2_SETUP_STATE_METHOD": "GET",
    "APP_V2_SETUP_SUBMIT_METHOD": "POST",
    "APP_V2_SETUP_AUTHENTICATION": "none",
    "APP_V2_SETUP_JSON_CONTENT_TYPE": "application/json",
    "APP_V2_SETUP_POST_BODY_LIMIT_NAME": "jsonBodyMaxBytes",
}
INTEGER_MACROS = {
    "APP_V2_SETUP_UNPROVISIONED_ROUTE_COUNT": 2,
    "APP_V2_SETUP_GET_UNPROVISIONED_STATUS": 200,
    "APP_V2_SETUP_POST_UNPROVISIONED_STATUS": 202,
    "APP_V2_SETUP_GET_PROVISIONED_STATUS": 404,
    "APP_V2_SETUP_POST_PROVISIONED_STATUS": 409,
    "APP_V2_SETUP_OTHER_API_ROUTES_UNAVAILABLE": 1,
}


def require_record(value: Any, context: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError(f"{context} must be an object")
    return value


def require_exact_keys(
    value: dict[str, Any], expected: set[str], context: str
) -> None:
    actual = set(value)
    if actual != expected:
        raise ValueError(
            f"{context} fields differ: expected {sorted(expected)}, got {sorted(actual)}"
        )


def validate_contract(document: Any) -> None:
    root = require_record(document, "root")
    require_exact_keys(root, {"format", "version", "unprovisioned", "provisioned"}, "root")
    if root["format"] != "esp32-macro-keyboard-setup-route-policy":
        raise ValueError("unsupported setup-route policy format")
    if root["version"] != 1:
        raise ValueError("unsupported setup-route policy version")

    unprovisioned = require_record(root["unprovisioned"], "unprovisioned")
    require_exact_keys(
        unprovisioned, {"apiRoutes", "otherApiRoutes"}, "unprovisioned"
    )
    routes = unprovisioned["apiRoutes"]
    if not isinstance(routes, list) or len(routes) != 2:
        raise ValueError("unprovisioned apiRoutes must contain exactly two routes")

    get_route = require_record(routes[0], "unprovisioned GET route")
    require_exact_keys(
        get_route,
        {
            "method",
            "path",
            "authentication",
            "requestBody",
            "successStatus",
            "responseContentType",
        },
        "unprovisioned GET route",
    )
    expected_get = {
        "method": "GET",
        "path": "/api/v1/setup",
        "authentication": "none",
        "requestBody": "none",
        "successStatus": 200,
        "responseContentType": "application/json",
    }
    if get_route != expected_get:
        raise ValueError("unprovisioned GET route differs from the approved contract")

    post_route = require_record(routes[1], "unprovisioned POST route")
    require_exact_keys(
        post_route,
        {
            "method",
            "path",
            "authentication",
            "requestContentType",
            "requestBodyLimit",
            "successStatus",
            "responseContentType",
        },
        "unprovisioned POST route",
    )
    expected_post = {
        "method": "POST",
        "path": "/api/v1/setup",
        "authentication": "none",
        "requestContentType": "application/json",
        "requestBodyLimit": "jsonBodyMaxBytes",
        "successStatus": 202,
        "responseContentType": "application/json",
    }
    if post_route != expected_post:
        raise ValueError("unprovisioned POST route differs from the approved contract")
    if unprovisioned["otherApiRoutes"] != "unavailable":
        raise ValueError("other API routes must be unavailable while unprovisioned")

    provisioned = require_record(root["provisioned"], "provisioned")
    require_exact_keys(
        provisioned, {"getSetupStatus", "postSetupStatus"}, "provisioned"
    )
    if provisioned != {"getSetupStatus": 404, "postSetupStatus": 409}:
        raise ValueError("provisioned setup statuses differ from the approved contract")


def parse_string_macro(header: str, name: str) -> str:
    match = re.search(
        rf"^#define\s+{re.escape(name)}\s+\"([^\"]*)\"\s*$",
        header,
        flags=re.MULTILINE,
    )
    if match is None:
        raise ValueError(f"missing string macro {name}")
    return match.group(1)


def parse_integer_macro(header: str, name: str) -> int:
    match = re.search(
        rf"^#define\s+{re.escape(name)}\s+UINT(?:8|16)_C\((\d+)\)\s*$",
        header,
        flags=re.MULTILINE,
    )
    if match is None:
        raise ValueError(f"missing integer macro {name}")
    return int(match.group(1))


def main() -> int:
    try:
        document = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
        validate_contract(document)
        header = HEADER_PATH.read_text(encoding="utf-8")

        for name, expected in STRING_MACROS.items():
            actual = parse_string_macro(header, name)
            if actual != expected:
                raise ValueError(f"{name}: expected {expected!r}, got {actual!r}")
        for name, expected in INTEGER_MACROS.items():
            actual = parse_integer_macro(header, name)
            if actual != expected:
                raise ValueError(f"{name}: expected {expected}, got {actual}")
    except (OSError, json.JSONDecodeError, ValueError) as error:
        print(f"v2 setup-route policy check failed: {error}", file=sys.stderr)
        return 1

    print("v2 setup-route policy contract is synchronized")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
