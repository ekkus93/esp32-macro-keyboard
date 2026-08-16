#!/usr/bin/env bash
# Regression tests for scripts/check-v2-snapshot-send-policy.py (SPEC_V2 §17).
#
# Both directions are tested. A guard that never fires is useless, and a guard
# that fires on ordinary code gets disabled.
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly checker="${repo_root}/scripts/check-v2-snapshot-send-policy.py"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

readonly src="${temporary_dir}/webapp/src"
mkdir -p -- "${src}/v2" "${src}/features"

pass_count=0

write_routing() {
	printf '%s\n' "export const screensV2 = [$1] as const;" >"${src}/v2/routingV2.ts"
}

write_feature() {
	printf '%s\n' "$1" >"${src}/features/Probe.tsx"
}

reset_fixture() {
	write_routing '"macros", "packages", "snapshots", "settings"'
	rm -f -- "${src}/features/Probe.tsx"
}

expect_pass() {
	local name="$1"
	if ! python3 "${checker}" --root "${temporary_dir}" >"${temporary_dir}/out" 2>&1; then
		printf 'FAIL: %s unexpectedly failed\n' "${name}" >&2
		cat -- "${temporary_dir}/out" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

expect_fail() {
	local name="$1"
	local expected="$2"
	if python3 "${checker}" --root "${temporary_dir}" >"${temporary_dir}/out" 2>&1; then
		printf 'FAIL: %s unexpectedly passed\n' "${name}" >&2
		exit 1
	fi
	if ! grep -F -- "${expected}" "${temporary_dir}/out" >/dev/null; then
		printf 'FAIL: %s did not report %s\n' "${name}" "${expected}" >&2
		cat -- "${temporary_dir}/out" >&2
		exit 1
	fi
	pass_count=$((pass_count + 1))
}

# --- allowed: explicit user actions and ordinary effects -------------------

reset_fixture
expect_pass 'baseline fixture'

write_feature 'export function Probe() {
  const onDelete = () => { void deleteSnapshot(id); };
  return null;
}'
expect_pass 'deleteSnapshot from a click handler'

write_feature 'export function Probe() {
  useEffect(() => { refreshList(); }, []);
  const onSave = () => { void saveWorkingCopyAsSnapshot(store); };
  return null;
}'
expect_pass 'an effect that does not mutate snapshots, beside an explicit save'

write_feature 'export function Probe() {
  useEffect(() => { setLabel("deleteSnapshot"); }, []);
  return null;
}'
expect_pass 'the mutation name appearing as a string, not a call'

# --- forbidden: snapshot mutation inside an effect -------------------------

write_feature 'export function Probe() {
  useEffect(() => { void deleteSnapshot(oldestId); }, [snapshots]);
  return null;
}'
expect_fail 'automatic deletion in an effect' 'deleteSnapshot() is called inside a useEffect'

write_feature 'export function Probe() {
  useEffect(() => { void saveWorkingCopyAsSnapshot(store); }, [dirty]);
  return null;
}'
expect_fail 'automatic creation in an effect' 'saveWorkingCopyAsSnapshot() is called inside a useEffect'

write_feature 'export function Probe() {
  useEffect(() => { void replaceSnapshotWithWorkingCopy(id, store); }, [id]);
  return null;
}'
expect_fail 'automatic replace in an effect' 'replaceSnapshotWithWorkingCopy() is called inside a useEffect'

rm -f -- "${src}/features/Probe.tsx"

# --- forbidden: a standalone send or confirmation screen -------------------

write_routing '"macros", "send-confirm", "snapshots"'
expect_fail 'standalone confirmation screen' 'looks like a standalone send or confirmation step'

write_routing '"macros", "confirmation", "snapshots"'
expect_fail 'confirmation screen by another name' 'looks like a standalone send or confirmation step'

# --- the guard must notice when its own subject disappears ----------------

printf '%s\n' "export const somethingElse = [] as const;" >"${src}/v2/routingV2.ts"
expect_fail 'screensV2 union removed' 'could not locate the screensV2 union'

rm -f -- "${src}/v2/routingV2.ts"
expect_fail 'routing file removed' 'not found'

printf 'check-v2-snapshot-send-policy regression tests passed: %d\n' "${pass_count}"
