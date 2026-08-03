#!/usr/bin/env python3
"""Validate the reviewed v2 device-settings layout and its C constants."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent.parent
SCHEMA_PATH = ROOT / "contracts/v2/device-settings.json"
HEADER_PATH = (
    ROOT / "firmware/components/app_contracts_v2/include/device_settings_v2.h"
)

CONSTANTS = {
    "APP_V2_SETTINGS_MAGIC": ("magic", "UINT32_C"),
    "APP_V2_SETTINGS_RECORD_VERSION": ("schemaVersion", "UINT16_C"),
    "APP_V2_SETTINGS_RECORD_BYTES": ("recordLength", "UINT16_C"),
    "APP_V2_PASSWORD_ALGORITHM_VERSION": (
        "password.algorithmVersion",
        "UINT16_C",
    ),
    "APP_V2_PASSWORD_SALT_BYTES": ("password.saltBytes", "UINT32_C"),
    "APP_V2_PASSWORD_VERIFIER_BYTES": ("password.verifierBytes", "UINT32_C"),
    "APP_V2_DEVICE_NAME_MAX_BYTES": ("limits.deviceNameMaxBytes", "UINT32_C"),
    "APP_V2_WIFI_SSID_MAX_BYTES": ("limits.ssidMaxBytes", "UINT32_C"),
    "APP_V2_WIFI_PASSPHRASE_MIN_BYTES": (
        "limits.passphraseMinBytes",
        "UINT32_C",
    ),
    "APP_V2_WIFI_PASSPHRASE_MAX_BYTES": (
        "limits.passphraseMaxBytes",
        "UINT32_C",
    ),
    "APP_V2_UUID_TEXT_BYTES": ("limits.uuidTextBytes", "UINT32_C"),
}

OFFSET_NAMES = {
    "magic": "APP_V2_SETTINGS_OFFSET_MAGIC",
    "recordVersion": "APP_V2_SETTINGS_OFFSET_RECORD_VERSION",
    "recordLength": "APP_V2_SETTINGS_OFFSET_RECORD_LENGTH",
    "credentialVersion": "APP_V2_SETTINGS_OFFSET_CREDENTIAL_VERSION",
    "passwordAlgorithmVersion": "APP_V2_SETTINGS_OFFSET_PASSWORD_ALGORITHM",
    "passwordIterations": "APP_V2_SETTINGS_OFFSET_PASSWORD_ITERATIONS",
    "passwordSalt": "APP_V2_SETTINGS_OFFSET_PASSWORD_SALT",
    "passwordVerifier": "APP_V2_SETTINGS_OFFSET_PASSWORD_VERIFIER",
    "nextBlobId": "APP_V2_SETTINGS_OFFSET_NEXT_BLOB_ID",
    "sendMode": "APP_V2_SETTINGS_OFFSET_SEND_MODE",
    "snapshotRetentionTarget": "APP_V2_SETTINGS_OFFSET_RETENTION_TARGET",
    "showMacroSourcePreviews": "APP_V2_SETTINGS_OFFSET_SHOW_SOURCE",
    "requireSerialConfirmation": "APP_V2_SETTINGS_OFFSET_REQUIRE_CONFIRMATION",
    "provisioned": "APP_V2_SETTINGS_OFFSET_PROVISIONED",
    "stationConfigured": "APP_V2_SETTINGS_OFFSET_STATION_CONFIGURED",
    "reserved": "APP_V2_SETTINGS_OFFSET_RESERVED",
    "lastSelectedPackageId": "APP_V2_SETTINGS_OFFSET_LAST_SELECTED_PACKAGE",
    "deviceName": "APP_V2_SETTINGS_OFFSET_DEVICE_NAME",
    "apSsid": "APP_V2_SETTINGS_OFFSET_AP_SSID",
    "apPassphrase": "APP_V2_SETTINGS_OFFSET_AP_PASSPHRASE",
    "stationSsid": "APP_V2_SETTINGS_OFFSET_STATION_SSID",
    "stationPassphrase": "APP_V2_SETTINGS_OFFSET_STATION_PASSPHRASE",
}


def nested_value(document: dict[str, Any], path: str) -> int:
    value: Any = document
    for part in path.split("."):
        if not isinstance(value, dict) or part not in value:
            raise ValueError(f"missing schema value {path}")
        value = value[part]
    if not isinstance(value, int) or isinstance(value, bool):
        raise ValueError(f"schema value {path} must be an integer")
    return value


def parse_define(header: str, name: str, wrapper: str) -> int:
    pattern = rf"^#define {re.escape(name)} {re.escape(wrapper)}\((0x[0-9a-f]+|[0-9]+)\)$"
    match = re.search(pattern, header, re.MULTILINE)
    if match is None:
        raise ValueError(f"missing exact header definition for {name}")
    return int(match.group(1), 0)


def validate_fields(document: dict[str, Any]) -> list[dict[str, Any]]:
    fields = document.get("fields")
    if not isinstance(fields, list) or not fields:
        raise ValueError("settings fields must be a nonempty array")

    expected_offset = 0
    seen: set[str] = set()
    validated: list[dict[str, Any]] = []
    for field in fields:
        if not isinstance(field, dict):
            raise ValueError("every settings field must be an object")
        name = field.get("name")
        offset = field.get("offset")
        length = field.get("length")
        encoding = field.get("encoding")
        if (
            not isinstance(name, str)
            or name in seen
            or not isinstance(offset, int)
            or isinstance(offset, bool)
            or not isinstance(length, int)
            or isinstance(length, bool)
            or length <= 0
            or not isinstance(encoding, str)
        ):
            raise ValueError(f"invalid settings field {field!r}")
        if offset != expected_offset:
            raise ValueError(
                f"field {name} begins at {offset}, expected contiguous offset {expected_offset}"
            )
        if name not in OFFSET_NAMES:
            raise ValueError(f"field {name} has no reviewed C offset name")
        seen.add(name)
        expected_offset += length
        validated.append(field)

    if set(OFFSET_NAMES) != seen:
        missing = sorted(set(OFFSET_NAMES) - seen)
        raise ValueError(f"settings schema is missing fields: {missing}")
    record_length = nested_value(document, "recordLength")
    if expected_offset != record_length:
        raise ValueError(
            f"field layout ends at {expected_offset}, recordLength is {record_length}"
        )
    return validated


def main() -> int:
    try:
        document: Any = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
        if not isinstance(document, dict):
            raise ValueError("settings schema must be a JSON object")
        if document.get("format") != "esp32-macro-keyboard-device-settings":
            raise ValueError("settings schema format is invalid")
        if document.get("byteOrder") != "little-endian":
            raise ValueError("settings schema must remain little-endian")

        fields = validate_fields(document)
        header = HEADER_PATH.read_text(encoding="utf-8")
        for name, (path, wrapper) in CONSTANTS.items():
            expected = nested_value(document, path)
            actual = parse_define(header, name, wrapper)
            if actual != expected:
                raise ValueError(f"{name} is {actual}, expected {expected}")

        for field in fields:
            name = str(field["name"])
            expected = int(field["offset"])
            actual = parse_define(header, OFFSET_NAMES[name], "UINT16_C")
            if actual != expected:
                raise ValueError(
                    f"{OFFSET_NAMES[name]} is {actual}, expected {expected}"
                )
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    print("v2 device-settings JSON and C layout constants match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
