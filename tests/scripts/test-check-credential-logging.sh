#!/usr/bin/env bash
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
cat >"${temporary_dir}/firmware/components/generic_leak.c" <<'SOURCE'
void leak_setup(const char *setup_code) {
    ESP_LOGW(TAG, "%s", setup_code);
}
SOURCE
expect_fail 'generic format sensitive identifier' 'credential-bearing output is forbidden'

write_valid_fixture
printf '%s\n' 'CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG' \
	>"${temporary_dir}/firmware/components/legacy.h"
expect_fail 'legacy development option' 'legacy credential logging option is forbidden'

printf 'check-credential-logging regression tests passed: %d\n' "${pass_count}"
