#!/usr/bin/env bash
set -euo pipefail

readonly repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}/firmware"

idf.py set-target esp32s3
idf.py build

if [[ -f build/compile_commands.json ]]; then
    run-clang-tidy -p build \
        -header-filter='^(firmware/main|firmware/components|firmware/test_app)/'
fi
