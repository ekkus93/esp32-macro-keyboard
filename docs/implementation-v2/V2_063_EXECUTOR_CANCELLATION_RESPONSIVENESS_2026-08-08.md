# V2-063 — Executor cancellation responsiveness

**Phase:** 6 — Macro compiler, executor, USB HID, and send lifecycle
**Task IDs:** V2-063 (`docs/TODO_V2.md`)
**Date:** 2026-08-08
**Worktree:** `.claude/worktrees/agent-a27c84ca506f9385a`
**Branch:** `worktree-agent-a27c84ca506f9385a`
**Commit SHA (this task):** `8628b776ee83fd311e349ff012f471320ec62e10`
(implementation: tests, fixture, and TODO_V2.md checkboxes) plus a small
follow-up docs-only commit that corrected this SHA reference itself (a
commit cannot embed its own final hash in one step).

## Scope

Close as much of V2-063 as is possible without the physical ESP32-S3R8 board:
cancellation responsiveness during ordinary typing, during delay directives,
network/serial convergence on one state machine, and deterministic host tests
for cancellation races. The fifth checklist item (real-device last-keystroke
latency) requires hardware and is explicitly **not** claimed here.

## Investigation

Read `firmware/components/macro_executor/` end to end
(`macro_executor_engine.c`, `macro_executor.c`, `executor_health.c`) and the
two production call sites that can request cancellation:

- `firmware/components/web_server/web_server_send.c`'s `send_cancel_handler()`
  (the `DELETE /api/v1/send` route) calls `web_send_cancel_handle()`, whose
  `web_send_ops_t.cancel` is wired in `web_server_send.c`'s
  `executor_cancel_adapter()` to `macro_executor_cancel()`.
- `firmware/components/serial_console/serial_console.c`'s `command_cancel()`
  (the `cancel` console command) also calls `macro_executor_cancel()`
  directly.

`macro_executor_cancel()` (`macro_executor.c`) is a thin wrapper: it checks
the module's `shutting_down` flag and then calls
`macro_executor_engine_cancel(&engine)` on the single static `engine`
instance created by `macro_executor_init()`. There is no per-transport
cancellation state anywhere in the request path — both call sites reach the
identical function on the identical engine. **Network and serial
cancellation already converge on one state machine; no code change was
needed for that item.**

Cancellation responsiveness during execution is implemented by
`cancellable_delay()` in `macro_executor_engine.c`, which slices any wait
(a `MACRO_ACTION_DELAY` directive, or the `key_press_ms`/`inter_key_ms` dwell
around a key/chord press) into `CANCELLATION_SLICE_MS` (10 ms) chunks and
re-checks `engine->cancellation_requested` (and the absolute deadline) before
every chunk. The main execution loop in
`macro_executor_engine_execute()` also re-checks cancellation immediately
before starting each action, independent of `cancellable_delay()`. Both
`await_confirmation()` (the confirmation-wait phase) and `cancellable_delay()`
use the same 10 ms polling idiom. **Cancellation during typing and during
delay directives is therefore already bounded to a single 10 ms slice; no
code change was needed for those two items either.**

