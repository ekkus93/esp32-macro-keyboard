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

> "A package is a set of macros with some meta data. A repository is a set of
> packages."

- A **package** is one macro set: its macros in order, plus metadata. This is
  what §8.7 already called a set export.
- A **repository** is a list of packages plus its own metadata:
  `{"packages": [ … ], …}`.

Four operations, symmetric:

| | |
| --- | --- |
| download a package | export one set |
| upload / replace a package | import or replace one set |
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
a write duplicates only the set being edited.

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

### Not decided

- **A generation counter** in the repository metadata (two integers compared at
  boot) was *proposed by the assistant* as a cross-check for the case where the
  marker itself is lost. Phil has not ruled on it. It is not load-bearing — the
  marker carries the decision — and if adopted it should disagree loudly rather
  than silently repair.
- **Per-package checksums** were raised by Phil and then set aside as "a little
  extra overhead." Not needed for any case above. They would only detect silent
  flash corruption, where generations agree but content differs — and §13.6
  already says a corrupt file is reported, not silently repaired.
- **The package-count limit.** §10.7 says 50 macro sets. Phil said he does not
  plan on having 100 packages but named no number. 50 is from the founding spec
  and was not invented for this design. Unchanged until he sets one.
- **Concrete route paths.** The four operations are agreed; the URLs are not.
  §17 records the operations and marks the paths open.

## Implementation notes

- `scripts/check-removed-features.sh` fails the build on
  `transaction_(begin|commit|rollback|journal)` and on `staging/`, `trash/`,
  `transactions/` directories. The marker is a single file, not a directory, so
  the layout rule in §1.1 is not touched. **The naming still needs care** — this
  mechanism is deliberately not a resurrection of the 1,600-line transaction
  layer deleted in `8b550c6`, and it should not be named as though it were.
- The observation that prompted this design: restore currently clears the active
  set, because it rebuilds the index from scratch. Under this model the
  repository document carries its own metadata, so the active set is part of what
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
includes the active set — is reported as `referenced`, mapped to tests such as
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
