# TODO: everything still outstanding, as of 2026-08-02

**Document status:** Working list
**Supersedes for new work:** nothing — `docs/TODO_SPEC_ALIGNMENT_2026-08-02.md`
records the completed alignment sequence and its evidence, and stays as history
**Created:** 2026-08-02, at the end of the session that closed that sequence

## What this is

`TODO_SPEC_ALIGNMENT_2026-08-02.md` has zero open checkboxes: phases 0–8 are
done, and items 5.4 through 5.10 were closed with commit hashes and hardware
transcripts. That plan is finished.

It is not the same as the product being finished. This file collects what is
genuinely left, separated by whether it is *work*, a *decision*, or a *thing
that cannot be done on this bench*. Those three need different handling and get
conflated when they share a list.

Nothing here is speculative cleanup. Every item is either a measured gap, a
defect with a reproduction, or a decision the code is currently making
implicitly.

## Rules

Carried over from `CLAUDE.md` and the alignment plan, because they are what made
that sequence trustworthy:

1. **No checkbox is ticked without evidence** — the commit SHA, the exact
   command, and its result. "Builds clean" is not evidence.
2. **`./scripts/check-all.sh` must exit 0 before the commit, not after.** This
   was violated once (`48b18d0`, fixed in `51c520b`); the exit code was printed,
   read, and ignored.
3. **Hardware claims need a hardware transcript.** Host tests and CI builds do
   not count.
4. **A 4xx from a device whose own tests pass is evidence about the request
   first.** Three times this session a "device defect" was the test harness:
   `json.dumps` inserting whitespace, and a wrong request body shape twice. The
   one that was real was found by instrumenting, not by reasoning.
5. **Instrument before theorising.** The restore investigation produced two
   confident wrong root causes before a single log line settled it.

---

## Priority 1 — Finish the package deduplication — **done** (`db6abf2`)

Import-new, replace and restore shared `storage_package_reader.{c,h}`:
`package_tree_open`/`_close` and `package_parse_set_node`/`_macro_node`.

- [x] 1.1 — **not done as written, and deliberately.** The plan published
      `node_json` as `package_node_to_json`. Once the three call the repository's
      node parsers directly, `node_json` has no callers at all: publishing it
      would have been an API with no users, which is what 1.3 warns against. It
      is deleted.
- [x] 1.2 — the six object parsers are gone. Restore's round trip (serialize a
      cJSON node to text so the text parser could re-parse it) went with them.
- [x] 1.3 — `package_tree_open` / `package_tree_close`, adopted by all three.
      Import and replace keep a document struct wrapping it, because each needs
      `sets[0]` for opposite reasons: import to check every macro names it before
      restamping, replace to check it names the target.
- [x] 1.4 — `misc-include-cleaner` did catch the include churn, in
      `storage_object_json.h`, exactly as predicted.

**Result:** `firmware/components/storage` 6,590 → **6,462**. The estimate was
~6,450; three files went -267/+48.

**Evidence:** `./scripts/run-tests.sh` 51/51, `--sanitizers` 51/51,
`./scripts/check-all.sh` exit 0, all before the commit. Flashed to the attached
ESP32-S3 and `tests/hardware/test_backup_restore.py` passed all five steps.

That script gained a **replace** step in the same commit. It covered two of the
three apply paths while this change touched all three, and a refactor whose
verification skips a third of what it edits is not verified.

## Priority 2 — Decisions that the code is currently making by default

These are not tasks. Each is a question the implementation has already answered
implicitly, and the answer should be deliberate.

### 2.1 Do import, replace and restore become one operation?

Once 1.1–1.3 land, what is left between them is not duplicated code but three
different atomicity guarantees over three different units:

| Operation | Unit | Guarantee |
| --- | --- | --- |
| import as new | one set, new id | create or fail |
| replace | one set, known id, expected revision | swap or leave untouched |
| restore | whole repository | all-or-nothing across sets (SPEC 13.5) |

- [ ] **Decide:** one `apply_package` parameterised by unit, conflict policy and
      rollback scope — or three operations that happen to rhyme.
