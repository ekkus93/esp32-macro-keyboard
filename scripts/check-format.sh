#!/usr/bin/env bash
set -euo pipefail

# First-party C only. firmware/test_app/main (not firmware/test_app) excludes the
# fetched managed_components (TinyUSB) and build dirs nested under test_app. Host
# and v2 contract tests are first-party too; prune generated build directories.
mapfile -t c_files < <(find \
	firmware/main firmware/components firmware/test_app/main tests/host tests/v2_contracts \
	-type d -name 'build*' -prune -o \
	-type f \( -name '*.c' -o -name '*.h' \) -print | sort)
if ((${#c_files[@]} > 0)); then
	clang-format --dry-run --Werror "${c_files[@]}"
fi

# Prune fetched third-party (managed_components) and generated build dirs so only
# first-party CMakeLists are checked.
mapfile -t cmake_files < <(find firmware tests \
	-type d \( -name managed_components -o -name 'build*' \) -prune -o \
	-type f -name 'CMakeLists.txt' -print | sort)
if ((${#cmake_files[@]} > 0)); then
	if ! cmake-format --check "${cmake_files[@]}"; then
		for cmake_file in "${cmake_files[@]}"; do
			formatted_file="$(mktemp)"
			cmake-format "${cmake_file}" >"${formatted_file}"
			diff -u "${cmake_file}" "${formatted_file}" || true
			rm -f "${formatted_file}"
		done
		exit 1
	fi
	cmake-lint "${cmake_files[@]}"
fi

shfmt -d scripts/*.sh

cd webapp
npm run format:check
