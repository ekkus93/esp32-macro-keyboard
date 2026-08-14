# H5-053 — Commit-uncertain retry and reconciliation semantics — 2026-08-13

## Scope

This document records the H5-053 implementation from
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.

Starting SHA: `3f6afc0dd475656b2290298fbe2197b67f14dadf`.
Implementation/spec candidate before this evidence commit:
`bc567862da33b5bb0fe8e163c6685814f9136ec4`.

H5-053 defines what a caller must do when canonical activation may already have
occurred but the final durability acknowledgement failed. The purpose is to make
retry deterministic and to prevent an uncertain blob create from silently
creating a duplicate snapshot.

## Authoritative semantics

`docs/SPEC_V2.md` is synchronized with this implementation.

The storage boundary is now explicit:

- the final `rename()` is the canonical activation point;
- successful parent-directory synchronization is the final durability
  acknowledgement;
- a failure before rename is `NOT_COMMITTED`;
- rename success followed by required parent-directory sync failure is
  `COMMIT_UNCERTAIN`;
- an activated final blob is retained and is not cleanup debris;
- callers receiving `commit_uncertain` must reconcile canonical state before any
  retry and must not assume that nothing changed.

The frozen v2 specification now defines `commit_uncertain` in §13.2, the exact
Blob Add reconciliation protocol in §13.8, the activation/durability boundary in
§10.3, and HTTP 503 as subsystem-unavailable or storage-commit-durability
uncertain in §13.14.

## Distinct external error

`APP_ERROR_COMMIT_UNCERTAIN` is appended to `app_error_code_t`, preserving all
pre-existing numeric enum values.

Its stable machine string is:

```text
commit_uncertain
```

`web_api_http_status_for_error(APP_ERROR_COMMIT_UNCERTAIN)` returns 503.

This is deliberately not an alias for `APP_ERROR_STORAGE_UNAVAILABLE`: clients
must be able to distinguish "the mutation may already exist" from an ordinary
subsystem failure.

## Blob activation retention

`storage_blob_upload_commit_with_ops_result()` already identifies parent-sync
failure after rename as `APP_OPERATION_COMMIT_UNCERTAIN`.

H5-053 closes the destructive-cleanup gap. Once `final_path_owned` becomes true,
`storage_blob_upload_abort_with_ops_result()` no longer unlinks that final path.
The activated bytes remain available for canonical list/load reconciliation.

The production wrapper now exposes
`storage_blob_upload_commit_result()` and projects `COMMIT_UNCERTAIN` to the
public `APP_ERROR_COMMIT_UNCERTAIN` error. Only a fully acknowledged committed
result advances the in-memory committed-blob inventory shortcut; canonical blob
listing remains the reconciliation authority.

The HTTP blob-create handler therefore follows its existing error cleanup path
without deleting the activated blob: abort is a no-op for final-path ownership,
the original `commit_uncertain` error remains authoritative, and the response is
503 with the stable machine code.

## React reconciliation protocol

`webapp/src/v2/snapshotClient.ts` records, before each snapshot POST:

1. the set of blob IDs from the canonical `GET /api/v1/blob` list; and
2. the exact gzip bytes about to be sent.

On `503 commit_uncertain`, the client never automatically repeats the POST.
Instead it:

1. refreshes `GET /api/v1/blob`;
2. considers only IDs absent from the pre-create list;
3. downloads only those candidates as raw `application/gzip` bytes; and
4. compares each candidate byte-for-byte with the exact attempted payload.

The outcomes are explicit:

### Exactly one exact match

- report the matched blob ID;
- issue no duplicate POST;
- keep the working copy dirty because no `201 Created` was received;
- allow the user to resolve deliberately by loading the recovered snapshot.

### Authoritative no match

- end the current Save invocation without retrying its POST;
- clear the pending uncertainty latch;
- a later explicit Save action may begin a new create attempt.

### Unavailable reconciliation

