#!/usr/bin/env bash
set -euo pipefail

readonly repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly project_dir="${repo_root}/firmware/test_app"
readonly build_dir="${DEVICE_TEST_BUILD_DIR:-${project_dir}/build}"

"${repo_root}/scripts/verify-toolchain.sh" --firmware-only

cd "${project_dir}"
idf.py -B "${build_dir}" build
