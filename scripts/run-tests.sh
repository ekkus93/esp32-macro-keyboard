#!/usr/bin/env bash
set -euo pipefail

readonly repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly build_dir="${repo_root}/tests/host/build"
readonly valid_labels="support parser storage executor auth web startup usb controls wifi model"

label=""
if (( $# > 1 )); then
  printf 'usage: %s [label]\n' "$0" >&2
  exit 2
fi
if (( $# == 1 )); then
  label="$1"
  case " ${valid_labels} " in
    *" ${label} "*) ;;
    *)
      printf 'unknown host-test label: %s\nvalid labels: %s\n' "${label}" "${valid_labels}" >&2
      exit 2
      ;;
  esac
fi

cmake -S "${repo_root}/tests/host" -B "${build_dir}"
cmake --build "${build_dir}" --parallel

ctest_args=(--test-dir "${build_dir}" --output-on-failure)
if [[ -n "${label}" ]]; then
  ctest_args+=(-L "${label}")
fi
ctest "${ctest_args[@]}"
