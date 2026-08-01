#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-stack-usage.sh"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

pass_count=0

readonly build_dir="${temporary_dir}/build"
readonly allowlist="${temporary_dir}/allowlist.txt"

# A .su record is: <file>:<line>:<col>:<function>\t<bytes>\t<qualifier>
write_frames() {
	rm -rf -- "${build_dir}"
	mkdir -p -- "${build_dir}/esp-idf/storage"
	{
		printf '%s/firmware/components/storage/small.c:10:5:small_function\t512\tstatic\n' "${repo_root}"
		printf '%s/firmware/components/storage/big.c:20:5:big_function\t%s\tstatic\n' "${repo_root}" "$1"
		# third-party records must be ignored entirely
		printf '/home/x/esp-idf/components/foo/bar.c:1:1:idf_function\t99999\tstatic\n'
		printf '%s/firmware/managed_components/z/q.c:1:1:managed_function\t99999\tstatic\n' "${repo_root}"
	} >"${build_dir}/esp-idf/storage/frames.su"
}

run_checker() {
	bash "${checker}" --no-build --build-dir "${build_dir}" \
		--allowlist "${allowlist}" "$@" >"${temporary_dir}/out" 2>&1
}

expect_pass() {
	local name="$1"
	shift
	if ! run_checker "$@"; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/out" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1" expected="$2"
	shift 2
	if run_checker "$@"; then
		printf 'FAIL: %s unexpectedly passed\n' "${name}" >&2
		cat -- "${temporary_dir}/out" >&2
		exit 1
	fi
	if ! grep -F -- "${expected}" "${temporary_dir}/out" >/dev/null; then
		printf 'FAIL: %s did not report %s\n' "${name}" "${expected}" >&2
		cat -- "${temporary_dir}/out" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

# A frame under the threshold needs no allowlist entry.
write_frames 1024
printf '# empty\n' >"${allowlist}"
expect_pass 'all frames under threshold'

# A frame over the threshold with no allowlist entry must fail closed.
write_frames 9000
printf '# empty\n' >"${allowlist}"
expect_fail 'unlisted oversized frame' 'are not allowlisted'

# The same frame passes once recorded at its measured size.
printf '9000 firmware/components/storage/big.c:big_function\n' >"${allowlist}"
expect_pass 'oversized frame recorded in allowlist'

# Growth beyond the recorded size must fail: the allowlist is a ratchet.
printf '5000 firmware/components/storage/big.c:big_function\n' >"${allowlist}"
expect_fail 'allowlisted frame grew' 'grew beyond their recorded size'

# A frame that shrank below its recorded size still passes (the ratchet only
# blocks growth), so cleanups are never blocked by a stale-but-larger number.
write_frames 6000
printf '9000 firmware/components/storage/big.c:big_function\n' >"${allowlist}"
expect_pass 'allowlisted frame shrank'

# An allowlist entry matching no real frame must fail, so the file cannot rot.
write_frames 1024
printf '9000 firmware/components/storage/ghost.c:ghost_function\n' >"${allowlist}"
expect_fail 'stale allowlist entry' 'no longer match any frame'

# Third-party frames are ignored no matter how large.
write_frames 1024
printf '# empty\n' >"${allowlist}"
expect_pass 'third-party frames ignored'

# A custom threshold is honored.
write_frames 2048
printf '# empty\n' >"${allowlist}"
expect_fail 'custom threshold enforced' 'are not allowlisted' --threshold 1024

# A malformed allowlist line is an error, not silently skipped.
write_frames 1024
printf 'not-a-number some/path.c:fn\n' >"${allowlist}"
expect_fail 'malformed allowlist line' 'malformed allowlist line'

# No first-party records at all means the build was not instrumented; that must
# fail rather than silently "pass" with nothing analyzed.
rm -rf -- "${build_dir}"
mkdir -p -- "${build_dir}"
printf '# empty\n' >"${allowlist}"
expect_fail 'no stack-usage records' 'no first-party stack-usage records'

# The build path resolves the project directory explicitly. A missing idf.py
# must be reported as such rather than surfacing a confusing CMake error from
# whatever directory the script happened to be run in. (This case exists
# because every other test uses --no-build, which previously left the build
# path completely uncovered - and it shipped a real bug: idf.py was invoked
# without -C, so it looked for CMakeLists.txt in the repository root.)
write_frames 1024
printf '# empty\n' >"${allowlist}"
if PATH="/usr/bin:/bin" bash "${checker}" --project "${temporary_dir}/nonexistent" \
	--build-dir "${build_dir}" --allowlist "${allowlist}" \
	>"${temporary_dir}/out" 2>&1; then
	printf 'FAIL: missing idf.py unexpectedly passed\n' >&2
	cat -- "${temporary_dir}/out" >&2
	exit 1
fi
if ! grep -F -- 'idf.py not found' "${temporary_dir}/out" >/dev/null; then
	printf 'FAIL: missing idf.py did not report a clear error\n' >&2
	cat -- "${temporary_dir}/out" >&2
	exit 1
fi
pass_count=$((pass_count + 1))

printf 'check-stack-usage regression tests passed: %d\n' "${pass_count}"
