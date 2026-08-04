#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

# Ignore third-party (node_modules, fetched managed_components at any depth,
# including under firmware/test_app/) and generated build trees.
./webapp/node_modules/.bin/markdownlint-cli2 \
	'**/*.md' '#**/node_modules' '#**/managed_components' '#tests/host/build'
yamllint .github firmware/main/idf_component.yml
for schema in docs/schemas/*.json; do
	jq empty "${schema}"
done

# The v2 traceability worklist fingerprints both authoritative specifications.
# Any spec change must regenerate the checked-in report rather than leaving a
# stale or zero-row reassurance behind.
python3 scripts/generate-spec-traceability.py --check
