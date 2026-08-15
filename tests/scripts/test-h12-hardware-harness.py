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
runner = read("scripts/run-h12-122-hardware.py")
flasher = read("scripts/flash-release-manifest.py")
client = read("tests/hardware/device_client.py")
retired_reset = read("tests/hardware/test_acceptance_reset.py")
retired_smoke = read("tests/hardware/test_h12_release_smoke.py")
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
require('console_command("setup-code"' in provision,
        "provision helper must explicitly request the per-boot UART code")
require("setup_code.txt" not in provision, "ephemeral setup code must not be persisted")
require("setup_code" not in label.split("def derive_label", 1)[1].split("def main", 1)[0],
        "manufacturing label must not contain a setup-code field")

for required in (
    "--firmware-sha",
    "--flash-manifest",
    "--flash-port",
    "--console",
    "flash-release-manifest.py",
    '"/api/v1/send"',
    '"/api/v1/blob"',
    '"/api/v1/settings/change-password"',
    '"/api/v1/device/restart"',
    '"/api/v1/device/factory-reset"',
    "provision_device.provision",
    "Capture",
):
    require(required in runner, f"H12-122 runner is missing {required}")

for required in (
    'manifest.get("gitDirty") is not False',
    'manifest.get("buildType") != "production"',
    'manifest.get("espIdfVersion") != EXPECTED_IDF',
    '"webfs.bin"',
):
    require(required in flasher, f"release flasher is missing fail-closed check {required}")

require("except Exception:\n            pass" not in client,
        "Device.logout must not silently swallow cleanup failure")
require("is retired" in retired_reset,
        "retired v1 reset harness must fail explicitly instead of producing false evidence")
require("is retired" in retired_smoke,
        "interim H12 smoke must fail explicitly instead of competing with the final runner")
require("run-h12-122-hardware.py" in retired_reset and "run-h12-122-hardware.py" in retired_smoke,
        "retired hardware entry points must direct operators to the final H12 runner")

print("H12 hardware harness contract tests passed")
