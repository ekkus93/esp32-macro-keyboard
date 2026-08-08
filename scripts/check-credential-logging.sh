#!/usr/bin/env bash
set -euo pipefail

# V2 SPEC 12.3/12.4: the per-boot eight-digit setup code is intentionally shown
# on the trusted serial console. No other credential, token, passphrase, or
# secret material may be emitted by firmware.

readonly source_root="${1:-firmware}"

python3 - "${source_root}" <<'PY2'
import re
import sys
from pathlib import Path

APPROVED_PATH = Path("components/app_core/app_core.c")
APPROVED_SETUP_MESSAGE = "setup code: %s"
LEGACY_OPTIONS = {
    "CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG",
}
STRING_LITERAL_SOURCE = r'"(?:\\.|[^"\\])*"'
STRING_LITERAL = re.compile(STRING_LITERAL_SOURCE)
OUTPUT_CALL = re.compile(
    rf"(?<![A-Za-z0-9_])(?:ESP_LOG[A-Z]+|printf|fprintf)\s*\((?:{STRING_LITERAL_SOURCE}|[^\";])*\);",
    re.DOTALL,
)
SENSITIVE_WORD = re.compile(
    r"(?:password|passphrase|(?:session|csrf|api|access)[_ -]?token|setup[_ -]?code)",
    re.IGNORECASE,
)
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


def validate_approved_file(path: Path, text: str) -> None:
    approved_count = 0
    for match in OUTPUT_CALL.finditer(text):
        call = match.group(0)
        message = joined_literals(call)
        if not SENSITIVE_WORD.search(message) or not FORMAT_VALUE.search(message):
            continue
        if APPROVED_SETUP_MESSAGE not in message:
            fail(f"{path}: unapproved credential-bearing output")
        approved_count += 1
    if approved_count != 1:
        fail(
            f"{path}: expected exactly one serial setup-code output, found {approved_count}"
        )


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
        fail(f"approved setup-code source not found: {approved_path}")

    approved_seen = False
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
            continue
        text = path.read_text(encoding="utf-8")
        for option in LEGACY_OPTIONS:
            if option in text:
                fail(f"{path}: legacy credential logging option is forbidden")
        if path == approved_path:
            validate_approved_file(path, text)
            approved_seen = True
        else:
            validate_other_file(path, text)
    if not approved_seen:
        fail("approved setup-code source was not scanned")


validate(Path(sys.argv[1]))
print("V2 credential logging policy passed")
PY2
