# Development handoff — UI redesign ESP32-S3 qualification, 2026-10-17

**Repository:** `ekkus93/esp32-macro-keyboard`  
**Handoff update:** implementation merged to `master` in `21e564e` on
2026-10-17
**Current UI redesign implementation:** `master` at merge commit `21e564e`
(pushed to `origin`). The source branch
`codex/ui-ux-redesign-2026-10-16` remains on GitHub as a backup/reference.
**Purpose:** resume the remaining physical ESP32-S3 qualification for the UI
redesign on a computer with a board attached. This is a status snapshot, not a
specification. Read `CLAUDE.md` first. Normative requirements remain in
`docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md`.

## Current status

The UI redesign implementation and its automated validation are merged to
`master`. The product owner approved representative phone Macros, macro
editor, and dark-mode screenshots on 2026-10-17. The final full repository gate
passed at implementation commit `50ff2398109b51fb3d26a1f539f8409e997b7632`;
the merge commit `21e564e` also contains the later keyboard/accessibility
evidence and ESP32-S3 deferral records. Android Chrome offline fixture checks passed all 46 route
scenarios, but they do not exercise the device's own web server or constrained
ESP32 serving.

The active ledger is
[`UI_UX_REDESIGN_TODO_2026-10-15.md`](UI_UX_REDESIGN_TODO_2026-10-15.md).
Its only remaining hardware-dependent work is:

- **UXR-080:** test the built app served from the ESP32-S3 with internet
  unavailable. The static app must load from device webfs; exercise routes and
  error/recovery paths, and record any device-specific serving failures.
- **UXR-081:** confirm list/editor interactions remain responsive on target
  mobile browsers under constrained ESP32 serving conditions. Record device,
  browser/phone, network conditions, observations, and any measured timings;
  do not claim physical-phone coverage from an emulator.
- **Final exit gate:** remains open until the two checks above have reproducible
  evidence in the ledger. Keep their checkboxes unchecked until then.

These checks were explicitly deferred temporarily at the owner's request. They
were not waived or passed. The owner separately deferred Apple Safari because
it is unavailable and waived the spoken screen-reader qualification; do not
reopen those items as blockers for this handoff. The automated keyboard, focus,
accessible-name/live-region, axe, prompt-safety, and touch-target evidence is
recorded in the TODO. No physical ESP32-S3 was attached during the prior run.

## Start on the other computer

Fetch `master` and work from its current tip; the hardware evidence must match
the implementation being qualified:

```bash
git fetch origin
git switch master
git pull --ff-only origin master
git rev-parse HEAD
```

Confirm the resulting SHA is `21e564e` or a later commit on `master`. The
implementation branch was merged; continue hardware qualification and commit
its evidence directly to `master` under the repository's normal workflow.

Use the repository-pinned versions and check the board/ports before flashing:

```bash
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
./scripts/verify-toolchain.sh
lsusb | grep -E '303a|1a86|10c4'
ls -l /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

The USB-UART bridge is preferred for flashing; native USB is used for HID and
may need manual BOOT+RESET entry when flashing. The UART bridge console and
native USB logging are different ports. Follow the port table and reset notes
in `AGENTS.md` before choosing a device path. Do not assume `/dev/ttyACM0` is
the UART bridge.

Before hardware tests, build and capture the firmware/web assets from this
checkout. The full check is the reproducible software baseline:

```bash
PATH="$(go env GOPATH)/bin:$PATH" ./scripts/check-all.sh > /tmp/uiux-handoff-check-all.log 2>&1
status=$?
tail -n 40 /tmp/uiux-handoff-check-all.log
printf 'EXIT=%s\n' "$status"
```

`check-all.sh` includes firmware build/clang-tidy, webapp tests and browser
checks, webfs image generation, flash manifest generation, and release budgets.
Proceed only if it exits 0. Keep the log and generated manifest/ELF provenance
with the hardware evidence.

## Hardware qualification procedure

1. Record the checked-out commit SHA, board identifier/revision, ESP-IDF version,
   build ID, flash manifest digest, phone/browser version, and which USB port is
   used for flashing, console, and HID.
2. Flash the artifacts generated from this exact checkout, using the UART bridge
   for flashing where available. Verify the device boots and serves the UI from
   its own web server/webfs. Do not use the Android local fixture or `adb
   reverse` for this qualification.
3. Disconnect/block internet while preserving the phone-to-device connection.
   Load the device-served UI directly and exercise setup/sign-in as applicable,
   Macros, editor, send/recovery, snapshots, settings, diagnostics, and relevant
   error states. Capture failures and browser console/network errors. Confirm
   there are no required external asset requests.
4. With the target mobile browser and the actual device as server, exercise list
   and editor interactions, including opening/editing a macro with the virtual
   keyboard, scrolling around the keyboard, save/cancel, and returning to the
   list. Record any clipping, loss of focus, hidden controls, delayed rendering,
   or failed requests. Note observed timings and network conditions; compare
   against the existing release budgets and report actual results without
   inventing a new threshold.
5. Add dated, reproducible evidence to the matching UXR-080/081 items and the
   latest evidence section of the UI redesign TODO. Mark a checkbox complete
   only when its stated result is verified. Close the final exit gate only
   after both are complete and evidence is recorded.
6. Run `./scripts/check-docs.sh`, inspect `git diff --check`, then commit and
   push the evidence to `master`. Re-run any checks required by
   code changes; a device-only evidence update does not substitute for them.

Relevant commands for focused follow-up:

```bash
./scripts/check-android-offline-visuals.sh
./scripts/check-webapp.sh
./scripts/build-webfs-image.sh
./scripts/generate-flash-manifest.sh
```

The Android script is supplementary route/keyboard evidence only. It does not
close UXR-080 or UXR-081.

## Branch and merge status

The implementation, screenshot review artifacts, baselines, and TODO evidence
are on `master` after merge commit `21e564e`. The source branch
`codex/ui-ux-redesign-2026-10-16` remains on GitHub but is no longer needed to
continue this work. Record and push the eventual hardware evidence on `master`.
Do not force-push or rewrite history.

## Broader project context

This handoff covers the UI redesign qualification, not every repository TODO.
The firmware/webapp v2 requirements and implementation ledgers remain
`docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`, `docs/TODO_V2.md`, and the active
post-v2 hardening trackers. Read their current status before treating the
redesign exit gate as overall product/release completion. Historical `FIX*`,
`PHASE_*`, `PROPOSAL_*`, and `*_OUTSTANDING_*` docs may be retired; follow their
status headers and `CLAUDE.md`.
