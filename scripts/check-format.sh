#!/usr/bin/env bash
set -euo pipefail

# First-party C only. firmware/test_app/main (not firmware/test_app) excludes the
# fetched managed_components (TinyUSB) and build dirs nested under test_app. Host
# and v2 contract tests are first-party too; prune generated build directories.
#
# *.inc is included because the large host suites keep their test bodies in
# fragments that one test_*.c includes (auth, executor, web security, the
# web-server adapter). Those are first-party C by every measure except the file
# extension, and leaving them out silently exempted ~4,000 lines from formatting.
mapfile -t c_files < <(find \
	firmware/main firmware/components firmware/test_app/main tests/host tests/v2_contracts \
	-type d -name 'build*' -prune -o \
	-type f \( -name '*.c' -o -name '*.h' -o -name '*.inc' \) -print | sort)
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
