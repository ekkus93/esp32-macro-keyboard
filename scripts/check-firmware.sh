#!/usr/bin/env bash
set -euo pipefail

readonly repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly header_filter='^(firmware/main|firmware/components|firmware/test_app)/'

build_and_lint_project() {
    local project_dir="$1"
    local build_dir="${project_dir}/build"

    cd "${project_dir}"
    idf.py -B "${build_dir}" set-target esp32s3
    idf.py -B "${build_dir}" build

    if [[ -f "${build_dir}/compile_commands.json" ]]; then
        run-clang-tidy -p "${build_dir}" -header-filter="${header_filter}"
    fi
}

build_and_lint_project "${repo_root}/firmware"
build_and_lint_project "${repo_root}/firmware/test_app"
