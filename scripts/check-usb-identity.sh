#!/usr/bin/env bash
set -euo pipefail

# SPEC 11.1: "USB descriptors MUST use project-owned manufacturer, product, and
# serial strings."
#
# These strings are what the device calls itself to every computer it is ever
# plugged into, and they are the one part of the USB identity a user sees without
# tooling. The failure mode is not a crash: it is shipping a keyboard that
# announces itself as "TinyUSB Device", which is what every example this code
# started from does.
#
# usb_descriptors.c cannot be host-tested -- it is built out of tusb_desc_device_t
# and TUD_HID_REPORT_DESC_KEYBOARD(), which exist only inside the ESP-IDF
# TinyUSB component -- so a source check is the available enforcement.

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

readonly descriptors="firmware/components/usb_keyboard/usb_descriptors.c"

python3 - "${descriptors}" <<'PY'
import re
import sys
from pathlib import Path

# Placeholder identities carried by the upstream examples. Matched
# case-insensitively as whole strings, so a legitimate name that merely contains
# a vendor word ("ESP32 Macro Keyboard") is not flagged.
PLACEHOLDERS = {
    "tinyusb", "tinyusb device", "espressif", "espressif systems",
    "esp32", "example", "unknown", "generic", "123456", "1234", "000000000000",
}

path = Path(sys.argv[1])
if not path.is_file():
    raise SystemExit(f"error: {path} not found; has the USB identity moved? (SPEC 11.1)")

source = path.read_text(encoding="utf-8")
block = re.search(r"string_descriptors\[\]\s*=\s*\{(.*?)\};", source, re.S)
if block is None:
    raise SystemExit(f"error: no string_descriptors table in {path} (SPEC 11.1)")

strings = re.findall(r'"((?:[^"\\]|\\.)*)"', block.group(1))
# Index 0 is the LANGID, which is a byte array rather than a quoted string, so
# the quoted entries are manufacturer, product, serial in that order.
labels = ("manufacturer", "product", "serial")
if len(strings) < len(labels):
    raise SystemExit(
        f"error: {path} defines {len(strings)} descriptor strings; "
        f"SPEC 11.1 requires manufacturer, product, and serial")

failures = []
for label, value in zip(labels, strings):
    if not value.strip():
        failures.append(f"{label} string is empty")
    elif value.strip().lower() in PLACEHOLDERS:
        failures.append(f"{label} string is the placeholder {value!r}")

if failures:
    print("error: USB descriptors do not use project-owned strings (SPEC 11.1)", file=sys.stderr)
    for failure in failures:
        print(f"  {failure}", file=sys.stderr)
    raise SystemExit(1)

print("USB identity: project-owned " + ", ".join(
    f"{label}={value!r}" for label, value in zip(labels, strings)))
PY
