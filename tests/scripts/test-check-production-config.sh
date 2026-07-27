#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-production-config.sh"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

pass_count=0

write_fixture() {
	local body="$1"
	printf '%s\n' "${body}" >"${temporary_dir}/sdkconfig.defaults"
}

expect_pass() {
	local name="$1"
	if ! bash "${checker}" "${temporary_dir}/sdkconfig.defaults" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1"
	local expected="$2"
	if bash "${checker}" "${temporary_dir}/sdkconfig.defaults" >"${temporary_dir}/output" 2>&1; then
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

valid_config='CONFIG_IDF_TARGET="esp32s3"
CONFIG_NVS_ENCRYPTION=y
CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC=y
CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID=0
# CONFIG_APP_MANUFACTURING_PROVISIONING_LOG is not set'

write_fixture "${valid_config}"
expect_pass 'valid HMAC configuration'

write_fixture "${valid_config/CONFIG_NVS_ENCRYPTION=y/}"
expect_fail 'missing NVS encryption' 'CONFIG_NVS_ENCRYPTION must be'

write_fixture "${valid_config/CONFIG_NVS_ENCRYPTION=y/# CONFIG_NVS_ENCRYPTION is not set}"
expect_fail 'disabled NVS encryption' "found 'n'"

write_fixture "${valid_config/CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC=y/}"
expect_fail 'missing HMAC scheme' 'CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC must be'

write_fixture "${valid_config}"$'\nCONFIG_NVS_SEC_KEY_PROTECT_USING_FLASH_ENC=y'
expect_fail 'conflicting flash scheme' 'conflicts with the required HMAC NVS scheme'

write_fixture "${valid_config}"$'\nCONFIG_NVS_SEC_KEY_PROTECT_NONE=y'
expect_fail 'unprotected scheme' 'conflicts with the required HMAC NVS scheme'

invalid_key_config="${valid_config/CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID=0/"
invalid_key_config+="CONFIG_NVS_SEC_HMAC_EFUSE_KEY_ID=6}"
write_fixture "${invalid_key_config}"
expect_fail 'invalid key block' "must be '0'"

write_fixture "${valid_config}"$'\nCONFIG_NVS_ENCRYPTION=y'
expect_fail 'duplicate setting' 'duplicate setting CONFIG_NVS_ENCRYPTION'

manufacturing_config="${valid_config/# CONFIG_APP_MANUFACTURING_PROVISIONING_LOG is not set/"
manufacturing_config+="CONFIG_APP_MANUFACTURING_PROVISIONING_LOG=y}"
write_fixture "${manufacturing_config}"
expect_fail 'manufacturing credential logging' 'is forbidden in production configuration'

write_fixture "${valid_config}"$'\nCONFIG_APP_DEVELOPMENT_PROVISIONING_LOG=y'
expect_fail 'legacy credential logging' 'is forbidden in production configuration'

printf 'check-production-config regression tests passed: %d\n' "${pass_count}"
