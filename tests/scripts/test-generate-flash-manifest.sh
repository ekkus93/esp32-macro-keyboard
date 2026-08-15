#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly generator="${repo_root}/scripts/generate-flash-manifest.sh"
readonly fakes_dir="${repo_root}/tests/scripts/fakes"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

export PATH="${fakes_dir}:${PATH}"
export IDF_PATH="${fakes_dir}/fake_idf_path"

pass_count=0

readonly build_dir="${temporary_dir}/build"
readonly sdkconfig="${temporary_dir}/sdkconfig"
readonly component_lock="${temporary_dir}/dependencies.lock"
readonly frontend_lock="${temporary_dir}/package-lock.json"
readonly output_file="${temporary_dir}/flash-manifest.json"

common_args() {
	printf '%s\n' \
		--build-dir "${build_dir}" \
		--sdkconfig "${sdkconfig}" \
		--component-lock "${component_lock}" \
		--frontend-lock "${frontend_lock}" \
		--output "${output_file}"
}

write_fixtures() {
	local production="$1"
	rm -rf -- "${build_dir}"
	mkdir -p -- "${build_dir}/partition_table"
	cat >"${build_dir}/flasher_args.json" <<'JSON'
{
    "flash_settings": {"flash_mode": "dio", "flash_size": "8MB", "flash_freq": "80m"},
    "flash_files": {
        "0x0": "bootloader/bootloader.bin",
        "0x20000": "esp32_macro_keyboard.bin",
        "0x8000": "partition_table/partition-table.bin",
        "0xf000": "ota_data_initial.bin"
    }
}
JSON
	mkdir -p -- "${build_dir}/bootloader"
	printf 'fixture bootloader\n' >"${build_dir}/bootloader/bootloader.bin"
	: >"${build_dir}/partition_table/partition-table.bin"
	printf 'fixture ota data\n' >"${build_dir}/ota_data_initial.bin"
	printf 'fixture application image\n' >"${build_dir}/esp32_macro_keyboard.bin"
	if [ "${production}" = "production" ]; then
		printf '# CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG is not set\n' >"${sdkconfig}"
		printf '# CONFIG_APP_MANUFACTURING_PROVISIONING_LOG is not set\n' >>"${sdkconfig}"
		printf 'CONFIG_APP_RETRIEVE_LEN_ELF_SHA=39\n' >>"${sdkconfig}"
	else
		printf 'CONFIG_APP_MANUFACTURING_PROVISIONING_LOG=y\n' >"${sdkconfig}"
		printf 'CONFIG_APP_RETRIEVE_LEN_ELF_SHA=39\n' >>"${sdkconfig}"
	fi
	printf 'fixture component lock\n' >"${component_lock}"
	printf 'fixture frontend lock\n' >"${frontend_lock}"
	rm -f -- "${output_file}"
}

