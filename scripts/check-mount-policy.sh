#!/usr/bin/env bash
set -euo pipefail

# SPEC 13.2: "Firmware MUST NOT automatically format either filesystem."
# Formatting user data is allowed only through an explicit factory-reset/repair
# operation, never as a reaction to a mount failure.
#
# This is a one-field invariant in an ESP-IDF struct literal, so no host test can
# reach it: esp_littlefs.h does not exist on the host, and the mount seam that
# host tests do exercise (storage_mount_core) sits above the backend where the
# flag lives. A source check is the only enforcement available, and the
# requirement is worth enforcing: a device whose user data silently reformats on
# a bad mount destroys the macros it exists to store, and reports success.
#
# The inverse failure is also real and was hit on this project: with formatting
# correctly disabled, a corrupt or absent filesystem leaves the device unable to
# boot until an image is flashed. That is the intended trade -- visible failure
# over silent data loss (SPEC 13.2: "A mount failure is a visible fatal or
# degraded-storage state").

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

sources=(firmware/components firmware/main)
readonly sources

# Any assignment of a true-ish value to format_if_mount_failed is a violation.
if matches="$(grep -rn --include='*.c' --include='*.h' \
	-E '\.format_if_mount_failed[[:space:]]*=[[:space:]]*(true|1)\b' \
	"${sources[@]}" 2>/dev/null)"; then
	echo "error: firmware enables automatic filesystem formatting (SPEC 13.2)" >&2
	echo "${matches}" >&2
	exit 1
fi

# Every registration must set the field explicitly. Relying on a zero-initialized
# struct would satisfy the check above while leaving the policy undocumented at
# the call site, and a later refactor to a non-designated initializer would
# silently flip it.
mapfile -t registrations < <(
	grep -rln --include='*.c' 'esp_vfs_littlefs_conf_t' "${sources[@]}" 2>/dev/null || true
)
if [ "${#registrations[@]}" -eq 0 ]; then
	echo "error: no esp_vfs_littlefs_conf_t registration found; has the mount moved? (SPEC 13.2)" >&2
	exit 1
fi
for file in "${registrations[@]}"; do
	conf_count="$(grep -c 'esp_vfs_littlefs_conf_t' "${file}")"
	flag_count="$(grep -c '\.format_if_mount_failed[[:space:]]*=' "${file}" || true)"
	if [ "${flag_count}" -lt "${conf_count}" ]; then
		echo "error: ${file} declares ${conf_count} LittleFS configuration(s) but sets" >&2
		echo "       format_if_mount_failed in only ${flag_count} of them (SPEC 13.2)" >&2
		exit 1
	fi
done

echo "mount policy: automatic formatting disabled at every LittleFS registration"
