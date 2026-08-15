#!/usr/bin/env python3
"""Verify HTTP responsiveness during a physical-confirmation wait.

The ESP-IDF HTTP server uses a single request task. Confirmation-gated routes
must therefore move their wait to the asynchronous worker; otherwise an
unconfirmed operation blocks every client for the entire confirmation timeout.

Phase 2 uses the retained device-restart route as the blocking operation. No
repository, package, restore, or stored-macro API is involved.
"""

import json
import sys
import threading
import time
import urllib.error
import urllib.request

import hil_state
from device_client import Device

RESPONSIVE_MS = 2000


def check(label, condition, detail=""):
    print(f"  [{'PASS' if condition else 'FAIL'}] {label}{'  ' + detail if detail else ''}")
    return condition


def response_data(payload):
    if not isinstance(payload, dict):
        raise SystemExit(f"error: expected JSON object, got {payload!r}")
    return payload.get("data", payload)


def main():
    ip = hil_state.device_ip()
    device = Device(ip)
    device.login()
    results = []

    def raw(method, path, body=None, timeout=60):
        data = json.dumps(body, separators=(",", ":")).encode() if body is not None else None
        headers = {"Cookie": device.cookie}
        if data is not None:
            headers["Content-Type"] = "application/json"
        request = urllib.request.Request(
            f"http://{ip}{path}", data=data, headers=headers, method=method
        )
        start = time.time()
        try:
            with urllib.request.urlopen(request, timeout=timeout) as response:
                response.read()
                return response.status, time.time() - start
        except urllib.error.HTTPError as error:
            error.read()
            return error.code, time.time() - start
        except Exception as error:  # noqa: BLE001 - the result is reported
            return type(error).__name__, time.time() - start

    status, payload = device.get("/api/v1/settings")
    if status != 200:
        raise SystemExit(f"error: could not read settings: HTTP {status} {payload}")
    original = response_data(payload)
    original_required = bool(original["requireSerialConfirmation"])
    status, payload = device.put(
        "/api/v1/settings",
        {"requireSerialConfirmation": True},
    )
    if status != 200:
        raise SystemExit(
            f"error: could not enable serial confirmation: HTTP {status} {payload}"
        )
    configured = response_data(payload)
    configured_settings = configured.get("settings") if isinstance(configured, dict) else None
    if not isinstance(configured_settings, dict) or configured_settings.get(
        "requireSerialConfirmation"
    ) is not True:
        raise SystemExit(f"error: confirmation setting did not become active: {payload}")

    try:
        print(f"device {ip}\n")
        print("baseline (server idle):")
        for _ in range(3):
            baseline_status, elapsed = raw("GET", "/api/v1/status")
            print(
                "    GET /api/v1/status -> "
                f"{baseline_status} in {elapsed * 1000:.0f} ms"
            )

        print("\nserver stays responsive during the confirmation wait:")
        pending = {}

        def hold_confirmation():
            pending["restart"] = raw("POST", "/api/v1/device/restart", timeout=60)

        worker = threading.Thread(target=hold_confirmation, daemon=True)
        worker.start()
        time.sleep(2.0)

        latencies = []
        for index in range(4):
            request_status, elapsed = raw("GET", "/api/v1/status", timeout=60)
            latencies.append(elapsed * 1000)
            print(
                "    GET /api/v1/status -> "
                f"{request_status} in {elapsed * 1000:7.0f} ms"
            )
            if index < 3:
                time.sleep(1.0)
        worst = max(latencies)
        results.append(
            check(
                "unrelated requests stay fast",
                worst < RESPONSIVE_MS,
                f"worst {worst:.0f} ms (limit {RESPONSIVE_MS} ms)",
            )
        )

        request_status, elapsed = raw("POST", "/api/v1/device/restart", timeout=10)
        results.append(
            check(
                "second confirmation refused with 409",
                request_status == 409,
                f"HTTP {request_status} in {elapsed * 1000:.0f} ms",
            )
        )

        worker.join(timeout=60)
        results.append(
            check(
                "held request completed after timeout",
                not worker.is_alive(),
                f"result {pending.get('restart')}",
            )
        )

        print("\nsockets are released after the timed-out confirmation:")
        request_status, elapsed = raw("GET", "/api/v1/status")
        results.append(
            check(
                "server still accepting connections",
                request_status == 200,
                f"HTTP {request_status} in {elapsed * 1000:.0f} ms",
            )
        )
    finally:
        cleanup_failures = []
        status, payload = device.put(
            "/api/v1/settings",
            {"requireSerialConfirmation": original_required},
        )
        if status != 200:
            cleanup_failures.append(
                "could not restore requireSerialConfirmation: "
                f"HTTP {status} {payload}"
            )
        try:
            device.logout()
        except BaseException as error:  # cleanup must not become a false PASS
            cleanup_failures.append(f"could not close test session: {error}")

    if cleanup_failures:
        raise SystemExit("error: concurrency acceptance cleanup failed: " + "; ".join(cleanup_failures))

    print("\n" + "=" * 58)
    print(f"  {sum(1 for result in results if result)}/{len(results)} checks passed")
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
