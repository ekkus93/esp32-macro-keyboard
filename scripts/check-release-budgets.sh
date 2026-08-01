#!/usr/bin/env bash
set -euo pipefail

# FIX1 21.1: fail a release build when it is too close to its size/memory
# budgets to have any real headroom left. Requires a completed
# `idf.py -C firmware build` (reads its .bin/.map from --build-dir).
#
# Two of the five checks below need artifacts this repository does not yet
# produce automatically, and are optional rather than fatal when absent -
# see the per-check comments for why. Every check either passes, fails, or
# is loudly SKIPPED; none is silently treated as passing.

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

build_dir="firmware/build"
partitions_csv="firmware/partitions.csv"
webfs_image=""
stack_report=""

while [ $# -gt 0 ]; do
	case "$1" in
	--build-dir)
		build_dir="$2"
		shift 2
		;;
	--partitions)
		partitions_csv="$2"
		shift 2
		;;
	--webfs-image)
		webfs_image="$2"
		shift 2
		;;
	--stack-report)
		stack_report="$2"
		shift 2
		;;
	*)
		printf 'error: unknown argument: %s\n' "$1" >&2
		exit 2
		;;
	esac
done

app_bin="${build_dir}/esp32_macro_keyboard.bin"
app_map="${build_dir}/esp32_macro_keyboard.map"
[ -f "${app_bin}" ] || {
	printf 'error: application binary not found: %s (run idf.py -C firmware build first)\n' \
		"${app_bin}" >&2
	exit 2
}
[ -f "${app_map}" ] || {
	printf 'error: application map file not found: %s\n' "${app_map}" >&2
	exit 2
}
[ -f "${partitions_csv}" ] || {
	printf 'error: partition table not found: %s\n' "${partitions_csv}" >&2
	exit 2
}

if [ -z "${webfs_image}" ] && [ -f "${build_dir}/webfs.bin" ]; then
	webfs_image="${build_dir}/webfs.bin"
fi

# esp_idf_size needs the ESP-IDF Python environment (same interpreter idf.py
# itself uses); it is not a plain pip package on the host Python.
size_json="$(python3 -m esp_idf_size --format json "${app_map}")"
app_bin_bytes="$(stat -c '%s' "${app_bin}")"

python3 - "${partitions_csv}" "${app_bin_bytes}" "${webfs_image}" "${stack_report}" "${size_json}" <<'PY2'
import csv
import json
import sys
from pathlib import Path

partitions_path = Path(sys.argv[1])
app_bin_bytes = int(sys.argv[2])
webfs_image_path = sys.argv[3] or None
stack_report_path = sys.argv[4] or None
size_json = json.loads(sys.argv[5])

# Reasonable release-engineering defaults, not values dictated anywhere else
# in this repo's spec: chosen now, adjustable later as real usage data comes
# in. Each is documented with why that number, not just what it is.
OTA_BUDGET_RATIO = 0.80  # leave 20% of the OTA slot for future growth
WEBFS_BUDGET_RATIO = 0.85  # web assets need less update headroom than firmware
STATIC_RAM_BUDGET_RATIO = 0.75  # leave 25% of DIRAM for the runtime heap
USERDATA_STRUCTURAL_OVERHEAD_BYTES = 8 * 1024  # schema/index files, generously rounded
USERDATA_MINIMUM_USABLE_BYTES = 256 * 1024  # must still fit real macro/procedure content
STACK_MARGIN_RATIO = 0.20  # high-water mark must stay above 20% of configured stack

# Configured FreeRTOS stack sizes in words (xTaskCreate's stack_depth argument),
# not discoverable from a runtime report alone - see the cited source lines.
TASK_STACK_WORDS = {
    "controls": 2048,  # firmware/components/device_controls/device_controls.c xTaskCreate
    "executor": 4096,  # firmware/components/macro_executor/macro_executor.c xTaskCreate
}

failures = []
warnings = []


def check(label, used, budget, unit="bytes"):
    ratio = used / budget if budget else 1.0
    status = "OK" if used <= budget else "FAIL"
    print(f"[{status}] {label}: {used} / {budget} {unit} ({ratio:.1%})")
    if used > budget:
        failures.append(label)


def read_partition_sizes(path):
    sizes = {}
    with path.open(encoding="utf-8") as handle:
        rows = (
            line
            for line in handle
            if line.strip() and not line.lstrip().startswith("#")
        )
        for row in csv.reader(rows):
            if len(row) != 6:
                continue
            name = row[0].strip()
            raw_size = row[4].strip()
            if raw_size:
                sizes[name] = int(raw_size, 0)
    return sizes


