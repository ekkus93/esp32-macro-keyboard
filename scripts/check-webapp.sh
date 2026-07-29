#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}/webapp"

npm ci
npm run format:check
npm run typecheck
npm run lint
npm run stylelint
npm run test
npm run test:coverage
npm run build
"${repo_root}/scripts/verify-no-remote-assets.sh" "${repo_root}/webapp/dist"
