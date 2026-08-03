#!/usr/bin/env python3
"""Back up the whole repository and restore it, on real hardware.

    SPEC 24.2 item: partial restore reporting per-package outcomes

This is the last open item of the 2026-08-02 alignment plan (5.4) and
acceptance criterion 4: restore completes on real hardware without a watchdog
reset and reports per-package outcomes.

It is a hardware test because the thing that can go wrong is not logic. These
two routes carry the largest stack frames in the firmware -- restore_locked
around 20 KB against a 24 KiB httpd task stack -- and whether the whole call
chain fits is a property of the real build on the real chip. A chain that does
not fit trips the FreeRTOS stack canary and resets the device, which shows up
here as a request that never returns.

Physical confirmation: restore is confirmation-gated only when
requirePhysicalConfirmation is on (web_api_physical_confirmation_required).
Provisioning for these runs leaves it off, so no confirmation is needed. This
file used to instruct the operator to press a BOOT button, which this device
does not have (section 19) and has not needed since that setting was honoured.

Usage:
    python3 tests/hardware/test_backup_restore.py
"""

import json
import sys
import uuid
import time
import urllib.error
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import hil_state  # noqa: E402
from device_client import Device  # noqa: E402


def raw_get(ip, path, cookie):
    """Backup returns a raw package, not the JSON envelope."""
    request = urllib.request.Request(
        f"http://{ip}{path}", headers={"Cookie": cookie}, method="GET"
    )
    try:
        with urllib.request.urlopen(request, timeout=60) as response:  # noqa: S310
            return response.status, response.read().decode()
    except urllib.error.HTTPError as error:
        return error.code, error.read().decode()


def alive(device):
    try:
        return device.get("/api/v1/status")[0] == 200
    except (urllib.error.URLError, TimeoutError, OSError):
        return False


