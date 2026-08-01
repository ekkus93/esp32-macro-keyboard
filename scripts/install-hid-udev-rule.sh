#!/usr/bin/env bash
set -euo pipefail

# Grants your user read access to the ESP32 Macro Keyboard's HID nodes so
# hardware-in-the-loop tests can verify the exact keystrokes the device
# emits, without running the tests as root.
#
# Scope: ONLY this project's USB VID/PID (303a:4001, from
# firmware/components/usb_keyboard/usb_descriptors.c). Your real keyboard,
# mouse, and every other input device are untouched. No group membership
# changes, no re-login needed.
#
# Run with: sudo bash install-hid-udev-rule.sh
# Undo with: sudo rm /etc/udev/rules.d/99-esp32-macro-keyboard.rules \
#            && sudo udevadm control --reload-rules

readonly RULE_PATH=/etc/udev/rules.d/99-esp32-macro-keyboard.rules
readonly VENDOR=303a
readonly PRODUCT=4001
readonly GROUP=plugdev

if [ "$(id -u)" -ne 0 ]; then
	printf 'error: must run as root (use: sudo bash %s)\n' "$0" >&2
	exit 1
fi

if ! getent group "${GROUP}" >/dev/null; then
	printf 'error: group %s does not exist on this system\n' "${GROUP}" >&2
	exit 1
fi

target_user="${SUDO_USER:-}"
if [ -n "${target_user}" ] && ! id -nG "${target_user}" | tr ' ' '\n' | grep -qx "${GROUP}"; then
	printf 'warning: user %s is not in group %s; the rule will install but\n' \
		"${target_user}" "${GROUP}" >&2
	printf '         will not grant that user access until they are added.\n' >&2
fi

cat >"${RULE_PATH}" <<RULE
# Installed by install-hid-udev-rule.sh for ESP32 macro keyboard HIL testing.
# Scoped to ${VENDOR}:${PRODUCT} only.
SUBSYSTEM=="input", ATTRS{idVendor}=="${VENDOR}", ATTRS{idProduct}=="${PRODUCT}", MODE="0640", GROUP="${GROUP}"
KERNEL=="hidraw*", ATTRS{idVendor}=="${VENDOR}", ATTRS{idProduct}=="${PRODUCT}", MODE="0640", GROUP="${GROUP}"
RULE
chmod 644 "${RULE_PATH}"
printf 'installed %s\n' "${RULE_PATH}"

udevadm control --reload-rules
udevadm trigger --subsystem-match=input --subsystem-match=hidraw
printf 'udev rules reloaded and re-triggered\n\n'

printf 'current permissions on the device nodes:\n'
found=0
for node in /dev/input/by-id/*ESP32_Macro_Keyboard*event-kbd; do
	[ -e "${node}" ] || continue
	found=1
	printf '  %s -> %s\n' "${node}" "$(readlink -f "${node}")"
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
	printf '  (no ESP32 macro keyboard currently enumerated - plug it in, or\n'
	printf '   the rule will apply automatically next time it enumerates)\n'
fi
