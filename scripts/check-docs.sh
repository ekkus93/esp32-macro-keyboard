#!/usr/bin/env bash
set -euo pipefail

readonly repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

./webapp/node_modules/.bin/markdownlint-cli2 \
    '**/*.md' '#webapp/node_modules' '#firmware/managed_components' '#tests/host/build'
yamllint .github firmware/main/idf_component.yml
for schema in docs/schemas/*.json; do
    jq empty "${schema}"
done
