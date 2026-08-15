#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
readonly repo_root
readonly generator="${repo_root}/scripts/generate-setup-label.py"
temporary_dir="$(mktemp -d)"
readonly temporary_dir
trap 'rm -rf -- "${temporary_dir}"' EXIT

python3 - <<'PY' >"${temporary_dir}/key.bin"
import sys
sys.stdout.buffer.write(bytes(range(32)))
PY

actual="$(python3 "${generator}" "${temporary_dir}/key.bin" '10:20:30:A0:B0:C0')"
readonly actual
readonly expected='{"ap_passphrase": "0665630870D7FE643BA4B540", "ap_ssid": "ESP32-Macro-A0B0C0", "device_id": "102030A0B0C0"}'
if [[ "${actual}" != "${expected}" ]]; then
	printf 'FAIL: label vector mismatch\nexpected: %s\nactual:   %s\n' \
		"${expected}" "${actual}" >&2
	exit 1
fi

if python3 "${generator}" "${temporary_dir}/key.bin" 'not-a-mac' \
	>"${temporary_dir}/output" 2>&1; then
	printf 'FAIL: invalid MAC unexpectedly passed\n' >&2
	exit 1
fi
if ! grep -F -- 'exactly 12 hexadecimal digits' "${temporary_dir}/output" >/dev/null; then
	printf 'FAIL: invalid MAC error was not specific\n' >&2
	cat -- "${temporary_dir}/output" >&2
	exit 1
fi

printf 'short' >"${temporary_dir}/short-key.bin"
if python3 "${generator}" "${temporary_dir}/short-key.bin" '102030A0B0C0' \
	>"${temporary_dir}/output" 2>&1; then
	printf 'FAIL: short HMAC key unexpectedly passed\n' >&2
	exit 1
fi
if ! grep -F -- 'exactly 32 bytes' "${temporary_dir}/output" >/dev/null; then
	printf 'FAIL: short-key error was not specific\n' >&2
	cat -- "${temporary_dir}/output" >&2
	exit 1
fi

printf 'setup-label generator tests passed\n'