- [ ] If merged, derive the three from it; do not add three flags to one
      function, which is the same complexity wearing one hat instead of three.
- [ ] If not merged, record why in `SPEC.md` so the next reader does not
      re-litigate it.

### 2.2 ~~Is the 3,000-line storage target still the goal?~~ — **struck; it was fabricated**

There is no 3,000-line target. `docs/SPEC.md` §25 lists eighteen acceptance
criteria and not one of them counts lines.

The number came from `docs/TODO_SPEC_ALIGNMENT_2026-08-02.md`'s own Phase 4
*sizing estimate* — a guess at how big the rewrite would come out — and was then
promoted, inside that same document, into "acceptance criterion 6". A plan
inventing a completion gate for itself is not a criterion. Phase 4a had already
recorded that the estimate was wrong and why (package handling was never in
scope for any phase), and this file went on to treat it as a live requirement
anyway.

Struck, with nothing replacing it. Line count was never evidence of the thing it
was standing in for, and a re-baselined number would have been the same mistake
with better arithmetic.

### 2.3 The layer depth

Writing one JSON file goes `storage_repository` → `_sets` → `_set_operations` →
`_document` → `storage_atomic` → `storage_fs_ops`.

- [ ] **Decide** whether that is worth collapsing. Note the real cost before
      deciding: the same defect class — a writer rebuilding a shared struct and
      silently dropping fields it does not know about — has appeared **three
      times** (`storage_set_reorder` dropping the active set, first-run setup
      dropping the station network, and credential reset which would have done
      the same had it been written that way). Fewer layers copying
      `provisioning_config_t` and `macro_set_t` around would make that class
      structurally harder rather than something a test has to catch.

### 2.4 Keep or remove the package stage diagnostics

`PACKAGE_DIAG` in `storage_package.c` (5 sites) and
`storage_package_restore.c` (3) is failure-only and `ESP_PLATFORM`-guarded.

- [ ] **Decide:** keep permanently — a 422 with no indication of which stage
      rejected a document is what made the restore investigation expensive, and
      SPEC 20.1 wants failures explicit — or remove it as scaffolding. Leaning
      keep; it has already paid for itself once.

---

## Priority 3 — Defects and inconsistencies with reproductions

### 3.1 `/select` and `/device/restart` disagree about request bodies

- [ ] `POST /api/v1/sets/{id}/select` requires an empty JSON object `{}` and a
      content type; `POST /api/v1/device/restart` rejects a body outright. A
      client that guesses wrong gets 415 or 422 with no hint which.
- [ ] This is the same trap shape as the `expectedRevision` field that started
      the traceability work: the route's contract is discoverable only by trial.
- [ ] Pick one convention, apply it to both, and state it in SPEC 17.

### 3.2 SPEC 15.2's join-failure path is unproven

- [ ] The specification requires that a station join which fails or times out is
      logged and ignored, leaving the device on its access point.
- [ ] It cannot be produced from this bench: `wifi-connect` verifies credentials
      by joining *before* storing them, so there is no supported way to persist a
      network that will not answer.
- [ ] Options: a host test around `app_core`'s startup sequence (there is none
      today — `app_core.c` has no host test at all), or a console command that
      stores credentials without verifying, which is a product change.

### 3.3 `app_core.c` has no host test

- [ ] Noticed while trying to cover 3.2. The startup sequence — stage ordering,
      what is fatal, what is degraded — is only covered through
      `test_app_core.c`'s fixture of the *sequence* module, not the adapters in
      `app_core.c` itself.
- [ ] Scope this before committing to it; it may be that the sequence module is
      the right seam and `app_core.c` is thin glue. Measure first.

---

## Priority 4 — Hardware acceptance (SPEC 24.6), 6 of 11 left

Five of §24.6's eleven items are proved on hardware and recorded: power-cycle
persistence, factory reset, credential reset, a full set of macros sent in order,
and cancellation over both the API and the console. Six remain, listed below —
plus one item from §24.3, which is a different section and is marked as such.

### 4.1 Reachable with the current bench

