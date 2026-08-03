#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=${V2_CONTRACT_BUILD_DIR:-"${repository_root}/build-v2-contracts"}
run_web=true

if [[ ${1:-} == "--native-only" ]]; then
	run_web=false
	shift
fi
if (($# != 0)); then
	printf 'usage: %s [--native-only]\n' "$0" >&2
	exit 2
fi

cd "${repository_root}"

python3 scripts/check-v2-limits.py
python3 scripts/check-v2-settings-schema.py
python3 scripts/check-v2-setup-route-policy.py

rm -rf "${build_dir}"
cmake -S tests/v2_contracts -B "${build_dir}" -DCMAKE_BUILD_TYPE=Debug
cmake --build "${build_dir}" --parallel
ctest --test-dir "${build_dir}" --output-on-failure

if [[ ${run_web} == true ]]; then
	npm --prefix webapp ci
	npm --prefix webapp run test -- \
		tests/v2-repository.test.ts \
		tests/v2-repository-validation.test.ts \
		tests/v2-api-contracts.test.ts \
		tests/v2-api-requests.test.ts \
		tests/v2-setup-route-policy.test.ts \
		tests/v2-macro-conformance.test.ts \
		tests/v2-macro-canonical-tokens.test.ts
fi
