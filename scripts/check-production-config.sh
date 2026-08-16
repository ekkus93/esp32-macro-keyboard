#!/usr/bin/env bash
set -euo pipefail

readonly config_file="${1:-firmware/sdkconfig.defaults}"

python3 - "${config_file}" <<'PY2'
import re
import sys
from pathlib import Path

REQUIRED_VALUES = {
    "CONFIG_NVS_ENCRYPTION": "y",
    "CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC": "y",
    "CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID": "0",
    "CONFIG_APP_RETRIEVE_LEN_ELF_SHA": "39",
    # SPEC_V2 §5.3: the reference module is the ESP32-S3R8 with 8 MB embedded
    # octal PSRAM, and "a quad-PSRAM build is not interchangeable with the
    # reference hardware". These were set in sdkconfig.defaults but unverified
    # until the V2-156 audit (2026-08-16) -- a SPIRAM-off or quad build passed
    # every gate.
    "CONFIG_SPIRAM": "y",
    "CONFIG_SPIRAM_MODE_OCT": "y",
    "CONFIG_SPIRAM_USE_MALLOC": "y",
    # SPEC_V2 §5.3: "FreeRTOS task stacks MUST remain in internal SRAM."
    # ESP-IDF defaults this to y when SPIRAM is enabled, which only *permits*
    # external stacks via xTaskCreateStatic -- the firmware never does that, so
    # the requirement held by construction alone. Pinning it to n makes an
    # external task stack impossible rather than merely unused, and requiring
    # the key to be present stops a silent revert to the IDF default.
    "CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY": "n",
}
FORBIDDEN_NVS_SCHEMES = {
    "CONFIG_NVS_SEC_KEY_PROTECT_USING_FLASH_ENC",
    "CONFIG_NVS_SEC_KEY_PROTECT_NONE",
}
FORBIDDEN_PRODUCTION_OPTIONS = {
    "CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG",
    "CONFIG_APP_MANUFACTURING_PROVISIONING_LOG",
}
ASSIGNMENT = re.compile(r"^(CONFIG_[A-Z0-9_]+)=(.*)$")
NOT_SET = re.compile(r"^# (CONFIG_[A-Z0-9_]+) is not set$")


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def parse_config(path: Path) -> dict[str, str]:
    if not path.is_file():
        fail(f"production configuration not found: {path}")
    values: dict[str, str] = {}
    for line_number, raw_line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), start=1
    ):
        line = raw_line.strip()
        if not line:
            continue
        assignment = ASSIGNMENT.fullmatch(line)
        not_set = NOT_SET.fullmatch(line)
        if assignment is not None:
            name, value = assignment.groups()
        elif not_set is not None:
            name = not_set.group(1)
            value = "n"
        elif line.startswith("#"):
            continue
        else:
            continue
        if name in values:
            fail(f"duplicate setting {name} at line {line_number}")
        values[name] = value
    return values


def validate(values: dict[str, str]) -> None:
    for name, expected in REQUIRED_VALUES.items():
        actual = values.get(name)
        if actual != expected:
            rendered = "missing" if actual is None else repr(actual)
            fail(f"{name} must be {expected!r}; found {rendered}")
    for name in FORBIDDEN_NVS_SCHEMES:
        if values.get(name) == "y":
            fail(f"{name}=y conflicts with the required HMAC NVS scheme")
    for name in FORBIDDEN_PRODUCTION_OPTIONS:
        if values.get(name) == "y":
            fail(f"{name}=y is forbidden in production configuration")

    raw_key_id = values["CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID"]
    try:
        key_id = int(raw_key_id, 10)
    except ValueError:
        fail("CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID must be an integer")
    if key_id < 0 or key_id > 5:
        fail("CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID must be between 0 and 5")


configuration_path = Path(sys.argv[1])
validate(parse_config(configuration_path))
print("production configuration uses HMAC NVS encryption and a 39-character diagnostics ELF SHA")
PY2