partitions = read_partition_sizes(partitions_path)
for required in ("ota_0", "webfs", "userdata"):
    if required not in partitions:
        print(f"error: partition table has no {required} partition", file=sys.stderr)
        sys.exit(2)

# 1. Application binary vs. OTA slot budget.
check(
    "application binary vs. OTA slot budget",
    app_bin_bytes,
    int(partitions["ota_0"] * OTA_BUDGET_RATIO),
)

# 2. webfs image vs. partition budget - optional: no script in this repo
# generates the webfs LittleFS image yet (FIX1 20.1 finding: SPEC 23's
# packaging pipeline - gzip generation, image generation - is not
# automated), so there is usually nothing to measure. Skip rather than fail
# the whole gate over a separately-tracked, larger gap.
def littlefs_used_bytes(image_path, block_size=4096):
    # A LittleFS image file is always padded to the full partition size
    # (--fs-size), so raw file size always equals capacity, never usage.
    # Count non-erased (non-0xFF) blocks instead, matching how FIX1 20.1's
    # webfs measurement was taken.
    data = Path(image_path).read_bytes()
    erased_block = b"\xff" * block_size
    used_blocks = sum(
        1
        for offset in range(0, len(data), block_size)
        if data[offset:offset + block_size] != erased_block
    )
    return used_blocks * block_size


if webfs_image_path is None:
    warnings.append(
        "webfs image budget SKIPPED: no image found (no build step generates "
        "one yet; pass --webfs-image to check a manually-built one)"
    )
    print(f"[SKIP] {warnings[-1]}")
else:
    webfs_used_bytes = littlefs_used_bytes(webfs_image_path)
    check(
        "webfs image vs. partition budget",
        webfs_used_bytes,
        int(partitions["webfs"] * WEBFS_BUDGET_RATIO),
    )

# 3. userdata partition must still hold a meaningful amount of real content
# after fixed structural overhead (schema/index files) - not a check against
# the theoretical maximum every object at its own size cap could reach
# (50 sets x 100 max-size macros would need far more than any reasonable
# flash budget; real usage stays well under each object's own cap).
userdata_usable = partitions["userdata"] - USERDATA_STRUCTURAL_OVERHEAD_BYTES
status = "OK" if userdata_usable >= USERDATA_MINIMUM_USABLE_BYTES else "FAIL"
print(
    f"[{status}] userdata usable space after overhead: {userdata_usable} / "
    f"{USERDATA_MINIMUM_USABLE_BYTES} bytes minimum"
)
if userdata_usable < USERDATA_MINIMUM_USABLE_BYTES:
    failures.append("userdata minimum free-space requirement")

# 4. Static RAM (DIRAM: .data + .bss + the portion of .text placed in DIRAM)
# vs. budget, leaving headroom for the runtime heap.
diram_total = size_json["diram_total"]
used_diram = size_json["used_diram"]
check(
    "static RAM (DIRAM) vs. budget",
    used_diram,
    int(diram_total * STATIC_RAM_BUDGET_RATIO),
)

# 5. Task stack high-water marks vs. minimum margin - optional: this is a
# runtime measurement (uxTaskGetStackHighWaterMark), not obtainable from a
# static build artifact. Pass --stack-report with a JSON object such as
# {"controls_stack_words": 1208, "executor_stack_words": 3200} (e.g.
# captured from a real /api/v1/diagnostics response) to check it.
if stack_report_path is None:
    warnings.append(
        "task stack margin SKIPPED: no --stack-report given (this is a "
        "runtime measurement, not available from a build alone)"
    )
    print(f"[SKIP] {warnings[-1]}")
else:
    report = json.loads(Path(stack_report_path).read_text(encoding="utf-8"))
    for task_name, configured_words in TASK_STACK_WORDS.items():
        key = f"{task_name}_stack_words"
        if key not in report:
            print(f"error: --stack-report is missing required key {key!r}", file=sys.stderr)
            sys.exit(2)
        label = f"{task_name} task stack high-water mark"
        measured = report[key]
        minimum_required = int(configured_words * STACK_MARGIN_RATIO)
        status = "OK" if measured >= minimum_required else "FAIL"
        print(
            f"[{status}] {label}: {measured} words measured, "
            f"{minimum_required} words minimum ({configured_words} configured)"
        )
        if measured < minimum_required:
            failures.append(label)

if warnings:
    print(f"\n{len(warnings)} check(s) skipped - see [SKIP] lines above.")
if failures:
    print(f"\nerror: {len(failures)} release budget(s) exceeded: {', '.join(failures)}", file=sys.stderr)
    sys.exit(1)
print("\nrelease budgets within threshold")
PY2
