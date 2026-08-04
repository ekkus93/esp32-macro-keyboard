#!/usr/bin/env python3
"""Verify the reviewed v2 API route manifest and its C mirror."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent.parent
CONTRACT_PATH = ROOT / "contracts/v2/api/routes.json"
HEADER_PATH = ROOT / "firmware/components/app_contracts_v2/include/api_routes_v2.h"

EXPECTED_IDENTITIES = (
    ("setupGet", "GET", "/api/v1/setup"),
    ("setupPost", "POST", "/api/v1/setup"),
    ("login", "POST", "/api/v1/auth/login"),
    ("logout", "POST", "/api/v1/auth/logout"),
    ("session", "GET", "/api/v1/auth/session"),
    ("status", "GET", "/api/v1/status"),
    ("limits", "GET", "/api/v1/limits"),
    ("blobList", "GET", "/api/v1/blob"),
    ("blobCreate", "POST", "/api/v1/blob"),
    ("blobLoad", "GET", "/api/v1/blob/{blob_id}"),
    ("blobDelete", "DELETE", "/api/v1/blob/{blob_id}"),
    ("sendCreate", "POST", "/api/v1/send"),
    ("sendGet", "GET", "/api/v1/send"),
    ("sendCancel", "DELETE", "/api/v1/send"),
    ("settingsGet", "GET", "/api/v1/settings"),
    ("settingsPut", "PUT", "/api/v1/settings"),
    ("passwordChange", "POST", "/api/v1/settings/change-password"),
    ("restart", "POST", "/api/v1/device/restart"),
    ("resetSettings", "POST", "/api/v1/device/reset-settings"),
    ("factoryReset", "POST", "/api/v1/device/factory-reset"),
    ("diagnostics", "GET", "/api/v1/diagnostics"),
)
METHODS = {"GET", "POST", "PUT", "DELETE"}
AUTHENTICATION = {
    "none-unprovisioned-only",
    "none-provisioned-only",
    "session",
}
JSON_BODIES = {
    "setupRequest",
    "loginRequest",
    "sendRequest",
    "settingsUpdateRequest",
    "passwordChangeRequest",
    "resetSettingsRequest",
    "factoryResetRequest",
}
ALLOWED_STATUSES = {
    200,
    201,
    202,
    204,
    400,
    401,
    403,
    404,
    409,
    413,
    415,
    422,
    429,
    500,
    503,
    507,
}
ROW_PATTERN = re.compile(
    r'X\("([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*'
    r'"([^"]+)",\s*"([^"]*)",\s*"([^"]*)",\s*"([^"]*)",\s*'
    r'UINT16_C\((\d+)\),\s*"([^"]*)"\)'
)


def require_record(value: Any, context: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise ValueError(f"{context} must be an object")
    return value


def require_exact_keys(
    value: dict[str, Any], expected: tuple[str, ...], context: str
) -> None:
    if tuple(value) != expected:
        raise ValueError(
            f"{context} keys/order differ: expected {expected}, got {tuple(value)}"
        )


def validate_request(value: Any, context: str) -> tuple[str, str, str]:
    request = require_record(value, context)
    require_exact_keys(request, ("body", "contentType", "maximumBytes"), context)
    body = request["body"]
    content_type = request["contentType"]
    maximum = request["maximumBytes"]
    if body == "none":
        if content_type is not None or maximum is not None:
            raise ValueError(f"{context}: bodyless request must have null metadata")
    elif body == "binaryBlob":
        if content_type != "application/gzip" or maximum != "blobMaxBytes":
            raise ValueError(f"{context}: binary blob contract differs")
    elif body in JSON_BODIES:
        if content_type != "application/json" or maximum != "jsonBodyMaxBytes":
            raise ValueError(f"{context}: JSON request contract differs")
    else:
        raise ValueError(f"{context}: unsupported body {body!r}")
    return body, content_type or "", maximum or ""


def validate_response(value: Any, context: str) -> tuple[str, int]:
    response = require_record(value, context)
    require_exact_keys(response, ("contentType", "successStatus"), context)
    content_type = response["contentType"]
    status = response["successStatus"]
    if not isinstance(status, int) or isinstance(status, bool) or status not in ALLOWED_STATUSES:
        raise ValueError(f"{context}: invalid success status")
    if status == 204:
        if content_type is not None:
            raise ValueError(f"{context}: 204 response must not have content type")
    elif content_type not in {"application/json", "application/gzip"}:
        raise ValueError(f"{context}: invalid response content type")
    return content_type or "", status


def validate_errors(value: Any, success_status: int, context: str) -> tuple[int, ...]:
    if not isinstance(value, list) or not value:
        raise ValueError(f"{context} must be a nonempty array")
    if any(not isinstance(item, int) or isinstance(item, bool) for item in value):
        raise ValueError(f"{context} contains a noninteger")
    statuses = tuple(value)
    if statuses != tuple(sorted(set(statuses))):
        raise ValueError(f"{context} must be strictly increasing and unique")
    if success_status in statuses or any(item not in ALLOWED_STATUSES for item in statuses):
        raise ValueError(f"{context} contains an invalid status")
    return statuses


def load_expected_rows() -> list[tuple[str, ... | int]]:
    document: Any = json.loads(CONTRACT_PATH.read_text(encoding="utf-8"))
    root = require_record(document, "root")
    require_exact_keys(root, ("format", "version", "routes"), "root")
    if root["format"] != "esp32-macro-keyboard-api-routes" or root["version"] != 1:
        raise ValueError("unsupported route manifest identity")
    routes = root["routes"]
    if not isinstance(routes, list) or len(routes) != len(EXPECTED_IDENTITIES):
        raise ValueError("route count differs from reviewed manifest")

    rows: list[tuple[str, ... | int]] = []
    for index, (raw_route, identity) in enumerate(zip(routes, EXPECTED_IDENTITIES, strict=True)):
        context = f"routes[{index}]"
        route = require_record(raw_route, context)
        require_exact_keys(
            route,
            (
                "id",
                "method",
                "path",
                "authentication",
                "request",
                "response",
                "errorStatuses",
            ),
            context,
        )
        if (route["id"], route["method"], route["path"]) != identity:
            raise ValueError(f"{context}: identity or order differs")
        if route["method"] not in METHODS or route["authentication"] not in AUTHENTICATION:
            raise ValueError(f"{context}: method or authentication differs")
        if index < 2 and route["authentication"] != "none-unprovisioned-only":
            raise ValueError(f"{context}: setup authentication differs")
        if index == 2 and route["authentication"] != "none-provisioned-only":
            raise ValueError(f"{context}: login authentication differs")
        if index > 2 and route["authentication"] != "session":
            raise ValueError(f"{context}: authenticated route differs")
        if any(token in route["path"] for token in ("/package", "/macro", "/executions")):
            raise ValueError(f"{context}: prohibited v1 route appears")

        body, request_type, maximum = validate_request(route["request"], f"{context}.request")
        response_type, success_status = validate_response(
            route["response"], f"{context}.response"
        )
        errors = validate_errors(
            route["errorStatuses"], success_status, f"{context}.errorStatuses"
        )
        rows.append(
            (
                route["id"],
                route["method"],
                route["path"],
                route["authentication"],
                body,
                request_type,
                maximum,
                response_type,
                success_status,
                ",".join(str(item) for item in errors),
            )
        )
    return rows


def load_header_rows() -> list[tuple[str, ... | int]]:
    header = HEADER_PATH.read_text(encoding="utf-8")
    count_match = re.search(
        r"^#define\s+APP_V2_API_ROUTE_COUNT\s+UINT32_C\((\d+)\)\s*$",
        header,
        flags=re.MULTILINE,
    )
    if count_match is None:
        raise ValueError("C mirror is missing APP_V2_API_ROUTE_COUNT")
    declared_count = int(count_match.group(1))
    flattened = header.replace("\\\n", " ")
    rows: list[tuple[str, ... | int]] = []
    for match in ROW_PATTERN.finditer(flattened):
        rows.append((*match.groups()[:8], int(match.group(9)), match.group(10)))
    if declared_count != len(rows):
        raise ValueError(
            f"C mirror declared {declared_count} routes but contains {len(rows)} rows"
        )
    return rows


def main() -> int:
    try:
        expected = load_expected_rows()
        actual = load_header_rows()
        if actual != expected:
            for index, (expected_row, actual_row) in enumerate(
                zip(expected, actual, strict=False)
            ):
                if expected_row != actual_row:
                    raise ValueError(
                        f"C route row {index} differs:\n"
                        f"  expected {expected_row}\n  actual   {actual_row}"
                    )
            raise ValueError("C route mirror length differs")
    except (OSError, json.JSONDecodeError, ValueError) as error:
        print(f"v2 API route check failed: {error}", file=sys.stderr)
        return 1

    print(f"v2 API route manifest and C mirror match ({len(expected)} routes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
