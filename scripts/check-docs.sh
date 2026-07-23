#!/usr/bin/env bash
set -euo pipefail

npx --yes markdownlint-cli2 '**/*.md' '#webapp/node_modules' '#firmware/managed_components'
yamllint .github firmware/main/idf_component.yml
jq --exit-status empty docs/schemas/*.json
