#!/usr/bin/env bash
# Fake esptool.py for flash-manifest regression tests. It only implements
# `image_info`, returning a deterministic full ELF SHA-256.
set -euo pipefail

if [ "${1:-}" = "image_info" ] && [ -n "${2:-}" ]; then
	printf 'Application Information\n'
	printf 'ELF file SHA256: %s\n' "${FAKE_ELF_SHA256:-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa}"
	exit 0
fi

printf 'fake esptool.py: unexpected arguments: %s\n' "$*" >&2
exit 2
