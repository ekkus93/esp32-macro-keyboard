#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
# Absolute-path anchored first-party selector. Trailing slashes exclude ESP-IDF
# core and managed_components (third-party code must not be linted), including the
# managed_components nested under firmware/test_app/ (matched by test_app/main/,
# not a bare test_app/).
readonly first_party='/firmware/(main/|components/|test_app/main/)'
# Third-party header roots whose diagnostics are excluded *before* emission so that
# run-clang-tidy's exit status is a trustworthy fail-closed signal. The FreeRTOS
# include cycle in idf_additions.h is additionally handled by
# misc-header-include-cycle.IgnoredFilesList in .clang-tidy, which
# -exclude-header-filter does not cover.
readonly third_party_headers='(esp-idf|managed_components)'

# Analyze the first-party translation units of one ESP-IDF project and fail closed.
# The gate fails on any of: run-clang-tidy nonzero exit, missing/invalid compile
# database, zero selected first-party translation units, or a first-party finding.
# The complete analyzer report is printed on failure. Output grep is only an
# additional assertion after the analyzer has run successfully -- never a way to
# convert a nonzero analyzer status into success.
run_first_party_clang_tidy() {
	local project_dir="$1"
	local build_dir="${project_dir}/build-clang"
	local compile_database="${build_dir}/compile_commands.json"

	if ! command -v run-clang-tidy >/dev/null 2>&1; then
		printf 'error: run-clang-tidy not found; source the ESP-IDF clang toolchain\n' >&2
		return 1
	fi
	if [[ ! -f "${compile_database}" ]]; then
		printf 'error: missing compile database: %s\n' "${compile_database}" >&2
		return 1
	fi
	if ! jq empty "${compile_database}" >/dev/null 2>&1; then
		printf 'error: invalid compile database: %s\n' "${compile_database}" >&2
		return 1
	fi

	local translation_units
	translation_units="$(
		jq --arg pattern "${first_party}" \
			'[.[] | select(.file | test($pattern))] | length' "${compile_database}"
	)"
	if [[ ! "${translation_units}" =~ ^[0-9]+$ ]] || ((translation_units == 0)); then
		printf 'error: no first-party translation units selected in %s\n' \
			"${compile_database}" >&2
		return 1
	fi

	local report_file status
	report_file="$(mktemp)"
	set +e
	run-clang-tidy -p "${build_dir}" \
		-header-filter="${first_party}" \
		-exclude-header-filter="${third_party_headers}" \
		"${first_party}" >"${report_file}" 2>&1
	status=$?
	set -e

	if ((status != 0)); then
		cat -- "${report_file}"
		printf 'error: run-clang-tidy failed for %s with status %d\n' \
			"${project_dir}" "${status}" >&2
		rm -f -- "${report_file}"
		return 1
	fi

	local findings
	findings="$(
		grep -E ':[0-9]+:[0-9]+: (warning|error):' "${report_file}" |
			grep -E "${first_party}" || :
	)"
	rm -f -- "${report_file}"
	if [[ -n "${findings}" ]]; then
		printf '%s\n' "${findings}"
		printf 'error: first-party clang-tidy findings in %s\n' "${project_dir}" >&2
		return 1
	fi
}

build_and_lint_project() {
	local project_dir="$1"

	cd "${project_dir}"
	# The GCC (default) toolchain build validates the firmware compiles and links.
	idf.py -B "${project_dir}/build" set-target esp32s3
	idf.py -B "${project_dir}/build" build

	# clang-tidy needs a clang-toolchain compile database: the GCC database uses
	# GCC-only flags clang rejects (e.g. -fno-tree-switch-conversion) and omits
	# GCC's implicit system-include paths, so clang cannot even parse it.
	IDF_TOOLCHAIN=clang idf.py -B "${project_dir}/build-clang" set-target esp32s3 >/dev/null

	run_first_party_clang_tidy "${project_dir}"
}

# Run the gate only when executed directly. Sourcing (e.g. from the script
# regression tests) exposes run_first_party_clang_tidy without triggering builds.
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
	build_and_lint_project "${repo_root}/firmware"
	build_and_lint_project "${repo_root}/firmware/test_app"
fi
