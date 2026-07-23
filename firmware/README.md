# Firmware

This directory contains the ESP-IDF v5.5.5 firmware for the ESP32-S3 USB macro
keyboard.

## Layout

- `main/` contains the production `app_main` entry point.
- `components/` contains only first-party ESP-IDF components.
- `test_app/` is a separate ESP-IDF Unity application for physical device tests.
- `webfs/` is the staging location for the immutable frontend LittleFS image.
- `partitions.csv` defines the OTA-ready application and LittleFS partitions.

The production startup sequence is owned by `app_core`. Subsystems return stable
application errors, and startup rollback is explicit. The firmware does not
auto-format failed LittleFS mounts and does not fall back to an open Wi-Fi network.

## Build

Activate the pinned toolchain and build from the repository root:

```bash
./scripts/install-esp-idf.sh
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
./scripts/check-firmware.sh
```

The separate device-test image is built with:

```bash
./scripts/build-device-tests.sh
```

All warnings and lint findings in first-party firmware code are defects. ESP-IDF,
managed components, generated build directories, and other third-party code are
outside the first-party lint scope.
