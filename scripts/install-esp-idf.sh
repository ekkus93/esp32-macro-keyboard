#!/usr/bin/env bash
set -euo pipefail

readonly expected_tag="v5.5.5"
readonly install_root="${ESP_IDF_INSTALL_ROOT:-${HOME}/esp}"
readonly destination="${install_root}/esp-idf-${expected_tag}"
readonly repository="https://github.com/espressif/esp-idf.git"

mkdir -p -- "${install_root}"

if [[ -e "${destination}" && ! -d "${destination}/.git" ]]; then
    printf 'error: %s exists but is not an ESP-IDF git checkout\n' "${destination}" >&2
    exit 1
fi

if [[ ! -d "${destination}/.git" ]]; then
    git clone --branch "${expected_tag}" --depth 1 --recurse-submodules \
        "${repository}" "${destination}"
else
    git -C "${destination}" fetch --tags --force origin "refs/tags/${expected_tag}:refs/tags/${expected_tag}"
    git -C "${destination}" checkout --detach "${expected_tag}"
    git -C "${destination}" submodule sync --recursive
    git -C "${destination}" submodule update --init --recursive
fi

actual="$(git -C "${destination}" describe --tags --exact-match)"
if [[ "${actual}" != "${expected_tag}" ]]; then
    printf 'error: expected %s but installed %s\n' "${expected_tag}" "${actual}" >&2
    exit 1
fi

"${destination}/install.sh" esp32s3
printf 'ESP-IDF %s installed at %s\n' "${expected_tag}" "${destination}"
