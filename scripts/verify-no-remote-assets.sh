#!/usr/bin/env bash
set -euo pipefail

readonly directory="${1:?usage: verify-no-remote-assets.sh DIRECTORY}"
if grep -RInE --include='*.html' --include='*.css' --include='*.js' \
    '(https?:)?//' "${directory}"; then
    printf 'error: production web assets reference a remote URL\n' >&2
    exit 1
fi
