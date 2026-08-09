# V2-143 (partial) — README and stale-doc v2-authority sync

**Date:** 2026-08-09
**Scope:** Track F of Phase 14 / V2-143 only — `README.md`'s authority pointer
and closely related stale-doc sync. This is a narrow slice of V2-143, not the
full task (V2-143 also covers `CLAUDE.md`, development/API/test/hardware/
recovery documentation, `docs/mockups/v2/`, and status-vs-intent labeling;
none of that is addressed here).
**Commit:** `5b1b771ff9e036486dee05ccd736206ee26befb7` (docs-only, this worktree,
branch `worktree-agent-ac8d5dfbe0f813b2d`)

## What was wrong

`README.md`'s opening section said:

> The authoritative design is in `docs/SPEC.md`. The mandatory implementation
> sequence is in `docs/TODO.md`.

`docs/SPEC.md` is the frozen, retired v1 stub (14 lines, points to
`docs/SPEC_V2.md`) and `docs/TODO.md` is likewise a retired compatibility
pointer to `docs/TODO_V2.md`. This exact gap was already called out, verbatim,
in `docs/TODO_V2.md`'s Phase 15 "Final sign-off checklist" (the line
`docs/TODO.md`, `README.md`, and `CLAUDE.md` point to the v2 authority set` —
noting `docs/TODO.md` and `CLAUDE.md` already did, `README.md` did not).

## What was changed

### `README.md`

1. Opening authority sentence now points to `docs/SPEC_V2.md` +
   `docs/UI_UX_SPEC_V2.md` (design) and `docs/TODO_V2.md` (implementation
   sequence), with `docs/SPEC.md`/`docs/TODO.md` explicitly named as retired
   v1 compatibility pointers — matching the phrasing `docs/TODO.md` and
   `CLAUDE.md` already use.
2. The "Per-capability validation state ... tracked in
   `docs/UNIT_TESTS1_PROGRESS.md` and `docs/IMPLEMENTATION_STATUS.md`"
   sentence implied those two files carry current status. Reworded past-tense
   ("was tracked for the pre-rebuild codebase") with an explicit note that
   both predate the v1→v2 rebuild (started 2026-08-03) and a pointer to
   `docs/TODO_V2.md` / `docs/implementation-v2/` for current status.
3. The "Verified on real hardware (2026-08-02)" section is dated *before* the
   v1→v2 rebuild began (2026-08-03) and its last bullet — "all three ways a
   package is applied — whole-repository restore ... import as a new package
   ... replacing an existing package's contents" — describes the retired
   firmware-owned package/repository model. Per `CLAUDE.md` and
   `docs/implementation-v2/V2_MIGRATION_MAP.md`, firmware no longer owns that
   semantics; it stores/serves opaque blobs only, and the webapp now owns
   package/macro modeling. Added an explicit disclaimer ahead of the bullet
   list naming this, distinguishing it from the typing/cancellation/
   persistence bullets (which remain architecturally applicable), and pointing
   at `docs/TODO_V2.md` Phase 15 (V2-150–V2-155) for current, not-yet-complete
   v2 hardware-validation scope. The historical bullet list itself was left
   intact — it's a true record of what that 2026-08-02 run actually did, not
   a claim I could rewrite without inventing new v2 evidence.
4. Fixed two dead section citations: `docs/SPEC.md` §16.2 and §16.5 (serial
   console / session-cookie trust model) don't exist any more — `docs/SPEC.md`
   is now a 14-line stub. Verified the equivalent content is present, unchanged
   in substance, in `docs/SPEC_V2.md` §12.2 (`HttpOnly`/`SameSite=Strict`
   session cookie) and §12.4 ("The serial console is a trusted development
   surface...") before retargeting the citations there.
5. The "Still not verified on hardware ... open items in `docs/SPEC.md` §24.6"
   sentence cited another now-dead section. Reworded as historical
   ("those were the open items in the retired v1 specification's
   hardware-acceptance section") and pointed at `docs/TODO_V2.md` Phase 15 for
   current scope.
6. The host/frontend test-count table was stale. Measured against a clean run
   in this worktree and corrected:
   - Host C: 52→**53** `test_*.c` (including `tests/host/support/`) plus
     20→**26** `.inc` fragments, **50** CTest suites, **100% passing**
     (`./scripts/run-tests.sh`, full output tail captured — Total Test time
     2.39s).
   - Frontend: 17→**37** vitest files, 118→**352** tests, ~2s→**~3s**
     (`npm --prefix webapp run test -- --run`: `Test Files 37 passed (37)`,
     `Tests 352 passed (352)`, `Duration 3.40s`).

### `docs/README.md` (the docs index)

This file, separately from `README.md`, currently and unconditionally listed
`SPEC.md` and `TODO.md` as "Authoritative documents" — a live, current
misstatement, not a historical record. Rewrote that list to name
`SPEC_V2.md`/`UI_UX_SPEC_V2.md`/`TODO_V2.md` as authoritative, `SPEC.md`/
`TODO.md` as retired v1 compatibility pointers (linking
`implementation-v2/V2_MIGRATION_MAP.md`), and noted that
`IMPLEMENTATION_STATUS.md`/`UNIT_TESTS1_PROGRESS.md` are retired v1-era
snapshots rather than current status sources.

### `docs/IMPLEMENTATION_STATUS.md` and `docs/UNIT_TESTS1_PROGRESS.md`

Both are pre-rebuild (2026-07-24 and 2026-08-01, before the 2026-08-03 v2
rebuild start) status/progress snapshots describing the retired FIX1/v1
firmware-owned package architecture, and were still being cited from
`README.md`/`docs/README.md` as if they were live status sources. Following
the same "retired compatibility pointer" pattern already used for
`docs/TODO.md`, added a short banner to each (not a rewrite) stating the
document is retired, why, and where current v2 status lives
(`docs/TODO_V2.md`, `docs/implementation-v2/`). Bodies were left completely
intact as historical record — no content was deleted or rewritten, only a
banner was added.

This matches what
`docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md` §1 already told a
future implementer: "Do not use ... `docs/IMPLEMENTATION_STATUS.md` as a
current V2 status source. It was last updated on 2026-08-01 and explicitly
describes the older FIX1/V1-era state." That statement existed only in a
handoff doc; it's now stated in the file itself.

## Commands run

```bash
# Node pinned per .nvmrc, via nvm (nvm was already installed with 24.18.0)
export PATH="$HOME/.nvm/versions/node/v24.18.0/bin:$PATH"
node -v                                    # v24.18.0

