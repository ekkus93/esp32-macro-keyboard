#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
readonly output_file="${1:-${repo_root}/firmware/clang-tidy-report.txt}"

# Absolute-path anchored so it matches the compile database's absolute file
# paths and excludes ESP-IDF core components and managed_components (third-party
# code must not be linted).
readonly first_party='/firmware/(main|components|test_app)/'

if ! command -v idf.py >/dev/null 2>&1 || ! command -v run-clang-tidy >/dev/null 2>&1; then
	printf 'idf.py and run-clang-tidy are required; source the ESP-IDF environment first\n' >&2
	exit 1
fi

# clang-tidy needs a compile database produced by the clang toolchain. The GCC
# database omits GCC's implicit system-include paths, so clang cannot resolve
# <string.h> and similar; a clang configure lists every include path explicitly.
# A dedicated build-clang directory keeps the normal GCC build untouched.
configure_clang_build() {
	local project_dir="$1"
	(
		cd -- "${project_dir}"
		IDF_TOOLCHAIN=clang idf.py -B "${project_dir}/build-clang" set-target esp32s3 >/dev/null
	)
}

: >"${output_file}"
for project in "${repo_root}/firmware" "${repo_root}/firmware/test_app"; do
	configure_clang_build "${project}"
	# run-clang-tidy exits non-zero when findings exist; capture them either way.
	run-clang-tidy -p "${project}/build-clang" \
		-header-filter="${first_party}" \
		"${first_party}" >>"${output_file}" 2>&1 || true
done

printf '\nFirst-party clang-tidy findings by check:\n'
grep -E ':[0-9]+:[0-9]+: (warning|error):' "${output_file}" |
	grep -E "${first_party}" |
	grep -oE '\[[a-z][a-z0-9-]+' | sed 's/\[//' | sort | uniq -c | sort -rn

total="$(grep -E ':[0-9]+:[0-9]+: (warning|error):' "${output_file}" | grep -cE "${first_party}" || true)"
printf '\nTotal first-party findings: %s\nFull report: %s\n' "${total}" "${output_file}"
