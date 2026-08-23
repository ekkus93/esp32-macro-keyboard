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

pure_policy_files=(
	"firmware/components/macro_executor/macro_executor_engine.c"
	"firmware/components/auth/auth_core_common.c"
	"firmware/components/auth/auth_core_password.c"
	"firmware/components/auth/auth_core_session.c"
	"firmware/components/auth/auth_core_rate_limit.c"
	"firmware/components/web_server/web_cookie.c"
	"firmware/components/web_server/web_static_path.c"
	"firmware/components/web_server/web_content.c"
	"firmware/components/web_server/web_setup_core.c"
	"firmware/components/web_server/web_setup_json.c"
	"firmware/components/app_core/app_core_sequence.c"
	"firmware/components/device_controls/device_controls_logic.c"
	"firmware/components/device_controls/device_controls_reset.c"
	"firmware/components/wifi_ap/wifi_ap_state.c"
	"firmware/components/wifi_ap/wifi_ap_station.c"
	"firmware/components/provisioning/provisioning_bootstrap_core.c"
	"firmware/components/app_contracts_v2/settings_contract_v2.c"
	"firmware/components/web_server/web_settings.c"
	"firmware/components/web_server/web_device_actions.c"
)

for policy_file in "${pure_policy_files[@]}"; do
	if [[ ! -f "${repo_root}/${policy_file}" ]]; then
		printf 'pure-policy coverage source does not exist: %s\n' "${policy_file}" >&2
		exit 1
	fi
done

rm -rf -- "${output_dir}"
mkdir -p -- "${output_dir}"

"${repo_root}/scripts/run-tests.sh" --coverage

for policy_file in "${pure_policy_files[@]}"; do
	if ! find "${build_dir}" -type f -path "*/${policy_file}.gcno" -print -quit | grep -q .; then
		printf 'pure-policy coverage source was not instrumented: %s\n' "${policy_file}" >&2
		exit 1
	fi
done

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
)
for policy_file in "${pure_policy_files[@]}"; do
	policy_filter="${policy_file//./\\.}"
	pure_policy_args+=(--filter "^${policy_filter}$")
done

gcovr "${pure_policy_args[@]}" \
	--txt "${output_dir}/pure-policy-coverage.txt" \
	--print-summary \
	--fail-under-line 90 \
	--fail-under-branch 80

cat -- "${output_dir}/line-coverage.txt"
cat -- "${output_dir}/branch-coverage.txt"
cat -- "${output_dir}/pure-policy-coverage.txt"
