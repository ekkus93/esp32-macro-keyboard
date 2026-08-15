#!/usr/bin/env python3
"""Static regression guard for the H12 exact-SHA hardware evidence path."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


provision = read("tests/hardware/provision_device.py")
release_smoke = read("tests/hardware/test_h12_release_smoke.py")
client = read("tests/hardware/device_client.py")
retired_reset = read("tests/hardware/test_acceptance_reset.py")
label = read("scripts/generate-setup-label.py")

for retired in (
    "/api/v1/setup-state",
    "/api/v1/setup/credentials",
    "/api/v1/setup/restart",
    "expectedRevision",
    "requirePhysicalConfirmation",
    "alwaysSelectPackage",
):
    require(retired not in provision, f"provision helper still references retired contract {retired}")

require('"GET", "/api/v1/setup"' in provision, "provision helper must read current setup state")
require('"POST", "/api/v1/setup"' in provision, "provision helper must use one-shot v2 setup")
require("capture_setup_code" in provision, "provision helper must capture the per-boot UART code")
require("setup_code.txt" not in provision, "ephemeral setup code must not be persisted")
require("setup_code" not in label.split("def derive_label", 1)[1].split("def main", 1)[0],
        "manufacturing label must not contain a setup-code field")

for required in (
    "--firmware-sha",
    "--flash-manifest",
    "--allow-destructive",
    '"gitCommit"',
    "load_flash_manifest",
    "verify_firmware_provenance",
    '"/api/v1/send"',
    '"/api/v1/blob"',
    '"/api/v1/settings/change-password"',
    '"/api/v1/device/restart"',
    '"/api/v1/device/factory-reset"',
    "provision_device.provision",
):
    require(required in release_smoke, f"H12 release smoke is missing {required}")

require("except Exception:\n            pass" not in client,
        "Device.logout must not silently swallow cleanup failure")
require("is retired" in retired_reset,
        "retired v1 reset harness must fail explicitly instead of producing false evidence")

print("H12 hardware harness contract tests passed")
