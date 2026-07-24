---
name: lint-n-test
description: Lint all files and run the full test suite for this ESP32 macro-keyboard repo. Use when the user wants to lint and test everything (not just changed files) before committing or merging.
model: haiku
---

# lint-n-test

Lint every file and run all tests using the repo's authoritative scripts. Do NOT invent commands — these enforce the "no failure-hiding" policy (no `|| true`, no warning suppression, no first-party exclusions).

## Steps

1. **Source the ESP-IDF toolchain** (required for firmware lint/build):

   ```bash
   . "$HOME/esp/esp-idf-v5.5.5/export.sh"
   ```

2. **Run the full lint + test gate** from the repo root:

   ```bash
   ./scripts/check-all.sh
   ```

   This runs, in order: `verify-toolchain.sh`, `check-format.sh` (clang-format / cmake-format / shfmt / prettier), `check-firmware.sh` (build + clang-tidy), `check-webapp.sh` (typecheck / eslint / stylelint / vitest / build), `check-scripts.sh` (shellcheck / shfmt), `check-docs.sh` (markdownlint / yamllint), and `run-tests.sh` (host C tests).

3. **If the full gate can't run** (e.g. ESP-IDF toolchain unavailable), fall back to the parts that can, and say which steps were skipped and why:
   - Lint only: `./scripts/check-format.sh` and `./scripts/check-scripts.sh`
   - Host tests: `./scripts/run-tests.sh`
   - Frontend lint + tests: `./scripts/check-webapp.sh`

4. **Report results honestly.** Show any failing output and fix the underlying defect — never suppress warnings, add `|| true`, or exclude first-party code. Every first-party warning is a real defect here.
