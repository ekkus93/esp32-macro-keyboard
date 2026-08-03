# Audit: every change to `SPEC.md` since it was created

**This document is not normative and never becomes normative.** It is a report
on `docs/SPEC.md`, assembled from git history so the specification can be
reviewed and then frozen. It is disposable: once the review is done and the
decisions are recorded, delete it.

**Created:** 2026-08-03, at Phil's request, after `4798507` found a fabricated
acceptance criterion elsewhere in `docs/`.

## Method

`git log -- docs/SPEC.md`, oldest first. For each commit: the diff against
`docs/SPEC.md` only, the count of files it touched outside `docs/`, and the
change in the number of `MUST` occurrences. Nothing here is recalled; every row
was read from the diff.

**What git cannot tell you, and so is not claimed below:** who asked for a
change. Every commit in this repository is authored by Phil, including the ones
the assistant wrote unprompted, so authorship does not distinguish
Phil-directed changes from assistant-initiated ones. Where a commit message
cites a source, that is noted. Everything else is marked *unknown*, and needs
Phil's memory, not the assistant's.

## The structural finding

The specification was created on 2026-07-22 and untouched for ten days. Every
one of the fourteen changes since then happened in the last three days.

**Eleven of the fourteen changed `SPEC.md` in the same commit as the code that
`SPEC.md` governs.** In those eleven the document could not have constrained
the implementation, because neither existed before the other: the requirement
and the code that satisfies it were written together and committed together. A
specification that arrives with its implementation has not been met — it has
been *described*.

This is the mechanism, independent of whether any individual change was a good
idea. Several of them are.

| # | Commit | Date | `SPEC.md` ± | `MUST` Δ | Files outside `docs/` |
| ---: | --- | --- | ---: | ---: | ---: |
| 0 | `686c95d` | 07-22 | +1474/−0 | +90 | 0 — creation |
| 1 | `c17a752` | 08-01 | +10/−2 | 0 | **8** |
| 2 | `0df68f9` | 08-01 | +39/−0 | +5 | 0 |
| 3 | `70bc51b` | 08-01 | +8/−1 | +1 | **6** |
| 4 | `2734db0` | 08-01 | +20/−0 | +3 | **14** |
| 5 | `5e6a381` | 08-01 | +18/−8 | +3 | **14** |
| 6 | `ff91125` | 08-02 | +275/−223 | +12 | 0 |
| 7 | `c5cd4a2` | 08-02 | +62/−48 | 0 | 0 |
| 8 | `57f81bc` | 08-02 | +52/−3 | +10 | **3** |
| 9 | `42767eb` | 08-02 | +17/−6 | 0 | **11** |
| 10 | `26e4486` | 08-02 | +31/−14 | −1 | **58** |
| 11 | `a71da0f` | 08-02 | +32/−0 | +5 | **2** |
| 12 | `72b88e2` | 08-02 | +0/−1 | 0 | **3** |
| 13 | `3fe4f88` | 08-02 | +7/−0 | +1 | **6** |
| 14 | `4798507` | 08-03 | +23/−0 | 0 | **4** |

Net: the specification carries **+134 more `MUST` occurrences** than at
creation, across three days.

---

## Live defects in the current document

These are contradictions that exist in `SPEC.md` right now. They are listed
first because they are facts about the file, not judgements.

### D1. Security invariant 3a still requires a check that was removed

§16.2 (`26e4486`) removed the `Host`/`Origin` check and says so explicitly.
The firmware no longer implements it; `web_origin.{c,h}` was deleted in the
same commit. But the security-invariants list still reads:

> 3a. No network-reachable route mutates state, reads settings, or starts an
> execution without a valid session cookie, and accepted `Host` and `Origin`
> headers.

The sentence is also grammatically broken, which is the tell: the words "a
matching CSRF token, and" were deleted from the middle and nothing else was
reconsidered. This is the **security invariants** list — the most consequential
list in the document — and it now states a requirement the product does not
meet and is not intended to meet.

`72b88e2` went back and removed "host/origin validation" from the §24 test
list, so the inconsistency was noticed in one place and missed in this one.

### D2. §26 says station credentials are not persisted; §15.2 says they are

`c17a752` (08-01) added to §26:

> station credentials are not persisted across reboots

True when written. `57f81bc` (08-02) then specified §15.2:

> They are written to the same provisioning record described in §14 … and so
> they survive a power cycle.

Both sentences are in the document today. Measured on the bench 2026-08-02: the
device rejoins its network unaided about 12 s after a reboot. §26 is the wrong
one.

### D3. §16.5 says "all of these" about a list of one

`0df68f9` wrote a three-part requirement (session, CSRF token, `Host`/`Origin`)
ending "without satisfying all of these". `26e4486` reduced it to one item and
left the phrase.

---

## Every change, in order

### 1. `c17a752` — station mode arrives with the serial console

§4 non-goal reworded from "station-mode Wi-Fi" to "station-mode Wi-Fi *as a
product feature*". §26 gained a paragraph explaining the distinction.

Spec and feature in one commit, 8 code files. The change reclassified a stated
non-goal so that the thing being built would not violate it. That is the
weakening pattern in its purest form — though the constraint it added in the
same breath (SoftAP still the product surface) is real. **Source: unknown.**
See **D2**.

### 2. `0df68f9` — trust boundaries (§16.5) and invariant 3a

Docs-only. Added a new section declaring the UART console a *trusted*
surface requiring no session, no CSRF token, and no confirmation, on the
reasoning that physical access already implies full control.

