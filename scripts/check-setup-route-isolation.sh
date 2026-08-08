#!/usr/bin/env bash
set -euo pipefail

readonly lifecycle_file="${1:-firmware/components/web_server/web_server_lifecycle.c}"

python3 - "${lifecycle_file}" <<'PY2'
import re
import sys
from pathlib import Path

REQUIRED_SETUP_ROUTES = {
    ("/api/v1/setup", "HTTP_GET"),
    ("/api/v1/setup", "HTTP_POST"),
    ("/*", "HTTP_GET"),
}
ARRAY = re.compile(
    r"static\s+const\s+httpd_uri_t\s+(?P<name>normal_routes|setup_routes)\[\]\s*=\s*\{(?P<body>.*?)\n\};",
    re.DOTALL,
)
ENTRY = re.compile(
    r'\{\s*\.uri\s*=\s*"(?P<uri>[^"]+)"\s*,\s*\.method\s*=\s*(?P<method>HTTP_[A-Z]+)\s*,',
)


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


path = Path(sys.argv[1])
if not path.is_file():
    fail(f"route source not found: {path}")
text = path.read_text(encoding="utf-8")
tables = {
    match.group("name"): [
        (entry.group("uri"), entry.group("method"))
        for entry in ENTRY.finditer(match.group("body"))
    ]
    for match in ARRAY.finditer(text)
}
if set(tables) != {"normal_routes", "setup_routes"}:
    fail("normal_routes and setup_routes must each appear exactly once")

setup_routes = tables["setup_routes"]
if len(setup_routes) != len(set(setup_routes)):
    fail("setup route table contains a duplicate URI/method pair")
if set(setup_routes) != REQUIRED_SETUP_ROUTES:
    missing = sorted(REQUIRED_SETUP_ROUTES - set(setup_routes))
    extra = sorted(set(setup_routes) - REQUIRED_SETUP_ROUTES)
    fail(f"setup route table mismatch; missing={missing}, extra={extra}")

normal_routes = tables["normal_routes"]
if any(uri == "/api/v1/setup" or uri.startswith("/api/v1/setup/") for uri, _ in normal_routes):
    fail("setup route exposed in normal route table")
if normal_routes.count(("/*", "HTTP_GET")) != 1:
    fail("normal route table must contain exactly one static GET wildcard")
if setup_routes.count(("/*", "HTTP_GET")) != 1:
    fail("setup route table must contain exactly one static GET wildcard")

retired = (
    "/api/v1/setup-state",
    "/api/v1/setup/credentials",
    "/api/v1/setup/complete",
    "/api/v1/setup/restart",
)
if any(item in text for item in retired):
    fail("retired setup route remains in production route source")

print("V2 setup route isolation policy passed")
PY2
