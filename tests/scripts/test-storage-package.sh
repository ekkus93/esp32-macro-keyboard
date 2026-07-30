#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
build_dir="$(mktemp -d)"
readonly build_dir
trap 'rm -rf -- "${build_dir}"' EXIT

mapfile -t cjson_flags < <(pkg-config --cflags --libs libcjson | xargs -n1 printf '%s\n')

cc \
	-std=c11 \
	-Wall \
	-Wextra \
	-Werror \
	-Wshadow \
	-Wconversion \
	-Wsign-conversion \
	-Wformat=2 \
	-Wundef \
	-Wdouble-promotion \
	-Wmissing-declarations \
	-Wstrict-prototypes \
	-I"${repo_root}/tests/host/support" \
	-I"${repo_root}/firmware/components/macro_model/include" \
	-I"${repo_root}/firmware/components/macro_parser/include" \
	-I"${repo_root}/firmware/components/storage/include" \
	-I"${repo_root}/firmware/components/storage" \
	"${repo_root}/tests/host/test_storage_package.c" \
	"${repo_root}/tests/host/support/test_assert.c" \
	"${repo_root}/firmware/components/macro_model/app_error.c" \
	"${repo_root}/firmware/components/macro_model/app_uuid.c" \
	"${repo_root}/firmware/components/macro_model/macro_model.c" \
	"${repo_root}/firmware/components/macro_parser/macro_parser.c" \
	"${repo_root}/firmware/components/macro_parser/macro_keymap_us.c" \
	"${repo_root}/firmware/components/storage/storage_json.c" \
	"${repo_root}/firmware/components/storage/storage_repository_json.c" \
	"${repo_root}/firmware/components/storage/storage_repository_objects_json.c" \
	"${repo_root}/firmware/components/storage/storage_package.c" \
	"${cjson_flags[@]}" \
	-o "${build_dir}/storage-package-tests"

"${build_dir}/storage-package-tests"
