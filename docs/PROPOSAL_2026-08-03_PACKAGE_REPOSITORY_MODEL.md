# Proposal: the package / repository model and its durability

**Status:** Design agreed in conversation with Phil, 2026-08-03, concluding
01:40 PDT. Not implemented.
**Spec impact:** §8.7, §13.3, §13.5, §17 amended in the same change; see
"Specification changes" below.
**Supersedes:** decision 2.1 of `TODO_OUTSTANDING_2026-08-02.md`, and the
reverted §13.8.

## Provenance

Every decision below is Phil's, made in conversation on 2026-08-03. His words are
quoted where the wording matters. Where something is *not* decided it says so;
nothing here is inferred from silence.

This is recorded because the assistant previously wrote its own conclusions into
`SPEC.md` as though they were product decisions (§13.8, reverted in `a5668fc`),
and because an audit of every prior spec change (`SPEC_CHANGE_AUDIT_2026-08-03.md`)
could establish a source for exactly one of fourteen. `SPEC.md` is now frozen:
the assistant proposes, Phil approves, and the approval is dated.

## The model

> "A package is a package of macros with some meta data. A repository is a package of
> packages."

- A **package** is one package: its macros in order, plus metadata. This is
  what §8.7 already called a package export.
- A **repository** is a list of packages plus its own metadata:
  `{"packages": [ … ], …}`.

Four operations, symmetric:

| | |
| --- | --- |
| download a package | export one package |
| upload / replace a package | import or replace one package |
| download a repository | back up everything |
| upload / replace a repository | restore everything |

This replaces five routes with four. The pair that collapses is import-as-new and
replace, which today are separate routes doing the same job:

> "When uploading a package to an id that doesn't exist, that package gets
> created."

So uploading a package is a PUT. If the id exists it is replaced; if not it is
created. `expectedRevision` stays as optimistic concurrency (§13.7) and is
orthogonal to which of the two happened.

Uploading a repository replaces the whole thing, not merges:

> "You would have to clear out all of the packages first then upload the packages
> for a repository. If you don't do that, there might be packages on the esp32
> that aren't in the repository that we want to upload. Then you would have stale
> packages left over on the esp32."

The same semantics one level down: replacing a package replaces its macros
wholesale, because the whole package file is rewritten.

Repository upload and download are **backup and restore only**. Creating and
editing individual macros keeps its own routes and is the normal path.

## Storage layout

One file per package, not one file for the whole repository. Phil's reason:
otherwise "you would have to update the whole repository file just to update a
package." §13.3 already required this for a different but compatible reason —
a write duplicates only the package being edited.

Two repositories on the device:

> "There are only going to be two repositories - the current repository and the
> backup repository."

The backup is invisible to the user — an automatic snapshot, not a product
feature. It exists so a failed repository upload can be rolled back.

**Measured on the bench device, 2026-08-03**, because the earlier concern was
that per-file overhead would make this expensive:

```text
8 empty package files          8,192 bytes total  (~1 KB each, 8 KiB granularity)
50 packages                   ~51 KB
two repositories at 50 each  ~102 KB of 524,288   (20%)
```

LittleFS inlines small files into metadata blocks, so a package file costs about
a kilobyte. The concern was unfounded. Phil also noted he does not plan on
having 100 packages, so the realistic figure is far below this.

## Durability

> "Uploading a repository should be atomic. If it fails, it should get rolled
> back."

A **package** upload is one `rename()` and is already atomic under §13.4. It
needs no transaction and no rollback.

A **repository** upload writes N package files and deletes the ones not in the
upload. That is not atomic, and it is the only operation that needs the
mechanism below.

### The backup is synced after the commit, not before

Phil's refinement, and it is the right one:

> "it might be better to just copy over the changes from the current repository
> to the backup repository after an atomic transaction like modifying a package
> or a repository has been completed. If you do it before the transaction, then
> the user will have to wait before the backup created. Plus if you do it after
> the atomic transaction, you only have to copy over the diffs."