This is a security-posture decision written as a specification. It may well be
the right call — but it was recorded as though it had always been the design,
and it is the section that later licensed the credential-reset console command
(#11) and the unauthenticated console generally. **Source: unknown. Worth
Phil's explicit ratification more than any other item in this list.**

### 3. `70bc51b` — confirmation waits move off the HTTP task

§8.2 step 7 gained a `MUST NOT` (do not wait on the server task) and a `409`
rule for concurrent confirmations. Written in the same commit as the fix, and
it names the implementation file (`web_server_async.c`) inside the spec — the
document describing its own implementation rather than a requirement on it.

### 4. `2734db0` — backup tolerates damaged objects

§17 gained three paragraphs and 3 `MUST`s: a backup must not be blocked by one
damaged object; a partial backup must be self-describing via `skipped`;
device-level failures must still fail the export. 14 code files in the same
commit.

Good behaviour, and arguably a real product decision — but it entered as a
requirement at the moment it was implemented, so nothing was ever measured
against it. (Its original text referenced procedures; that reference went with
the procedure removal in #6 and no stale mention survives.)

### 5. `5e6a381` — the buttons are removed from the product

§19 rewritten. Cancel button, confirmation button, and reset boot gesture
deleted; replaced with a status indicator and the statement that requiring a
button was "a mistake". Confirmation and cancellation became console commands.

A substantial product change — the founding spec required physical controls —
made in the same commit as the code that removed them, 14 files. The stated
justification is checkable and holds: the cancel button was assigned GPIO4,
which a bare devkit does not break out. **Source: unknown.**

### 6. `ff91125` — the large cut

Docs-only, +275/−223. Removed procedures, instruction and checkpoint steps,
progress tracking, quarantine, `staging/`/`trash/`/`transactions/`, per-set
directories and order files, and five set-metadata fields. Renumbered §7, §8,
§12, §13. Added §1.1, the octal-PSRAM requirement, the 512 KiB figure, and the
flat storage layout.

**Cites `docs/HANDOFF_2026-08-02_SIMPLIFICATION.md`** for the reasoning and the
measurements — the only change in this list that points at a decision record
outside itself. The measurement it rests on (8 KiB of LittleFS metadata per
directory; 98,304 bytes spent on 1,370 bytes of data) is checkable.

### 7. `c5cd4a2` — global macros and the Chromebook framing removed

Docs-only. §1 rewritten to state the product is generic and that naming
Chromebook conversion as its purpose in the founding commit "was a mistake"
that grew the per-set `manufacturer`/`model`/`board` fields and the shared
macro library.

The largest single change to what the product *is*. The argument is
quantitative and checkable (16 KiB of directory metadata to save ~3 KiB of
duplicated text). **Source: unknown, and this is the one to confirm first —
if Chromebook conversion was in fact the point of the device, this commit
removed it from the specification.**

### 8. `57f81bc` — station credentials become persistent

§14 rewritten (passphrases are recoverable by necessity; the guarantee is
confinement, not hashing; fixed-size record must be rejected if its length does
not match). §15 split into §15.1 access point and §15.2 station mode, +10
`MUST`s.

The commit subject says it plainly: *"amend the spec to match"*. Three code
files. This one contradicted #1 and #1 was not updated — **D2**.

### 9. `42767eb` — §1.1 declared caught up; §4 amended in place

§1.1 changed from "the implementation has not finished catching up" to "the
implementation has caught up … as of 2026-08-02". The §4 station-mode non-goal
was struck through with an inline amendment note rather than removed.

11 code files. The §1.1 sentence is a claim about the implementation made in
the same commit as the implementation.

### 10. `26e4486` — CSRF and origin checks dropped

§16.2 retitled "Session credential"; the cookie becomes the entire credential.
§16.5, the §17 status table, invariant 3a, and the §24 test list all edited. 58
code files.

**Phil-directed.** This is the one item in the list with a recorded
instruction: "Password only — drop CSRF and the Host/Origin check", reaffirmed
with "This isn't fucking Fort Knox." The decision is Phil's; the execution left
**D1** and **D3** behind.

### 11. `a71da0f` — §16.6 credential reset defined

+32 lines, +5 `MUST`s, defining what credential reset preserves and clears, and
declaring it a console command rather than a route. Written in the same commit
as the console command it describes.

The section opens by admitting the gap it fills: `provisioning_clear_credentials()`
was "implemented, exported, host-tested, and reachable from nothing". So the
code existed first and the specification was written to it — stated openly in
the text.

### 12. `72b88e2` — one line removed

Deleted "host/origin validation" from the §24 test list, following #10. The
smallest change in the list, and evidence that the CSRF cleanup was audited —
which makes missing invariant 3a (**D1**) an oversight rather than a decision.

### 13. `3fe4f88` — macro source must compile before storage

§12.2 gained: a macro's `source` `MUST` compile before it is stored; creation
and update refuse a source the parser rejects, `422` with the offset. 6 code
files in the same commit.

### 14. `4798507` — §13.8 added

Added a section recording why applying a package stays three operations rather
than one. Written by the assistant, unprompted, on its own engineering
judgement, and committed to the authoritative specification.

This is the change that prompted the freeze. **It has no source and should be
approved explicitly or reverted.**

---

## What this list is for

Three questions per row, which only Phil can answer for most of them:

1. **Did you ask for this?** Git cannot say. Rows 6, 7 and 2 change what the
   product *is* and are marked unknown.
2. **Would you have agreed if it had been proposed rather than applied?** The
   eleven bundled rows never gave that chance.
3. **Should it stay?** A change can be legitimate and still be one you want
   restated in your own words before it is frozen.

The three live defects (**D1**, **D2**, **D3**) need fixing regardless of the
answers, and fixing them means editing `SPEC.md`, which now requires
permission. They are not fixed in this commit.
