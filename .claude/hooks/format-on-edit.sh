#!/usr/bin/env bash
# PostToolUse(Write|Edit) hook: format the file Claude just touched.
# Dispatches by path/extension; each formatter is skipped if not installed,
# so this is a safe no-op until the repo toolchain is present.
# Never fails the tool call (always exits 0).
set -u

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Resolve the clang-format the GATE uses, which is not apt's. check-format.sh
# does a bare PATH lookup, and .github/workflows/quality.yml sources ESP-IDF's
# export.sh immediately before ./scripts/check-all.sh -- so CI formats and
# checks with esp-clang's clang-format (LLVM 19.1.2). Measured at a68f6dc: of
# 314 first-party .c/.h files, 0 are dirty under esp-clang 19 and 2 are dirty
# under apt 18. The tree is 19-formatted.
#
# Prefer esp-clang explicitly so this hook agrees with CI even in a shell that
# never sourced export.sh; fall back to PATH when the toolchain is absent, which
# keeps the hook a no-op on a machine without ESP-IDF rather than a source of
# wrong-version reformatting.
clang_format_bin() {
	local candidate
	for candidate in "$HOME"/.espressif/tools/esp-clang/*/esp-clang/bin/clang-format; do
		if [ -x "$candidate" ]; then
			printf '%s\n' "$candidate"
			return 0
		fi
	done
	command -v clang-format 2>/dev/null
}
f="$(jq -r '.tool_response.filePath // .tool_input.file_path // empty' 2>/dev/null)"
[ -z "$f" ] && exit 0
[ -f "$f" ] || exit 0

# Normalize to a path relative to the repo root for matching.
case "$f" in
"$repo_root"/*) rel="${f#"$repo_root"/}" ;;
*) rel="$f" ;;
esac

case "$rel" in
firmware/*.c | firmware/*.h | firmware/*.inc | tests/host/*.c | tests/host/*.h | tests/host/*.inc | tests/v2_contracts/*.c | tests/v2_contracts/*.h)
	formatter="$(clang_format_bin)"
	[ -n "$formatter" ] && "$formatter" -i "$f"
	;;
scripts/*.sh)
	command -v shfmt >/dev/null 2>&1 && shfmt -w "$f"
	;;
webapp/*)
	# Prettier only handles types it knows; --ignore-unknown makes other files a no-op.
	bin="$repo_root/webapp/node_modules/.bin/prettier"
	[ -x "$bin" ] && "$bin" --write --ignore-unknown "$f"
	;;
esac

exit 0
