#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-release-budgets.sh"
readonly fakes_dir="${repo_root}/tests/scripts/fakes"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

export PYTHONPATH="${fakes_dir}${PYTHONPATH:+:${PYTHONPATH}}"

pass_count=0

readonly build_dir="${temporary_dir}/build"
readonly partitions_csv="${temporary_dir}/partitions.csv"

write_partitions() {
	cat >"${partitions_csv}" <<'CSV'
# Name,      Type, SubType, Offset,   Size,     Flags
nvs,         data, nvs,     0x9000,   0x6000,
otadata,     data, ota,     0xf000,   0x2000,
phy_init,    data, phy,     0x11000,  0x1000,
nvs_keys,    data, nvs_keys,0x12000,  0x1000,   encrypted
ota_0,       app,  ota_0,   0x20000,  0x280000,
ota_1,       app,  ota_1,   ,         0x280000,
webfs,       data, littlefs,,         0x100000,
userdata,    data, littlefs,,         0x80000,
coredump,    data, coredump,,         0x10000,
CSV
}

write_build() {
	local bin_bytes="$1"
	rm -rf -- "${build_dir}"
	mkdir -p -- "${build_dir}"
	head -c "${bin_bytes}" /dev/zero >"${build_dir}/esp32_macro_keyboard.bin"
	: >"${build_dir}/esp32_macro_keyboard.map"
}

expect_pass() {
	local name="$1"
	shift
	if ! bash "${checker}" --build-dir "${build_dir}" --partitions "${partitions_csv}" "$@" \
		>"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1"
	local expected="$2"
	shift 2
	if bash "${checker}" --build-dir "${build_dir}" --partitions "${partitions_csv}" "$@" \
		>"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly passed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	if ! grep -F -- "${expected}" "${temporary_dir}/output" >/dev/null; then
		printf 'FAIL: %s did not report %s\n' "${name}" "${expected}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

# A binary comfortably under the 80% OTA budget, and default (healthy) DIRAM
# usage from the fake esp_idf_size: everything else should pass, and the two
# optional checks (webfs image, stack margin) should be skipped rather than
# fatal when no artifact is given for them.
write_partitions
write_build $((1024 * 1024))
export FAKE_ESP_IDF_SIZE_JSON='{"diram_total": 341760, "used_diram": 113571}'
expect_pass 'healthy build, optional checks skipped'

# A binary over the 80% OTA slot budget must fail closed.
write_build $((2200 * 1024))
export FAKE_ESP_IDF_SIZE_JSON='{"diram_total": 341760, "used_diram": 113571}'
expect_fail 'oversized binary' 'application binary vs. OTA slot budget'

# Static RAM usage over the 75% DIRAM budget must fail closed.
write_build $((1024 * 1024))
export FAKE_ESP_IDF_SIZE_JSON='{"diram_total": 341760, "used_diram": 300000}'
expect_fail 'oversized static RAM' 'static RAM (DIRAM) vs. budget'

# A userdata partition too small to clear the minimum-usable-space floor
# must fail closed, even though nothing else changed.
write_build $((1024 * 1024))
export FAKE_ESP_IDF_SIZE_JSON='{"diram_total": 341760, "used_diram": 113571}'
cat >"${partitions_csv}" <<'CSV'
# Name,      Type, SubType, Offset,   Size,     Flags
nvs,         data, nvs,     0x9000,   0x6000,
otadata,     data, ota,     0xf000,   0x2000,
phy_init,    data, phy,     0x11000,  0x1000,
nvs_keys,    data, nvs_keys,0x12000,  0x1000,   encrypted
ota_0,       app,  ota_0,   0x20000,  0x280000,
ota_1,       app,  ota_1,   ,         0x280000,
webfs,       data, littlefs,,         0x100000,
userdata,    data, littlefs,,         0x10000,
coredump,    data, coredump,,         0x10000,
CSV
expect_fail 'undersized userdata partition' 'userdata minimum free-space requirement'
write_partitions

# A stack report below the 20%-of-configured-size margin must fail closed.
write_build $((1024 * 1024))
export FAKE_ESP_IDF_SIZE_JSON='{"diram_total": 341760, "used_diram": 113571}'
printf '{"controls_stack_words": 50, "executor_stack_words": 3200}' >"${temporary_dir}/stack.json"
expect_fail 'thin controls stack margin' 'controls task stack high-water mark' \
	--stack-report "${temporary_dir}/stack.json"

# A healthy stack report passes explicitly rather than being skipped.
printf '{"controls_stack_words": 1200, "executor_stack_words": 3200}' >"${temporary_dir}/stack.json"
expect_pass 'healthy stack report' --stack-report "${temporary_dir}/stack.json"

# Missing required build artifacts must fail closed with a clear message.
rm -rf -- "${build_dir}"
mkdir -p -- "${build_dir}"
expect_fail 'missing application binary' 'application binary not found'

printf 'check-release-budgets regression tests passed: %d\n' "${pass_count}"
