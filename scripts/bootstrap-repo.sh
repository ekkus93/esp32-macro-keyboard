#!/usr/bin/env bash
set -euo pipefail

readonly script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly repo_root="$(cd -- "${script_dir}/.." && pwd)"

readonly -a directories=(
  firmware/main
  firmware/components/app_core
  firmware/components/auth
  firmware/components/device_controls
  firmware/components/macro_executor
  firmware/components/macro_model
  firmware/components/macro_parser
  firmware/components/storage
  firmware/components/usb_keyboard
  firmware/components/web_server
  firmware/components/wifi_ap
  firmware/test_app
  firmware/webfs
  webapp/src/api
  webapp/src/components
  webapp/src/features/auth
  webapp/src/features/execution
  webapp/src/features/macros
  webapp/src/features/procedures
  webapp/src/features/sets
  webapp/src/features/settings
  webapp/src/pages
  webapp/src/types
  webapp/tests
  scripts
  tests
  .github/workflows
)

for directory in "${directories[@]}"; do
  mkdir -p -- "${repo_root}/${directory}"
done

printf 'Created or verified %d project directories under %s\n' \
  "${#directories[@]}" "${repo_root}"
