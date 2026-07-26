#!/usr/bin/env bash
# Regression tests for the fail-closed clang-tidy logic in scripts/check-firmware.sh
# (FIX1 Phase 2.2). These exercise run_first_party_clang_tidy against a fake
# run-clang-tidy and controlled compile databases, so no real analyzer or build is
# required. They prove the gate fails closed on every infrastructure failure and on
# first-party findings, while ignoring third-party-only findings.
set -euo pipefail

test_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly test_root
readonly fakes_dir="${test_root}/tests/scripts/fakes"

# Expose run_first_party_clang_tidy without running the real gate. The sourced
# script owns `repo_root`, `first_party`, and `third_party_headers`.
# shellcheck source=/dev/null
source "${test_root}/scripts/check-firmware.sh"

work_dir="$(mktemp -d)"
trap 'rm -rf -- "${work_dir}"' EXIT

tests_run=0
tests_failed=0

# make_project <name> <compile-db-json>: create a fake project with the given
# build-clang/compile_commands.json content and echo its path.
make_project() {
	local name="$1" db="$2"
	local dir="${work_dir}/${name}/build-clang"
	mkdir -p "${dir}"
	printf '%s' "${db}" >"${dir}/compile_commands.json"
	printf '%s' "${work_dir}/${name}"
}

# A compile database with one first-party translation unit.
first_party_db='[{"directory":"/x","command":"cc -c a.c","file":"/x/firmware/components/foo/a.c"}]'
# A compile database with only a third-party translation unit (zero first-party).
third_party_only_db='[{"directory":"/x","command":"cc -c b.c","file":"/x/firmware/managed_components/tinyusb/b.c"}]'

# expect_result <expected: pass|fail> <description> -- reads the run result from
# the global RESULT set by the caller.
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

# run_case <project> [status] [stdout] -- runs run_first_party_clang_tidy with the
# fake analyzer on PATH, driving its exit status and report, and echoes pass/fail.
run_case() {
	local project="$1"
	export FAKE_RCT_STATUS="${2:-0}" FAKE_RCT_STDOUT="${3:-}"
	local result
	if PATH="${fakes_dir}:${PATH}" run_first_party_clang_tidy "${project}" >/dev/null 2>&1; then
		result=pass
	else
		result=fail
	fi
	unset FAKE_RCT_STATUS FAKE_RCT_STDOUT
	printf '%s' "${result}"
}

# 1. Analyzer executable missing: strip the esp-clang toolchain (only provider of
#    run-clang-tidy) from PATH so command -v fails; jq/coreutils remain.
run_case_no_analyzer() {
	local project="$1" stripped
	stripped="$(printf '%s' "${PATH}" | tr ':' '\n' | grep -v 'esp-clang' | paste -sd: -)"
	if PATH="${stripped}" run_first_party_clang_tidy "${project}" >/dev/null 2>&1; then
		printf 'pass'
	else
		printf 'fail'
	fi
}

project_ok="$(make_project ok "${first_party_db}")"
project_thirdparty="$(make_project thirdparty "${third_party_only_db}")"

# 1. Analyzer executable missing -> fail closed.
check fail "$(run_case_no_analyzer "${project_ok}")" "analyzer executable missing fails"

# 2. Analyzer exits nonzero with no warning-shaped output -> fail closed.
check fail \
	"$(run_case "${project_ok}" 2 "clang-tidy: internal error; aborting")" \
	"analyzer nonzero exit with no warnings fails"

# 3. Analyzer exits zero but reports a first-party warning -> fail closed.
check fail \
	"$(run_case "${project_ok}" 0 "/x/firmware/components/foo/a.c:10:5: warning: bad thing [some-check]")" \
	"zero exit with a first-party warning fails"

# 3b. Analyzer exits zero but reports a first-party include-cycle finding (an
#     `error:`-worded misc-header-include-cycle diagnostic) -> fail closed. Proves
#     the post-success assertion catches first-party include cycles, not just
#     `warning:`-worded findings (FIX1 RESPONSES Q1).
check fail \
	"$(run_case "${project_ok}" 0 \
		"/x/firmware/components/foo/a.h:3:10: error: circular header file dependency detected while including 'b.h' [misc-header-include-cycle]")" \
	"zero exit with a first-party include-cycle finding fails"

# 4. Analyzer exits zero with only third-party warnings -> pass (not our code).
check pass \
	"$(run_case "${project_ok}" 0 "/x/esp-idf/components/freertos/x.h:1:1: warning: cycle [misc]")" \
	"zero exit with only third-party warnings passes"

# 5. Compile database missing -> fail closed.
check fail "$(run_case "${work_dir}/does-not-exist")" "missing compile database fails"

# 6. Compile database invalid JSON -> fail closed.
project_bad="$(make_project bad 'this is not json {')"
check fail "$(run_case "${project_bad}")" "invalid compile database fails"

# 7. Zero selected first-party translation units -> fail closed.
check fail "$(run_case "${project_thirdparty}")" "zero first-party translation units fails"

# 8. Valid clean run -> pass.
check pass "$(run_case "${project_ok}" 0 "")" "clean run passes"

printf '\n%d tests, %d failed\n' "${tests_run}" "${tests_failed}"
((tests_failed == 0))
