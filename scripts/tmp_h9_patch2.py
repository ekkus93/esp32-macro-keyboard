from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one anchor, found {count}: {old[:80]!r}")
    target.write_text(text.replace(old, new, 1))


# Make the permanent credential-output policy match the current manufacturing-label design:
# no plaintext credential value, including setup code, is allowed in ordinary firmware output.
Path("scripts/check-credential-logging.sh").write_text(r'''#!/usr/bin/env bash
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
OUTPUT_CALL = re.compile(
    rf"(?<![A-Za-z0-9_])(?:ESP_LOG[A-Z]+|printf|fprintf)\s*\((?:{STRING_LITERAL_SOURCE}|[^\";])*\);",
    re.DOTALL,
)
SENSITIVE_WORD = re.compile(
    r"(?:password|passphrase|setup[_ -]?code|(?:session|csrf|api|access)[_ -]?token|salt|verifier)",
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
            message = joined_literals(match.group(0))
            if SENSITIVE_WORD.search(message) and FORMAT_VALUE.search(message):
                fail(f"{path}: credential-bearing output is forbidden")


validate(Path(sys.argv[1]))
print("V2 credential logging policy passed")
PY2
''')

Path("tests/scripts/test-check-credential-logging.sh").write_text(r'''#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-credential-logging.sh"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

pass_count=0

write_valid_fixture() {
	rm -rf -- "${temporary_dir}/firmware"
	mkdir -p -- "${temporary_dir}/firmware/components/app_core"
	cat >"${temporary_dir}/firmware/components/app_core/app_core.c" <<'SOURCE'
void report_setup_ready(void) {
    ESP_LOGI(TAG, "setup credentials are available from the manufacturing label");
}
SOURCE
}

expect_pass() {
	local name="$1"
	if ! bash "${checker}" "${temporary_dir}/firmware" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1"
	local expected="$2"
	if bash "${checker}" "${temporary_dir}/firmware" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly passed\n' "${name}" >&2
		exit 1
	fi
	if ! grep -F -- "${expected}" "${temporary_dir}/output" >/dev/null; then
		printf 'FAIL: %s did not report %s\n' "${name}" "${expected}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

write_valid_fixture
expect_pass 'non-secret setup readiness log'

for credential in password passphrase setup_code session_token salt verifier; do
	write_valid_fixture
	cat >"${temporary_dir}/firmware/components/leak.c" <<SOURCE
void leak(const char *value) {
    ESP_LOGW(TAG, "${credential}: %s", value);
}
SOURCE
	expect_fail "${credential} output" 'credential-bearing output is forbidden'
done

write_valid_fixture
printf '%s\n' 'CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG' \
	>"${temporary_dir}/firmware/components/legacy.h"
expect_fail 'legacy development option' 'legacy credential logging option is forbidden'

printf 'check-credential-logging regression tests passed: %d\n' "${pass_count}"
''')

replace_once(
    "scripts/check-scripts.sh",
    "python3 scripts/check-web-route-dispatch-sync.py\npython3 scripts/check-v2-auth-policy.py\n",
    "python3 scripts/check-web-route-dispatch-sync.py\npython3 scripts/check-h9-architecture.py\npython3 scripts/check-v2-auth-policy.py\n",
)
