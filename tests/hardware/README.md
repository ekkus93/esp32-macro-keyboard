# Hardware-in-the-loop tests

These tests drive a **real, attached, provisioned ESP32-S3** and verify the
keystrokes it actually puts on the USB wire. They are deliberately **not** part
of `scripts/check-all.sh` and cannot run in CI: they need physical hardware.

They exist because the host suites cannot see this layer. Host tests run on x86
against fakes for every hardware backend; they proved the executor's logic long
before the device could survive its own first authenticated API call.

## What they verify

| Script | FIX1 §20.2 items |
| --- | --- |
| `test_typing.py` | printable text, chords, release-all observation |
| `test_cancellation.py` | delay cancellation, rapid typing cancellation |

Evidence comes from the kernel's `hidraw` node for this project's USB VID/PID
(`303a:4001`), decoded as 8-byte boot-protocol reports — the bytes the device
sent, not text scraped from an editor. That is what makes it possible to assert
things a screen capture cannot: that a chord set the modifier bit *concurrently*
with the usage code, and that the final report is all-zero (no key left held).

## Prerequisites

1. **A provisioned device.** An unprovisioned device never starts the USB stack,
   so it will not enumerate as a keyboard at all.

2. **The native USB port connected.** On the ESP32-S3 the USB-Serial-JTAG
   peripheral and USB-OTG share one PHY: the port is *either* a debug serial
   port *or* the HID keyboard. Confirm with:

   ```bash
   lsusb | grep 303a:4001
   ```

3. **The UART port connected** (the devkit's other USB connector, via its
   CP210x/CH340 bridge). This carries the serial console used to join Wi-Fi,
   and it works while the native port is busy being a keyboard.

4. **Read access to the HID nodes**, which the kernel restricts to root because
   anything that can read a keyboard device can log every keystroke on the
   system:

   ```bash
   sudo bash scripts/install-hid-udev-rule.sh     # scoped to 303a:4001 only
   sudo bash scripts/uninstall-hid-udev-rule.sh   # to revert
   ```

5. **`pyserial`** for the Wi-Fi console command.

## Bench-specific state (never committed)

Credentials and per-device state live **outside the repository**, in
`~/.config/esp32-macro-keyboard/hil/` by default, or wherever `HIL_STATE_DIR`
points:

| File | Contents |
| --- | --- |
| `wifi.json` | `{"ssid": "...", "password": "..."}` |
| `admin_password.txt` | the device's administrator password |
| `device_ip.txt` | written by `hil_state.connect_wifi()` |
| `fixture.json` | written by `create_fixture.py` |

Nothing here reads or writes inside the repository, so no `git` operation can
capture a credential.

This used to default to `tests/hardware/.local/` — inside the checkout, relying
on `.gitignore`. That was wrong for a public repository: `.gitignore` stops
`git add`, but not `git add -f`, not an archive of the working tree, not a
backup tool, and not an editor that indexes the checkout. The default is now
outside the tree.

## Running

```bash
cd tests/hardware

# 1. join the device to Wi-Fi (station mode is not persisted, so repeat
#    after every reboot or reflash)
python3 -c 'import hil_state; print(hil_state.connect_wifi())'

# 2. create the macro set and macros (once per device wipe)
python3 create_fixture.py

# 3. run the tests
python3 test_typing.py
python3 test_cancellation.py
```

> **These type into whatever window has focus.** The device is a real keyboard.
> The fixtures are deliberately harmless — lowercase text and `{CTRL+A}`, no
> Enter and no shell metacharacters — but focus something disposable anyway.

## Interpreting results

Both scripts exit non-zero on failure and print the decoded keystrokes beside
what was expected. For cancellation they also report how long after the cancel
request the last keystroke appeared, which is the cancellation latency §20.5
asks about — though note that measures an **HTTP-initiated** cancel, not the
physical cancel button.

## What these do NOT cover

- **ChromeOS.** Every §20.2 item requires both Linux and ChromeOS; only Linux
  has been exercised, which is why those checkboxes remain open.
- **disconnect/reconnect, suspend/resume, disconnect-during-execution.** These
  need physical cable and host manipulation that cannot be automated from here.
- **§20.5 physical controls.** Cancellation here is HTTP-initiated; the button
  path is untested.
