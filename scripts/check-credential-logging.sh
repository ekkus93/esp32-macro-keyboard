#!/usr/bin/env bash
set -euo pipefail

readonly source_root="${1:-firmware}"

python3 - "${source_root}" <<'PY2'
import re
import sys
from pathlib import Path

APPROVED_PATH = Path("components/app_core/app_core.c")
MANUFACTURING_GUARD = "#if CONFIG_APP_MANUFACTURING_PROVISIONING_LOG"
MANUFACTURING_BANNER = (
    "MANUFACTURING MODE ENABLED: plaintext one-time credentials follow; "
    "never deploy this build"
)
APPROVED_MESSAGES = {
    "manufacturing-only AP SSID: %s",
    "manufacturing-only AP passphrase: %s",
    "manufacturing-only web password: %s",
}
LEGACY_OPTION = "CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG"
OUTPUT_CALL = re.compile(
    r"(?:ESP_LOG[A-Z]+|printf|fprintf)\s*\([^;]*?\);",
    re.DOTALL,
)
SENSITIVE_WORD = re.compile(
    r"(?:password|passphrase|ssid|session[_ -]?token|csrf[_ -]?token|setup[_ -]?code)",
    re.IGNORECASE,
)
STRING_LITERAL = re.compile(r'"(?:\\.|[^"\\])*"')
FORMAT_VALUE = re.compile(r"%(?:\.\*)?[a-zA-Z]")
SOURCE_SUFFIXES = {".c", ".h", ".cc", ".cpp", ".hpp"}


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def joined_literals(call: str) -> str:
    pieces = []
    for match in STRING_LITERAL.finditer(call):
        literal = match.group(0)[1:-1]
        pieces.append(bytes(literal, "utf-8").decode("unicode_escape"))
    return "".join(pieces)


def manufacturing_region(text: str, path: Path) -> tuple[int, int]:
    guard_start = text.find(MANUFACTURING_GUARD)
    if guard_start < 0:
        fail(f"{path}: missing manufacturing credential guard")
    guard_end = text.find("#else", guard_start)
    if guard_end < 0:
        fail(f"{path}: manufacturing credential guard has no #else")
    if MANUFACTURING_BANNER not in text[guard_start:guard_end]:
        fail(f"{path}: missing permanent manufacturing warning banner")
    return guard_start, guard_end


def validate_approved_file(root: Path, path: Path, text: str) -> None:
    region_start, region_end = manufacturing_region(text, path)
    seen: dict[str, int] = {message: 0 for message in APPROVED_MESSAGES}
    for match in OUTPUT_CALL.finditer(text):
        call = match.group(0)
        message = joined_literals(call)
        if not SENSITIVE_WORD.search(message) or not FORMAT_VALUE.search(message):
            continue
        if not (region_start <= match.start() < region_end):
            fail(f"{path}: credential-bearing output exists outside manufacturing guard")
        approved = [item for item in APPROVED_MESSAGES if item in message]
        if len(approved) != 1:
            fail(f"{path}: unapproved manufacturing credential output")
        seen[approved[0]] += 1
    for message, count in seen.items():
        if count != 1:
            fail(f"{path}: expected one approved output for {message!r}, found {count}")

    relative = path.relative_to(root)
    if relative != APPROVED_PATH:
        fail(f"approved credential output must be in {APPROVED_PATH}, not {relative}")


def validate_other_file(path: Path, text: str) -> None:
    for match in OUTPUT_CALL.finditer(text):
        message = joined_literals(match.group(0))
        if SENSITIVE_WORD.search(message) and FORMAT_VALUE.search(message):
            fail(f"{path}: credential-bearing output is forbidden")


def validate(root: Path) -> None:
    if not root.is_dir():
        fail(f"source root not found: {root}")
    approved_path = root / APPROVED_PATH
    if not approved_path.is_file():
        fail(f"approved manufacturing source not found: {approved_path}")

    approved_seen = False
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
            continue
        text = path.read_text(encoding="utf-8")
        if LEGACY_OPTION in text:
            fail(f"{path}: legacy credential logging option is forbidden")
        if path == approved_path:
            validate_approved_file(root, path, text)
            approved_seen = True
        else:
            validate_other_file(path, text)
    if not approved_seen:
        fail("approved manufacturing source was not scanned")


validate(Path(sys.argv[1]))
print("credential logging policy passed")
PY2