npm --prefix webapp ci                     # 430 packages, 0 vulnerabilities
npm --prefix webapp run test -- --run      # Test Files 37 passed (37); Tests 352 passed (352); 3.40s

./scripts/run-tests.sh                     # 50/50 CTest suites passed, 2.39s total
                                            # 53 test_*.c, 26 .inc fragments (measured with `find`)

./scripts/check-docs.sh                    # exit 0; only pre-existing, unrelated
                                            # yamllint warnings on .github/workflows/*.yml
                                            # and firmware/main/idf_component.yml
                                            # (files this change did not touch)
```

## What I deliberately left alone

- **`CLAUDE.md`** — already correctly states `docs/SPEC.md`/`docs/TODO.md` are
  retired v1 stubs and points to `SPEC_V2.md`/`TODO_V2.md`. Not in this
  track's file surface; not touched.
- **`docs/TODO.md`** — already a clean, fully-retired compatibility pointer.
  Not in this track's file surface; not touched. (Its own V2-143 sub-bullet
  therefore already appears satisfied, but I did not do that work in this
  session, so I did not check that box — see below.)
- **`docs/UNIT_TESTS1_TODO.md`** — the plan `UNIT_TESTS1_PROGRESS.md` tracks
  against. Not referenced directly from `README.md`, and outside the two
  files this track's instructions named for investigation. Likely deserves a
  similar retired banner in a future pass, but I did not touch it to avoid
  guessing at scope.
- **Dated historical narrative docs** that cite `docs/SPEC.md`/`docs/TODO.md`
  as they existed *at the time they were written* — e.g. the `FIX1_*` family,
  `RALPH_LOOP_PHASE19_HANDOFF_2026-07-31.md`,
  `HANDOFF_2026-08-02_SIMPLIFICATION.md`,
  `PROPOSAL_2026-08-03_PACKAGE_REPOSITORY_MODEL.md`,
  `TODO_SPEC_ALIGNMENT_2026-08-02.md`, `SPEC_CHANGE_AUDIT_2026-08-03.md`,
  `ESP32_MACRO_KEYBOARD_CODE_REVIEW_FIXES_TODO_2026-07-28.md`,
  `UNIT_TESTS1_HANDOFF_STATUS_2026-07-24.md`,
  `docs/CLAUDE_CODE_HANDOFF_2026-07-31.md`. These are accurate as historical
  snapshots of what was authoritative when each was written; rewriting them to
  retroactively claim v2 authority would misrepresent history, not correct it.
- **`docs/RELEASE_NOTES.md`** and **`docs/TODO_EVIDENCE.md`** — both already
  carry their own "historical snapshot from early implementation (pre-FIX1),
  stale" disclaimers pointing to the FIX1 doc. That chain is itself now
  outdated (FIX1 → should really point to v2), but re-pointing a doc that
  already flags itself as pre-FIX1 straight past FIX1 to v2 felt like more
  interpretation than this narrow track should make unilaterally; left alone
  per "do less rather than invent an interpretation."
- **`docs/SPEC.md` §16.5` citation inside `docs/RELEASE_NOTES.md`** — same
  reasoning: the surrounding paragraph is already self-labeled historical.
