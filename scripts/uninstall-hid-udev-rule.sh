#!/usr/bin/env bash
set -euo pipefail

# Reverts scripts/install-hid-udev-rule.sh: removes the udev rule that grants
# the plugdev group read access to the ESP32 Macro Keyboard's HID nodes, then
# reloads udev so the kernel's default (root-only) permissions apply again.
#
# Run with: sudo bash scripts/uninstall-hid-udev-rule.sh
#
# This only removes the rule file this project installed. It does not touch
# any other udev rule, group membership, or device.

readonly RULE_PATH=/etc/udev/rules.d/99-esp32-macro-keyboard.rules
readonly VENDOR=303a
readonly PRODUCT=4001

if [ "$(id -u)" -ne 0 ]; then
	printf 'error: must run as root (use: sudo bash %s)\n' "$0" >&2
	exit 1
fi

if [ ! -e "${RULE_PATH}" ]; then
	printf 'nothing to do: %s is not installed\n' "${RULE_PATH}"
	exit 0
fi

rm -f -- "${RULE_PATH}"
printf 'removed %s\n' "${RULE_PATH}"

udevadm control --reload-rules
udevadm trigger --subsystem-match=input --subsystem-match=hidraw
printf 'udev rules reloaded and re-triggered\n\n'

printf 'permissions now (expect root-only again):\n'
found=0
for node in /dev/input/by-id/*ESP32_Macro_Keyboard*event-kbd; do
	[ -e "${node}" ] || continue
	found=1
	ls -la "$(readlink -f "${node}")"
done
for node in /dev/hidraw*; do
	[ -e "${node}" ] || continue
	device="$(readlink -f "/sys/class/hidraw/$(basename "${node}")/device" 2>/dev/null || true)"
	case "${device}" in
	*"${VENDOR^^}:${PRODUCT^^}"* | *"${VENDOR}:${PRODUCT}"*)
		found=1
		ls -la "${node}"
		;;
	esac
done

if [ "${found}" -eq 0 ]; then
	printf '  (no ESP32 macro keyboard currently enumerated; the rule is gone,\n'
	printf '   so default permissions apply whenever it next enumerates)\n'
fi
