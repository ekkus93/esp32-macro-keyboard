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

## Priority 1 — Finish the package deduplication

Straightforward, measured, and blocked on nothing. This work was done once, went
green on host (51/51) and under sanitizers, and was reverted only because
`import-new` failed on hardware and could not be separated from a suspected
pre-existing fault. That fault turned out to be the harness (5.10), so the change
was almost certainly correct. It is being redone rather than restored from
memory.

**Why it is safe now:** `tests/hardware/test_backup_restore.py` exercises backup,
restore, **and** import-new against the device. Any apply path this breaks will
say so. That verification did not exist when the change was reverted.

### 1.1 Publish `package_node_to_json`

- [ ] Move `node_json` into `storage_package_writer.{c,h}` as
      `package_node_to_json(const struct cJSON *node, char **out_json, size_t *out_length)`.
- [ ] Delete the three private copies in `storage_package_import.c`,
      `storage_package_replace.c`, `storage_package_restore.c`.
- [ ] Note when writing the commit: import's and replace's copies are
      byte-identical; restore's is equivalent but assigns through the out-params
      instead of via a local. Take import's.

### 1.2 Delete the six redundant object parsers

- [ ] Remove `parse_set_node` and `parse_macro_node` from all three files.
- [ ] Call `storage_repository_parse_set_node` / `..._parse_macro_node` instead.
      These already exist and ship (`04ea1cf`); `storage_package.c` uses them.
- [ ] Restore's version currently serialises a cJSON node back to text purely to
      re-parse it. That round trip disappears — worth saying so in the commit,
      because it is the clearest evidence the duplication was accidental rather
      than deliberate.

### 1.3 Extract `open_document`

- [ ] Add `package_open_document` / `package_close_document` to the writer,
      returning the tree plus its `sets` and `macros` arrays.
- [ ] Adopt in all three. Import additionally parses the source set out of the
      first array element; that stays in import, it is not shared behaviour.
- [ ] **Do not commit this helper unless all three adopt it.** An unused API was
      added and removed once already in this file's history.

### 1.4 Clean up and verify

- [ ] Prune includes that become unused. `misc-include-cleaner` runs as an error
      and caught `storage_object_json.h` and `storage_json.h` in exactly this
      situation last time.
- [ ] `./scripts/run-tests.sh` and `./scripts/run-tests.sh --sanitizers`.
- [ ] `./scripts/check-all.sh` exits 0.
- [ ] Flash, then `tests/hardware/test_backup_restore.py`: backup export,
      restore, import as new, repository intact — all pass.

**Expected:** about 140 lines net; `firmware/components/storage` 6,590 → ~6,450.

---

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

### 2.2 Is the 3,000-line storage target still the goal?

Acceptance criterion 6 wants `firmware/components/storage` under 3,000 lines. It
is **6,590**, down from 14,557.

The target was set when the plan assumed the remainder after deleting
procedures, progress, quarantine and transactions would be mostly repository
CRUD. It was not: package handling was never in scope for any phase and is now
the largest block. The estimate was made against a wrong model of what would be
left, not missed through the deletions falling short.

- [ ] **Decide:** re-baseline the number with a stated rationale, or keep 3,000
      and accept that 2.1 and 2.3 are required to reach it.
- [ ] Whichever, write the reasoning down. A proxy metric nobody believes is
      worse than no metric.

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

## Priority 4 — Hardware acceptance (SPEC 24.6), 5 of 11 left

Proved on hardware and recorded: power-cycle persistence, factory reset,
credential reset, a full set of macros sent in order, cancellation over both the
API and the console.

### 4.1 Reachable with the current bench

- [ ] **Linux host** — arguably already demonstrated by every HIL run today, but
      it has no explicit assertion. Cheapest item on this list.
- [ ] **SPEC 24.3 descriptor enumeration** — the device already reports
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

## Priority 5 — Traceability, 24 statements unmapped

`docs/SPEC_TEST_TRACEABILITY.md` is generated; `check-docs.sh` fails if it
drifts. Current unmapped, by section:

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

- [ ] Six files, **5,951 lines**, carrying headers that strike anything naming a
      removed subsystem. They were retired in place rather than deleted, on the
      grounds that striking 356 individual lines would be change for its own
      sake.
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