- **`webapp/src/features/settings/README.md`**,
  **`firmware/components/wifi_ap/include/wifi_ap.h`**,
  **`firmware/components/serial_console/include/serial_console.h`** — each has
  one `SPEC.md` reference, but all are under `webapp/src/` / `firmware/`,
  explicitly out of this track's scope.
- **The on-device Unity test-menu selector table in `README.md`** — while
  auditing test counts I found it's missing the `[benchmark]` tag
  (`firmware/test_app/main/test_auth.c`'s PBKDF2 timing test carries
  `[device][auth][benchmark]`). This is real staleness but belongs to
  V2-143's "Update ... test ... documentation" sub-bullet, not the
  README-authority-pointer sub-bullet this track closes; left unedited and
  flagged here instead.
- **`docs/TODO_V2.md`'s Phase 15 "Final sign-off checklist" line** (`docs/TODO.md`,
  `README.md`, and `CLAUDE.md` point to the v2 authority set...) — this line
  now describes stale state (it says README.md does not point to v2 authority;
  after this change it does), but it sits outside the V2-143 section this
  track's instructions scoped edits to, and other tracks are concurrently
  editing this file. Left untouched; flagging for whoever next audits that
  checklist.

## TODO_V2.md checkbox closed

Only the first sub-bullet of V2-143 is checked off, with an evidence note
pointing at this report and the commit above:

> - [x] Update `README.md` to point to both v2 specifications and this TODO.

The remaining V2-143 sub-bullets (`CLAUDE.md`, `docs/TODO.md` itself,
development/API/test/hardware/recovery documentation, `docs/mockups/v2/`
references, and status-vs-intent labeling) remain open/unchecked — none of
that work was done in this track.

## Statement

No task is being claimed complete beyond the single V2-143 sub-bullet checked
above. No unchecked V2-143 item, and no Phase 14 exit-gate item, is claimed
satisfied by this change.
