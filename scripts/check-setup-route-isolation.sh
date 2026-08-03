#!/usr/bin/env bash
set -euo pipefail

readonly lifecycle_file="${1:-firmware/components/web_server/web_server_lifecycle.c}"

python3 - "${lifecycle_file}" <<'PY2'
import re
import sys
from pathlib import Path

REQUIRED_SETUP_ROUTES = {
    "/api/v1/setup-state",
    "/api/v1/setup/credentials",
    "/api/v1/setup/complete",
    "/api/v1/setup/restart",
    "/*",
}
FORBIDDEN_SETUP_PREFIXES = (
    "/api/v1/auth",
    "/api/v1/executions",
    "/api/v1/status",
    "/api/v1/limits",
    "/api/v1/package",
    "/api/v1/macros",
    "/api/v1/procedures",
    "/api/v1/settings",
    "/api/v1/admin",
)
ARRAY = re.compile(
    r"static\s+const\s+httpd_uri_t\s+(?P<name>normal_routes|setup_routes)\[\]\s*=\s*\{(?P<body>.*?)\n\};",
    re.DOTALL,
)
URI = re.compile(r"\.uri\s*=\s*\"([^\"]+)\"")


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


path = Path(sys.argv[1])
if not path.is_file():
    fail(f"route source not found: {path}")
text = path.read_text(encoding="utf-8")
tables = {
    match.group("name"): URI.findall(match.group("body"))
    for match in ARRAY.finditer(text)
}
if set(tables) != {"normal_routes", "setup_routes"}:
    fail("normal_routes and setup_routes must each appear exactly once")

setup_routes = tables["setup_routes"]
if len(setup_routes) != len(set(setup_routes)):
    fail("setup route table contains a duplicate URI")
if set(setup_routes) != REQUIRED_SETUP_ROUTES:
    missing = sorted(REQUIRED_SETUP_ROUTES - set(setup_routes))
    extra = sorted(set(setup_routes) - REQUIRED_SETUP_ROUTES)
    fail(f"setup route table mismatch; missing={missing}, extra={extra}")
for route in setup_routes:
    if any(route.startswith(prefix) for prefix in FORBIDDEN_SETUP_PREFIXES):
        fail(f"normal-operation route exposed during setup: {route}")

normal_routes = tables["normal_routes"]
if any(route.startswith("/api/v1/setup") for route in normal_routes):
    fail("setup route exposed in normal route table")
if normal_routes.count("/*") != 1 or setup_routes.count("/*") != 1:
    fail("each route table must contain exactly one static wildcard route")

print("setup route isolation policy passed")
PY2
