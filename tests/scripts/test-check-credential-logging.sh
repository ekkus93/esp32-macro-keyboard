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


write_valid_fixture
cat >"${temporary_dir}/firmware/components/dynamic_format_leak.c" <<'SOURCE'
void leak_dynamic_format(const char *setup_code) {
    const char *format = "%s";
    ESP_LOGW(TAG, format, setup_code);
}
SOURCE
expect_fail 'dynamic format sensitive identifier' 'credential-bearing output is forbidden'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/aliased_leak.c" <<'SOURCE'
void leak_alias(const char *session_token) {
    const char *value = session_token;
    ESP_LOGI(TAG, "%s", value);
}
SOURCE
expect_fail 'one-hop sensitive alias' 'credential-bearing output is forbidden'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/puts_leak.c" <<'SOURCE'
void leak_puts(const char *password) {
    puts(password);
}
SOURCE
expect_fail 'puts sensitive identifier' 'credential-bearing output is forbidden'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/buffer_dump_leak.c" <<'SOURCE'
void leak_buffer(const unsigned char *verifier, unsigned length) {
    ESP_LOG_BUFFER_HEX(TAG, verifier, length);
}
SOURCE
expect_fail 'ESP log buffer sensitive identifier' 'credential-bearing output is forbidden'


write_valid_fixture
cat >"${temporary_dir}/firmware/components/direct_log_write_leak.c" <<'SOURCE'
void leak_direct_log_write(const char *session_token) {
    esp_log_write(ESP_LOG_WARN, TAG, "%s", session_token);
}
SOURCE
expect_fail 'direct esp_log_write sensitive identifier' 'credential-bearing output is forbidden'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/direct_log_buffer_leak.c" <<'SOURCE'
void leak_direct_log_buffer(const unsigned char *verifier, unsigned length) {
    esp_log_buffer_hex(TAG, verifier, length);
}
SOURCE
expect_fail 'direct esp_log_buffer sensitive identifier' 'credential-bearing output is forbidden'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/rom_printf_leak.c" <<'SOURCE'
void leak_rom_printf(const char *setup_code) {
    esp_rom_printf("%s", setup_code);
}
SOURCE
expect_fail 'ROM printf sensitive identifier' 'credential-bearing output is forbidden'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/formatted_buffer_leak.c" <<'SOURCE'
void leak_formatted_buffer(const char *password) {
    char message[128];
    snprintf(message, sizeof(message), "credential=%s", password);
    ESP_LOGI(TAG, "%s", message);
}
SOURCE
expect_fail 'formatted-buffer secret laundering' 'credential-bearing output is forbidden'

write_valid_fixture
cat >"${temporary_dir}/firmware/components/copied_buffer_leak.c" <<'SOURCE'
void leak_copied_buffer(const char *session_token) {
    char message[128];
    memcpy(message, session_token, 16U);
    message[16] = '\0';
    ESP_LOGI(TAG, "%s", message);
}
SOURCE
expect_fail 'copied-buffer secret laundering' 'credential-bearing output is forbidden'

printf 'check-credential-logging regression tests passed: %d\n' "${pass_count}"
