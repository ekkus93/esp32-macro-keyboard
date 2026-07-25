#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
# Absolute-path anchored to match the compile database's absolute file paths.
# The trailing slashes are deliberate: they exclude ESP-IDF core components and
# managed_components (third-party code must not be linted), including the
# managed_components nested under firmware/test_app/ (matched by test_app/main/,
# not a bare test_app/).
readonly first_party='/firmware/(main/|components/|test_app/main/)'

build_and_lint_project() {
	local project_dir="$1"

	cd "${project_dir}"
	# The GCC (default) toolchain build validates the firmware compiles and
	# links for the target.
	idf.py -B "${project_dir}/build" set-target esp32s3
	idf.py -B "${project_dir}/build" build

	# clang-tidy needs a clang-toolchain compile database: the GCC database uses
	# GCC-only flags clang rejects (e.g. -fno-tree-switch-conversion) and omits
	# GCC's implicit system-include paths, so clang cannot even parse it. A
	# dedicated clang configure lists every include path explicitly. The
	# positional first-party filter keeps analysis off ESP-IDF/managed_components.
	IDF_TOOLCHAIN=clang idf.py -B "${project_dir}/build-clang" set-target esp32s3 >/dev/null

	# A few diagnostics are emitted against third-party headers included by our
	# code (e.g. a FreeRTOS include cycle) that we cannot fix; -header-filter does
	# not suppress all of them. Gate on findings whose *location* is first-party.
	local report
	report="$(run-clang-tidy -p "${project_dir}/build-clang" \
		-header-filter="${first_party}" "${first_party}" 2>&1 || true)"
	local findings
	findings="$(printf '%s\n' "${report}" |
		grep -E ':[0-9]+:[0-9]+: (warning|error):' | grep -E "${first_party}" || true)"
	if [[ -n "${findings}" ]]; then
		printf '%s\n' "${findings}"
		printf 'First-party clang-tidy findings above (%s).\n' "${project_dir}" >&2
		return 1
	fi
}

build_and_lint_project "${repo_root}/firmware"
build_and_lint_project "${repo_root}/firmware/test_app"
