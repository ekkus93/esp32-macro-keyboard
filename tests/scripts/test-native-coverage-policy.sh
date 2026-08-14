#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly source_script="${repo_root}/scripts/generate-native-coverage.sh"

mapfile -t policy_files < <(
	sed -n '/^pure_policy_files=(/,/^)/p' "${source_script}" |
		sed -n '2,$p' | sed '$d' | sed -E 's/^[[:space:]]*"([^"]+)"$/\1/'
)
if ((${#policy_files[@]} == 0)); then
	printf 'failed to extract pure-policy coverage source list\n' >&2
	exit 1
fi

make_fixture() {
	local root="$1"
	mkdir -p -- "${root}/scripts" "${root}/tests/host/build-coverage" "${root}/bin"
	cp -- "${source_script}" "${root}/scripts/generate-native-coverage.sh"
	chmod +x -- "${root}/scripts/generate-native-coverage.sh"
	for policy_file in "${policy_files[@]}"; do
		mkdir -p -- "${root}/$(dirname -- "${policy_file}")"
		: >"${root}/${policy_file}"
	done
	cat >"${root}/scripts/run-tests.sh" <<'RUN_TESTS'
#!/usr/bin/env bash
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
while IFS= read -r policy_file; do
	mkdir -p -- "${root}/tests/host/build-coverage/fake/${policy_file%/*}"
	: >"${root}/tests/host/build-coverage/fake/${policy_file}.gcno"
done <"${root}/policy-files.txt"
RUN_TESTS
	chmod +x -- "${root}/scripts/run-tests.sh"
	printf '%s\n' "${policy_files[@]}" >"${root}/policy-files.txt"
	cat >"${root}/bin/gcov" <<'GCOV'
#!/usr/bin/env bash
exit 0
GCOV
	cat >"${root}/bin/gcovr" <<'GCOVR'
#!/usr/bin/env bash
set -euo pipefail
while (($# > 0)); do
	case "$1" in
	--txt|--html-details|--json-summary)
		shift
		mkdir -p -- "$(dirname -- "$1")"
		: >"$1"
		;;
	esac
	shift
done
GCOVR
	chmod +x -- "${root}/bin/gcov" "${root}/bin/gcovr"
}

run_expect_failure() {
	local root="$1"
	local expected="$2"
	local output
	if output="$(PATH="${root}/bin:${PATH}" "${root}/scripts/generate-native-coverage.sh" 2>&1)"; then
		printf 'expected native coverage policy validation to fail\n' >&2
		exit 1
	fi
	if [[ "${output}" != *"${expected}"* ]]; then
		printf 'missing expected failure text: %s\n%s\n' "${expected}" "${output}" >&2
		exit 1
	fi
}

tmp_root="$(mktemp -d)"
trap 'rm -rf -- "${tmp_root}"' EXIT

missing_source_root="${tmp_root}/missing-source"
make_fixture "${missing_source_root}"
missing_source="${policy_files[0]}"
rm -- "${missing_source_root}/${missing_source}"
run_expect_failure "${missing_source_root}" "pure-policy coverage source does not exist: ${missing_source}"

missing_instrumentation_root="${tmp_root}/missing-instrumentation"
make_fixture "${missing_instrumentation_root}"
missing_instrumentation="${policy_files[1]}"
grep -Fvx -- "${missing_instrumentation}" "${missing_instrumentation_root}/policy-files.txt" \
	>"${missing_instrumentation_root}/policy-files.filtered"
mv -- "${missing_instrumentation_root}/policy-files.filtered" \
	"${missing_instrumentation_root}/policy-files.txt"
run_expect_failure "${missing_instrumentation_root}" \
	"pure-policy coverage source was not instrumented: ${missing_instrumentation}"

passing_root="${tmp_root}/passing"
make_fixture "${passing_root}"
PATH="${passing_root}/bin:${PATH}" "${passing_root}/scripts/generate-native-coverage.sh" >/dev/null

printf 'native coverage policy source validation tests passed\n'
