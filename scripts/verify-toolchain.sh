#!/usr/bin/env bash
set -euo pipefail

# SPEC 5.1: "The firmware MUST build against the exact signed ESP-IDF tag" and
# "The build MUST reject an unrecognized ESP-IDF version."
# SPEC 5.4: "The Node.js major version MUST be pinned in the repository."
# Both are properties of the environment a build runs in, so this is where they
# are enforced rather than in any test.
readonly expected_idf="v5.5.5"
readonly expected_target="esp32s3"
readonly expected_node="24.18.0"
readonly idf_path="${IDF_PATH:?IDF_PATH is not set; source the ESP-IDF export.sh first}"

verify_node=true
if (($# > 1)); then
	printf 'usage: %s [--firmware-only]\n' "$0" >&2
	exit 2
fi
if (($# == 1)); then
	if [[ "$1" != "--firmware-only" ]]; then
		printf 'usage: %s [--firmware-only]\n' "$0" >&2
		exit 2
	fi
	verify_node=false
fi

if [[ ! -d "${idf_path}/.git" ]]; then
	printf 'error: IDF_PATH is not an ESP-IDF git checkout: %s\n' "${idf_path}" >&2
	exit 1
fi

actual_idf="$(git -C "${idf_path}" describe --tags --exact-match 2>/dev/null || printf 'unknown')"
if [[ "${actual_idf}" != "${expected_idf}" ]]; then
	printf 'error: ESP-IDF %s is required; active version is %s\n' \
		"${expected_idf}" "${actual_idf}" >&2
	exit 1
fi

if [[ "${IDF_TARGET:-${expected_target}}" != "${expected_target}" ]]; then
	printf 'error: IDF_TARGET must be %s\n' "${expected_target}" >&2
	exit 1
fi

if [[ "${verify_node}" == true ]]; then
	actual_node="$(node --version 2>/dev/null || printf 'missing')"
	if [[ "${actual_node}" != "v${expected_node}" ]]; then
		printf 'error: Node.js v%s is required; active version is %s\n' \
			"${expected_node}" "${actual_node}" >&2
		exit 1
	fi
	printf 'Toolchain verified: ESP-IDF %s, target %s, Node.js v%s\n' \
		"${expected_idf}" "${expected_target}" "${expected_node}"
else
	printf 'Firmware toolchain verified: ESP-IDF %s, target %s\n' \
		"${expected_idf}" "${expected_target}"
fi
