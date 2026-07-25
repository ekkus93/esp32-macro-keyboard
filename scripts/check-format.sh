#!/usr/bin/env bash
set -euo pipefail

# First-party C only. firmware/test_app/main (not firmware/test_app) excludes the
# fetched managed_components (TinyUSB) and build dirs nested under test_app.
mapfile -t c_files < <(find firmware/main firmware/components firmware/test_app/main \
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
	cmake-format --check "${cmake_files[@]}"
	cmake-lint "${cmake_files[@]}"
fi

shfmt -d scripts/*.sh

cd webapp
npm run format:check
