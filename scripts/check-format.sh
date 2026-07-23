#!/usr/bin/env bash
set -euo pipefail

mapfile -t c_files < <(find firmware/main firmware/components firmware/test_app \
    -type f \( -name '*.c' -o -name '*.h' \) -print | sort)
if ((${#c_files[@]} > 0)); then
    clang-format --dry-run --Werror "${c_files[@]}"
fi

mapfile -t cmake_files < <(find firmware tests -type f -name 'CMakeLists.txt' -print | sort)
if ((${#cmake_files[@]} > 0)); then
    cmake-format --check "${cmake_files[@]}"
    cmake-lint "${cmake_files[@]}"
fi

shfmt -d scripts/*.sh

cd webapp
npm run format:check
