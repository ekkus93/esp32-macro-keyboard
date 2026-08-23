#!/usr/bin/env bash
set -euo pipefail

# firmware/components/storage/storage_mount.c hardcodes
# `.format_if_mount_failed = false` (SPEC 13.2: a device whose user data
# silently reformats on a transient read glitch is worse than one that halts
# and asks for help) - and nothing anywhere in firmware ever calls a format
# function for the userdata partition. That means a genuinely blank userdata
# partition (an unwritten flash chip, or one that was erased) fails to mount
# exactly like a corrupted one: storage_mount halts app_core before AP/HTTP
# ever come up, with no serial-console or web-API recovery path, because
# both live behind the same storage_mount stage. This was found and worked
# around by hand during a hardware bring-up session (2026-08-23) with
# scripts/build-webfs-image.sh's pattern; this script generalizes that fix
# so a properly-formatted, empty userdata image can be baked into a release
# flash the same way webfs.bin already is - a device shipped with this image
# pre-flashed at the userdata partition's offset boots straight into its
# normal first-run/setup flow instead of halting.
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

partitions_csv="firmware/partitions.csv"
build_dir="firmware/build"
output_image=""

while [ $# -gt 0 ]; do
	case "$1" in
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
	*)
		printf 'error: unknown argument: %s\n' "$1" >&2
		exit 2
		;;
	esac
done

[ -n "${output_image}" ] || output_image="${build_dir}/userdata.bin"

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

# Resolve the userdata partition's configured size from the partition table
# (same parsing approach as check-release-budgets.sh's read_partition_sizes
# and build-webfs-image.sh's webfs_size lookup).
userdata_size="$(
	python3 - "${partitions_csv}" <<'PY'
import csv
import sys
from pathlib import Path

partitions_path = Path(sys.argv[1])
with partitions_path.open(encoding="utf-8") as handle:
    rows = (line for line in handle if line.strip() and not line.lstrip().startswith("#"))
    for row in csv.reader(rows):
        if len(row) == 6 and row[0].strip() == "userdata":
            print(int(row[4].strip(), 0))
            break
    else:
        sys.exit("error: partition table has no userdata partition")
PY
)"

# Package an empty source tree into a LittleFS image sized to the partition -
# unlike webfs.bin, this partition ships with no content: it is the
# device's mutable repository blob store, and the only thing this image
# needs to provide is a valid, mountable, empty filesystem. --name-max=64
# matches CONFIG_LITTLEFS_OBJ_NAME_LEN's Kconfig default (see
# firmware/managed_components/joltwallet__littlefs/Kconfig) - the same value
# littlefs_create_partition_image() would pass, since firmware/sdkconfig.defaults
# does not override it. If that Kconfig option is ever overridden, update this
# constant to match, or the resulting image will not match a CMake-generated one.
empty_dir="$(mktemp -d)"
trap 'rm -rf -- "${empty_dir}"' EXIT

mkdir -p -- "${build_dir}"
littlefs-python create "${empty_dir}" "${output_image}" \
	--fs-size="${userdata_size}" --name-max=64 --block-size=4096

printf 'userdata image written: %s (%s bytes, partition budget %s bytes)\n' \
	"${output_image}" "$(stat -c '%s' "${output_image}")" "${userdata_size}"