The invariant is *the backup holds the last committed state*. Syncing after a
commit maintains it: when the next upload begins, the backup already holds that
upload's pre-state. Same guarantee as a pre-copy, off the critical path, and only
the changed packages are copied.

### The marker

One small file recording which operation is in flight. Boot reads it and repairs
in the direction it names. Two states matter, and they repair in *opposite*
directions — which is why a bare "do the two repositories match?" comparison is
not sufficient on its own:

| marker | what it means | repair |
| --- | --- | --- |
| absent | clean | none |
| `applying` | current is half-written | rebuild current from backup — **rollback** |
| `backing-up` | current is committed, backup is stale | resync backup from current — **redo** |

Phil, arriving at the second row:

> "if you crash in the middle of backing up, the backup repository is in a bad
> state but the current repository is the good state. … If it's in the backing up
> state, then the current repository is good and it shouldn't get overwritten on
> boot up. The backup repository should get overwritten by the current repository
> instead."

This is why the marker carries a state rather than merely existing. Both crash
windows leave two repositories that disagree, and the files alone cannot say
which one is authoritative. Rolling back in the `backing-up` case would discard a
successfully committed repository — the user's real data — to restore an older
copy.

Because the marker names the direction, the sync does not need to be atomic. It
only needs to be **repeatable**: crash halfway, boot runs it again. That is what
lets the backup stay one-file-per-package and keeps the diff copy Phil wanted.

### Integrity

Decided 2026-08-03: **a CRC32 per package, stored in the repository metadata.**
Phil's words: "Checksums live in the repository metadata. CRC32."

Metadata rather than inside each package file is what makes the boot comparison
cheap — one read of each repository's metadata, then N integer comparisons, with
no need to open a single package. Verifying that a package on disk is intact
means recomputing its CRC32 and comparing it against the metadata entry.

CRC32 rather than a cryptographic digest because the threat is flash corruption,
not tampering: anyone holding a session can rewrite a package legitimately, so a
digest would defend nothing that is not already open. The ESP32-S3 computes
CRC32 in hardware, and it costs 4 bytes per package instead of 32.

§8.7 has always required "integrity metadata" in a package and it has never been
implemented. This is that requirement, finally given a definition.

**A generation counter is not wanted.** The assistant proposed one as a
cross-check; Phil ruled it out — "I don't think that we need a generation
counter." The marker carries the repair direction and the checksums catch
divergence, so a third mechanism earns nothing.

### Routes

Decided 2026-08-03: **five**, not four. The four operations plus a listing,
because without it a client has no way to learn a valid package id and the
package API cannot be used on its own.

Singular nouns throughout, matching the vocabulary this proposal establishes —
Phil: "It should be 'package' though, not 'packages'."

```text
GET  /api/v1/package              list the package ids
GET  /api/v1/package/{id}         download a package
PUT  /api/v1/package/{id}         upload a package  (creates if absent)
GET  /api/v1/repository           download the repository
PUT  /api/v1/repository           upload the repository
```

### Vocabulary

Decided 2026-08-03: **"set" stops existing. The noun is "package".** Phil:
"'set' should stop existing. Use 'package' instead."

A package and a macro set were the same object under two names, which is what
made `import` and `import-new` look like different operations. One noun means
the live-editing routes move under `/api/v1/package/{package_id}/macros` as
well, so this is not only a transfer-format change.

Two consequences worth recording, both found while doing it:

- **`package` is a reserved word in JavaScript strict mode.** A bare binding
  cannot be called `package`, so the webapp uses `pkg` for local variables. Type
  names, wire fields, route paths and all user-facing text say "package"; `pkg`
  appears only where the language forbids the real word.
- **Historical documents were deliberately left saying "set".** The FIX1
  documents, the handoffs, the alignment plan and the spec-change audit record
  what was written at the time. Rewriting them to the new vocabulary would
  falsify the record.

### The listing

