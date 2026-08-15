#!/usr/bin/env bash
set -euo pipefail

# Guards one property: scripts/check-format.sh must format-check *.inc fragments,
# not just *.c and *.h.
#
# The large host suites keep their test bodies in .inc fragments included by one
# test_*.c (auth, executor, web security, the web-server adapter). Those were
# outside the glob until this test existed, and two of them had silently drifted
# out of clang-format compliance as a result. Dropping *.inc from the glob again
# would exempt several thousand lines of first-party C without failing anything.
#
# Method: plant a deliberately misformatted fragment, assert check-format.sh
# fails AND names it. Naming it matters -- a bare non-zero exit would also be
# produced by an unrelated pre-existing violation, which would make this test
# pass for the wrong reason.

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-format.sh"
readonly fixture="${repo_root}/tests/host/zz_check_format_coverage_fixture.inc"
output="$(mktemp)"
readonly output

cleanup() {
	rm -f -- "${fixture}" "${output}"
}
trap cleanup EXIT

if [ -e "${fixture}" ]; then
	printf 'FAIL: fixture path %s already exists; refusing to overwrite\n' "${fixture}" >&2
	exit 1
fi

pass_count=0

# Misformatted on purpose: doubled spaces, brace placement, no space around '='.
cat >"${fixture}" <<'FRAGMENT'
static void  test_check_format_coverage_fixture( void )
{
    int   value=1;
    (void)value;
}
FRAGMENT

if (cd -- "${repo_root}" && "${checker}") >"${output}" 2>&1; then
	printf 'FAIL: check-format.sh passed with a misformatted .inc fragment present\n' >&2
	printf '      *.inc is not covered by the first-party C glob.\n' >&2
	exit 1
fi
pass_count=$((pass_count + 1))

if ! grep -F -- 'zz_check_format_coverage_fixture.inc' "${output}" >/dev/null; then
	printf 'FAIL: check-format.sh failed, but not because of the planted fragment.\n' >&2
	printf '      Some other violation is masking this test. Output:\n' >&2
	cat -- "${output}" >&2
	exit 1
fi
pass_count=$((pass_count + 1))

# Removing the fragment must restore the previous verdict, otherwise the two
# assertions above could be satisfied by a tree that was already failing.
rm -f -- "${fixture}"
if ! (cd -- "${repo_root}" && "${checker}") >"${output}" 2>&1; then
	printf 'FAIL: check-format.sh still fails after removing the planted fragment.\n' >&2
	printf '      The tree was already unformatted; fix that first. Output:\n' >&2
	cat -- "${output}" >&2
	exit 1
fi
pass_count=$((pass_count + 1))

printf 'check-format .inc coverage regression tests passed: %d\n' "${pass_count}"
