#!/usr/bin/env bash
set -euo pipefail

if (( $# != 1 )); then
  printf 'usage: %s <artifact-directory>\n' "$0" >&2
  exit 2
fi

readonly artifact_dir="$1"
if [[ ! -d "${artifact_dir}" ]]; then
  printf 'artifact directory does not exist: %s\n' "${artifact_dir}" >&2
  exit 1
fi

if find "${artifact_dir}" -type l -print -quit | grep -q .; then
  printf 'tagged artifacts must not contain symbolic links\n' >&2
  exit 1
fi

while IFS= read -r path; do
  relative_path="${path#${artifact_dir}/}"
  case "/${relative_path}/" in
    */.git/*|*/node_modules/*|*/.npm/*|*/CMakeFiles/*|*/Testing/Temporary/*)
      printf 'forbidden cache or dependency path in tagged artifacts: %s\n' "${relative_path}" >&2
      exit 1
      ;;
  esac
  case "${relative_path}" in
    *.env|*.pem|*.key|*.p12|*.pfx|*credentials*|*secret*|sdkconfig)
      printf 'forbidden secret-bearing filename in tagged artifacts: %s\n' "${relative_path}" >&2
      exit 1
      ;;
  esac
done < <(find "${artifact_dir}" -mindepth 1 -print | LC_ALL=C sort)

readonly file_count="$(find "${artifact_dir}" -type f | wc -l)"
if (( file_count == 0 )); then
  printf 'tagged artifact directory is empty: %s\n' "${artifact_dir}" >&2
  exit 1
fi

(
  cd -- "${artifact_dir}"
  find . -type f ! -name MANIFEST.sha256 -print0 \
    | LC_ALL=C sort -z \
    | xargs -0 sha256sum \
    > MANIFEST.sha256
)

if [[ ! -s "${artifact_dir}/MANIFEST.sha256" ]]; then
  printf 'failed to create tagged artifact manifest\n' >&2
  exit 1
fi
