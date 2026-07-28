#!/usr/bin/env python3
"""Apply the Phase 17 authenticated frontend foundation deterministically."""
from __future__ import annotations

import base64
import io
import runpy
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD_DIR = ROOT / "scripts" / "phase17-foundation"
chunks = sorted(PAYLOAD_DIR.glob("payload-*.txt"))
if not chunks:
    raise SystemExit("Phase 17 frontend payload is missing")
encoded = "".join(path.read_text(encoding="ascii").strip() for path in chunks)
payload = base64.b64decode(encoded, validate=True)
with tarfile.open(fileobj=io.BytesIO(payload), mode="r:gz") as archive:
    for member in archive.getmembers():
        parts = Path(member.name).parts
        if not member.isfile() or not member.name.startswith("webapp/") or ".." in parts:
            raise SystemExit(f"unsafe archive member: {member.name}")
    archive.extractall(ROOT, filter="data")

runpy.run_path(str(PAYLOAD_DIR / "patch-backend.py"), run_name="__main__")
runpy.run_path(str(PAYLOAD_DIR / "patch-docs.py"), run_name="__main__")
(ROOT / "docs" / "CI_PHASE17_FRONTEND_FOUNDATION_FAILURE.md").unlink(missing_ok=True)
print("Phase 17 authenticated frontend foundation applied")