- keep the working copy dirty;
- retain the pending uncertainty latch;
- later Save actions perform GET-only reconciliation and cannot issue another
  POST until canonical state can be determined.

### Ambiguous reconciliation

- if more than one new blob exactly matches the attempted bytes, keep the working
  copy dirty;
- retain the pending uncertainty latch;
- block later POSTs until the user/canonical state resolves the ambiguity.

The latch is keyed to the in-memory working-copy store. A successful deliberate
snapshot load replaces the working copy and clears the latch. No repository bytes
or reconciliation state are persisted in browser storage.

## Regression coverage

### Storage host regression

`tests/host/test_storage_blob_upload.c` proves a parent-directory sync failure
after rename:

- returns detailed `COMMIT_UNCERTAIN`;
- retains the final blob;
- leaves no temporary;
- performs no unlink when abort is called afterward;
- returns `APP_ERROR_COMMIT_UNCERTAIN` from the production compatibility wrapper;
- does not falsely record a fully durable commit in the in-memory inventory.

### HTTP host regression

The blob-route fake models uncertain activation by creating the final record but
not marking the upload fully committed.

`test_blob_create_commit_uncertain_returns_503_and_retains_blob()` proves:

- HTTP status is `503 Service Unavailable`;
- the JSON machine code is `commit_uncertain`;
- the final blob remains present; and
- its bytes are exactly the POST body.

`test_web_api_core.c` independently locks the app-error-to-503 mapping.

### Frontend unit regressions

`webapp/tests/v2-snapshot-client.test.ts` proves:

- an exact new match produces only one POST across the uncertain Save and a
  subsequent Save attempt;
- dirty state is retained;
- authoritative no-match does not retry in the same invocation but permits a
  later explicit Save;
- unavailable reconciliation keeps the latch and blocks later POSTs.

`webapp/tests/v2-snapshot-commit-reconciliation.test.ts` adds edge coverage:

- two exact new matches are ambiguous and remain latched, with no second POST;
- a byte-identical blob that existed before the POST is ignored; reconciliation
  inspects only newly observed IDs.

## Settings wording audit

The H5 tracker mentions determining whether a canonical "blob/settings" value
exists after durability uncertainty. Current production source has no caller of
`storage_atomic_write()` or its detailed variants outside the atomic-write module
itself; the only current live H5 mutation path that can cross this filesystem
rename/parent-sync uncertainty boundary is blob creation.

Device settings use their separate NVS transaction path and do not currently
consume the filesystem atomic-write commit-state API. H5-053 therefore does not
invent a settings reconciliation protocol for a nonexistent production caller.
If a future settings path adopts the atomic-write primitive, it must project and
reconcile `COMMIT_UNCERTAIN` before retrying just as required by §13.2.

## Validation status

The repository checkout is not available in this sandbox and direct GitHub DNS
resolution is unavailable, so the literal repository gates were not executed
locally here. Source and target wiring were inspected through the GitHub
connector; no CI success is inferred from that inspection.

Authoritative validation for an exact descendant containing the implementation
candidate is:

```text
./scripts/run-tests.sh storage
./scripts/run-tests.sh --sanitizers storage
./scripts/check-all.sh
```

The frontend-specific checks should also include the pinned Node.js 24.18.0
project environment and, at minimum:

```text
npm --prefix webapp run test -- --run \
  tests/v2-snapshot-client.test.ts \
  tests/v2-snapshot-commit-reconciliation.test.ts
npm --prefix webapp run typecheck
npm --prefix webapp run lint
npm --prefix webapp run format:check
```

Do not mark the authoritative H5-053 checkboxes complete until those exact gates
pass on a descendant containing this implementation.

## Remaining H5 work

H5-054 remains the broader storage regression/acceptance pass. It should aggregate
the complete primary+cleanup matrix, uncertain-commit behavior, mount rollback,
and a real-browser snapshot uncertainty scenario proving that UI interaction
cannot issue a duplicate POST.

H5-055 remains the physical interrupted-write/power-cycle durability sanity pass.
