#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
readonly webapp_dir="${repo_root}/webapp"

if ! command -v npm >/dev/null 2>&1; then
	printf 'npm is required for frontend coverage generation\n' >&2
	exit 1
fi

rm -rf -- "${webapp_dir}/coverage"

# The coverage provider (@vitest/coverage-v8) is a committed devDependency, so a
# plain lockfile-reproducible install is sufficient — no dynamic add step.
npm --prefix "${webapp_dir}" ci --no-audit --no-fund
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
