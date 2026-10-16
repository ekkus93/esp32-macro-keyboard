#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
adb_args=()
if [[ -n "${ANDROID_SERIAL:-}" ]]; then
	adb_args+=(--serial "${ANDROID_SERIAL}")
fi

adb "${adb_args[@]}" get-state >/dev/null
wifi_before="$(adb "${adb_args[@]}" shell settings get global wifi_on | tr -d '\r')"
data_before="$(adb "${adb_args[@]}" shell settings get global mobile_data | tr -d '\r')"
fixture_port="${ANDROID_FIXTURE_PORT:-39371}"
cdp_port="${ANDROID_CDP_PORT:-9222}"

restore_emulator() {
	if [[ "${wifi_before}" == "1" ]]; then
		adb "${adb_args[@]}" shell svc wifi enable >/dev/null
	else
		adb "${adb_args[@]}" shell svc wifi disable >/dev/null
	fi
	if [[ "${data_before}" == "1" ]]; then
		adb "${adb_args[@]}" shell svc data enable >/dev/null
	else
		adb "${adb_args[@]}" shell svc data disable >/dev/null
	fi
	adb "${adb_args[@]}" reverse --remove "tcp:${fixture_port}" >/dev/null
	adb "${adb_args[@]}" forward --remove "tcp:${cdp_port}" >/dev/null
}
trap restore_emulator EXIT

adb "${adb_args[@]}" shell svc wifi disable
adb "${adb_args[@]}" shell svc data disable
adb "${adb_args[@]}" reverse "tcp:${fixture_port}" "tcp:${fixture_port}"
adb "${adb_args[@]}" forward "tcp:${cdp_port}" localabstract:chrome_devtools_remote
adb "${adb_args[@]}" shell am start -a android.intent.action.VIEW -d about:blank

ANDROID_CDP_URL="http://127.0.0.1:${cdp_port}" \
	ANDROID_FIXTURE_PORT="${fixture_port}" \
	node "${repo_root}/webapp/tests/browser/run-android-offline-visuals.mjs"
