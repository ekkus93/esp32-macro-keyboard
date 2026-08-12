#!/usr/bin/env bash
set -euo pipefail

# H9 / PROVISIONING_SECURITY.md: bootstrap credentials are delivered by the
# controlled manufacturing label/QR path. Ordinary firmware logs must never
# emit a plaintext password, passphrase, setup code, token, salt, or verifier.
readonly source_root="${1:-firmware}"

python3 - "${source_root}" <<'PY2'
import re
import sys
from pathlib import Path

LEGACY_OPTIONS = {"CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG"}
STRING_LITERAL_SOURCE = r'"(?:\\.|[^"\\])*"'
STRING_LITERAL = re.compile(STRING_LITERAL_SOURCE)
OUTPUT_SINK = (
    r"(?:ESP_(?:EARLY_|DRAM_)?LOG[A-Z_]*|"
    r"esp_log_(?:writev?|buffer_(?:hex|char|hexdump))|"
    r"esp_rom_printf|ets_printf|printf|fprintf|puts|fputs|vprintf|vfprintf)"
)
OUTPUT_CALL = re.compile(
    rf"(?<![A-Za-z0-9_]){OUTPUT_SINK}\s*\((?:{STRING_LITERAL_SOURCE}|[^\";])*\);",
    re.DOTALL,
)
SENSITIVE_WORD = re.compile(
    r"(?:password|passphrase|setup[_ -]?code|(?:session|csrf|api|access)[_ -]?token|salt|verifier)",
    re.IGNORECASE,
)
SENSITIVE_IDENTIFIER = re.compile(
    r"(?:password|passphrase|setup[_]?code|(?:session|csrf|api|access)[_]?token|salt|verifier)",
    re.IGNORECASE,
)
FORMAT_VALUE = re.compile(r"%(?:\.\*)?[a-zA-Z]")
IDENTIFIER = re.compile(r"\b[A-Za-z_]\w*\b")
ASSIGNMENT = re.compile(r"\b([A-Za-z_]\w*)\s*=\s*([^;]+);")
SIMPLE_ALIAS = re.compile(
    r"^\s*(?:\([^)]+\)\s*)?([A-Za-z_]\w*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*)*)\s*$"
)
TAINT_TRANSFER = re.compile(
    r"\b(?:memcpy|memmove|strcpy|strncpy|strlcpy|strcat|strncat|snprintf|sprintf)\s*\("
    r"\s*(?P<target>[A-Za-z_]\w*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*)*)\s*,"
    r"(?P<rest>[^;]*)\)\s*;",
    re.DOTALL,
)
COMMENT_OR_LITERAL = re.compile(
    r"//[^\n]*|/\*.*?\*/|" + STRING_LITERAL_SOURCE + r"|'(?:\\.|[^'\\])*'",
    re.DOTALL,
)
SOURCE_SUFFIXES = {".c", ".h", ".cc", ".cpp", ".hpp"}


def fail(message: str) -> None:
    raise SystemExit(f"error: {message}")


def joined_literals(call: str) -> str:
    pieces = []
    for match in STRING_LITERAL.finditer(call):
        literal = match.group(0)[1:-1]
        pieces.append(bytes(literal, "utf-8").decode("unicode_escape"))
    return "".join(pieces)


def without_literals(text: str) -> str:
    return STRING_LITERAL.sub('""', text)


def mask_comments_and_literals(text: str) -> str:
    def replacement(match: re.Match[str]) -> str:
        return "".join("\n" if char == "\n" else " " for char in match.group(0))

    return COMMENT_OR_LITERAL.sub(replacement, text)


def enclosing_scope_start(text: str, position: int) -> int:
    masked = mask_comments_and_literals(text[:position])
    depth = 0
    start = 0
    for index, char in enumerate(masked):
        if char == "{":
            if depth == 0:
                start = index + 1
            depth += 1
        elif char == "}" and depth > 0:
            depth -= 1
    return start


def tainted_aliases(scope_prefix: str) -> set[str]:
    scope_code = mask_comments_and_literals(scope_prefix)
    identifiers = IDENTIFIER.findall(scope_code)
    tainted = {identifier for identifier in identifiers if SENSITIVE_IDENTIFIER.search(identifier)}
    changed = True
    while changed:
        changed = False
        for assignment in ASSIGNMENT.finditer(scope_code):
            target = assignment.group(1)
            rhs = assignment.group(2).strip()
            alias = SIMPLE_ALIAS.fullmatch(rhs)
            if alias is None:
                continue
            sources = set(IDENTIFIER.findall(alias.group(1)))
            if sources & tainted and target not in tainted:
                tainted.add(target)
                changed = True
        for transfer in TAINT_TRANSFER.finditer(scope_code):
            target_identifiers = IDENTIFIER.findall(transfer.group("target"))
            if not target_identifiers:
                continue
            target = target_identifiers[-1]
            sources = set(IDENTIFIER.findall(transfer.group("rest")))
            sources.discard(target)
            if sources & tainted and target not in tainted:
                tainted.add(target)
                changed = True
    return tainted


def validate(root: Path) -> None:
    if not root.is_dir():
        fail(f"source root not found: {root}")
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
            continue
        text = path.read_text(encoding="utf-8")
        for option in LEGACY_OPTIONS:
            if option in text:
                fail(f"{path}: legacy credential logging option is forbidden")
        for match in OUTPUT_CALL.finditer(text):
            call = match.group(0)
            message = joined_literals(call)
            call_without_literals = without_literals(call)
            scope_start = enclosing_scope_start(text, match.start())
            aliases = tainted_aliases(text[scope_start : match.start()])
            call_identifiers = set(IDENTIFIER.findall(call_without_literals))
            if (
                SENSITIVE_IDENTIFIER.search(call_without_literals)
                or call_identifiers & aliases
                or (FORMAT_VALUE.search(message) and SENSITIVE_WORD.search(message))
            ):
                fail(f"{path}: credential-bearing output is forbidden")


validate(Path(sys.argv[1]))
print("V2 credential logging policy passed")
PY2
