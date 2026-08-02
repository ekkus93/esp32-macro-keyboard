#!/usr/bin/env python3
"""FIX1 20.3 network-security checks against real firmware.

The USB-UART console is deliberately trusted (physical access implies trust).
The network surface is not: anything arriving over Wi-Fi must present a valid
session cookie, and repeated bad logins must be throttled. These tests assert
that boundary on the device.

The CSRF token and the Host/Origin pair were removed on 2026-08-02 (SPEC 16.2):
the session cookie is HttpOnly and SameSite=Strict, which is what stops another
site driving the device through a browser. The checks for them are gone from
here too, because a test that asserts a removed policy fails for the right
reason and the wrong cause.
"""

import json
import sys
import urllib.error
import urllib.request

import hil_state
from device_client import Device


def raw_request(ip, method, path, headers=None, body=None):
    """Issue a request with full control over headers (bypassing Device)."""
    data = json.dumps(body).encode() if body is not None else None
    merged = {}
    if data is not None:
        merged["Content-Type"] = "application/json"
    merged.update(headers or {})
    # a header explicitly set to None means "omit it"
    merged = {k: v for k, v in merged.items() if v is not None}
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

    # --- unauthenticated access -------------------------------------------
    print("unauthenticated access is refused:")
    for path in ("/api/v1/sets", "/api/v1/settings", "/api/v1/diagnostics"):
        status, _ = raw_request(ip, "GET", path)
        results.append(check(f"GET {path}", status in (401, 403), f"HTTP {status}"))

    status, _ = raw_request(ip, "POST", "/api/v1/executions",
                            body={"setId": "x", "macroId": "y", "macroRevision": 1})
    results.append(check("POST /api/v1/executions", status in (400, 401, 403), f"HTTP {status}"))

    # --- authenticated baseline -------------------------------------------
    print("\nauthenticated session works:")
    device = Device(ip)
    device.login()
    status, _ = device.get("/api/v1/sets")
    results.append(check("GET /api/v1/sets with session", status == 200, f"HTTP {status}"))

    # --- the cookie is the whole credential (SPEC 16.2) ----------------------
    print("\nmutations need the session cookie and nothing else:")
    status, _ = raw_request(ip, "POST", "/api/v1/executions/current/cancel",
                            headers={"Cookie": device.cookie})
    results.append(check("mutation with only the session cookie",
                         status not in (401, 403), f"HTTP {status}"))
    status, _ = raw_request(ip, "POST", "/api/v1/executions/current/cancel")
    results.append(check("mutation with no cookie is refused",
                         status in (401, 403), f"HTTP {status}"))

    # --- session validity ---------------------------------------------------
    print("\nforged/expired sessions are refused:")
    status, _ = raw_request(ip, "GET", "/api/v1/sets",
                            headers={"Cookie": "session=" + "a" * 64})
    results.append(check("forged session cookie", status in (401, 403), f"HTTP {status}"))

    # --- login throttling ---------------------------------------------------
    print("\nrepeated bad logins are throttled:")
    throttled_at = None
    for attempt in range(1, 12):
        status, _ = raw_request(ip, "POST", "/api/v1/auth/login",
                                body={"password": f"wrong-password-{attempt}"})
        if status == 429:
            throttled_at = attempt
            break
    results.append(check("bad logins eventually rate-limited",
                         throttled_at is not None,
                         f"HTTP 429 after {throttled_at} attempts" if throttled_at
                         else "never throttled in 11 attempts"))

    print("\n" + "=" * 58)
    passed = sum(1 for r in results if r)
    print(f"  {passed}/{len(results)} checks passed")
    device.logout()
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
