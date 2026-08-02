#!/usr/bin/env python3
"""Exercise the package paths that carry the largest stack frames.

NOTE: POST /api/v1/restore hard-requires physical confirmation
(web_api_core.c: WEB_API_ROUTE_RESTORE), independently of the
requirePhysicalConfirmation setting. The device blocks up to
APP_PHYSICAL_CONFIRM_TIMEOUT_MS (20 s) waiting for the BOOT button, so the
client timeout here must exceed that or a timeout will be misread as a crash.

That gate is enforced in web_request_policy_evaluate() BEFORE handler
dispatch, so a 403 means restore_locked never ran. A 403 therefore cannot
count as having exercised the restore stack chain - it only proves the
confirmation gate works. Scoring it as a pass would report coverage this test
did not obtain.

These four routes were never executed on hardware. They contain the biggest
remaining frames (restore_locked ~20 KB, import_locked ~19 KB,
storage_package_replace_set ~19 KB, snapshot_load_progress ~15 KB) against a
24 KiB httpd task stack, so whether their whole call chains fit is exactly
what this checks. A chain that does not fit trips the FreeRTOS stack canary
and resets the device, which shows up here as the request never returning.
"""

import json
import sys
import time
import urllib.error
import urllib.request

import hil_state
from device_client import Device


def raw_get(ip, path, cookie):
    """Backup returns a raw package, not the JSON envelope."""
    request = urllib.request.Request(
        f"http://{ip}{path}",
        headers={"Host": ip, "Origin": f"http://{ip}", "Cookie": cookie},
        method="GET",
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return response.status, response.read().decode()
    except urllib.error.HTTPError as error:
        return error.code, error.read().decode()


def alive(device):
    """A reset device loses station mode, so failure here means it rebooted."""
    try:
        status, payload = device.get("/api/v1/status")
        return status == 200
    except Exception:
        return False


def main():
    ip = hil_state.device_ip()
    device = Device(ip)
    device.login()
    results = {}

    print("baseline: device responsive:", alive(device))
    sets_before = device.get("/api/v1/sets")[1]["data"]
    print(f"sets before: {len(sets_before)}")

    # --- GET /api/v1/backup : snapshot_load_locked + snapshot_load_progress ---
    print("\n--- GET /api/v1/backup ---")
    start = time.time()
    try:
        status, body = raw_get(ip, "/api/v1/backup", device.cookie)
        elapsed = time.time() - start
        ok = status == 200
        print(f"    HTTP {status} in {elapsed:.2f}s, {len(body)} bytes")
        if ok:
            package = json.loads(body)
            print(f"    package_type={package.get('package_type')} "
                  f"sets={len(package.get('sets', []))} "
                  f"macros={len(package.get('macros', []))} "
                  f"procedures={len(package.get('procedures', []))}")
        results["backup export"] = ok and alive(device)
    except Exception as error:
        print(f"    {type(error).__name__}: {error}")
        print("    -> request never returned; the device likely reset")
        results["backup export"] = False
        body = None
    print(f"    device still alive: {alive(device)}")

    # --- POST /api/v1/restore : restore_locked, the largest frame -----------
    if results.get("backup export") and body:
        print("\n--- POST /api/v1/restore (round-trip of that same backup) ---")
        # Each attempt only opens a 20 s window, which is easy to miss. Retry so
        # a press anywhere in ~60 s counts, rather than failing on timing alone.
        attempts = 3
        results["restore"] = False
        for attempt in range(1, attempts + 1):
            print(f"\n    attempt {attempt}/{attempts}")
            print("    >>> PRESS AND RELEASE THE BOOT BUTTON NOW - 20 s window <<<")
            start = time.time()
            try:
                status, payload = device.post("/api/v1/restore", json.loads(body))
                elapsed = time.time() - start
                print(f"    HTTP {status} in {elapsed:.2f}s: {str(payload)[:160]}")
                survived = alive(device)
                print(f"    device still alive: {survived}")
                if status == 403:
                    # Gate rejected before dispatch: restore_locked never ran, so
                    # this attempt obtained no evidence about the restore chain.
                    print("    -> no press seen; restore_locked NEVER RAN.")
                    continue
                results["restore"] = status in (200, 202) and survived
                break
            except Exception as error:
                print(f"    {type(error).__name__}: {error}")
                print("    -> request never returned; the device likely reset")
                break
        if not results["restore"]:
            print("\n    restore chain still unexercised: no BOOT press was detected.")
    else:
        print("\n--- POST /api/v1/restore skipped: no backup to restore ---")
        results["restore"] = False

    # --- data must survive unharmed ----------------------------------------
    print("\n--- repository intact afterwards ---")
    try:
        sets_after = device.get("/api/v1/sets")[1]["data"]
        print(f"    sets after: {len(sets_after)} (was {len(sets_before)})")
        results["repository intact"] = len(sets_after) == len(sets_before)
    except Exception as error:
        print(f"    {type(error).__name__}")
        results["repository intact"] = False

    print("\n" + "=" * 58)
    for name, ok in results.items():
        print(f"  {name:<20} {'PASS' if ok else 'FAIL'}")
    device.logout()
    return 0 if all(results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
