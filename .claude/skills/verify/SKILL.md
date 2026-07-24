---
name: verify
description: Run the right quality checks for what changed in this ESP32 macro-keyboard repo (host tests, webapp checks, format check, firmware build, or the full gate). Sources the ESP-IDF toolchain first. Use after making changes and before committing.
---

# verify

Run the appropriate `scripts/*.sh` quality checks for what changed. Do NOT invent commands — these scripts are the authoritative entry points, and they enforce the repo's "no failure-hiding" policy.

## Steps

1. **Determine scope.** Look at `git status --short` / `git diff --name-only` to see which areas changed:
   - `firmware/**` or `tests/host/**` C code → host tests (and firmware build if firmware C changed)
   - `webapp/**` → webapp checks
   - `scripts/**` → script lint
   - `docs/**`, `*.md` → docs lint
   - Multiple areas or unsure → run the full gate.

2. **Source the ESP-IDF toolchain** before any firmware/device/full-gate command (host tests and webapp do NOT need it):

   ```bash
   . "$HOME/esp/esp-idf-v5.5.5/export.sh"
   ```

3. **Run the matching check(s)** from the repo root:
   - Host C tests: `./scripts/run-tests.sh` — add `--sanitizers` when touching memory/pointer-heavy code, `--coverage` when checking coverage gates, or a single label (`support parser storage executor auth web startup usb controls wifi model`) to scope one suite.
   - Firmware (build + clang-tidy): `./scripts/check-firmware.sh`
   - Frontend: `./scripts/check-webapp.sh`
   - Format only: `./scripts/check-format.sh`
   - Scripts: `./scripts/check-scripts.sh`
   - Docs: `./scripts/check-docs.sh`
   - Everything: `./scripts/check-all.sh`

4. **Report results honestly.** If a check fails, show the failing output and fix the underlying defect — never suppress warnings, add `|| true`, or exclude first-party code. Every first-party warning is a real defect here.

## Notes

- `$ARGUMENTS` may name a specific scope (e.g. `webapp`, `host`, `firmware`, `full`, or a test label) — honor it instead of auto-detecting.
- Frontend auto-fix (formatting only) is `npm --prefix webapp run format:write`; there is no auto-fix for clang-tidy/lint findings.