Decided 2026-08-03: **id, name and CRC32.** Phil: "id + name. I don't think that
we need revision. Date or checksum is probably better than revision."

Not a date, and that is not a preference: the device has no wall clock. There is
no SNTP call, no RTC synchronisation and no `time(NULL)` anywhere in the
firmware, and §4 rules out internet access, so any date it stamped would be
invented. §8.7 now forbids reporting one.

Whether the CRC32 should also replace `expectedRevision` as the concurrency
token — the ETag pattern — was raised and is **not decided**.

### Still not decided

- **The package-count limit and the maximum package size.** They have to be
  chosen together: at 20% leeway a repository holds 206,438 bytes, so a 4 KiB cap
  allows 50 packages, 8 KiB allows 25, 32 KiB allows 6. Today's §10.7 declares 50
  packages, 100 macros per package, 4096 bytes per macro and 32 KiB per package
  file, which contradict each other — 100 macros at 4 KiB is 400 KiB against a
  32 KiB file cap. Current bench usage is ~440 bytes per package.
- **Whether the CRC32 replaces `expectedRevision`.**
- **Whether `POST /api/v1/package` survives** now that `PUT` creates, and whether
  the macros sub-resource survives once a `GET` of a package returns its macros
  inline. Both are recorded as open in §17.

## Implementation notes

- `scripts/check-removed-features.sh` fails the build on
  `transaction_(begin|commit|rollback|journal)` and on `staging/`, `trash/`,
  `transactions/` directories. The marker is a single file, not a directory, so
  the layout rule in §1.1 is not touched. **The naming still needs care** — this
  mechanism is deliberately not a resurrection of the 1,600-line transaction
  layer deleted in `8b550c6`, and it should not be named as though it were.
- The observation that prompted this design: restore currently clears the active
  package, because it rebuilds the index from scratch. Under this model the
  repository document carries its own metadata, so the active package is part of what
  is restored and the defect cannot occur. That bug is still live and unfixed at
  the time of writing.
- One loose end from the bench measurement: after deleting eight probe packages,
  reported usage stayed at 53,248 rather than returning to 45,056. Not
  investigated. It matters for a two-repository budget.

## Specification changes made in this change

`SPEC.md` is frozen; these were made with Phil's explicit permission on
2026-08-03 and each carries a dated note in the document itself.

| Section | Change |
| --- | --- |
| §8.7 | package and repository defined; conflict list replaced by PUT semantics |
| §13.3 | current and backup repositories, and the marker file, added to the layout |
| §13.5 | **reversed** — repository upload is now atomic with rollback, where it previously said restore "MUST NOT pretend to be" atomic |
| §17 | four operations recorded; concrete paths marked open |

### The traceability document now lies about these, and it should be believed less

Adding these amendments took `SPEC_TEST_TRACEABILITY.md` from 259 statements to
267. **The unmapped count did not move: it stayed at 23.** Every one of the eight
new requirements — repository upload is atomic, a failed upload is rolled back,
the marker carries a direction, stale packages are removed, repository metadata
includes the active package — is reported as `referenced`, mapped to tests such as
`storage_package_export → deterministic_export_and_filtering`.

None of them is implemented. None is tested. They were written twenty minutes
ago.

The cause is that a citation naming a whole section covers every statement in it,
including statements added later. So a test written in July now vouches for a
requirement written in August. Nobody did anything wrong; the mechanism simply
cannot tell the difference, and it fails in the direction that overstates
coverage.

Do not read that document as evidence for anything in this proposal. The fix is
either item-level citations for these sections (the generator already supports
`SPEC 8.7 item: …`) or an explicit pending state for statements known to be
unimplemented. Neither is done.

### On §13.5

§13.5 is the significant one. Its previous wording was written by the assistant
in the founding-spec revision and removed transactions on a storage-cost argument
that applied to their *implementation* — three directories at 8 KiB each — not to
the guarantee they provided. This mechanism costs one file.
