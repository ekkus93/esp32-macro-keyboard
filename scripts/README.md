# Project Scripts

These first-party scripts are the authoritative local entry points:

- `bootstrap-repo.sh` creates the tracked repository directory structure.
- `install-esp-idf.sh` installs exactly ESP-IDF v5.5.5 for ESP32-S3.
- `verify-toolchain.sh` rejects the wrong ESP-IDF or Node.js version.
- `check-format.sh` checks first-party C/CMake/shell/frontend formatting.
- `check-firmware.sh` builds and analyzes the production firmware.
- `build-device-tests.sh` builds the ESP32-S3 Unity device-test image.
- `check-webapp.sh`, `check-scripts.sh`, and `check-docs.sh` run scoped checks.
- `run-tests.sh` configures, builds, and runs the host CTest suite.
- `check-all.sh` runs the complete authoritative quality gate.

Scripts use `set -euo pipefail`, preserve diagnostics, and propagate failing exit
codes. Do not add `|| true`, output redirection that hides failures, warning
suppression, or exclusions for first-party files.
