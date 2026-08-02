#!/usr/bin/env python3
"""The web server must stay responsive during a physical-confirmation wait.

esp_http_server is a single task running select() over every socket - there is
no worker pool - and physical confirmation blocks for up to
APP_PHYSICAL_CONFIRM_TIMEOUT_MS (20 s) waiting for the button. Performing that
wait on the httpd task froze every other client for the whole window: measured
on hardware, an unrelated GET /api/v1/status took 18.0 s against a 25-43 ms idle
baseline. From a browser that is indistinguishable from a hang, and it applies
to seven routes including POST /api/v1/executions whenever
requirePhysicalConfirmation is enabled - the ordinary "run a macro" path.

web_server_async.c moves those requests onto a worker task via
httpd_req_async_handler_begin(). These tests assert the properties that fix
depends on:

  1. unrelated requests stay fast while a confirmation is pending;
  2. a second confirmation is refused (409) rather than queued, because one
     button press cannot disambiguate two pending confirmations;
  3. sockets are not leaked - an async request left incomplete never releases
     its socket, and the server eventually stops accepting connections.
"""

import json
import sys
import threading
import time
import urllib.error
import urllib.request

import hil_state
from device_client import Device

# A request served while the worker waits should be in the tens of
# milliseconds; anything near the 20 s window means it queued behind the wait.
RESPONSIVE_MS = 2000


def check(label, condition, detail=""):
    print(f"  [{'PASS' if condition else 'FAIL'}] {label}{'  ' + detail if detail else ''}")
    return condition


def main():
    ip = hil_state.device_ip()
    device = Device(ip)
    device.login()
    results = []

    def raw(method, path, body=None, timeout=60):
        data = json.dumps(body).encode() if body is not None else None
        headers = {"Host": ip, "Origin": f"http://{ip}", "Cookie": device.cookie}
        if data is not None:
            headers["Content-Type"] = "application/json"
            headers["X-CSRF-Token"] = device.csrf
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
        except Exception as error:  # noqa: BLE001 - reported, not handled
            return type(error).__name__, time.time() - start

    print(f"device {ip}\n")
    print("baseline (server idle):")
    for _ in range(3):
        status, elapsed = raw("GET", "/api/v1/status")
        print(f"    GET /api/v1/status -> {status} in {elapsed * 1000:.0f} ms")

    request = urllib.request.Request(
        f"http://{ip}/api/v1/backup",
        headers={"Host": ip, "Origin": f"http://{ip}", "Cookie": device.cookie},
        method="GET",
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        backup = json.loads(response.read().decode())

    # --- 1. responsiveness while a confirmation is pending ------------------
    print("\nserver stays responsive during the confirmation wait:")
    pending = {}

    def hold_confirmation():
        pending["restore"] = raw("POST", "/api/v1/restore", backup, timeout=60)

    worker = threading.Thread(target=hold_confirmation, daemon=True)
    worker.start()
    time.sleep(2.0)  # let the restore reach the confirmation wait

    latencies = []
    for index in range(4):
        status, elapsed = raw("GET", "/api/v1/status", timeout=60)
        latencies.append(elapsed * 1000)
        print(f"    GET /api/v1/status -> {status} in {elapsed * 1000:7.0f} ms")
        if index < 3:
            time.sleep(1.0)
    worst = max(latencies)
    results.append(check("unrelated requests stay fast", worst < RESPONSIVE_MS,
                         f"worst {worst:.0f} ms (limit {RESPONSIVE_MS} ms)"))

    # --- 2. a second confirmation is refused, not queued --------------------
    status, elapsed = raw("POST", "/api/v1/restore", backup, timeout=60)
    results.append(check("second confirmation refused with 409", status == 409,
                         f"HTTP {status} in {elapsed * 1000:.0f} ms"))

    worker.join(timeout=60)
    print(f"    held request returned: {pending.get('restore')}")

    # --- 3. sockets are released ------------------------------------------
    print("\nsockets are released across repeated confirmation cycles:")
    for cycle in range(2):
        status, elapsed = raw("POST", "/api/v1/restore", backup, timeout=60)
        print(f"    cycle {cycle + 1}: restore -> {status} in {elapsed:.1f}s")
    status, elapsed = raw("GET", "/api/v1/status")
    results.append(check("server still accepting connections", status == 200,
                         f"HTTP {status} in {elapsed * 1000:.0f} ms"))

    print("\n" + "=" * 58)
    print(f"  {sum(1 for r in results if r)}/{len(results)} checks passed")
    device.logout()
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
