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
void log_setup_code(const char *setup_code) {
    ESP_LOGW(TAG, "setup code: %s", setup_code);
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
expect_pass 'single V2 serial setup code'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/ordinary.c" <<'SOURCE'
void leak(const char *password) {
    ESP_LOGI(TAG, "administrator password: %s", password);
}
SOURCE
expect_fail 'ordinary credential log' 'credential-bearing output is forbidden'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/ordinary.c" <<'SOURCE'
void leak(const char *setup_code) {
    printf("setup code: %s", setup_code);
}
SOURCE
expect_fail 'setup code outside approved source' 'credential-bearing output is forbidden'

write_valid_fixture
cat >>"${temporary_dir}/firmware/components/app_core/app_core.c" <<'SOURCE'
void leak_ap(const char *passphrase) {
    ESP_LOGW(TAG, "AP passphrase: %s", passphrase);
}
SOURCE
expect_fail 'AP passphrase in approved source' 'unapproved credential-bearing output'

write_valid_fixture
cat >>"${temporary_dir}/firmware/components/app_core/app_core.c" <<'SOURCE'
void duplicate(const char *setup_code) {
    ESP_LOGW(TAG, "setup code: %s", setup_code);
}
SOURCE
expect_fail 'duplicate setup code output' 'expected exactly one serial setup-code output'

write_valid_fixture
sed -i '/ESP_LOGW/d' "${temporary_dir}/firmware/components/app_core/app_core.c"
expect_fail 'missing setup code output' 'expected exactly one serial setup-code output'

write_valid_fixture
printf '%s\n' 'CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG' \
	>"${temporary_dir}/firmware/components/legacy.h"
expect_fail 'legacy development option' 'legacy credential logging option is forbidden'

printf 'check-credential-logging regression tests passed: %d\n' "${pass_count}"
