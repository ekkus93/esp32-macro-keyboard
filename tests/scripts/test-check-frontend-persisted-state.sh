#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-frontend-persisted-state.sh"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

pass_count=0

write_valid_fixture() {
	rm -rf -- "${temporary_dir}/src"
	mkdir -p -- "${temporary_dir}/src/features/sets"
	cat >"${temporary_dir}/src/features/sets/SetSelectionPage.tsx" <<'SOURCE'
const recentSetsKey = "esp32-macro-keyboard.recent-set-ids";

function readRecentSetIds(): string[] {
  const raw = window.localStorage.getItem(recentSetsKey);
  return raw === null ? [] : (JSON.parse(raw) as string[]);
}

function recordRecentSet(setId: string): void {
  window.localStorage.setItem(recentSetsKey, JSON.stringify([setId]));
}
SOURCE
}

expect_pass() {
	local name="$1"
	if ! bash "${checker}" "${temporary_dir}/src" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1"
	local expected="$2"
	if bash "${checker}" "${temporary_dir}/src" >"${temporary_dir}/output" 2>&1; then
		printf 'FAIL: %s unexpectedly passed\n' "${name}" >&2
		exit 1
	fi
	if ! grep -F -- "${expected}" "${temporary_dir}/output" >/dev/null; then
		printf 'FAIL: %s did not report %s\n' "${name}" "${expected}" >&2
		cat -- "${temporary_dir}/output" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

write_valid_fixture
expect_pass 'approved localStorage key in the approved file'

write_valid_fixture
mkdir -p "${temporary_dir}/src/features/auth"
cat >"${temporary_dir}/src/features/auth/SessionCache.ts" <<'SOURCE'
export function cacheSessionToken(token: string): void {
  sessionStorage.setItem("session-token", token);
}
SOURCE
expect_fail 'sessionStorage use' 'uses a persisted-storage API outside the FIX1 18.5 allowlist'

write_valid_fixture
mkdir -p "${temporary_dir}/src/features/db"
cat >"${temporary_dir}/src/features/db/Cache.ts" <<'SOURCE'
export async function openCache(): Promise<IDBDatabase> {
  return new Promise((resolve) => {
    const request = indexedDB.open("app");
    request.onsuccess = () => resolve(request.result);
  });
}
SOURCE
expect_fail 'indexedDB use' 'uses a persisted-storage API outside the FIX1 18.5 allowlist'

write_valid_fixture
mkdir -p "${temporary_dir}/src/features/pwa"
cat >"${temporary_dir}/src/features/pwa/Worker.ts" <<'SOURCE'
export function registerWorker(): void {
  void navigator.serviceWorker.register("/sw.js");
}
SOURCE
expect_fail 'service worker use' 'uses a persisted-storage API outside the FIX1 18.5 allowlist'

write_valid_fixture
sed -i 's/esp32-macro-keyboard.recent-set-ids/some-other-key/' \
	"${temporary_dir}/src/features/sets/SetSelectionPage.tsx"
expect_fail 'unapproved key' 'is not in the FIX1 18.5 approved key list'

write_valid_fixture
mkdir -p "${temporary_dir}/src/features/other"
cat >"${temporary_dir}/src/features/other/Elsewhere.ts" <<'SOURCE'
const key = "esp32-macro-keyboard.recent-set-ids";
export function readElsewhere(): string | null {
  return window.localStorage.getItem(key);
}
SOURCE
expect_fail 'localStorage outside approved file' 'localStorage use outside the FIX1 18.5 approved file list'

printf 'check-frontend-persisted-state regression tests passed: %d\n' "${pass_count}"