This matches the prior finding recorded in
`docs/implementation-v2/V2_060_061_062_COMPILER_EXECUTOR_RELEASE_ALL_2026-08-08.md`
("Cancellation during typing and delay is already responsive... and is
host-tested"), which left V2-063 open only because it was out of that task's
assigned scope, not because anything was found broken.

The one gap actually found and fixed is in the **test suite**, not
production code: `tests/host/executor_validation_tests.inc`'s
`test_cancel_unlock_failure_is_reported()` submitted a request (queuing an
owned plan into the fake's `queued` field) but never executed or otherwise
freed it, leaking 12 bytes on every sanitizer run. This predates this task
(confirmed via `git stash`; reproducible on `2fd5e5d`, this branch's parent
commit, before any change here) and made `./scripts/run-tests.sh --sanitizers
executor` fail 1/2 despite `docs/implementation-v2/V2_060_061_062_...md`
recording a clean 2/2 sanitizer run — the leaking code path was present then
too, so either the sanitizer build configuration or environment differed at
that time. Fixed narrowly by freeing the queued-but-never-executed plan at
the end of that one test.

## Changes

- `tests/host/executor_cancellation_race_tests.inc` (new): five deterministic
  cancellation-race tests —
  - `test_network_and_serial_cancellation_converge_on_one_engine()`: two
    simulated callers (modeling `DELETE /api/v1/send` and the serial
    console's `cancel`) both call `macro_executor_engine_cancel()` back to
    back while a delay action is running; the first is recorded (`NONE`),
    the second is the documented idempotent conflict (`CONFLICT`), and the
    run still ends `EXECUTION_CANCELLED` after exactly one 10 ms wait.
  - `test_network_and_serial_cancellation_converge_during_confirmation_wait()`:
    the same two-caller race while `EXECUTION_AWAITING_CONFIRMATION`, and no
    key press occurs.
  - `test_cancellation_during_typing_is_bounded_to_one_slice()`: a key
    action with `key_press_ms` set to `APP_DELAY_MAX_MS` (10,000 ms, the
    contract maximum) is cancelled after exactly one 10 ms wait, not after
    the full dwell.
  - `test_cancellation_during_delay_directive_is_bounded_to_one_slice()`: the
    same bound for a `MACRO_ACTION_DELAY` of `APP_DELAY_MAX_MS`.
  - `test_cancellation_racing_natural_completion_loses_cleanly()`: a
    cancellation that arrives strictly after a run's own natural completion
    is `NOT_FOUND`, not a silent mutation of an already-terminal send.
- `tests/host/executor_test_fixture.h` / `tests/host/executor_test_fixture.inc`:
  added a `race_cancel_on_wait` fixture hook (plus
  `race_cancel_first_result`/`race_cancel_second_result`) that, on the
  `wait_ms()` call whose 1-based ordinal matches, invokes
  `macro_executor_engine_cancel()` twice in immediate succession to model two
  independent callers racing, without needing real OS threads.
- `tests/host/executor_validation_tests.inc`: fixed the pre-existing plan
  leak in `test_cancel_unlock_failure_is_reported()` described above.
- `tests/host/test_macro_executor.c`: wires the new
  `executor_run_cancellation_race_tests()` into the suite's `main()`.
- `docs/TODO_V2.md`: checked V2-063's first four boxes; left "Measure
  real-device last-keystroke latency after cancellation" unchecked.

No production firmware source was changed. `firmware/components/macro_executor/`
and `firmware/components/serial_console/` are unmodified — the investigation
found both cancellation paths already correct and already convergent.

## Commands executed and results

All commands run from the repository root of this worktree.

- `./scripts/run-tests.sh executor`: **100% tests passed, 0 tests failed out
  of 2** (`macro_executor`, `executor_health`).
- `./scripts/run-tests.sh --sanitizers executor` (ASan+UBSan): **100% tests
  passed, 0 tests failed out of 2** — clean after the leak fix above (before
  the fix: 1/2, `macro_executor` failing with a 12-byte LeakSanitizer
  report at `test_cancel_unlock_failure_is_reported()`).
- `./scripts/run-tests.sh` (all labels, no sanitizers): **100% tests passed,
  0 tests failed out of 48.**
- `./scripts/run-tests.sh --sanitizers` (all labels, ASan+UBSan): **100%
  tests passed, 0 tests failed out of 48.**
- ESP-IDF environment sourced (`. "$HOME/esp/esp-idf-v5.5.5/export.sh"`,
  `idf.py --version` reports `v5.5.5`), then `./scripts/check-firmware.sh`:
  GCC build of `firmware/` and `firmware/test_app/` (both link and produce
  flashable images), followed by the esp-clang (LLVM 19) `run-clang-tidy`
  pass over both projects' first-party translation units — **exit code 0,
  zero first-party findings** in either project. This also confirms the
  test-only changes above did not regress the firmware build (they touch
  only `tests/host/`, which `check-firmware.sh` does not compile, but the
  full gate was run per the assigning task's instructions regardless).

New test names added by this task (5, all passing): `executor_cancellation_race_tests.inc`'s
`test_network_and_serial_cancellation_converge_on_one_engine`,
`test_network_and_serial_cancellation_converge_during_confirmation_wait`,
`test_cancellation_during_typing_is_bounded_to_one_slice`,
`test_cancellation_during_delay_directive_is_bounded_to_one_slice`,
`test_cancellation_racing_natural_completion_loses_cleanly`.

## Checklist disposition

- [x] Make cancellation responsive during ordinary typing. Already true
  (`cancellable_delay()`, `CANCELLATION_SLICE_MS` = 10 ms); now covered by
  `test_cancellation_during_typing_is_bounded_to_one_slice`.
- [x] Make cancellation responsive during delay directives. Already true,
  same mechanism; now covered by
  `test_cancellation_during_delay_directive_is_bounded_to_one_slice`.
- [x] Make network and serial cancellation converge on the same state
  machine. Already true by construction (`macro_executor_cancel()` is the
  single call target for both `web_server_send.c` and `serial_console.c`);
  now covered by
  `test_network_and_serial_cancellation_converge_on_one_engine` and
  `test_network_and_serial_cancellation_converge_during_confirmation_wait`.
- [x] Add deterministic host tests for cancellation races. Five new tests in
  `tests/host/executor_cancellation_race_tests.inc`, all deterministic (no
  timing-dependent sleeps; races are simulated synchronously via the fake's
  `wait_ms()` reentrancy hook, the same technique the pre-existing
  `cancel_on_wait`/`confirm_on_wait` fixture hooks use).
- [ ] Measure real-device last-keystroke latency after cancellation. **Not
  done and not claimed.** This requires the physical ESP32-S3R8 board
  (HID report capture on a host while cancellation fires mid-send) per
  `docs/SPEC_V2.md` §18.3's hardware-in-the-loop requirements and this
  repository's rule that compilation, host fakes, and CI device builds are
  never hardware validation. Left open for whichever track next has bench
  access.

## Unresolved limitations and deferred evidence

- The real-device latency measurement above is the only remaining V2-063
  gap and is explicitly deferred, not claimed.
- This task's investigation is scoped to the host-testable engine
  (`macro_executor_engine.c`) and the two call sites that reach it. It does
  not, and cannot from a host build, validate the actual USB HID timing of
  `usb_keyboard_press()`/`usb_keyboard_release_all()` or TinyUSB's own report
  queuing latency — those remain hardware-only concerns, same as the
  deferred checklist item.
- No unchecked task in this report is being claimed complete.
