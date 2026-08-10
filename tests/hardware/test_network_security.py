#!/usr/bin/env python3
"""Network-security and retired-route checks against real firmware.

The USB-UART console is deliberately trusted because physical possession of the
board implies trust. Network requests must present a valid HttpOnly,
SameSite=Strict session cookie, and repeated bad logins must be throttled.
Phase 2 also requires the removed package and execution routes to stay absent.
"""

import json
import sys
import urllib.error
import urllib.request

import hil_state
from device_client import Device


def raw_request(ip, method, path, headers=None, body=None):
    """Issue a request with full control over headers, bypassing Device."""
    data = json.dumps(body, separators=(",", ":")).encode() if body is not None else None
    merged = {}
    if data is not None:
        merged["Content-Type"] = "application/json"
    merged.update(headers or {})
    merged = {key: value for key, value in merged.items() if value is not None}
    request = urllib.request.Request(
        f"http://{ip}{path}", data=data, headers=merged, method=method
    )
    try:
        with urllib.request.urlopen(request, timeout=15) as response:
            return response.status, response.read().decode()
    except urllib.error.HTTPError as error:
        return error.code, error.read().decode()


def check(label, condition, detail=""):
    print(f"  [{'PASS' if condition else 'FAIL'}] {label}{'  ' + detail if detail else ''}")
    return condition


def main():
    ip = hil_state.device_ip()
    results = []
    print(f"device {ip}\n")

    print("unauthenticated active routes are refused:")
    for path in ("/api/v1/settings", "/api/v1/diagnostics"):
        status, _ = raw_request(ip, "GET", path)
        results.append(check(f"GET {path}", status in (401, 403), f"HTTP {status}"))

    print("\nauthenticated session works:")
    device = Device(ip)
    device.login()
    status, _ = device.get("/api/v1/settings")
    results.append(check("GET /api/v1/settings with session", status == 200, f"HTTP {status}"))

    print("\nretired Phase 2 routes stay absent:")
    for method, path in (
        ("GET", "/api/v1/package"),
        ("GET", "/api/v1/executions/current"),
        ("POST", "/api/v1/executions/current/cancel"),
        ("GET", "/api/v1/repository"),
        ("POST", "/api/v1/restore"),
    ):
        status, _ = raw_request(ip, method, path, headers={"Cookie": device.cookie})
        results.append(check(f"{method} {path}", status == 404, f"HTTP {status}"))

    print("\nmutations require the session cookie:")
    invalid_settings = {
        "expectedRevision": 0,
        "requirePhysicalConfirmation": False,
        "alwaysSelectPackage": True,
    }
    status, _ = raw_request(
        ip,
        "PUT",
        "/api/v1/settings",
        headers={"Cookie": device.cookie},
        body=invalid_settings,
    )
    results.append(
        check("authenticated invalid mutation reaches validation", status == 422, f"HTTP {status}")
    )
    status, _ = raw_request(ip, "PUT", "/api/v1/settings", body=invalid_settings)
    results.append(
        check("mutation with no cookie is refused", status in (401, 403), f"HTTP {status}")
    )

    print("\nforged sessions are refused:")
    status, _ = raw_request(
        ip,
        "GET",
        "/api/v1/settings",
        headers={"Cookie": "MKSESSION=" + "a" * 64},
    )
    results.append(check("forged session cookie", status in (401, 403), f"HTTP {status}"))

    print("\nrepeated bad logins are throttled:")
    throttled_at = None
    for attempt in range(1, 12):
        status, _ = raw_request(
            ip,
            "POST",
            "/api/v1/auth/login",
            body={"adminPassword": f"wrong-password-{attempt}"},
        )
        if status == 429:
            throttled_at = attempt
            break
    results.append(
        check(
            "bad logins eventually rate-limited",
            throttled_at is not None,
            f"HTTP 429 after {throttled_at} attempts"
            if throttled_at
            else "never throttled in 11 attempts",
        )
    )

    print("\n" + "=" * 58)
    passed = sum(1 for result in results if result)
    print(f"  {passed}/{len(results)} checks passed")
    device.logout()
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
