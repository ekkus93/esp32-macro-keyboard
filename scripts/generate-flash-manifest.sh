#!/usr/bin/env bash
set -euo pipefail

# Automates the last stage of SPEC §23's build pipeline ("generate flash
# manifest"). SPEC §23 requires the build to record: git commit, dirty/clean
# state, ESP-IDF version, managed-component lock hash, frontend lockfile
# hash, build type, and build timestamp. This script records exactly those
# fields, plus the resolved flash image list (bootloader/partition-table/
# otadata/app from ESP-IDF's own flasher_args.json, and webfs.bin's
# resolved partition offset if scripts/build-webfs-image.sh has already
# produced one), into one JSON artifact.
#
# The timestamp is metadata about *this manifest*, not embedded into the
# flashed binaries themselves - it does not weaken "release builds MUST be
# reproducible from committed sources and lockfiles": two builds from the
# identical commit and lockfiles still produce byte-identical flashable
# artifacts, just with a different manifest timestamp recording when each
# was assembled.
#
# Requires a completed `idf.py -C firmware build` with the ESP-IDF
# environment sourced (`idf.py` on PATH).

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

build_dir="firmware/build"
sdkconfig="firmware/sdkconfig"
component_lock="firmware/dependencies.lock"
frontend_lock="webapp/package-lock.json"
output_file=""

while [ $# -gt 0 ]; do
	case "$1" in
	--build-dir)
		build_dir="$2"
		shift 2
		;;
	--sdkconfig)
		sdkconfig="$2"
		shift 2
		;;
	--component-lock)
		component_lock="$2"
		shift 2
		;;
	--frontend-lock)
		frontend_lock="$2"
		shift 2
		;;
	--output)
		output_file="$2"
		shift 2
		;;
	*)
		printf 'error: unknown argument: %s\n' "$1" >&2
		exit 2
		;;
	esac
done

[ -n "${output_file}" ] || output_file="${build_dir}/flash-manifest.json"

flasher_args="${build_dir}/flasher_args.json"
[ -f "${flasher_args}" ] || {
	printf 'error: %s not found (run idf.py -C firmware build first)\n' "${flasher_args}" >&2
	exit 2
}

# ESP-IDF resolves sdkconfig at the project root, not inside build_dir.
[ -f "${sdkconfig}" ] || {
	printf 'error: %s not found (run idf.py -C firmware build first)\n' "${sdkconfig}" >&2
	exit 2
}

partition_table_bin="${build_dir}/partition_table/partition-table.bin"
[ -f "${partition_table_bin}" ] || {
	printf 'error: %s not found (run idf.py -C firmware build first)\n' "${partition_table_bin}" >&2
	exit 2
}

[ -f "${component_lock}" ] || {
	printf 'error: managed-component lockfile not found: %s\n' "${component_lock}" >&2
	exit 2
}

[ -f "${frontend_lock}" ] || {
	printf 'error: frontend lockfile not found: %s\n' "${frontend_lock}" >&2
	exit 2
}

command -v idf.py >/dev/null 2>&1 || {
	printf 'error: idf.py not found; source the pinned ESP-IDF v5.5.5 export.sh first (see CLAUDE.md).\n' >&2
	exit 2
}

command -v esptool.py >/dev/null 2>&1 || {
	printf 'error: esptool.py not found; source the pinned ESP-IDF v5.5.5 export.sh first (see CLAUDE.md).\n' >&2
	exit 2
}

idf_version="$(idf.py --version)"
[ -n "${idf_version}" ] || {
	printf 'error: idf.py --version produced no output\n' >&2
	exit 2
}

app_relative="$(
	python3 - "${flasher_args}" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as handle:
    flash_files = json.load(handle)["flash_files"]

candidates = [
    path for path in flash_files.values()
    if path.endswith(".bin") and path.rsplit("/", 1)[-1] == "esp32_macro_keyboard.bin"
]
if len(candidates) != 1:
    raise SystemExit("expected exactly one esp32_macro_keyboard.bin application image")
print(candidates[0])
PY
)"
app_image="${build_dir}/${app_relative}"
[ -f "${app_image}" ] || {
	printf 'error: application image not found: %s\n' "${app_image}" >&2
	exit 2
}
app_image_sha256="$(sha256sum -- "${app_image}" | awk '{print $1}')"
image_info="$(esptool.py image_info --version 2 "${app_image}")"
app_elf_sha256="$(printf '%s\n' "${image_info}" | python3 -c 'import re,sys; text=sys.stdin.read(); match=re.search(r"ELF file SHA256:\s*([0-9A-Fa-f]{64})", text); print(match.group(1).lower() if match else "")')"
if ! printf '%s' "${app_elf_sha256}" | grep -Eq '^[0-9a-f]{64}$'; then
	printf 'error: esptool.py image_info did not report a full ELF file SHA256\n' >&2
	exit 2
