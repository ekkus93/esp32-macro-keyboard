#!/usr/bin/env bash
set -euo pipefail

readonly repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "${repo_root}/tests/host" -B "${repo_root}/tests/host/build"
cmake --build "${repo_root}/tests/host/build" --parallel
ctest --test-dir "${repo_root}/tests/host/build" --output-on-failure
