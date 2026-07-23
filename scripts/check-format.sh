#!/usr/bin/env bash
set -euo pipefail

mapfile -t c_files < <(find firmware/main firmware/components firmware/test_app \
    -type f \( -name '*.c' -o -name '*.h' \) -print | sort)
if ((${#c_files[@]} > 0)); then
    clang-format --dry-run --Werror "${c_files[@]}"
fi

shfmt -d scripts/*.sh

cd webapp
npm run format:check