expect_pass() {
	local name="$1"
	shift
	local -a args
	mapfile -t args < <(common_args)
	if ! bash "${generator}" "${args[@]}" "$@" >"${temporary_dir}/output" 2>&1; then
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
	local -a args
	mapfile -t args < <(common_args)
	if bash "${generator}" "${args[@]}" "$@" >"${temporary_dir}/output" 2>&1; then
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

field() {
	python3 -c "import json,sys; print(json.load(open(sys.argv[1]))[sys.argv[2]])" \
		"${output_file}" "$1"
}

# Happy path, no webfs image present: the manifest is written with every
# required SPEC §23 field and the four ESP-IDF flash files, unchanged from
# flasher_args.json.
write_fixtures production
expect_pass 'happy path, no webfs image'
[ "$(field buildType)" = "production" ] || {
	printf 'FAIL: expected buildType=production\n' >&2
	exit 1
}
[ "$(field espIdfVersion)" = "ESP-IDF v5.5.5" ] || {
	printf 'FAIL: unexpected espIdfVersion: %s\n' "$(field espIdfVersion)" >&2
	exit 1
}
[ "$(python3 -c "import json; print(len(json.load(open('${output_file}'))['gitCommit']))")" = 40 ] || {
	printf 'FAIL: gitCommit is not a 40-character SHA\n' >&2
	exit 1
}
[ "$(field appImage)" = "esp32_macro_keyboard.bin" ] || {
	printf 'FAIL: unexpected appImage: %s\n' "$(field appImage)" >&2
	exit 1
}
[ "$(field appElfSha256)" = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" ] || {
	printf 'FAIL: unexpected appElfSha256: %s\n' "$(field appElfSha256)" >&2
	exit 1
}
[ "$(field diagnosticsBuildId)" = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" ] || {
	printf 'FAIL: unexpected diagnosticsBuildId: %s\n' "$(field diagnosticsBuildId)" >&2
	exit 1
}
[ "$(python3 -c "import json,re; value=json.load(open('${output_file}'))['appImageSha256']; print(bool(re.fullmatch(r'[0-9a-f]{64}', value)))")" = True ] || {
	printf 'FAIL: appImageSha256 is not a full lowercase SHA-256\n' >&2
	exit 1
}
[ "$(python3 -c "import json; print('webfs.bin' in json.load(open('${output_file}'))['flashFiles'].values())")" = "False" ] || {
	printf 'FAIL: webfs.bin listed without a webfs image present\n' >&2
	exit 1
}
[ "$(python3 -c "import json; print(len(json.load(open('${output_file}'))['flashFileSha256']))")" = "4" ] || {
	printf 'FAIL: expected SHA-256 entries for all four flash files\n' >&2
	exit 1
}
[ "$(python3 -c "import json; print(json.load(open('${output_file}'))['flashFileSha256']['0x20000'])")" = "$(field appImageSha256)" ] || {
	printf 'FAIL: application flash hash did not match appImageSha256\n' >&2
	exit 1
}

# A manufacturing-provisioning-log build must be recorded as "development",
# not silently reported as production.
write_fixtures development
expect_pass 'development build type'
[ "$(field buildType)" = "development" ] || {
	printf 'FAIL: expected buildType=development, got %s\n' "$(field buildType)" >&2
	exit 1
}

# When scripts/build-webfs-image.sh has already produced a webfs.bin, the
# manifest must include it at its real resolved partition offset.
write_fixtures production
: >"${build_dir}/webfs.bin"
FAKE_WEBFS_OFFSET=0x520000 expect_pass 'webfs image present'
[ "$(python3 -c "import json; print(json.load(open('${output_file}'))['flashFiles'].get('0x520000'))")" = "webfs.bin" ] || {
	printf 'FAIL: webfs.bin not recorded at the resolved offset\n' >&2
	exit 1
}
[ "$(python3 -c "import json; print(json.load(open('${output_file}'))['flashFileSha256'].get('0x520000'))")" = "$(sha256sum "${build_dir}/webfs.bin" | awk '{print $1}')" ] || {
	printf 'FAIL: webfs.bin hash not recorded at the resolved offset\n' >&2
	exit 1
}

# A lockfile hash must actually reflect the lockfile's real content, not a
# placeholder - changing the fixture must change the recorded hash.
write_fixtures production
expect_pass 'baseline lock hash'
baseline_hash="$(field managedComponentLockSha256)"
printf 'different fixture content\n' >"${component_lock}"
expect_pass 'changed lock hash'
[ "$(field managedComponentLockSha256)" != "${baseline_hash}" ] || {
	printf 'FAIL: managedComponentLockSha256 did not change with lockfile content\n' >&2
	exit 1
}

# The board-visible diagnostics build ID must be the exact 39-character prefix
# produced by esp_app_get_elf_sha256 with the production diagnostics buffer.
write_fixtures production
FAKE_ELF_SHA256=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef expect_pass 'ELF build ID derivation'
[ "$(field diagnosticsBuildId)" = "0123456789abcdef0123456789abcdef0123456" ] || {
	printf 'FAIL: diagnosticsBuildId did not match the ELF SHA prefix\n' >&2
	exit 1
}

write_fixtures production
sed -i 's/CONFIG_APP_RETRIEVE_LEN_ELF_SHA=39/CONFIG_APP_RETRIEVE_LEN_ELF_SHA=9/' "${sdkconfig}"
expect_fail 'short resolved diagnostics build ID' 'CONFIG_APP_RETRIEVE_LEN_ELF_SHA=39'

write_fixtures production
rm -f -- "${build_dir}/esp32_macro_keyboard.bin"
expect_fail 'missing application image' 'application image not found'

write_fixtures production
rm -f -- "${build_dir}/bootloader/bootloader.bin"
expect_fail 'missing flash file' 'flash file not found'

write_fixtures production
FAKE_ELF_SHA256=not-a-sha expect_fail 'invalid ELF SHA' 'did not report a full ELF file SHA256'

# Missing required build artifacts must fail closed with a clear message,
# not a bare Python traceback.
write_fixtures production
rm -f -- "${build_dir}/flasher_args.json"
expect_fail 'missing flasher_args.json' 'flasher_args.json not found'

write_fixtures production
rm -f -- "${sdkconfig}"
expect_fail 'missing sdkconfig' "${sdkconfig} not found"

write_fixtures production
rm -f -- "${component_lock}"
expect_fail 'missing component lockfile' 'managed-component lockfile not found'

write_fixtures production
rm -f -- "${frontend_lock}"
expect_fail 'missing frontend lockfile' 'frontend lockfile not found'

# idf.py missing from PATH entirely must fail closed with the exact
# export.sh guidance, not a bare "command not found". Every other required
# artifact check runs first, so this must be the only thing removed.
write_fixtures production
if PATH="/usr/bin:/bin" IDF_PATH="${IDF_PATH}" bash "${generator}" \
	--build-dir "${build_dir}" --sdkconfig "${sdkconfig}" \
	--component-lock "${component_lock}" --frontend-lock "${frontend_lock}" \
	--output "${output_file}" >"${temporary_dir}/output" 2>&1; then
	printf 'FAIL: missing idf.py unexpectedly passed\n' >&2
	cat -- "${temporary_dir}/output" >&2
	exit 1
fi
if ! grep -F -- 'source the pinned ESP-IDF v5.5.5 export.sh first' "${temporary_dir}/output" >/dev/null; then
	printf 'FAIL: missing idf.py did not report the export.sh guidance\n' >&2
	cat -- "${temporary_dir}/output" >&2
	exit 1
fi
pass_count=$((pass_count + 1))

printf 'generate-flash-manifest regression tests passed: %d\n' "${pass_count}"
