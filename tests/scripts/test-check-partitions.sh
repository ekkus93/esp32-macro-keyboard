#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-partitions.sh"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

pass_count=0

write_fixture() {
	local body="$1"
	printf '%s\n' "${body}" >"${temporary_dir}/partitions.csv"
}

expect_pass() {
	local name="$1"
	if ! "${checker}" "${temporary_dir}/partitions.csv" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1"
	local expected="$2"
	if "${checker}" "${temporary_dir}/partitions.csv" >"${temporary_dir}/output" 2>&1; then
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

valid_table='# Name, Type, SubType, Offset, Size, Flags
nvs, data, nvs, 0x9000, 0x6000,
otadata, data, ota, 0xf000, 0x2000,
phy_init, data, phy, 0x11000, 0x1000,
nvs_keys, data, nvs_keys, 0x12000, 0x1000, encrypted
ota_0, app, ota_0, 0x20000, 0x280000,
ota_1, app, ota_1, , 0x280000,
webfs, data, littlefs, , 0x100000,
userdata, data, littlefs, , 0x80000,
coredump, data, coredump, , 0x10000,'

write_fixture "${valid_table}"
expect_pass 'valid table'

write_fixture "${valid_table}"$'\nextra_keys, data, nvs_keys, , 0x1000, encrypted'
expect_fail 'duplicate key partition' 'exactly one data/nvs_keys partition'

write_fixture "${valid_table/nvs_keys, data, nvs_keys, 0x12000, 0x1000, encrypted/ordinary, data, undefined, 0x12000, 0x1000,}"
expect_fail 'missing key partition' 'exactly one data/nvs_keys partition'

write_fixture "${valid_table/0x12000, 0x1000, encrypted/0x12000, 0x2000, encrypted}"
expect_fail 'wrong key size' 'exactly 0x1000 bytes'

write_fixture "${valid_table/0x12000, 0x1000, encrypted/0x12000, 0x1000,}"
expect_fail 'missing encrypted flag' 'must include the encrypted flag'

write_fixture "${valid_table/ota_0, app, ota_0, 0x20000/ota_0, app, ota_0, 0x21000}"
expect_fail 'misaligned app' 'is not aligned to 0x10000'

write_fixture "${valid_table/ota_0, app, ota_0, 0x20000/ota_0, app, ota_0, 0x10000}"
expect_fail 'overlap' 'overlaps the previous partition'

write_fixture "${valid_table/ota_1, app, ota_1, , 0x280000/ota_1, app, ota_1, , 0x270000}"
expect_fail 'unequal OTA slots' 'must have equal sizes'

write_fixture "${valid_table/coredump, data, coredump, , 0x10000,/coredump, data, coredump, 0x7f0000, 0x20000,}"
expect_fail 'flash overflow' 'beyond 8 MiB flash'

printf 'check-partitions regression tests passed: %d\n' "${pass_count}"
