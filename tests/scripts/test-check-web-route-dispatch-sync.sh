#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-web-route-dispatch-sync.py"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

copy_fixtures() {
	cp -- "${repo_root}/firmware/components/web_server/web_server_lifecycle.c" \
		"${temporary_dir}/lifecycle.c"
	cp -- "${repo_root}/firmware/components/web_server/web_api_administration.c" \
		"${temporary_dir}/administration.c"
	cp -- "${repo_root}/firmware/components/web_server/web_api_core.h" \
		"${temporary_dir}/core.h"
}

run_checker() {
	python3 "${checker}" "${temporary_dir}/lifecycle.c" \
		"${temporary_dir}/administration.c" "${temporary_dir}/core.h"
}

expect_pass() {
	local name="$1"
	if ! run_checker >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
}

expect_fail() {
	local name="$1"
	local expected="$2"
	if run_checker >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly passed\n' "${name}" >&2
		exit 1
	fi
	if ! grep -F -- "${expected}" "${temporary_dir}/output" >/dev/null; then
		printf 'FAIL: %s did not report %s\n' "${name}" "${expected}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
}

copy_fixtures
expect_pass 'current route/dispatch partition'

copy_fixtures
sed -i '/\.uri = "\/api\/v1\/status"/d' "${temporary_dir}/lifecycle.c"
expect_fail 'dedicated route removed' 'dedicated route table mismatch'

copy_fixtures
sed -i \
	's/DEFINE_RESET_GUARDED_HANDLER(reset_guarded_status_handler, status_handler)/DEFINE_RESET_GUARDED_HANDLER(reset_guarded_status_handler, limits_handler)/' \
	"${temporary_dir}/lifecycle.c"
expect_fail 'reset guard delegates to wrong handler' 'dedicated route table mismatch'

copy_fixtures
sed -i '/DEFINE_RESET_GUARDED_HANDLER(reset_guarded_status_handler, status_handler)/d' \
	"${temporary_dir}/lifecycle.c"
expect_fail 'registered reset guard has no mapping' \
	'registered reset-guard wrapper has no delegate mapping'

copy_fixtures
sed -i '/case WEB_API_ROUTE_UNKNOWN:/i\    case WEB_API_ROUTE_STATUS:\n        return APP_ERROR_NOT_FOUND;' \
	"${temporary_dir}/administration.c"
expect_fail 'dedicated route duplicated in wildcard dispatch' \
	'dedicated routes also handled by wildcard administration dispatch'

copy_fixtures
sed -i '/WEB_API_ROUTE_SETUP/i\    WEB_API_ROUTE_FUTURE,' "${temporary_dir}/core.h"
expect_fail 'new route left unclassified' 'API route dispatch classification mismatch'

printf 'web route/dispatch synchronization regression tests passed\n'
