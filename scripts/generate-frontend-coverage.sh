#!/usr/bin/env bash
set -euo pipefail

readonly repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly webapp_dir="${repo_root}/webapp"
readonly coverage_provider_version="3.2.4"

if ! command -v npm >/dev/null 2>&1; then
  printf 'npm is required for frontend coverage generation\n' >&2
  exit 1
fi

rm -rf -- "${webapp_dir}/coverage"

npm --prefix "${webapp_dir}" ci --ignore-scripts --no-audit --no-fund
npm --prefix "${webapp_dir}" install \
  --no-save \
  --package-lock=false \
  --ignore-scripts \
  --no-audit \
  --no-fund \
  "@vitest/coverage-v8@${coverage_provider_version}"
npm --prefix "${webapp_dir}" run test:coverage

readonly required_reports=(
  "${webapp_dir}/coverage/coverage-summary.json"
  "${webapp_dir}/coverage/index.html"
  "${webapp_dir}/coverage/lcov.info"
)
for report in "${required_reports[@]}"; do
  if [[ ! -s "${report}" ]]; then
    printf 'frontend coverage report is missing or empty: %s\n' "${report}" >&2
    exit 1
  fi
done