- [ ] **Linux host** — arguably already demonstrated by every HIL run today, but
      it has no explicit assertion. Cheapest item on this list.
- [ ] **Descriptor enumeration (SPEC 24.3, not 24.6)** — the device already reports
      `303a:4001 ESP32 Macro Keyboard Project`. An assertion in `hid_capture.py`
      or the acceptance script, not new work.
- [ ] **Repeated USB reconnects** — needs either physical replug or cycling the
      USB stack from the console. Decide which counts as evidence before
      writing it.

### 4.2 Needs hardware or hosts not present

- [ ] **ChromeOS host** — needs the machine.
- [ ] **Windows host** — needs the machine.
- [ ] **Repeated AP reconnects** — needs a client that can be cycled against the
      device's own access point.
- [ ] **User-data preservation across a firmware slot switch** — needs an OTA
      slot switch; the partition table has `ota_0`/`ota_1` and nothing has ever
      exercised the switch.

### 4.3 The on-device Unity suite has never been run

- [ ] `firmware/test_app/` compiles in CI and has never been flashed and run.
      `README.md` says so, and that claim is still accurate.
- [ ] Flash it, run `*`, record the serial transcript.
- [ ] Note the port table in `CLAUDE.md` first: the interactive console is the
      CH340 bridge, not native USB.

---

## Priority 5 — Traceability, 23 statements unmapped

`docs/SPEC_TEST_TRACEABILITY.md` is generated; `check-docs.sh` fails if it
drifts. Current unmapped, by section — 23, which is what the table below has
always summed to; the heading said 24 until 2026-08-02:

| Section | Count | Nature |
| --- | ---: | --- |
| 20.1 | 9 | Project conduct rules (`swallow an esp_err_t`, `return success after partial completion`) |
| 24.6 | 6 | The hardware items in Priority 4 |
| 5.3 | 3 | Dependency pinning by committed manifest and lock files |
| 2, 3, 6, 24.3, 27 | 5 | Spec-meta and list-introducing prose |

- [ ] **5.3 is gate-enforceable** — a check that the lockfiles exist, are
      committed, and match the manifests. Three statements for one small script.
- [ ] **20.1 is partly enforceable** — "no `|| true`", "no discarded error
      result" are greppable; "return success after partial completion" is not.
      Do the enforceable subset, and say plainly in the document that the rest is
      a review rule rather than a testable one.
- [ ] **Leave 2, 3, 6, 27 unmapped.** They are prose that introduces lists, or
      statements about the document itself. Citing them would assert nothing,
      and a citation that asserts nothing is exactly what this document exists to
      prevent.

---

## Priority 6 — Documentation debt

### 6.1 The FIX1 documents

- [ ] Six files, **5,951 lines** (verified), carrying headers that strike
      anything naming a removed subsystem. They were retired in place rather
      than deleted, on the grounds that striking the individual lines would be
      change for its own sake.
- [ ] The alignment plan puts that at "356 references". **Treat that number as
      unverified**: no method was recorded with it, and counting lines naming a
      removed subsystem today gives 417 by one reasonable definition. Either
      recount with the method written down, or drop the figure.
- [ ] **Decide:** delete them (git history keeps them) or keep them. They are
      currently the largest documentation artefact in the repository and describe
      a product that no longer exists.

### 6.2 Keep the validation claims honest

- [ ] `README.md`'s hardware-verification list was corrected once this session
      after going stale within a day. It is accurate as of `72b88e2`.
- [ ] Whenever a Priority 4 item lands, update that list in the same commit.
      That paragraph is the first thing a reader believes, and it was wrong for
      exactly as long as nobody re-read it.

---

## Suggested order

1. **Priority 1** — contained, verified, unblocks 2.1.
2. **2.1 and 2.2** — decisions, cheap to make, and they determine whether any
   further storage work is worth starting.
3. **4.1 and 4.3** — the hardware items reachable from this bench, including the
   Unity suite that has never run.
4. **3.1** — small, and it removes a class of trap the project has already been
   bitten by once.
5. Everything else as the hardware and the appetite allow.
