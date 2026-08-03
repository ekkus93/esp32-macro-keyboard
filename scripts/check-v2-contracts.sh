#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=${V2_CONTRACT_BUILD_DIR:-"${repository_root}/build-v2-contracts"}

cd "${repository_root}"

python3 scripts/check-v2-limits.py
python3 scripts/check-v2-settings-schema.py

rm -rf "${build_dir}"
cmake -S tests/v2_contracts -B "${build_dir}" -DCMAKE_BUILD_TYPE=Debug
cmake --build "${build_dir}" --parallel
ctest --test-dir "${build_dir}" --output-on-failure

npm --prefix webapp run test -- \
    tests/v2-repository.test.ts \
    tests/v2-repository-validation.test.ts \
    tests/v2-api-contracts.test.ts \
    tests/v2-api-requests.test.ts \
    tests/v2-macro-conformance.test.ts \
    tests/v2-macro-canonical-tokens.test.ts
