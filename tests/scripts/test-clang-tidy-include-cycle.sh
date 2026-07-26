#!/usr/bin/env bash
# Regression test for first-party include-cycle detection (FIX1 RESPONSES Q1).
#
# The fail-closed gate keeps misc-header-include-cycle ENABLED for first-party
# headers and narrows only the ESP-IDF / managed-component roots via
# misc-header-include-cycle.IgnoredFilesList in the repo .clang-tidy. This test
# proves that configuration against the real analyzer on crafted fixtures:
#
#   * a genuine first-party header cycle is reported and fails; and
#   * an otherwise identical cycle located under a managed_components root is
#     excluded cleanly (no finding, zero exit).
#
# It also runs a no-cycle negative control so a fixture that always failed cannot
# masquerade as a pass. The pinned esp-clang clang-tidy is preferred when the
# ESP-IDF toolchain is on PATH; otherwise the CI-pinned apt clang-tidy is used.
# Both honor the IgnoredFilesList option identically. The gate never silently
# skips: if no compatible clang-tidy is available the test fails.
set -euo pipefail

test_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly test_root
readonly config_file="${test_root}/.clang-tidy"

clang_tidy="${CLANG_TIDY:-}"
if [[ -z "${clang_tidy}" ]]; then
	for candidate in clang-tidy clang-tidy-18; do
		if command -v "${candidate}" >/dev/null 2>&1; then
			clang_tidy="${candidate}"
			break
		fi
	done
fi
if [[ -z "${clang_tidy}" ]] || ! command -v "${clang_tidy}" >/dev/null 2>&1; then
	printf 'error: no clang-tidy found; source the ESP-IDF clang toolchain or install clang-tidy\n' >&2
	exit 1
fi
if [[ ! -f "${config_file}" ]]; then
	printf 'error: missing repo .clang-tidy: %s\n' "${config_file}" >&2
	exit 1
fi

work_dir="$(mktemp -d)"
trap 'rm -rf -- "${work_dir}"' EXIT

tests_run=0
tests_failed=0

# check <expected> <actual> <description>: compare and tally.
check() {
	local expected="$1" actual="$2" desc="$3"
	tests_run=$((tests_run + 1))
	if [[ "${actual}" == "${expected}" ]]; then
		printf 'ok    %s\n' "${desc}"
	else
		printf 'FAIL  %s (expected %s, got %s)\n' "${desc}" "${expected}" "${actual}"
		tests_failed=$((tests_failed + 1))
	fi
}

# make_cycle <dir>: write a mutually-including a.h <-> b.h pair plus a unit.c that
# includes a.h, all under <dir>. Include guards keep the preprocessor finite; the
# static include edges still form the cycle the check detects.
make_cycle() {
	local dir="$1"
	mkdir -p "${dir}"
	printf '#ifndef CY_A_H\n#define CY_A_H\n#include "b.h"\n#endif\n' >"${dir}/a.h"
	printf '#ifndef CY_B_H\n#define CY_B_H\n#include "a.h"\n#endif\n' >"${dir}/b.h"
	printf '#include "a.h"\nint cycle_unit(void) { return 0; }\n' >"${dir}/unit.c"
}

# make_unit <dir>: write a first-party unit.c with a self-contained header, no
# cycle (negative control).
make_unit() {
	local dir="$1"
	mkdir -p "${dir}"
	printf '#ifndef OK_H\n#define OK_H\nint ok_value(void);\n#endif\n' >"${dir}/ok.h"
	printf '#include "ok.h"\nint ok_value(void) { return 0; }\n' >"${dir}/unit.c"
}

last_output="${work_dir}/clang-tidy-output.txt"

# run_analyzer <dir>: lint <dir>/unit.c with the repo config, restricted to the
# include-cycle check, and echo pass|fail by the analyzer's exit status.
run_analyzer() {
	local dir="$1" result
	if "${clang_tidy}" --config-file="${config_file}" --quiet \
		--checks='-*,misc-header-include-cycle' \
		"${dir}/unit.c" -- -std=c11 -I"${dir}" >"${last_output}" 2>&1; then
		result=pass
	else
		result=fail
	fi
	printf '%s' "${result}"
}

# reported: pass "yes" if the last analyzer run named the include-cycle check.
reported() {
	if grep -q 'misc-header-include-cycle' "${last_output}"; then
		printf 'yes'
	else
		printf 'no'
	fi
}

# 1. First-party include cycle -> the analyzer fails.
first_party_dir="${work_dir}/firmware/components/foo"
make_cycle "${first_party_dir}"
check fail "$(run_analyzer "${first_party_dir}")" "first-party include cycle fails"
check yes "$(reported)" "first-party include cycle is reported"

# 2. The same cycle under a managed_components root -> excluded cleanly.
third_party_dir="${work_dir}/firmware/managed_components/dep"
make_cycle "${third_party_dir}"
check pass "$(run_analyzer "${third_party_dir}")" "third-party include cycle is excluded"
check no "$(reported)" "third-party include cycle produces no finding"

# 3. Negative control: a first-party unit with no cycle -> passes.
clean_dir="${work_dir}/firmware/components/bar"
make_unit "${clean_dir}"
check pass "$(run_analyzer "${clean_dir}")" "first-party unit without a cycle passes"

printf '\n%d tests, %d failed\n' "${tests_run}" "${tests_failed}"
((tests_failed == 0))
