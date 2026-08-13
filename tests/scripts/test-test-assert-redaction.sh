#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT
readonly secret_sentinel='h9-assertion-secret-sentinel'

cat >"${temporary_dir}/redaction_probe.c" <<'SOURCE'
#include <stdint.h>
#include <string.h>

#include "test_assert.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        return 2;
    }
    if (strcmp(argv[1], "condition") == 0) {
        TEST_CHECK(strcmp("h9-assertion-secret-sentinel", "different") == 0);
    }
    if (strcmp(argv[1], "string") == 0) {
        TEST_CHECK_EQ_STRING("h9-assertion-secret-sentinel", "different");
    }
    if (strcmp(argv[1], "integer") == 0) {
        TEST_CHECK_EQ_U64(UINT64_C(9876543210123456789), UINT64_C(1));
    }
    return 3;
}
SOURCE

cc -std=c11 -Wall -Wextra -Werror \
	-I"${repo_root}/tests/host/support" \
	"${repo_root}/tests/host/support/test_assert.c" \
	"${temporary_dir}/redaction_probe.c" \
	-o "${temporary_dir}/redaction_probe"

pass_count=0
for probe in condition string integer; do
	output="${temporary_dir}/${probe}.out"
	if "${temporary_dir}/redaction_probe" "${probe}" >"${output}" 2>&1; then
		printf 'FAIL: %s assertion probe unexpectedly succeeded\n' "${probe}" >&2
		exit 1
	fi
	if grep -F -- "${secret_sentinel}" "${output}" >/dev/null; then
		printf 'FAIL: %s assertion output disclosed the secret sentinel\n' "${probe}" >&2
		cat -- "${output}" >&2
		exit 1
	fi
	if grep -F -- '9876543210123456789' "${output}" >/dev/null; then
		printf 'FAIL: %s assertion output disclosed the compared integer\n' "${probe}" >&2
		cat -- "${output}" >&2
		exit 1
	fi
	if ! grep -F -- 'test failure at ' "${output}" >/dev/null; then
		printf 'FAIL: %s assertion output lost the generic failure diagnostic\n' "${probe}" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
done

printf 'test assertion redaction regression tests passed: %d\n' "${pass_count}"
