#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly builder="${repo_root}/scripts/build-userdata-image.sh"
readonly fakes_dir="${repo_root}/tests/scripts/fakes"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

export PATH="${fakes_dir}:${PATH}"

pass_count=0

readonly build_dir="${temporary_dir}/build"
readonly partitions_csv="${temporary_dir}/partitions.csv"
readonly args_file="${temporary_dir}/littlefs_args"

write_partitions() {
	cat >"${partitions_csv}" <<'CSV'
# Name,      Type, SubType, Offset,   Size,     Flags
nvs,         data, nvs,     0x9000,   0x6000,
ota_0,       app,  ota_0,   0x20000,  0x280000,
ota_1,       app,  ota_1,   ,         0x280000,
webfs,       data, littlefs,,         0x100000,
userdata,    data, littlefs,,         0x80000,
CSV
}

reset_dirs() {
	rm -rf -- "${build_dir}" "${args_file}"
}

common_args() {
	printf '%s\n' \
		--partitions "${partitions_csv}" \
		--build-dir "${build_dir}"
}

expect_pass() {
	local name="$1"
	shift
	local -a args
	mapfile -t args < <(common_args)
	if ! FAKE_LITTLEFS_ARGS_FILE="${args_file}" bash "${builder}" "${args[@]}" "$@" \
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
	local -a args
	mapfile -t args < <(common_args)
	if FAKE_LITTLEFS_ARGS_FILE="${args_file}" bash "${builder}" "${args[@]}" "$@" \
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

# Happy path: fake littlefs-python succeeds. Assert the image lands at
# build_dir/userdata.bin, littlefs-python was invoked with the userdata
# partition's exact configured size plus the expected fixed flags, and the
# source directory it packaged was empty (this partition ships with no
# content -- only a valid, mountable, empty filesystem).
write_partitions
reset_dirs
expect_pass 'happy path'
[ -f "${build_dir}/userdata.bin" ] || {
	printf 'FAIL: happy path did not write %s\n' "${build_dir}/userdata.bin" >&2
	exit 1
}
grep -qx -- '--fs-size=524288' "${args_file}" || {
	printf 'FAIL: littlefs-python was not called with the userdata partition size\n' >&2
	cat -- "${args_file}" >&2
	exit 1
}
grep -qx -- '--name-max=64' "${args_file}" || {
	printf 'FAIL: littlefs-python was not called with --name-max=64\n' >&2
	exit 1
}
grep -qx -- '--block-size=4096' "${args_file}" || {
	printf 'FAIL: littlefs-python was not called with --block-size=4096\n' >&2
	exit 1
}
source_dir="$(sed -n '2p' -- "${args_file}")"
[ -z "$(find "${source_dir}" -mindepth 1 -print -quit 2>/dev/null)" ] || {
	printf 'FAIL: happy path packaged a non-empty source directory: %s\n' "${source_dir}" >&2
	exit 1
}

# --output overrides the default build_dir/userdata.bin location.
reset_dirs
expect_pass '--output override' --output "${temporary_dir}/custom.bin"
[ -f "${temporary_dir}/custom.bin" ] || {
	printf 'FAIL: --output override did not write to the requested path\n' >&2
	exit 1
}

# A partition table missing the userdata partition must fail closed.
reset_dirs
cat >"${partitions_csv}" <<'CSV'
# Name,      Type, SubType, Offset,   Size,     Flags
nvs,         data, nvs,     0x9000,   0x6000,
ota_0,       app,  ota_0,   0x20000,  0x280000,
CSV
expect_fail 'partition table missing userdata' 'partition table has no userdata partition'
write_partitions

# A missing partition table file must fail closed before anything runs.
reset_dirs
rm -f -- "${partitions_csv}"
expect_fail 'missing partition table file' 'partition table not found'
write_partitions

# A littlefs-python failure must propagate as a real failure, not a silent
# skip or false success.
reset_dirs
FAKE_LITTLEFS_STATUS=1 expect_fail 'littlefs-python failure' \
	'fake littlefs-python: simulated failure'
unset FAKE_LITTLEFS_STATUS

# littlefs-python missing from PATH entirely must fail closed with the exact
# pinned-install instruction, not a bare "command not found". The
# littlefs-python check runs before anything else, so a minimal system-only
# PATH is safe here - it must exclude the fakes dir and any directory a real
# or pip-user-installed littlefs-python might sit in, not just the fakes
# dir, so this is deliberately a fixed allowlist rather than "the current
# PATH minus one entry".
reset_dirs
if PATH="/usr/bin:/bin" bash "${builder}" \
	--partitions "${partitions_csv}" --build-dir "${build_dir}" \
	>"${temporary_dir}/output" 2>&1; then
	printf 'FAIL: missing littlefs-python unexpectedly passed\n' >&2
	cat -- "${temporary_dir}/output" >&2
	exit 1
fi
if ! grep -F -- 'pip install littlefs-python==0.15.0' "${temporary_dir}/output" >/dev/null; then
	printf 'FAIL: missing littlefs-python did not report the pinned install command\n' >&2
	cat -- "${temporary_dir}/output" >&2
	exit 1
fi
pass_count=$((pass_count + 1))

printf 'build-userdata-image regression tests passed: %d\n' "${pass_count}"
