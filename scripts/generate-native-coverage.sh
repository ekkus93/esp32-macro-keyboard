#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
readonly build_dir="${repo_root}/tests/host/build-coverage"
readonly output_dir="${1:-${repo_root}/tests/host/coverage}"

if ! command -v gcovr >/dev/null 2>&1; then
	printf 'gcovr is required for native coverage generation\n' >&2
	exit 1
fi
if ! command -v gcov >/dev/null 2>&1; then
	printf 'gcov is required for native coverage generation\n' >&2
	exit 1
fi

rm -rf -- "${output_dir}"
mkdir -p -- "${output_dir}"

"${repo_root}/scripts/run-tests.sh" --coverage

cd -- "${repo_root}"
common_args=(
	"${build_dir}"
	--root "${repo_root}"
	--object-directory "${build_dir}"
	--gcov-executable gcov
	--no-markers
	--no-color
	--filter '^firmware/components/'
)

gcovr "${common_args[@]}" \
	--txt "${output_dir}/line-coverage.txt" \
	--html-details "${output_dir}/index.html" \
	--json-summary "${output_dir}/summary.json" \
	--json-summary-pretty \
	--print-summary

gcovr "${common_args[@]}" \
	--txt-metric branch \
	--txt "${output_dir}/branch-coverage.txt"

pure_policy_args=(
	"${build_dir}"
	--root "${repo_root}"
	--object-directory "${build_dir}"
	--gcov-executable gcov
	--no-markers
	--no-color
	--filter '^firmware/components/macro_executor/macro_executor_engine\.c$'
	--filter '^firmware/components/auth/auth_core\.c$'
	--filter '^firmware/components/web_server/web_cookie\.c$'
	--filter '^firmware/components/web_server/web_origin\.c$'
	--filter '^firmware/components/web_server/web_static_path\.c$'
	--filter '^firmware/components/web_server/web_content\.c$'
	--filter '^firmware/components/app_core/app_core_sequence\.c$'
	--filter '^firmware/components/device_controls/device_controls_logic\.c$'
	--filter '^firmware/components/wifi_ap/wifi_ap_state\.c$'
)

gcovr "${pure_policy_args[@]}" \
	--txt "${output_dir}/pure-policy-coverage.txt" \
	--print-summary \
	--fail-under-line 90 \
	--fail-under-branch 80

cat -- "${output_dir}/line-coverage.txt"
cat -- "${output_dir}/branch-coverage.txt"
cat -- "${output_dir}/pure-policy-coverage.txt"
