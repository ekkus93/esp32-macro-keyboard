#!/usr/bin/env bash
set -euo pipefail

# FIX1 TODO §20.1 found that SPEC §23's webfs packaging pipeline (build the
# production frontend, generate gzip variants, stage them under
# firmware/webfs, package them into a LittleFS image) had no automation:
# `idf.py -C firmware build` alone never produces a populated webfs image.
# This script automates exactly that pipeline. It writes
# firmware/build/webfs.bin by default - the path
# scripts/check-release-budgets.sh already auto-detects, so that script's
# webfs-image budget check flips from a documented [SKIP] to a real
# enforced gate once this runs before it.
#
# Requires littlefs-python==0.15.0 (the exact version
# firmware/managed_components/joltwallet__littlefs/image-building-requirements.txt
# pins, so this produces the same image shape the managed component's own
# CMake macro, littlefs_create_partition_image(), would if it were wired
# into the ESP-IDF build):
#   pip install littlefs-python==0.15.0

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

webapp_dir="webapp"
staging_dir="firmware/webfs/dist"
partitions_csv="firmware/partitions.csv"
build_dir="firmware/build"
output_image=""
skip_frontend_build=0

while [ $# -gt 0 ]; do
	case "$1" in
	--webapp-dir)
		webapp_dir="$2"
		shift 2
		;;
	--staging-dir)
		staging_dir="$2"
		shift 2
		;;
	--partitions)
		partitions_csv="$2"
		shift 2
		;;
	--build-dir)
		build_dir="$2"
		shift 2
		;;
	--output)
		output_image="$2"
		shift 2
		;;
	--skip-frontend-build)
		skip_frontend_build=1
		shift
		;;
	*)
		printf 'error: unknown argument: %s\n' "$1" >&2
		exit 2
		;;
	esac
done

[ -n "${output_image}" ] || output_image="${build_dir}/webfs.bin"

command -v littlefs-python >/dev/null 2>&1 || {
	printf 'error: littlefs-python not found. Install the exact pinned version:\n' >&2
	printf '  pip install littlefs-python==0.15.0\n' >&2
	printf '(the version firmware/managed_components/joltwallet__littlefs/image-building-requirements.txt pins)\n' >&2
	exit 2
}

[ -f "${partitions_csv}" ] || {
	printf 'error: partition table not found: %s\n' "${partitions_csv}" >&2
	exit 2
}

# 1. Build production frontend assets.
if [ "${skip_frontend_build}" -eq 0 ]; then
	npm --prefix "${webapp_dir}" run build
fi

dist_dir="${webapp_dir}/dist"
if [ ! -d "${dist_dir}" ] || [ -z "$(find "${dist_dir}" -type f -print -quit)" ]; then
	printf 'error: no built frontend assets found at %s (run "npm --prefix %s run build" first, or omit --skip-frontend-build)\n' \
		"${dist_dir}" "${webapp_dir}" >&2
	exit 2
fi

# 2. Stage a clean copy - never carry over a previous run's stale content,
# and never risk shipping firmware/webfs/README.md inside the image.
rm -rf -- "${staging_dir}"
mkdir -p -- "${staging_dir}"
cp -R "${dist_dir}/." "${staging_dir}/"

# 3. Generate a .gz sibling for every staged file, matching
# web_adapter_open_static_file's compressed-variant lookup
# (firmware/components/web_server/web_server_adapter_static_stream.c).
# The full file list is captured before any gzip runs, so newly created
# .gz siblings are never themselves re-gzipped.
mapfile -d '' staged_files < <(find "${staging_dir}" -type f -print0)
for file in "${staged_files[@]}"; do
	gzip -9 -k -f -- "${file}"
done

# 4. Resolve the webfs partition's configured size from the partition table
# (same parsing approach as check-release-budgets.sh's read_partition_sizes).
webfs_size="$(
	python3 - "${partitions_csv}" <<'PY'
import csv
import sys
from pathlib import Path

partitions_path = Path(sys.argv[1])
with partitions_path.open(encoding="utf-8") as handle:
    rows = (line for line in handle if line.strip() and not line.lstrip().startswith("#"))
    for row in csv.reader(rows):
        if len(row) == 6 and row[0].strip() == "webfs":
            print(int(row[4].strip(), 0))
            break
    else:
        sys.exit("error: partition table has no webfs partition")
PY
)"

# 5. Package the staged assets into a LittleFS image sized to the partition.
# --name-max=64 matches CONFIG_LITTLEFS_OBJ_NAME_LEN's Kconfig default (see
# firmware/managed_components/joltwallet__littlefs/Kconfig) - the same value
# littlefs_create_partition_image() would pass, since firmware/sdkconfig.defaults
# does not override it. If that Kconfig option is ever overridden, update this
# constant to match, or the resulting image will not match a CMake-generated one.
mkdir -p -- "${build_dir}"
littlefs-python create "${staging_dir}" "${output_image}" \
	--fs-size="${webfs_size}" --name-max=64 --block-size=4096

printf 'webfs image written: %s (%s bytes, partition budget %s bytes)\n' \
	"${output_image}" "$(stat -c '%s' "${output_image}")" "${webfs_size}"
