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
}
FORBIDDEN_NVS_SCHEMES = {
    "CONFIG_NVS_SEC_KEY_PROTECT_USING_FLASH_ENC",
    "CONFIG_NVS_SEC_KEY_PROTECT_NONE",
}
FORBIDDEN_PRODUCTION_OPTIONS = {
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
print("production NVS configuration uses HMAC encryption with HMAC_KEY0")
PY2
