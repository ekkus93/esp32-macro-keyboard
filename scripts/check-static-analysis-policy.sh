#!/usr/bin/env bash
set -euo pipefail

# Enforce the FIX1 static-analysis exception policy (RESPONSES Q2 /
# docs/STATIC_ANALYSIS_EXCEPTIONS.md). Fails when:
#   - the set of disabled clang-tidy checks differs from the approved three
#     (a fourth disable, a misspelling, or a broadening wildcard changes the set);
#   - WarningsAsErrors is weakened from '*';
#   - a first-party NOLINT, GCC diagnostic pragma, or -Wno-* suppression appears.

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root

# The exact approved disabled checks. Keep in sync with the exception register.
readonly approved_disabled='clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling
concurrency-mt-unsafe
readability-non-const-parameter'

# First-party C source roots scanned for local suppressions.
readonly -a first_party_roots=(firmware/main firmware/components firmware/test_app/main)

# extract_disabled_checks <clang-tidy-file>: print the specific disabled checks
# (leading '-' entries other than '-*') from the Checks block, one per line, sorted.
extract_disabled_checks() {
	local file="$1"
	# [:blank:] is space and tab only -- unlike [:space:] it keeps newlines, so the
	# one-check-per-line structure of the Checks block is preserved. `|| true`
	# yields an empty (not aborting) result when a config disables nothing.
	awk '/^Checks:/{inblock=1; next} /^[A-Za-z]/{inblock=0} inblock' "${file}" |
		tr -d '[:blank:],' |
		grep -E '^-[A-Za-z]' |
		grep -vx -- '-\*' |
		sed 's/^-//' |
		sort -u || true
}

check_disabled_checks() {
	local file="$1" actual expected
	actual="$(extract_disabled_checks "${file}")"
	expected="$(printf '%s\n' "${approved_disabled}" | sort -u)"
	if [[ "${actual}" != "${expected}" ]]; then
		printf 'error: disabled clang-tidy checks do not match the approved register.\n' >&2
		printf -- '--- approved ---\n%s\n--- found ---\n%s\n' "${expected}" "${actual}" >&2
		printf 'Update docs/STATIC_ANALYSIS_EXCEPTIONS.md and this policy to change an exception.\n' >&2
		return 1
	fi
}

check_warnings_as_errors() {
	local file="$1" value
	value="$(grep -E '^WarningsAsErrors:' "${file}" | head -1 |
		sed -E "s/^WarningsAsErrors:[[:space:]]*//; s/['\"]//g; s/[[:space:]]*\$//")"
	if [[ "${value}" != '*' ]]; then
		printf "error: WarningsAsErrors must be '*' (found: '%s')\n" "${value}" >&2
		return 1
	fi
}

check_no_local_suppressions() {
	local hits
	hits="$(grep -rInE \
		'NOLINT|#[[:space:]]*pragma[[:space:]]+GCC[[:space:]]+diagnostic[[:space:]]+ignored|-Wno-' \
		"$@" 2>/dev/null || true)"
	if [[ -n "${hits}" ]]; then
		printf 'error: first-party local static-analysis suppression is prohibited:\n%s\n' \
			"${hits}" >&2
		return 1
	fi
}

# main [clang-tidy-file] [source-root...]: defaults to the repository .clang-tidy
# and first-party roots; overridable for regression tests.
main() {
	local clang_tidy="${1:-${repo_root}/.clang-tidy}"
	shift || true
	local -a roots=("$@")
	if ((${#roots[@]} == 0)); then
		roots=("${first_party_roots[@]/#/${repo_root}/}")
	fi

	# Explicit propagation: when main runs as an `if` condition (e.g. the
	# regression tests) set -e is suspended, so each check must be checked directly.
	check_disabled_checks "${clang_tidy}" || return 1
	check_warnings_as_errors "${clang_tidy}" || return 1
	check_no_local_suppressions "${roots[@]}" || return 1
	printf 'static-analysis policy: ok\n'
}

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
	main "$@"
fi
