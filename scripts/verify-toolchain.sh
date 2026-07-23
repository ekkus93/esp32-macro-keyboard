#!/usr/bin/env bash
set -euo pipefail

readonly expected_idf="v5.5.5"
readonly expected_target="esp32s3"
readonly expected_node="24.18.0"
readonly idf_path="${IDF_PATH:?IDF_PATH is not set; source the ESP-IDF export.sh first}"

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

actual_node="$(node --version 2>/dev/null || printf 'missing')"
if [[ "${actual_node}" != "v${expected_node}" ]]; then
    printf 'error: Node.js v%s is required; active version is %s\n' \
        "${expected_node}" "${actual_node}" >&2
    exit 1
fi

if [[ "${IDF_TARGET:-${expected_target}}" != "${expected_target}" ]]; then
    printf 'error: IDF_TARGET must be %s\n' "${expected_target}" >&2
    exit 1
fi

printf 'Toolchain verified: ESP-IDF %s, target %s, Node.js v%s\n' \
    "${expected_idf}" "${expected_target}" "${expected_node}"
