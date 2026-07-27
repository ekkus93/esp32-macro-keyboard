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
static void log_credentials(const char *ssid,
                            const char *passphrase,
                            const char *setup_code) {
#if CONFIG_APP_MANUFACTURING_PROVISIONING_LOG
    ESP_LOGE(TAG,
             "MANUFACTURING MODE ENABLED: plaintext one-time credentials follow; "
             "never deploy this build");
    ESP_LOGW(TAG, "manufacturing-only AP SSID: %s", ssid);
    ESP_LOGW(TAG, "manufacturing-only AP passphrase: %s", passphrase);
    ESP_LOGW(TAG, "manufacturing-only setup code: %s", setup_code);
#else
    ESP_LOGE(TAG, "credential log event rejected outside manufacturing mode");
#endif
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
expect_pass 'guarded manufacturing output'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/ordinary.c" <<'SOURCE'
int encode_state(char *output, unsigned long output_size, const char *ssid) {
    return snprintf(output, output_size, "{\"apSsid\":\"%s\"}", ssid);
}
SOURCE
expect_pass 'snprintf state encoding is not a log sink'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/ordinary.c" <<'SOURCE'
void leak(const char *password) {
    ESP_LOGI(TAG, "administrator password: %s", password);
}
SOURCE
expect_fail 'ordinary credential log' 'credential-bearing output is forbidden'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/ordinary.c" <<'SOURCE'
void leak(const char *token) {
    printf("session token: %s", token);
}
SOURCE
expect_fail 'printf token leak' 'credential-bearing output is forbidden'

write_valid_fixture
printf '%s\n' 'CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG' \
	>"${temporary_dir}/firmware/components/legacy.h"
expect_fail 'legacy option' 'legacy credential logging option is forbidden'

write_valid_fixture
sed -i '/#if CONFIG_APP_MANUFACTURING_PROVISIONING_LOG/d' \
	"${temporary_dir}/firmware/components/app_core/app_core.c"
expect_fail 'missing guard' 'missing manufacturing credential guard'

write_valid_fixture
sed -i 's/MANUFACTURING MODE ENABLED/FACTORY MODE/' \
	"${temporary_dir}/firmware/components/app_core/app_core.c"
expect_fail 'missing warning' 'missing permanent manufacturing warning banner'

write_valid_fixture
sed -i 's/manufacturing-only setup code/manufacturing-only API token/' \
	"${temporary_dir}/firmware/components/app_core/app_core.c"
expect_fail 'unapproved message' 'unapproved manufacturing credential output'

write_valid_fixture
printf '%s\n' 'ESP_LOGI(TAG, "setup code: %s", setup_code);' \
	>>"${temporary_dir}/firmware/components/app_core/app_core.c"
expect_fail 'output outside guard' 'credential-bearing output exists outside manufacturing guard'

printf 'check-credential-logging regression tests passed: %d\n' "${pass_count}"
