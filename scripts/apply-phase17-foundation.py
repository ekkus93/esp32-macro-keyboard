#!/usr/bin/env python3
"""Apply the Phase 17 authenticated frontend foundation deterministically."""
from __future__ import annotations

import base64
import gzip
import hashlib
import io
import shutil
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD_DIR = ROOT / "scripts" / "phase17-foundation"
PAYLOAD_VERSION = 5

MANIFEST = {
    "backend": [
        ("safe-backend-v5-00.txt", "57a8fd57442cb0bd376c595956e8281b0a4ae593"),
        ("safe-backend-v5-01.txt", "55caa580507fda18ded66ee96c1baf919fa1d1cd"),
        ("safe-backend-v5-02.txt", "c7827984849bc13a696e70777098b8964547b8eb"),
        ("safe-backend-v5-03.txt", "3c332accf5cc0fc5c2bfb9ce1e82da463d7c7d42"),
    ],
    "docs": [
        ("safe-docs-00.txt", "ebd8123f764704fba2dded49767e8e3e678c65d9"),
    ],
    "frontend": [
        ("safe-frontend-00.txt", "f2a93fe4a0124e9be4e844ab959399eb63eed177"),
        ("safe-frontend-01.txt", "f4a66d937123d8882f7f17f3292117d89571c935"),
        ("safe-frontend-02.txt", "b7e10a616fc52558feecac6ad1bfdde172c619c7"),
        ("safe-frontend-03.txt", "d1cd4b83d87541d0f4cd771ad4b0e92fc2f932d6"),
        ("safe-frontend-04.txt", "24d8f17f127e34e5036c8c21c8b6bb4d8d2a425c"),
        ("safe-frontend-05.txt", "6709586e20e126a6172d06dbc03fe433018398e5"),
        ("safe-frontend-06.txt", "010c3f09c6d2d8304920040666e80b00934d1fe8"),
        ("safe-frontend-07.txt", "9e1fc3f26909ae2cec41400ef3262b536b18198c"),
        ("safe-frontend-08.txt", "5f446983a091d6f006fb98e0c104d76bfd683494"),
        ("safe-frontend-09.txt", "cdefe4e02a5115e407e8da145e26de008e5ad3d8"),
    ],
}


def git_blob_sha(data: bytes) -> str:
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()


def decode_chunks(group: str) -> bytes:
    encoded_parts: list[str] = []
    for filename, expected_sha in MANIFEST[group]:
        path = PAYLOAD_DIR / filename
        data = path.read_bytes()
        actual_sha = git_blob_sha(data)
        if actual_sha != expected_sha:
            raise SystemExit(
                f"Phase 17 payload integrity failure for {filename}: "
                f"expected {expected_sha}, got {actual_sha}"
            )
        encoded_parts.append(data.decode("ascii").strip())
    return base64.b64decode("".join(encoded_parts), validate=True)


payload = decode_chunks("frontend")
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