fi
diagnostics_build_id="${app_elf_sha256:0:39}"

git_commit="$(git -C "${repo_root}" rev-parse HEAD)"
if [ -n "$(git -C "${repo_root}" status --porcelain)" ]; then
	git_dirty=true
else
	git_dirty=false
fi

component_lock_sha256="$(sha256sum -- "${component_lock}" | awk '{print $1}')"
frontend_lock_sha256="$(sha256sum -- "${frontend_lock}" | awk '{print $1}')"

# "Build type" in this codebase's actual safety model is production vs.
# development/manufacturing - whether the resolved sdkconfig for this
# specific build has either credential-logging Kconfig option enabled
# (scripts/check-production-config.sh enforces these are both off in the
# *committed* firmware/sdkconfig.defaults; this checks the *resolved* build
# config, which could differ if flashed from an uncommitted local override).
build_type="production"
if grep -qE '^CONFIG_APP_(DEVELOPMENT|MANUFACTURING)_PROVISIONING_LOG=y$' "${sdkconfig}"; then
	build_type="development"
fi
if ! grep -qx 'CONFIG_APP_RETRIEVE_LEN_ELF_SHA=39' "${sdkconfig}"; then
	printf 'error: resolved build must set CONFIG_APP_RETRIEVE_LEN_ELF_SHA=39 for diagnostics provenance\n' >&2
	exit 2
fi

build_timestamp="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

webfs_relative=""
webfs_offset=""
if [ -f "${build_dir}/webfs.bin" ]; then
	webfs_relative="webfs.bin"
	webfs_offset="$(
		python3 "${IDF_PATH}/components/partition_table/gen_esp32part.py" \
			"${partition_table_bin}" 2>/dev/null |
			awk -F',' '$1 == "webfs" {print $4}'
	)"
	[ -n "${webfs_offset}" ] || {
		printf 'error: could not resolve the webfs partition offset from %s\n' \
			"${partition_table_bin}" >&2
		exit 2
	}
fi

python3 - "${output_file}" "${flasher_args}" "${git_commit}" "${git_dirty}" \
	"${idf_version}" "${component_lock_sha256}" "${frontend_lock_sha256}" \
	"${build_type}" "${build_timestamp}" "${webfs_relative}" "${webfs_offset}" \
	"${app_relative}" "${app_image_sha256}" "${app_elf_sha256}" "${diagnostics_build_id}" <<'PY'
import hashlib
import json
import os
import sys

(output_path, flasher_args_path, git_commit, git_dirty, idf_version,
 component_lock_sha256, frontend_lock_sha256, build_type, build_timestamp,
 webfs_relative, webfs_offset, app_relative, app_image_sha256,
 app_elf_sha256, diagnostics_build_id) = sys.argv[1:16]

with open(flasher_args_path, encoding="utf-8") as handle:
    flasher_args = json.load(handle)

flash_files = dict(flasher_args["flash_files"])
if webfs_relative:
    flash_files[webfs_offset] = webfs_relative

build_dir = os.path.dirname(flasher_args_path)
flash_file_sha256 = {}
for offset, relative_path in flash_files.items():
    full_path = os.path.join(build_dir, relative_path)
    if not os.path.isfile(full_path):
        raise SystemExit(f"flash file not found: {full_path}")
    digest = hashlib.sha256()
    with open(full_path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    flash_file_sha256[offset] = digest.hexdigest()

manifest = {
    "gitCommit": git_commit,
    "gitDirty": git_dirty == "true",
    "espIdfVersion": idf_version,
    "managedComponentLockSha256": component_lock_sha256,
    "frontendLockSha256": frontend_lock_sha256,
    "buildType": build_type,
    "buildTimestamp": build_timestamp,
    "appImage": app_relative,
    "appImageSha256": app_image_sha256,
    "appElfSha256": app_elf_sha256,
    "diagnosticsBuildId": diagnostics_build_id,
    "flashSettings": flasher_args["flash_settings"],
    "flashFiles": dict(sorted(flash_files.items(), key=lambda item: int(item[0], 16))),
    "flashFileSha256": dict(
        sorted(flash_file_sha256.items(), key=lambda item: int(item[0], 16))
    ),
}

with open(output_path, "w", encoding="utf-8") as handle:
    json.dump(manifest, handle, indent=2, sort_keys=False)
    handle.write("\n")

print(f"flash manifest written: {output_path}")
PY
