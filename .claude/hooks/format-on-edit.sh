#!/usr/bin/env bash
# PostToolUse(Write|Edit) hook: format the file Claude just touched.
# Dispatches by path/extension; each formatter is skipped if not installed,
# so this is a safe no-op until the repo toolchain is present.
# Never fails the tool call (always exits 0).
set -u

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
f="$(jq -r '.tool_response.filePath // .tool_input.file_path // empty' 2>/dev/null)"
[ -z "$f" ] && exit 0
[ -f "$f" ] || exit 0

# Normalize to a path relative to the repo root for matching.
case "$f" in
"$repo_root"/*) rel="${f#"$repo_root"/}" ;;
*) rel="$f" ;;
esac

case "$rel" in
firmware/*.c | firmware/*.h | tests/host/*.c | tests/host/*.h)
	# Absolute path, not PATH lookup: sourcing ESP-IDF's export.sh puts
	# esp-clang's clang-format (LLVM 19.1.2) ahead of apt's 18.1.3, and CI
	# checks with 18. Formatting here with 19 would leave the file failing
	# check-format.sh -- reformatted by the hook, rejected by the gate.
	[ -x /usr/bin/clang-format ] && /usr/bin/clang-format -i "$f"
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
