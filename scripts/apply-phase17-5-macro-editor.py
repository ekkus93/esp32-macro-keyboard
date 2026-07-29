#!/usr/bin/env python3
"""Decode and apply the integrity-checked Phase 17.5 macro editor payload."""

from __future__ import annotations

import base64
import gzip
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD = ROOT / "scripts" / "phase17-5-macro-editor" / "payload.b64"
EXPECTED_GZIP_SHA256 = "ddb56f8ff8653819db0681e6e1bbf4dc9074faa31eb0bc1175c1c271a5498325"
EXPECTED_SOURCE_SHA256 = "fa539445cab116ea07eba3f92c6813ba32edfeea0ef949f8b7e02e5d43cfb0d7"

encoded = PAYLOAD.read_text(encoding="ascii").strip()
compressed = base64.b64decode(encoded, validate=True)
actual_gzip_sha256 = hashlib.sha256(compressed).hexdigest()
if actual_gzip_sha256 != EXPECTED_GZIP_SHA256:
    raise SystemExit(
        "Phase 17.5 compressed payload integrity failure: "
        f"expected {EXPECTED_GZIP_SHA256}, got {actual_gzip_sha256}"
    )
source = gzip.decompress(compressed)
actual_source_sha256 = hashlib.sha256(source).hexdigest()
if actual_source_sha256 != EXPECTED_SOURCE_SHA256:
    raise SystemExit(
        "Phase 17.5 source payload integrity failure: "
        f"expected {EXPECTED_SOURCE_SHA256}, got {actual_source_sha256}"
    )
module_path = ROOT / "scripts" / "phase17-5-macro-editor" / "payload.py"
virtual_entrypoint = ROOT / "scripts" / "apply-phase17-5-macro-editor.py"
exec(
    compile(source.decode("utf-8"), str(module_path), "exec"),
    {"__name__": "__main__", "__file__": str(virtual_entrypoint)},
)
