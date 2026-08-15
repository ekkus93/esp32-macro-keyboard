#!/usr/bin/env python3
"""Derive one device's stable bootstrap AP label from its HMAC eFuse key."""

from __future__ import annotations

import argparse
import hashlib
import hmac
import json
import re
from pathlib import Path

KEY_BYTES = 32
DEVICE_ID_BYTES = 6
SECRET_HEX_BYTES = 24
AP_DOMAIN = b"macro-setup-ap-v1\0\0\0"
MAC_PATTERN = re.compile(r"^[0-9A-Fa-f]{12}$")


def parse_device_id(raw_value: str) -> bytes:
    compact = raw_value.replace(":", "").replace("-", "")
    if MAC_PATTERN.fullmatch(compact) is None:
        raise ValueError("device MAC must contain exactly 12 hexadecimal digits")
    device_id = bytes.fromhex(compact)
    if len(device_id) != DEVICE_ID_BYTES:
        raise ValueError("device MAC must decode to exactly 6 bytes")
    return device_id


def read_key(path: Path) -> bytes:
    key = path.read_bytes()
    if len(key) != KEY_BYTES:
        raise ValueError("HMAC key file must contain exactly 32 bytes")
    return key


def derive_secret(key: bytes, domain: bytes, device_id: bytes) -> str:
    digest = hmac.new(key, domain + device_id, hashlib.sha256).hexdigest().upper()
    return digest[:SECRET_HEX_BYTES]


def derive_label(key: bytes, device_id: bytes) -> dict[str, str]:
    device_hex = device_id.hex().upper()
    return {
        "device_id": device_hex,
        "ap_ssid": f"ESP32-Macro-{device_hex[-6:]}",
        "ap_passphrase": derive_secret(key, AP_DOMAIN, device_id),
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Derive the stable bootstrap AP label that firmware will derive "
            "from HMAC_KEY0 for a specific ESP32-S3. The per-boot setup code is not a label value."
        )
    )
    parser.add_argument("key_file", type=Path, help="32-byte HMAC_UP key file")
    parser.add_argument("device_mac", help="SoftAP MAC, with or without separators")
    arguments = parser.parse_args()

    try:
        key = read_key(arguments.key_file)
        device_id = parse_device_id(arguments.device_mac)
    except (OSError, ValueError) as error:
        parser.error(str(error))

    print(json.dumps(derive_label(key, device_id), sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
