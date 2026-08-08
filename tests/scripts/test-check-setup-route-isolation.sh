#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-setup-route-isolation.sh"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

pass_count=0

write_valid_fixture() {
	cat >"${temporary_dir}/routes.c" <<'SOURCE'
static const httpd_uri_t normal_routes[] = {
    {.uri = "/api/v1/status", .method = HTTP_GET, .handler = status_handler},
    {.uri = "/api/v1/auth/login", .method = HTTP_POST, .handler = login_handler},
    {.uri = "/*", .method = HTTP_GET, .handler = static_handler},
};

static const httpd_uri_t setup_routes[] = {
    {.uri = "/api/v1/setup", .method = HTTP_GET, .handler = setup_state_handler},
    {.uri = "/api/v1/setup", .method = HTTP_POST, .handler = setup_submit_handler},
    {.uri = "/*", .method = HTTP_GET, .handler = static_handler},
};
SOURCE
}

expect_pass() {
	local name="$1"
	if ! bash "${checker}" "${temporary_dir}/routes.c" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1"
	local expected="$2"
	if bash "${checker}" "${temporary_dir}/routes.c" >"${temporary_dir}/output" 2>&1; then
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
expect_pass 'V2 isolated route tables'

write_valid_fixture
sed -i '/setup_submit_handler/a\    {.uri = "/api/v1/auth/login", .method = HTTP_POST, .handler = login_handler},' \
	"${temporary_dir}/routes.c"
expect_fail 'normal route in setup' 'setup route table mismatch'

write_valid_fixture
sed -i '/setup_submit_handler/d' "${temporary_dir}/routes.c"
expect_fail 'missing setup POST' 'setup route table mismatch'

write_valid_fixture
sed -i '/setup_state_handler/a\    {.uri = "/api/v1/setup", .method = HTTP_GET, .handler = setup_state_handler},' \
	"${temporary_dir}/routes.c"
expect_fail 'duplicate setup method' 'duplicate URI/method pair'

write_valid_fixture
sed -i '/normal_routes\[\]/,/^};/ s#"/api/v1/status"#"/api/v1/setup"#' \
	"${temporary_dir}/routes.c"
expect_fail 'setup route in normal table' 'setup route exposed in normal route table'

write_valid_fixture
sed -i 's#"/api/v1/setup", .method = HTTP_POST#"/api/v1/setup/credentials", .method = HTTP_POST#' \
	"${temporary_dir}/routes.c"
expect_fail 'retired setup route' 'setup route table mismatch'

printf 'check-setup-route-isolation regression tests passed: %d\n' "${pass_count}"
