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


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"expected one {label}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


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

app_path = ROOT / "webapp" / "src" / "App.tsx"
replace_once(
    app_path,
    '  const [route, setRoute] = useState<Screen>(() => routeFromHash());\n',
    '  const [route, setRoute] = useState<Screen>(() => routeFromHash());\n'
    '  const [routeHash, setRouteHash] = useState(() => window.location.hash);\n',
    "route hash state",
)
replace_once(
    app_path,
    '      setRoute(routeFromHash());\n',
    '      setRoute(routeFromHash());\n'
    '      setRouteHash(window.location.hash);\n',
    "route hash update",
)

editor_path = ROOT / "webapp" / "src" / "features" / "macros" / "MacroEditorPage.tsx"
replace_once(
    editor_path,
    '  utf8ByteLength,\n',
    '',
    "unused UTF-8 byte length import",
)
replace_once(
    editor_path,
    '  const resolveConflict = (): void => {\n'
    '    if (persisted) {\n',
    '  const resolveConflict = (): void => {\n'
    '    if (activeSet === null) {\n'
    '      return;\n'
    '    }\n'
    '    if (persisted) {\n',
    "active set conflict guard",
)
