#!/usr/bin/env bash
# Fake idf.py for the generate-flash-manifest.sh regression tests. Only
# understands `--version`, printing FAKE_IDF_VERSION (or a default) so the
# manifest generator's toolchain-presence check and version field can be
# exercised deterministically without a real ESP-IDF environment.
set -euo pipefail

if [ "${1:-}" = "--version" ]; then
	printf '%s\n' "${FAKE_IDF_VERSION:-ESP-IDF v5.5.5}"
	exit 0
fi

printf 'fake idf.py: unexpected arguments: %s\n' "$*" >&2
exit 2
