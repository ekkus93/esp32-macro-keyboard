#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-h9-production-audit.py"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

pass_count=0

reset_fixture() {
	rm -rf -- "${temporary_dir}/tree"
	mkdir -p -- "${temporary_dir}/tree/firmware/components/demo" \
		"${temporary_dir}/tree/firmware/main" \
		"${temporary_dir}/tree/webapp/src/v2"
	cat >"${temporary_dir}/tree/firmware/components/demo/demo.c" <<'SOURCE'
int safe(void) {
    return 0;
}
SOURCE
	cat >"${temporary_dir}/tree/webapp/src/v2/demo.ts" <<'SOURCE'
export function safe(): number {
  return 0;
}
SOURCE
}

expect_pass() {
	local name="$1"
	if ! python3 "${checker}" --root "${temporary_dir}/tree" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1"
	local expected="$2"
	if python3 "${checker}" --root "${temporary_dir}/tree" >"${temporary_dir}/output" 2>&1; then
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

reset_fixture
expect_pass 'clean production fixture'

reset_fixture
cat >>"${temporary_dir}/tree/webapp/src/v2/demo.ts" <<'SOURCE'
try {
  safe();
} catch {}
SOURCE
expect_fail 'empty catch' 'empty catch is forbidden'

reset_fixture
cat >>"${temporary_dir}/tree/webapp/src/v2/demo.ts" <<'SOURCE'
Promise.resolve().catch(() => {});
SOURCE
expect_fail 'empty promise catch' 'empty Promise rejection handler is forbidden'

reset_fixture
cat >>"${temporary_dir}/tree/firmware/components/demo/demo.c" <<'SOURCE'
/* best-effort cleanup */
SOURCE
expect_fail 'best-effort marker' 'best-effort marker requires explicit H9 classification'

reset_fixture
cat >>"${temporary_dir}/tree/webapp/src/v2/demo.ts" <<'SOURCE'
// fallback to stale state
SOURCE
expect_fail 'unclassified fallback' 'new or changed fallback occurrence requires H9 classification'

reset_fixture
cat >>"${temporary_dir}/tree/webapp/src/v2/demo.ts" <<'SOURCE'
// timer failure falls back to immediate restart
SOURCE
expect_fail 'unclassified falls-back wording' 'new or changed fallback occurrence requires H9 classification'

reset_fixture
cat >>"${temporary_dir}/tree/firmware/components/demo/demo.c" <<'SOURCE'
void discard_result(void) {
    (void)dangerous_operation();
}
SOURCE
expect_fail 'unclassified discarded C result' 'new or changed explicit discarded C call requires H9 classification'

printf 'H9 production audit regression tests passed: %d\n' "${pass_count}"
