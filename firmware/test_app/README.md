# ESP32-S3 Device Tests

This ESP-IDF Unity application runs deterministic single-device tests on a physical
ESP32-S3. It currently covers hardware-RNG UUID generation, UUID validation, macro
parsing and compilation, parser failure atomicity, authoritative firmware limits,
authentication adapters (`[device][auth]`), PBKDF2 timing calibration
(`[device][auth][benchmark]`), the macro executor's idle/USB-not-ready behavior
(`[device][executor]`), and USB keyboard state initialization (`[device][usb]`).

This application is **device-build-tested** only: it compiles for the ESP32-S3 with
ESP-IDF v5.5.5 in CI. It is **not device-executed** here — no serial output from a
physical board has been reviewed. The application intentionally does not claim to
validate USB enumeration, a host keyboard connection, Wi-Fi clients, browser
workflows, physical buttons, or power-loss behavior. Those require dedicated
hardware-in-the-loop procedures.

Build from the repository root after activating ESP-IDF v5.5.5:

```bash
bash ./scripts/build-device-tests.sh
```

Then flash and monitor from this directory, replacing the port as needed:

```bash
idf.py -B build -p /dev/ttyUSB0 flash monitor
```

Press Enter to display the Unity menu. Enter `*` to run every test, or select one
of the tags `[device]`, `[uuid]`, `[macro_parser]`, `[limits]`, `[auth]`,
`[benchmark]`, `[executor]`, or `[usb]`.

For Phase 1 PBKDF2 calibration, run `[benchmark]` on the reference ESP32-S3R8 and
capture every line beginning with `PBKDF2_BENCH`. Each line reports the candidate
iteration count, sample count, median microseconds, p90 microseconds, and worst-case
microseconds. Record the board model, serial port, host operating system, exact
commit SHA, build/flash/monitor commands, and the raw benchmark lines in the Phase 1
implementation report before freezing a v2 iteration count.
