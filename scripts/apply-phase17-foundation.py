#!/usr/bin/env python3
"""Apply the Phase 17 authenticated frontend foundation deterministically."""
from __future__ import annotations

import base64
import gzip
import io
import shutil
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD_DIR = ROOT / "scripts" / "phase17-foundation"
PAYLOAD_VERSION = 2


def decode_chunks(prefix: str) -> bytes:
    chunks = sorted(PAYLOAD_DIR.glob(f"{prefix}-*.txt"))
    if not chunks:
        raise SystemExit(f"Phase 17 {prefix} payload is missing")
    encoded = "".join(path.read_text(encoding="ascii").strip() for path in chunks)
    return base64.b64decode(encoded, validate=True)


payload = decode_chunks("payload")
with tarfile.open(fileobj=io.BytesIO(payload), mode="r:gz") as archive:
    for member in archive.getmembers():
        parts = Path(member.name).parts
        if not member.isfile() or not member.name.startswith("webapp/") or ".." in parts:
            raise SystemExit(f"unsafe archive member: {member.name}")
    archive.extractall(ROOT, filter="data")

for module_name in ("backend", "docs"):
    source = gzip.decompress(decode_chunks(module_name)).decode("utf-8")
    module_path = PAYLOAD_DIR / f"patch-{module_name}.py"
    exec(
        compile(source, str(module_path), "exec"),
        {"__name__": "__main__", "__file__": str(module_path)},
    )

(ROOT / "docs" / "CI_PHASE17_FRONTEND_FOUNDATION_FAILURE.md").unlink(missing_ok=True)
shutil.rmtree(PAYLOAD_DIR)
print(f"Phase 17 authenticated frontend foundation applied (payload {PAYLOAD_VERSION})")
