"""Capture and decode raw USB HID keyboard reports from the device.

A boot-protocol keyboard report is 8 bytes:
    [0] modifier bitmap  (bit0 LCTRL, 1 LSHIFT, 2 LALT, 3 LGUI, 4 RCTRL, ...)
    [1] reserved
    [2..7] up to six concurrently-held HID usage codes

An all-zero report means "no keys held" - that is what proves release-all.
"""

import select
import threading
import time

# Resolved from the project's USB VID/PID so it does not matter which
# hidraw index the kernel happened to assign.
VENDOR_ID, PRODUCT_ID = "303A", "4001"

MODIFIERS = [
    (0x01, "CTRL"), (0x02, "SHIFT"), (0x04, "ALT"), (0x08, "GUI"),
    (0x10, "RCTRL"), (0x20, "RSHIFT"), (0x40, "RALT"), (0x80, "RGUI"),
]

# HID usage -> (unshifted, shifted) for the subset the parser can emit.
USAGE = {}
for i in range(26):
    USAGE[0x04 + i] = (chr(ord("a") + i), chr(ord("A") + i))
for i, (lo, hi) in enumerate(zip("1234567890", "!@#$%^&*()")):
    USAGE[0x1E + i] = (lo, hi)
USAGE.update({
    0x28: ("\n", "\n"), 0x29: ("<ESC>", "<ESC>"), 0x2A: ("<BKSP>", "<BKSP>"),
    0x2B: ("\t", "\t"), 0x2C: (" ", " "), 0x2D: ("-", "_"), 0x2E: ("=", "+"),
    0x2F: ("[", "{"), 0x30: ("]", "}"), 0x31: ("\\", "|"), 0x33: (";", ":"),
    0x34: ("'", '"'), 0x35: ("`", "~"), 0x36: (",", "<"), 0x37: (".", ">"),
    0x38: ("/", "?"),
})


def decode(modifier, usage):
    shifted = bool(modifier & 0x02 or modifier & 0x20)
    pair = USAGE.get(usage)
    if pair is None:
        return f"<0x{usage:02x}>"
    return pair[1] if shifted else pair[0]


def modifier_names(modifier):
    return [name for bit, name in MODIFIERS if modifier & bit]


def resolve_hidraw():
    """Find this project's keyboard among /dev/hidraw*.

    The kernel reassigns hidraw indices on every re-enumeration, and this
    device re-enumerates on every reflash, so hardcoding a path silently
    reads the wrong device (or fails) after a flash cycle.
    """
    from pathlib import Path as _Path

    wanted = (int(VENDOR_ID, 16), int(PRODUCT_ID, 16))
    for node in sorted(_Path("/dev").glob("hidraw*")):
        uevent = _Path(f"/sys/class/hidraw/{node.name}/device/uevent")
        try:
            text = uevent.read_text()
        except OSError:
            continue
        for line in text.splitlines():
            # HID_ID=<bus>:<vendor>:<product>, each zero-padded to 8 hex digits
            if not line.startswith("HID_ID="):
                continue
            fields = line.split("=", 1)[1].split(":")
            if len(fields) != 3:
                continue
            try:
                found = (int(fields[1], 16), int(fields[2], 16))
            except ValueError:
                continue
            if found == wanted:
                return str(node)
    raise SystemExit(
        f"error: no HID node found for {VENDOR_ID}:{PRODUCT_ID}.\n"
        "Is the device plugged into its NATIVE USB port and provisioned?\n"
        "(An unprovisioned device never starts the USB stack.)"
    )


class Capture:
    """Background reader of raw HID reports, with timestamps."""

    def __init__(self, path=None):
        self.path = path or resolve_hidraw()
        self.reports = []          # (monotonic_time, bytes)
        self._stop = threading.Event()
        self._thread = None
        self._handle = None
        self._reader_error = None

    def __enter__(self):
        try:
            self._handle = open(self.path, "rb", buffering=0)
        except PermissionError as error:
            raise SystemExit(
                f"error: cannot read {self.path} ({error}).\n"
                "Run: sudo bash scripts/install-hid-udev-rule.sh"
            ) from error
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()
        time.sleep(0.2)            # let the reader settle before callers act
        return self

    def _run(self):
        while not self._stop.is_set():
            try:
                # Wait with a timeout rather than blocking in read(): a bare
                # read() on hidraw parks forever when the device sends nothing,
                # so the thread could never observe _stop and __exit__ timed out
                # its join. Closing the fd from another thread does not reliably
                # interrupt a blocked read on Linux, so the wakeup has to come
                # from here.
                ready, _, _ = select.select([self._handle], [], [], 0.1)
                if not ready:
                    continue
                data = self._handle.read(8)
            except OSError as error:
                if not self._stop.is_set():
                    self._reader_error = error
                break
            if data:
                if len(data) != 8:
                    self._reader_error = RuntimeError(
                        f"short HID report from {self.path}: {len(data)} bytes"
                    )
                    break
                self.reports.append((time.monotonic(), data))

    def __exit__(self, exc_type, exc_value, traceback):
        self._stop.set()
        cleanup_errors = []
        # Join before closing: the reader owns the handle, and closing it out
        # from under a thread that is about to select() on it is a race.
        if self._thread is not None:
            self._thread.join(timeout=2.0)
            if self._thread.is_alive():
                cleanup_errors.append(f"HID reader for {self.path} did not stop")
        try:
            self._handle.close()
        except OSError as error:
            cleanup_errors.append(f"could not close {self.path}: {error}")
        if self._reader_error is not None:
            cleanup_errors.append(f"HID capture failed: {self._reader_error}")
        if cleanup_errors:
            detail = "; ".join(cleanup_errors)
            if exc_value is not None and hasattr(exc_value, "add_note"):
                exc_value.add_note(detail)
            elif exc_value is None:
                raise RuntimeError(detail)
        return False

    # --- analysis helpers -------------------------------------------------

    def typed_text(self):
        """Reconstruct the text produced, counting each key-down once."""
        out = []
        previous = set()
        for _, report in self.reports:
            modifier = report[0]
            held = {u for u in report[2:8] if u}
            for usage in held - previous:
                out.append(decode(modifier, usage))
            previous = held
        return "".join(out)

    def events(self):
        """Human-readable event list: presses (with modifiers) and releases."""
        result = []
        previous, base = set(), None
        for stamp, report in self.reports:
            if base is None:
                base = stamp
            modifier = report[0]
            held = {u for u in report[2:8] if u}
            for usage in sorted(held - previous):
                mods = modifier_names(modifier)
                label = "+".join(mods + [decode(modifier & ~0x02, usage)]) if mods \
                    else decode(modifier, usage)
                result.append((round(stamp - base, 3), "down", label))
            if previous and not held:
                result.append((round(stamp - base, 3), "release-all", ""))
            previous = held
        return result

    def ended_released(self):
        """True when the final report holds no keys and no modifiers."""
        if not self.reports:
            return False
        last = self.reports[-1][1]
        return last[0] == 0 and not any(last[2:8])
