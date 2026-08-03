#!/usr/bin/env bash
set -euo pipefail

readonly source_root="${1:-webapp/src}"

python3 - "${source_root}" <<'PY2'
import re
import sys
from pathlib import Path

APPROVED_LOCAL_STORAGE_FILES = {
    Path("features/package/PackageSelectionPage.tsx"),
}
APPROVED_LOCAL_STORAGE_KEYS = {
    "esp32-macro-keyboard.recent-package-ids",
}
# Not used anywhere today (FIX1 18.5): any future use is a deliberate policy
# decision that must extend this checker, not something that slips in
# unreviewed alongside an unrelated change.
FORBIDDEN_APIS = re.compile(r"\bsessionStorage\b|\bindexedDB\b|\bcaches\.|\bserviceWorker\b")
LOCAL_STORAGE_CALL = re.compile(
    r"localStorage\.(?:getItem|setItem|removeItem)\s*\(\s*([A-Za-z_$][A-Za-z0-9_$]*|\"[^\"]*\"|'[^']*')"
)
STRING_CONST = re.compile(
    r"""const\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=\s*(["'])((?:\\.|(?!\2)[^\\])*)\2"""
)
SOURCE_SUFFIXES = {".ts", ".tsx"}


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def resolve_key(token: str, constants: dict[str, str], path: Path) -> str:
    if token[0] in "\"'":
        return token[1:-1]
    if token not in constants:
        fail(f"{path}: localStorage key {token!r} is not a traceable string literal")
    return constants[token]


def validate_file(root: Path, path: Path, text: str) -> None:
    relative = path.relative_to(root)
    if FORBIDDEN_APIS.search(text):
        fail(f"{relative}: uses a persisted-storage API outside the FIX1 18.5 allowlist "
             f"(sessionStorage/indexedDB/Cache Storage/service worker)")
    if "localStorage" not in text:
        return
    if relative not in APPROVED_LOCAL_STORAGE_FILES:
        fail(f"{relative}: localStorage use outside the FIX1 18.5 approved file list")
    constants = {name: value for name, _, value in STRING_CONST.findall(text)}
    for match in LOCAL_STORAGE_CALL.finditer(text):
        key = resolve_key(match.group(1), constants, relative)
        if key not in APPROVED_LOCAL_STORAGE_KEYS:
            fail(f"{relative}: localStorage key {key!r} is not in the FIX1 18.5 approved key list")


def validate(root: Path) -> None:
    if not root.is_dir():
        fail(f"source root not found: {root}")
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix in SOURCE_SUFFIXES:
            validate_file(root, path, path.read_text(encoding="utf-8"))


validate(Path(sys.argv[1]))
print("frontend persisted-state policy passed")
PY2