def main() -> int:
    ip = hil_state.device_ip()
    device = Device(ip)
    device.login()
    results = {}
    try:
        if not alive(device):
            raise SystemExit("error: device is not responding before the test starts")
        sets_before = device.get("/api/v1/package")[1]["data"]
        print(f"baseline: {len(sets_before)} packages")

        print("\n--- GET /api/v1/repository ---")
        start = time.time()
        status, body = raw_get(ip, "/api/v1/repository", device.cookie)
        elapsed = time.time() - start
        print(f"    HTTP {status} in {elapsed:.2f}s, {len(body)} bytes")
        if status != 200:
            raise SystemExit(f"error: backup failed: {body[:200]}")
        package = json.loads(body)
        print(f"    package_type={package.get('package_type')} "
              f"packages={len(package.get('packages', []))} "
              f"macros={len(package.get('macros', []))}")
        if "skipped" in package:
            # SPEC 17: present only when something was actually dropped.
            print(f"    skipped={package['skipped']}")
        results["backup export"] = alive(device)

        print("\n--- POST /api/v1/restore (round-trip of that same backup) ---")
        start = time.time()
        try:
            status, payload = device.post("/api/v1/restore", package)
        except (urllib.error.URLError, TimeoutError, OSError) as error:
            raise SystemExit(
                f"error: restore never returned ({type(error).__name__}). The device "
                "most likely reset -- which is what a stack overflow in the restore "
                "chain looks like from here."
            ) from error
        elapsed = time.time() - start
        print(f"    HTTP {status} in {elapsed:.2f}s")
        if status == 403:
            raise SystemExit(
                "error: restore was refused for want of physical confirmation, so the "
                "restore chain never ran and this proves nothing about it. Provision "
                "with requirePhysicalConfirmation off, or give `confirm` on the console."
            )
        if status not in (200, 202):
            raise SystemExit(f"error: restore failed: {status} {str(payload)[:300]}")

        # Acceptance criterion 4 and SPEC 13.5: per-package outcomes, not a bare ok.
        data = payload.get("data", {})
        outcomes = data.get("packages")
        if not isinstance(outcomes, list) or not outcomes:
            raise SystemExit(f"error: restore reported no per-package outcomes: {data}")
        for entry in outcomes:
            mark = "ok  " if entry.get("restored") else "FAIL"
            suffix = "" if entry.get("restored") else f"  {entry.get('error')}"
            print(f"    {mark} {entry.get('packageId')}{suffix}")
        print(f"    restored={data.get('restored')} across {len(outcomes)} set(s)")

        survived = alive(device)
        print(f"    device still alive: {survived}")
        results["restore"] = survived and all(e.get("restored") for e in outcomes)

        print("\n--- POST /api/v1/package/import-new (a package package, as a new package) ---")
        # The other worker-routed apply path, and the one nothing exercised: the
        # host suite calls storage_package_import_set directly with a body in
        # hand, so the route, its body shape, and the new-package id never ran.
        package_id = sets_before[0]["id"]
        status, exported = raw_get(ip, f"/api/v1/package/{package_id}/export", device.cookie)
        if status != 200:
            raise SystemExit(f"error: package export failed: {status} {exported[:160]}")
        new_package_id = str(uuid.uuid4())
        status, payload = device.post(
            "/api/v1/package/import-new",
            {"newPackageId": new_package_id, "package": json.loads(exported)},
        )
        if status != 201:
            raise SystemExit(f"error: import-new failed: {status} {str(payload)[:200]}")
        imported = payload["data"]
        status, payload = device.get(f"/api/v1/package/{imported['id']}/macros")
        macros = payload["data"] if status == 200 else []
        print(f"    created {imported['id']} with {len(macros)} macro(s)")
        results["import as new"] = imported["id"] == new_package_id and len(macros) > 0

        print("\n--- POST /api/v1/package/import (replace that package's contents) ---")
        # The third apply path. It is exercised here rather than against an
        # existing package because replace is destructive: the package it lands on is
        # the one import-new just created, and is deleted immediately after.
        replacement = json.loads(exported)
        replacement["packages"][0]["id"] = new_package_id
        replacement["packages"][0]["revision"] = imported["revision"] + 1
        # Drop the last macro, so a success that changed nothing cannot pass.
        replacement["macros"] = replacement["macros"][:-1]
        for macro in replacement["macros"]:
            macro["package_id"] = new_package_id
        status, payload = device.post(
            "/api/v1/package/import",
            {
                "targetPackageId": new_package_id,
                "expectedRevision": imported["revision"],
                "package": replacement,
            },
        )
        if status != 200:
            raise SystemExit(f"error: replace failed: {status} {str(payload)[:200]}")
        replaced = payload["data"]
        status, payload = device.get(f"/api/v1/package/{new_package_id}/macros")
        after_macros = payload["data"] if status == 200 else []
        print(f"    revision {imported['revision']} -> {replaced['revision']}, "
              f"{len(macros)} macro(s) -> {len(after_macros)}")
        results["replace"] = (
            replaced["revision"] == imported["revision"] + 1
            and len(after_macros) == len(macros) - 1
        )

        # Leave the repository as it was found.
        device.delete(f"/api/v1/package/{new_package_id}",
                      {"expectedRevision": replaced["revision"]})

        print("\n--- repository intact afterwards ---")
        sets_after = device.get("/api/v1/package")[1]["data"]
        print(f"    packages after: {len(sets_after)} (was {len(sets_before)})")
        before_ids = sorted(item["id"] for item in sets_before)
        after_ids = sorted(item["id"] for item in sets_after)
        results["repository intact"] = before_ids == after_ids
        if not results["repository intact"]:
            print(f"    before: {before_ids}\n    after:  {after_ids}")

        print("\n" + "=" * 58)
        for name, ok in results.items():
            print(f"  {name:<20} {'PASS' if ok else 'FAIL'}")
        return 0 if all(results.values()) else 1
    finally:
        device.logout()


if __name__ == "__main__":
    raise SystemExit(main())
