#!/usr/bin/env bash
# Regression tests for scripts/check-static-analysis-policy.sh (FIX1 RESPONSES Q2).
# Proves the policy passes on the real configuration and fails on every prohibited
# change: a fourth disabled check, a broadening wildcard, weakened WarningsAsErrors,
# and a first-party NOLINT or -Wno- suppression.
set -euo pipefail

test_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly test_root

# main() is guarded; sourcing exposes it and the check_* helpers.
# shellcheck source=/dev/null
source "${test_root}/scripts/check-static-analysis-policy.sh"

work_dir="$(mktemp -d)"
trap 'rm -rf -- "${work_dir}"' EXIT

tests_run=0
tests_failed=0

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

# run_main <clang-tidy> <source-root> -> pass|fail
run_main() {
	if main "$1" "$2" >/dev/null 2>&1; then
		printf 'pass'
	else
		printf 'fail'
	fi
}

# A clean first-party source root with no suppressions.
clean_src="${work_dir}/clean"
mkdir -p "${clean_src}"
printf 'int ok_symbol;\n' >"${clean_src}/a.c"

# fixture_ct <name>: copy the real .clang-tidy to a fixture path and echo it.
fixture_ct() {
	local path="${work_dir}/$1"
	cp "${test_root}/.clang-tidy" "${path}"
	printf '%s' "${path}"
}

# 1. Real configuration + clean source -> pass.
check pass "$(run_main "${test_root}/.clang-tidy" "${clean_src}")" "real configuration passes"

# 2. A fourth disabled check -> fail.
ct4="$(fixture_ct ct4)"
sed -i 's/^  -concurrency-mt-unsafe$/  -concurrency-mt-unsafe,\n  -bugprone-assert-side-effect/' "${ct4}"
check fail "$(run_main "${ct4}" "${clean_src}")" "fourth disabled check fails"

# 3. An approved name broadened to a wildcard -> fail.
ctw="$(fixture_ct ctw)"
sed -i 's/-readability-non-const-parameter/-readability-*/' "${ctw}"
check fail "$(run_main "${ctw}" "${clean_src}")" "wildcard broadening fails"

# 4. WarningsAsErrors weakened -> fail.
ctwae="$(fixture_ct ctwae)"
sed -i "s/^WarningsAsErrors: '\*'/WarningsAsErrors: 'clang-diagnostic-*'/" "${ctwae}"
check fail "$(run_main "${ctwae}" "${clean_src}")" "weakened WarningsAsErrors fails"

# 5. A first-party NOLINT -> fail.
nolint_src="${work_dir}/nolint"
mkdir -p "${nolint_src}"
printf 'int y; /* NOLINT */\n' >"${nolint_src}/b.c"
check fail "$(run_main "${test_root}/.clang-tidy" "${nolint_src}")" "first-party NOLINT fails"

# 6. A first-party -Wno- suppression -> fail.
wno_src="${work_dir}/wno"
mkdir -p "${wno_src}"
printf 'target_compile_options(x PRIVATE -Wno-error)\n' >"${wno_src}/CMakeLists.txt"
check fail "$(run_main "${test_root}/.clang-tidy" "${wno_src}")" "first-party -Wno- fails"

printf '\n%d tests, %d failed\n' "${tests_run}" "${tests_failed}"
((tests_failed == 0))
