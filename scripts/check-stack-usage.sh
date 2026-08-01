#!/usr/bin/env bash
set -euo pipefail

# Fails the build when a first-party function's stack frame is large enough to
# threaten a FreeRTOS task stack.
#
# Why this gate exists: the host test suite runs on x86, where a thread stack is
# measured in megabytes, so a multi-kilobyte local is completely legal there and
# neither AddressSanitizer nor Valgrind has anything to report. On the ESP32-S3
# the same function runs on a task stack of a few kilobytes (main_task 8 KiB,
# httpd 24 KiB), where it silently corrupts memory or trips the FreeRTOS stack
# canary. That class of defect was previously only discoverable by crashing real
# hardware, one crash at a time. GCC's -fstack-usage reports every frame at
# compile time, so this gate converts it into a build failure.
#
# Frames above the threshold must be listed in the allowlist with their current
# size. The allowlist is a ratchet, not an amnesty: a listed function that grows
# beyond its recorded size fails, and any unlisted function over the threshold
# fails. Shrinking a frame means lowering its recorded number (or deleting the
# entry) in the same change.

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

project_dir="${repo_root}/firmware"
build_dir=""
allowlist="${repo_root}/scripts/stack-usage-allowlist.txt"
threshold=4096
do_build=1

while [ $# -gt 0 ]; do
	case "$1" in
	--project)
		project_dir="$2"
		shift 2
		;;
	--build-dir)
		build_dir="$2"
		shift 2
		;;
	--allowlist)
		allowlist="$2"
		shift 2
		;;
	--threshold)
		threshold="$2"
		shift 2
		;;
	--no-build)
		do_build=0
		shift
		;;
	*)
		printf 'error: unknown argument: %s\n' "$1" >&2
		exit 2
		;;
	esac
done

[ -n "${build_dir}" ] || build_dir="${project_dir}/build-stackusage"

if [ "${do_build}" -eq 1 ]; then
	command -v idf.py >/dev/null 2>&1 || {
		printf 'error: idf.py not found; source the pinned ESP-IDF v5.5.5 export.sh first.\n' >&2
		exit 2
	}
	# A dedicated build tree, mirroring how build-clang keeps the clang-tidy
	# database separate: -fstack-usage only adds .su side-outputs, but keeping
	# it out of the release build tree avoids churning that cache.
	idf.py -C "${project_dir}" -B "${build_dir}" -DCMAKE_C_FLAGS="-fstack-usage" build >/dev/null
fi

[ -d "${build_dir}" ] || {
	printf 'error: build directory not found: %s\n' "${build_dir}" >&2
	exit 2
}

[ -f "${allowlist}" ] || {
	printf 'error: allowlist not found: %s\n' "${allowlist}" >&2
	exit 2
}

# Collect every .su record whose source file lives in this project's own
# firmware tree, excluding ESP-IDF and managed (third-party) components.
frames="$(mktemp)"
trap 'rm -f -- "${frames}"' EXIT
find "${build_dir}" -name '*.su' -print0 |
	xargs -0 --no-run-if-empty cat |
	awk -F'\t' -v root="${repo_root}/firmware/" '
		$2 ~ /^[0-9]+$/ && index($1, root) == 1 &&
		$1 !~ /managed_components/ && $1 !~ /\/esp-idf\// {
			print $2 "\t" $1
		}' |
	sort -rn >"${frames}"

if [ ! -s "${frames}" ]; then
	printf 'error: no first-party stack-usage records found under %s\n' "${build_dir}" >&2
	printf '       (was the project built with -fstack-usage?)\n' >&2
	exit 2
fi

python3 - "${frames}" "${allowlist}" "${threshold}" "${repo_root}" <<'PY'
import sys
from pathlib import Path

frames_path, allowlist_path, threshold_raw, repo_root = sys.argv[1:5]
threshold = int(threshold_raw)
root = repo_root.rstrip("/") + "/"

allowed = {}
for raw in Path(allowlist_path).read_text(encoding="utf-8").splitlines():
    line = raw.strip()
    if not line or line.startswith("#"):
        continue
    parts = line.split(None, 1)
    if len(parts) != 2 or not parts[0].isdigit():
        sys.exit(f"error: malformed allowlist line: {raw!r}")
    allowed[parts[1].strip()] = int(parts[0])

# .su location format: <file>:<line>:<col>:<function>
def key_of(location):
    pieces = location.split(":")
    if len(pieces) < 4:
        return location
    path = ":".join(pieces[:-3])
    function = pieces[-1]
    if path.startswith(root):
        path = path[len(root):]
    return f"{path}:{function}"

seen, violations, regressions = {}, [], []
for raw in Path(frames_path).read_text(encoding="utf-8").splitlines():
    if not raw.strip():
        continue
    size_text, location = raw.split("\t", 1)
    size = int(size_text)
    name = key_of(location.strip())
    # A function can appear once per translation unit; keep the worst.
    if size <= seen.get(name, -1):
        continue
    seen[name] = size

for name, size in sorted(seen.items(), key=lambda kv: -kv[1]):
    if name in allowed:
        if size > allowed[name]:
            regressions.append((name, size, allowed[name]))
    elif size > threshold:
        violations.append((name, size))

stale = sorted(set(allowed) - set(seen))

if violations:
    print(f"error: {len(violations)} first-party stack frame(s) exceed "
          f"{threshold} bytes and are not allowlisted:", file=sys.stderr)
    for name, size in violations:
        print(f"  {size:>8}  {name}", file=sys.stderr)

if regressions:
    print(f"error: {len(regressions)} allowlisted frame(s) grew beyond their "
          f"recorded size:", file=sys.stderr)
    for name, size, limit in regressions:
        print(f"  {size:>8}  {name}  (allowed {limit})", file=sys.stderr)

if stale:
    print(f"error: {len(stale)} allowlist entr(ies) no longer match any frame; "
          f"remove them:", file=sys.stderr)
    for name in stale:
        print(f"  {name}", file=sys.stderr)

if violations or regressions or stale:
    print(f"\nSee {Path(allowlist_path).name} for how to record or fix a frame.",
          file=sys.stderr)
    raise SystemExit(1)

largest = max(seen.values()) if seen else 0
print(f"stack usage policy passed: {len(seen)} first-party frames analyzed, "
      f"largest {largest} bytes, {len(allowed)} allowlisted")
PY
