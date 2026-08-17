# Handoff — Tailwind Utility Migration, Ralph Loop In Progress

**Written:** 2026-08-17, mid-session, stopping because the user had to leave.
**Tree state:** clean, all work committed and pushed. HEAD is `a7402654`.
**Resume by:** re-invoking `/loop` (ralph-loop skill) with
`docs/WEBAPP_TAILWIND_UTILITY_MIGRATION_TODO_2026-08-17.md` — pick up at
**T3-3**, the next unchecked task. Everything below T3-3 in that file is
still accurate and does not need re-reading in full, just followed in order.

## What's done (15 tasks, all verified + committed + pushed)

Phase 0 (T0-1, T0-2), Phase 1 (T1-1 through T1-8, including a 4-task
gap-fill found mid-execution), Phase 2 (T2-1, T2-2, T2-3), and Phase 3's
T3-1+T3-2 (merged into one commit — see below). Every task's checkbox in
the TODO file is `[x]` with a real commit SHA and verification evidence.
`npm run test` is 544/544 and `check-webapp.sh` is EXIT=0 as of the last
commit.

## Two real defects the plan itself had, both fixed forward, not hidden

1. **§5.1's inventory listed 8 classes with no phase task assigned**
   (`metadata`, `management-list`, `management-card`, `reorder-actions`,
   `management-actions`, `card-actions`, `page-heading-title`,
   `storage-summary`). Found after T2-1, before T2-2. Patched the plan with
   T1-5 through T1-8, documented why, then executed them — see the "Gap
   found during execution" note right after T1-4 in the TODO file.

2. **§8.1's "this is the complete list" claim was wrong.** A third test
   (`tests/v2-app-v2-orientation.test.tsx:56`) depends on `.app-shell` as a
   bare class name, and it wasn't in the plan's audit. Found by running the
   full `npm run test` suite after T3-1's edit — one test failed
   (`expected null not to be null`) because `.app-shell` had been fully
   removed. Fixed by keeping `app-shell` as a class with no CSS rule, same
   pattern as `storage-summary`/`landscape-block`. **§8.1 has been corrected
   in the doc and now says explicitly not to trust it as exhaustive** — run
   the full suite after every phase regardless of what that table says.

## The recurring pattern to watch for (hit 3 times so far)

Classes that share a combined CSS selector, or where one class's element
still needs another still-CSS-driven class's name to exist as an ancestor
hook, **cannot be split across two commits/tasks** the way the original plan
assumed. Each time this happened, the fix was to merge the two plan tasks
into one commit and document why:

- T1-6/T1-7: `.metadata`/`.storage-summary` shared one `dl`; inlining one's
  margin-top without the other flipped which one won the cascade (verified
  wrong via a live probe before it was ever committed).
- T3-1/T3-2: `.bottom-nav button.active`'s CSS rule needed the ancestor
  `<nav>` to still literally carry the class `bottom-nav`; inlining
  `.bottom-nav` alone would have silently detached the active-tab styling.

**If you hit a fourth one, use the same move**: don't split it, merge the
two tasks into one commit, and write the coupling into the evidence line so
the next reader understands why the plan's own phase boundary got crossed.

## Exact next step

**T3-3** — `LandscapeBlockSurface.tsx`. Inline `.landscape-block` and its
descendants (`.landscape-block h1`, `.landscape-block .send-status`), **but
keep the `landscape-block` class itself on the element** — it's a confirmed
test hook (§8.1, `tests/v2-app-v2-orientation.test.tsx:128`,
`requiredElement(".landscape-block", HTMLElement)`).

Before touching the CSS, check the current rule (it moved slightly during
T3-1's edits, re-read it fresh rather than trusting the line numbers in an
older summary):

```bash
grep -n -A20 '\.landscape-block {' webapp/src/styles.css
```

`.landscape-block .send-status` is a descendant override on `.send-status`
(disposition B, Phase 4, not yet touched) — same shape as the `.header-actions`/
`.timing-grid` split in T1-8: extract only `.landscape-block`'s share of
that descendant rule, leave `.send-status`'s own base rule alone in CSS.

**Baseline method** (this component has no existing browser-test coverage of
its own visual state that's easy to reuse — check
`tests/v2-app-v2-orientation.test.tsx` first, it may already drive this
surface for a different assertion and give you a fixture to reuse): force
`matchMedia` for the landscape-phone query true via the same
`installFakeMatchMedia()` helper that test file already uses, or drive a
real Playwright browser context sized to a landscape-phone viewport
(e.g. 844×390) against an active send, matching the pattern used for T2-1's
short-viewport baseline (`webapp/tests/browser/fixtures/startupFixtureServer.mjs`
or `applicationServer.mjs`).

After T3-3, Phase 3 is complete. Phase 4 (8 tasks, component extraction —
`Card`, `FormStack`, `StandaloneScreen` first) is the largest remaining
phase; Phase 5 (status badges) and Phase 6 (sweep/close) follow. The TODO
file's own §6 has the full remaining task list with current context for
each — nothing else needs to be reconstructed from memory.

## Environment reminders (things that bit this session)

- Each `Bash` call may be a fresh shell — re-source
  `export NVM_DIR="$HOME/.nvm" && . "$NVM_DIR/nvm.sh" && nvm use 24.18.0`
  before any `node`/`npm`/`npx` command.
- `./scripts/check-webapp.sh` reliably exceeds the 120s foreground timeout —
  run it backgrounded (`(...) &`) and poll the log file, or use the
  `run_in_background` tool flag properly rather than a raw `&`.
- Tailwind escapes brackets in emitted class names (`.min-\[34rem\]\:grid`).
  Grep for the *declaration* (`grid-template`, `display:grid`) or the raw
  value, not the literal class-name string, when checking compiled CSS.
- `getComputedStyle().transform` reads `"none"` for Tailwind's `translate-*`
  utilities — they set the CSS `translate` property, not `transform`. Check
  `.translate`, not `.transform`, when verifying those.
- Delete every `*.tmp.mjs` probe script from `webapp/tests/browser/` before
  committing — `prettier`/`format:check` fails on stray ones.
- Every task in this loop used the two-commit pattern: implementation+tests
  first (real commit, real SHA), then a separate small `docs: record T#-#
  evidence` commit that cites the real SHA — a commit can't cite its own
  hash, learned the hard way at T0-1 in this same session.
