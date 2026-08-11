#!/usr/bin/env python3
"""Fail closed if direct HTTP registrations and wildcard API dispatch drift."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_LIFECYCLE = ROOT / "firmware/components/web_server/web_server_lifecycle.c"
DEFAULT_ADMIN = ROOT / "firmware/components/web_server/web_api_administration.c"
DEFAULT_CORE_HEADER = ROOT / "firmware/components/web_server/web_api_core.h"

EXPECTED_DIRECT_ROUTES = {
    ("/api/v1/status", "HTTP_GET", "status_handler", "WEB_API_ROUTE_STATUS"),
    ("/api/v1/limits", "HTTP_GET", "limits_handler", "WEB_API_ROUTE_LIMITS"),
    (
        "/api/v1/auth/login",
        "HTTP_POST",
        "login_handler",
        "WEB_API_ROUTE_AUTH_LOGIN",
    ),
    (
        "/api/v1/auth/logout",
        "HTTP_POST",
        "logout_handler",
        "WEB_API_ROUTE_AUTH_LOGOUT",
    ),
    ("/api/v1/blob", "HTTP_GET", "blob_list_handler", "WEB_API_ROUTE_BLOB_COLLECTION"),
    (
        "/api/v1/blob",
        "HTTP_POST",
        "blob_create_handler",
        "WEB_API_ROUTE_BLOB_COLLECTION",
    ),
    ("/api/v1/blob/*", "HTTP_GET", "blob_load_handler", "WEB_API_ROUTE_BLOB_ITEM"),
    (
        "/api/v1/blob/*",
        "HTTP_DELETE",
        "blob_delete_handler",
        "WEB_API_ROUTE_BLOB_ITEM",
    ),
    ("/api/v1/send", "HTTP_POST", "send_create_handler", "WEB_API_ROUTE_SEND"),
    ("/api/v1/send", "HTTP_GET", "send_get_handler", "WEB_API_ROUTE_SEND"),
    ("/api/v1/send", "HTTP_DELETE", "send_cancel_handler", "WEB_API_ROUTE_SEND"),
}
EXPECTED_WILDCARD_ROUTES = {
    ("/api/v1/*", method, "api_handler")
    for method in ("HTTP_GET", "HTTP_POST", "HTTP_PUT", "HTTP_DELETE")
}
EXPECTED_STATIC_ROUTE = ("/*", "HTTP_GET", "static_handler")

ARRAY = re.compile(
    r"static\s+const\s+httpd_uri_t\s+normal_routes\[\]\s*=\s*\{(?P<body>.*?)\n\};",
    re.DOTALL,
)
ENTRY = re.compile(
    r'\{\s*\.uri\s*=\s*"(?P<uri>[^"]+)"\s*,\s*'
    r"\.method\s*=\s*(?P<method>HTTP_[A-Z]+)\s*,\s*"
    r"\.handler\s*=\s*(?P<handler>[A-Za-z0-9_]+)\s*\}",
)
ENUM = re.compile(
    r"typedef\s+enum\s*\{(?P<body>.*?)\}\s*web_api_route_t\s*;",
    re.DOTALL,
)
ROUTE_TOKEN = re.compile(r"\bWEB_API_ROUTE_[A-Z0-9_]+\b")
ADMIN_FUNCTION = "app_error_code_t web_api_handle_administration"


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def function_body(text: str, marker: str) -> str:
    start = text.find(marker)
    if start < 0:
        fail(f"function not found: {marker}")
    brace = text.find("{", start)
    if brace < 0:
        fail(f"function body not found: {marker}")
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1 : index]
    fail(f"unterminated function body: {marker}")


def main() -> None:
    lifecycle_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_LIFECYCLE
    admin_path = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_ADMIN
    header_path = Path(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_CORE_HEADER
    for path in (lifecycle_path, admin_path, header_path):
        if not path.is_file():
            fail(f"source not found: {path}")

    lifecycle = lifecycle_path.read_text(encoding="utf-8")
    match = ARRAY.search(lifecycle)
    if match is None or ARRAY.search(lifecycle, match.end()) is not None:
        fail("normal_routes must appear exactly once")
    entries = [
        (entry.group("uri"), entry.group("method"), entry.group("handler"))
        for entry in ENTRY.finditer(match.group("body"))
    ]
    if len(entries) != len(set(entries)):
        fail("normal_routes contains a duplicate URI/method/handler entry")

    direct_by_registration = {
        entry for entry in entries if entry[2] not in {"api_handler", "static_handler"}
    }
    expected_registrations = {item[:3] for item in EXPECTED_DIRECT_ROUTES}
    if direct_by_registration != expected_registrations:
        missing = sorted(expected_registrations - direct_by_registration)
        extra = sorted(direct_by_registration - expected_registrations)
        fail(f"dedicated route table mismatch; missing={missing}, extra={extra}")

    wildcard = {entry for entry in entries if entry[2] == "api_handler"}
    if wildcard != EXPECTED_WILDCARD_ROUTES:
        missing = sorted(EXPECTED_WILDCARD_ROUTES - wildcard)
        extra = sorted(wildcard - EXPECTED_WILDCARD_ROUTES)
        fail(f"API wildcard route mismatch; missing={missing}, extra={extra}")
    if entries.count(EXPECTED_STATIC_ROUTE) != 1:
        fail("normal_routes must contain exactly one static GET fallback")

    admin = admin_path.read_text(encoding="utf-8")
    admin_cases = set(ROUTE_TOKEN.findall(function_body(admin, ADMIN_FUNCTION)))
    direct_route_enums = {item[3] for item in EXPECTED_DIRECT_ROUTES}
    overlap = sorted(direct_route_enums & admin_cases)
    if overlap:
        fail(f"dedicated routes also handled by wildcard administration dispatch: {overlap}")

    header = header_path.read_text(encoding="utf-8")
    enum_match = ENUM.search(header)
    if enum_match is None:
        fail("web_api_route_t enum not found")
    all_routes = set(ROUTE_TOKEN.findall(enum_match.group("body")))
    unknown = "WEB_API_ROUTE_UNKNOWN"
    if unknown not in all_routes or unknown not in admin_cases:
        fail("WEB_API_ROUTE_UNKNOWN must exist and be handled by administration default path")
    classified = direct_route_enums | (admin_cases - {unknown})
    expected_classified = all_routes - {unknown}
    if classified != expected_classified:
        missing = sorted(expected_classified - classified)
        extra = sorted(classified - expected_classified)
        fail(f"API route dispatch classification mismatch; missing={missing}, extra={extra}")

    print("web route/dispatch synchronization policy passed")


if __name__ == "__main__":
    main()
