#!/usr/bin/env python3
"""Decode and apply the integrity-checked Phase 17.6 procedure workflow."""

from __future__ import annotations

import base64
import gzip
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD_DIR = ROOT / "scripts" / "phase17-6-procedure-workflow"
CHUNKS = (('payload-00.txt', '08c303eb1e55a82ba37c51586f5a2cbd85395cbd2ea30bdc1895510847601ac0'), ('payload-01.txt', 'cfaf5ddeb203e73325ba62ad598cd8b4ce1336dc8ed589b2311c8305e8c5db96'), ('payload-02.txt', '42c4a91c9b234943c709a845bd0e5d4bf60f47809a426da04636d8663c0cadcf'), ('payload-03.txt', 'a6bda20dd6c1e38b4369c7add1d623fca23675370c39312c21c4ebb8c52d9cae'), ('payload-04.txt', '76c45ace74847e818b7e0bf2ca3a9b86a2689b331c9d3844a403690eea5b5af9'), ('payload-05.txt', 'c1f312ad82b7bf4aa174e03c2722d389da8c661b7f19c97f772b43040009ce5f'), ('payload-06.txt', '71e6f885aff1eeb45ef78ddf2782bec7cecb73a65accc113dc2e41745e87d5b1'), ('payload-07.txt', '57beef9565e8c07ca7f4bc503365316e575b395c6395e93cc8bdeed77a6237eb'), ('payload-08.txt', 'a962a7c830b308512be744871c0de6ff6667578cdd92128ce5c3365ecc61d61c'), ('payload-09.txt', '3dd730e69e9f1a7f4adc44c848c24680c77d89448d3d3b8520b3bb1e4d45a390'), ('payload-10.txt', 'dd51ba468417fc818e8c04a86634ed5d48ed9ff34df97f7fd9c10634b6cd503e'))
EXPECTED_GZIP_SHA256 = "e31eb845f964f97457db75f519f7c33ef3a56d5d9f7d70e4f8d87384cce0569e"
EXPECTED_SOURCE_SHA256 = "98239643a925652dbd5de5b641b4801b9585ba0c107baf0b266655be3562aa12"

encoded_parts: list[str] = []
for name, expected_sha256 in CHUNKS:
    data = (PAYLOAD_DIR / name).read_text(encoding="ascii").strip()
    actual_sha256 = hashlib.sha256(data.encode("ascii")).hexdigest()
    if actual_sha256 != expected_sha256:
        raise SystemExit(
            f"Phase 17.6 chunk integrity failure for {name}: "
            f"expected {expected_sha256}, got {actual_sha256}"
        )
    encoded_parts.append(data)

compressed = base64.b64decode("".join(encoded_parts), validate=True)
actual_gzip_sha256 = hashlib.sha256(compressed).hexdigest()
if actual_gzip_sha256 != EXPECTED_GZIP_SHA256:
    raise SystemExit(
        "Phase 17.6 compressed payload integrity failure: "
        f"expected {EXPECTED_GZIP_SHA256}, got {actual_gzip_sha256}"
    )

source = gzip.decompress(compressed)
actual_source_sha256 = hashlib.sha256(source).hexdigest()
if actual_source_sha256 != EXPECTED_SOURCE_SHA256:
    raise SystemExit(
        "Phase 17.6 source payload integrity failure: "
        f"expected {EXPECTED_SOURCE_SHA256}, got {actual_source_sha256}"
    )

module_path = PAYLOAD_DIR / "payload.py"
exec(
    compile(source.decode("utf-8"), str(module_path), "exec"),
    {"__name__": "__main__", "__file__": str(module_path)},
)
