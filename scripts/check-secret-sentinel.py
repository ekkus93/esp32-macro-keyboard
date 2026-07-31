#!/usr/bin/env python3
"""Fail closed when a known test secret appears in generated artifacts."""

from __future__ import annotations

import argparse
import base64
import json
from pathlib import Path
from urllib.parse import quote


def representations(secret: str) -> dict[str, bytes]:
    raw = secret.encode("utf-8")
    json_escaped = json.dumps(secret, ensure_ascii=True)[1:-1].encode("ascii")
    return {
        "raw": raw,
        "json-escaped": json_escaped,
        "url-encoded": quote(secret, safe="").encode("ascii"),
        "base64": base64.b64encode(raw),
        "base64url": base64.urlsafe_b64encode(raw).rstrip(b"="),
        "hex-lower": raw.hex().encode("ascii"),
        "hex-upper": raw.hex().upper().encode("ascii"),
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--secret-file", required=True, type=Path)
    parser.add_argument("outputs", nargs="+", type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        secret = args.secret_file.read_text(encoding="utf-8").rstrip("\r\n")
    except OSError as error:
        print(f"error: unable to read sentinel file: {error}")
        return 2
    if len(secret.encode("utf-8")) < 16:
        print("error: sentinel must be at least 16 UTF-8 bytes")
        return 2

    variants = representations(secret)
    failed = False
    for output in args.outputs:
        try:
            data = output.read_bytes()
        except OSError as error:
            print(f"error: unable to read output {output}: {error}")
            failed = True
            continue
        for label, value in variants.items():
            if value and value in data:
                print(f"error: secret sentinel representation {label} found in {output}")
                failed = True
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
