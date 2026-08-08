# Phase 7 — React repository core and persistence client

**Phase:** 7 — React repository core and persistence client
**Tasks:** V2-070, V2-071, V2-072, V2-073, V2-074, V2-075
**Branch:** `v2-phase7-repository-core` (not merged to `master`; integration is left to the
requester per this task's instructions)
**Status:** Software implementation and host-side (Vitest) test coverage complete for all
six tasks. No physical-device or live-firmware validation is claimed — Phase 5 (the live
`/api/v1/*` HTTP route implementations) is itself still open, so this work was built and
tested entirely against the Phase 1 contract layer (`webapp/src/v2/apiContracts.ts`,
`apiRouteManifest.ts`, `limits.ts`) and the fake-fetch test harness, per this task's
explicit scoping.

## Scope

This change adds the React-side repository data layer described in `docs/TODO_V2.md`'s
Phase 7 section: strict repository validation (already substantially present from Phase 1
and re-verified here), an in-memory working copy with dirty-state tracking, a
browser-storage prohibition enforced by both a static source scan and a runtime
behavioral test, a gzip/snapshot client (list/add/load/download/delete/export/import), a
package-selection resolver and settings-persistence helper, and the `sendMacro` polling
helper.

It deliberately does **not**:

- modify `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`, or `docs/TODO_V2.md` (frozen);
- modify the existing Phase 1 contract layer (`apiContracts.ts`, `apiGuards.ts`,
  `apiRequestGuards.ts`, `apiTypes.ts`, `apiRouteManifest.ts`, `limits.ts`, `repository.ts`,
  `repositoryValidation.ts`) — Phase 7 is built strictly on top of it;
- add any firmware package/macro CRUD client call — the only device routes used are
  `/api/v1/blob`, `/api/v1/blob/{blob_id}`, `/api/v1/send`, and `/api/v1/settings`, all of
  which already exist in `contracts/v2/api/routes.json`;
- wire this data layer into `App.tsx` or any page component. Per
  `docs/implementation-v2/V2_MIGRATION_MAP.md` §9 ("Build the React repository core before
  page-level CRUD migration"), that integration is explicitly Phase 8+'s job, not Phase 7's.
- add or upgrade any npm dependency; `webapp/package-lock.json` is unchanged.

## New files

All under `webapp/src/v2/` (production) and `webapp/tests/` (Vitest, per this repo's
convention that tests live in `webapp/tests/`, not beside the source):

| File | Task | Purpose |
| --- | --- | --- |
| `src/v2/apiClient.ts` | (shared) | v2-native `fetch` wrapper: v2 responses are the payload directly (no `{ok,data}` envelope like the v1 client), v2 errors are always `{error: {code, message, field?, byteOffset?, line?, column?}}`. Exports `V2ApiError`, `subscribeUnauthorized`, `v2GetJson`, `v2PostJson`, `v2PutJson`, `v2DeleteJson`, `v2DeleteNoContent`, `v2DeleteWithUnvalidatedJson`, `v2PostBinary`, `v2GetBinary`. |
| `src/v2/gzip.ts` | V2-073 | Feature detection (`isGzipSupported`) and `gzipCompress`/`gzipDecompress` via `CompressionStream("gzip")`/`DecompressionStream("gzip")`; `GzipUnsupportedError`, `GzipDecodeError`. No uncompressed fallback exists anywhere in the module. |
| `src/v2/repositoryCodec.ts` | V2-073 | `encodeRepositorySnapshot`/`decodeRepositorySnapshot`: serialize+gzip a `Repository`, and gzip-decompress+UTF-8-decode+JSON-parse stored bytes back to an `unknown` value (callers must still run it through `validateRepositoryForUse`). `RepositoryDecodeError` is distinct from `GzipUnsupportedError` so "unreadable blob" and "unsupported browser" can be handled differently, per SPEC_V2 §9. |
| `src/v2/repositoryWorkingCopy.ts` | V2-071 | `createRepositoryWorkingCopyStore(initial)`: an in-memory (closure-only) working-copy/baseline/dirty-flag store. See "V2-071" below for the exact transition contract. |
| `src/v2/snapshotClient.ts` | V2-073 | `listSnapshots`, `saveWorkingCopyAsSnapshot`, `loadSnapshotIntoWorkingCopy`, `downloadSnapshotBytes`, `deleteSnapshot`, `exportRepository`, `importRepository`. Wires the gzip codec, the working-copy store, and the blob API together. |
| `src/v2/packageSelection.ts` | V2-074 | `resolveSelectedPackage` (the exact 3-step UI_UX_SPEC_V2 §3.6 algorithm) and `persistSelectedPackageId` (change-gated `PUT /api/v1/settings`). |
| `src/v2/sendClient.ts` | V2-075 | `sendMacro(request, {onStatus, onComplete})`, `cancelSend`, `recoverSendState`, `isTerminalSendState`. |
| `tests/v2-api-client.test.ts` | — | 17 tests for the shared fetch wrapper. |
| `tests/v2-gzip.test.ts` | V2-073 | 8 tests for compression feature-detection, round-tripping, and error classes. |
| `tests/v2-repository-working-copy.test.ts` | V2-071 | 11 tests for every dirty-state transition. |
| `tests/v2-snapshot-client.test.ts` | V2-073 | 12 tests for list/add/load/download/delete/export/import, including failure paths. |
| `tests/v2-package-selection.test.ts` | V2-074 | 9 tests for resolution and change-gated persistence. |
| `tests/v2-send-client.test.ts` | V2-075 | 8 tests, including fake-timer-driven polling behavior. |
| `tests/v2-browser-storage-prohibition.test.ts` | V2-072 | 2 tests: a static source scan and a runtime behavioral check. |

Total: 67 new tests. Combined with the 231 pre-existing tests, the full suite is 298
tests across 33 files, all passing.

## Implemented behavior by task

### V2-070 — Strict repository validation

`repository.ts`/`repositoryValidation.ts` (Phase 1 work, unmodified) already implement
every V2-070 checklist item: exact root/package/macro field sets (`hasExactKeys`, so
`activePackageId` and any other unknown field is rejected with `invalid_fields`), UUID
v4 canonical-lowercase + uniqueness invariants, byte-length invariants (64 bytes for
names, 4096 for macro source), integer/bound invariants (`keyPressMs`/`interKeyMs` 0-10000),
and macro-language validation via the shared `compileMacro`. Because `validateRepository`
and `validateRepositoryForUse` take an `unknown` input and return either a fresh `Repository`
value or a list of `RepositoryValidationIssue`s, they cannot mutate a caller-held working
copy — "leave the existing working copy unchanged after any validation failure" holds by
construction once combined with the working-copy store's contract (below), which is
exercised directly in `v2-snapshot-client.test.ts`'s "leaves the working copy untouched"
tests. No changes were made to these files; this task's job was to confirm and build on
them, which the new snapshot-client/import tests do.

### V2-071 — In-memory working copy and dirty state

`repositoryWorkingCopy.ts`'s `createRepositoryWorkingCopyStore` holds `baseline`, `working`,
and `dirty` only in a JavaScript closure (never in any browser storage). Its API surface is
deliberately narrow:

- `applyContentChange(repository)` / `applyImport(repository)` — dirty the working copy
  (grouped separately in the API to document intent, though behaviorally identical today).
- `markSaved(savedRepository)` — clears dirty; the saved repository becomes the new baseline
  **and** working copy.
- `discardChanges()` — reverts the working copy to the current baseline; clears dirty.
- `replaceWorkingCopy(repository)` — a deliberate, non-dirtying replacement (loading another
  snapshot): both baseline and working copy become the new value; dirty is `false`.

Because package selection, sending, cancellation, snapshot deletion, and UI-preference
changes have **no corresponding method on this store at all**, those actions cannot dirty
the repository by construction — there is nothing for them to call. This is asserted
directly in a test ("exposes no method for package selection, sends, cancellation, snapshot
deletion, or UI preferences").

"Preserve a live dirty working copy across in-tab reauthentication" is satisfied by the
store's API containing no authentication-triggered reset: as long as a future React
provider holds one instance for the tab's lifetime, nothing here discards it on
reauthentication. This is necessarily a contract established at the data-layer level in
Phase 7; the actual React provider/context that holds the instance across a component
tree is Phase 8+'s job (see `V2_MIGRATION_MAP.md`'s explicit sequencing of "repository core
before page-level CRUD migration").

### V2-072 — Browser-storage prohibition

Two independent checks in `tests/v2-browser-storage-prohibition.test.ts`:

1. **Static scan.** Every `.ts`/`.tsx` file under `src/v2/` is read via Vite's
   `import.meta.glob` (not Node's `fs`, since this project's `tsconfig.app.json` `types`
   list deliberately excludes Node types for browser app code — see "Toolchain/typing note"
   below) and checked, after comment-stripping, against a pattern matching
   `localStorage`, `sessionStorage`, `indexedDB`, `caches.*`, `serviceWorker`, and
   `openDatabase`. None of the new or existing `src/v2/` files reference any of them.
2. **Runtime behavior.** A single test exercises the working-copy store, every
   `snapshotClient` operation, `packageSelection`, and `sendMacro`/`recoverSendState`
   against a mocked fetch, then asserts `Storage.prototype.setItem` was never called and
   both `localStorage.length`/`sessionStorage.length` remain `0`.

Scope note: this satisfies V2-072 for all Phase 7 code. It does **not** touch the
pre-existing v1 `webapp/src/features/package/PackageSelectionPage.tsx`, which still writes
recent-package IDs to `localStorage`. That file is out of Phase 7's scope — the migration
map explicitly assigns its rewrite ("Remove `localStorage` recent-package state ... use
device-wide opaque `lastSelectedPackageId`") to the page-level migration phases (9-11), not
Phase 7. Flagging this for whoever picks up that phase.

The "build scan" requirement is implemented as a Vitest test (run as part of
`npm run test` / `check-webapp.sh`, which is the project's existing "build gate") rather
than as a new standalone `scripts/` shell script, since a source-content scan close to the
existing test-fixture conventions (see other `v2-*.test.ts` files) was the more consistent
and lower-risk choice than introducing a new script-based gate. This is a scoping
call worth confirming with the integrator if a separate `scripts/check-*.sh` scan is
preferred for defense-in-depth.

### V2-073 — Gzip and snapshot client

- `gzip.ts` feature-detects `CompressionStream`/`DecompressionStream` via `isGzipSupported()`
  and throws `GzipUnsupportedError` from `gzipCompress`/`gzipDecompress` when unsupported —
  there is no uncompressed-storage code path anywhere in this module or any caller.
- `snapshotClient.ts` implements all five required client calls:
  - `listSnapshots()` — `GET /api/v1/blob`, validated by the existing
    `isBlobListResponse` guard (which itself enforces newest-first strict descending order).
  - `saveWorkingCopyAsSnapshot(store)` — serializes+gzips the current working copy,
    pre-flight-checks the compressed size against `v2Limits.blobMaxBytes` (raising
    `SnapshotTooLargeError` without ever calling `fetch` if it's too big), `POST`s it, and
    calls `store.markSaved()` **only** on a successful `201`. Any failure — including the
    pre-flight check, a network error, or a `413`/`507` from the device — leaves the working
    copy dirty and unchanged, satisfying "keep dirty work after failed save."
  - `loadSnapshotIntoWorkingCopy(id, store)` — downloads, decompresses, and runs
    `validateRepositoryForUse` **before** ever calling `store.replaceWorkingCopy()`; a typed
    `LoadSnapshotResult` (`gzip_unsupported` / `unreadable` / `invalid` / success) is returned
    on every path, and the store is provably untouched on failure (tested directly).
  - `downloadSnapshotBytes(id)` — raw pass-through bytes, no decompression/validation,
    matching SPEC_V2 §10.4's "no transform."
  - `deleteSnapshot(id)` — `DELETE /api/v1/blob/{id}`, expects `204`.
  - `exportRepository(repository)` — produces gzip bytes plus a filename ending in the
    spec's mandated suffix `.emk-repository.json.gz` (SPEC_V2 §8.8 specifies only the
    *suffix*, not a full filename, so a fixed descriptive base name was chosen) and MIME
    type `application/gzip`; performs no network call.
  - `importRepository(bytes)` — implements SPEC_V2 §8.8 steps 1-5 exactly (decompress,
    UTF-8 decode, JSON parse, full schema+macro-language validation) and returns package/macro
    counts on success. It deliberately does **not** touch a working-copy store or upload
    anything — per spec, import shows counts and only *then* the working copy should be
    replaced (via `store.applyImport`, which correctly marks it dirty) and only an explicit
    user action saves a snapshot afterward. Wiring "show counts, confirm, then call
    `applyImport`" into a UI flow is left to the page-level phase that builds the Import
    screen.

### V2-074 — Package selection preference

`packageSelection.ts`'s `resolveSelectedPackage(repository, lastSelectedPackageId)`
implements UI_UX_SPEC_V2 §3.6's exact three-step algorithm (verified against the literal
spec text, not a paraphrase) and returns a discriminated result telling the caller whether
a resolved selection should also be persisted (only true in the "auto-selected the sole
package" case). `persistSelectedPackageId(next, current)` returns `null` without calling
`fetch` at all when the value is unchanged, and otherwise issues `PUT /api/v1/settings`.
Because `Repository`/`RepositoryPackage` (from the unmodified `repository.ts`) have no
selection-related field, "never serialize selection into repository JSON" holds by
construction, not just by convention.

### V2-075 — React send helper

`sendClient.ts`'s `sendMacro(request, {onStatus, onComplete})`:

1. Calls `POST /api/v1/send`; the returned `Promise` rejects directly (no polling starts,
   no callback fires) if this fails — e.g., a `409` when a send is already in progress.
2. On acceptance, polls `GET /api/v1/send` exactly once per second (verified with Vitest
   fake timers: `vi.advanceTimersByTimeAsync(1000)` triggers exactly one more poll).
3. Calls `onStatus` only when `state` or `actionIndex` changes (verified: an unchanged
   poll result does not re-invoke it).
4. Calls `onComplete` exactly once, only after a terminal state (`completed`, `cancelled`,
   `failed`, `timed_out`), and polling stops permanently afterward.

`recoverSendState()` wraps `GET /api/v1/send` and returns `null` on a `404` (no send since
boot) rather than throwing, per SPEC_V2 §13.10/§13.11. `cancelSend()` issues
`DELETE /api/v1/send`.

**Open item for the integrator:** `DELETE /api/v1/send`'s success response body shape is
not defined anywhere in the frozen v2 contract layer (`apiTypes.ts`/`apiGuards.ts` define
guards for `SendAcceptedResponse` and `SendStatusResponse`, but nothing for the cancel
response; SPEC_V2 §13.10 specifies only the `202` status code, not a body schema).
`cancelSend()`/`v2DeleteWithUnvalidatedJson` therefore intentionally returns the parsed
JSON body **unvalidated**, with a comment explaining why, rather than guessing a shape and
asserting it with a guard that doesn't exist in the contract layer — CLAUDE.md's spec-freeze
rule reads as applying equally to inventing implicit contract-layer content. Whoever
completes Phase 5's firmware-side send-cancel implementation (or extends the Phase 1
contract layer) should add the real guard and switch `cancelSend()` to a validated call.

Duplicate-POST avoidance "across orientation changes and rerenders" is explicitly
documented in `sendClient.ts` as **not** something this framework-agnostic module can
enforce on its own (a React component calling `sendMacro` twice for one user action is a
caller-side bug, typically guarded with a ref in the effect that calls it) — what the
module does guarantee on its own (exactly one POST per call, at-most-once `onComplete`,
change-gated `onStatus`) is spelled out in the file's doc comment.

## Toolchain/typing note

`webapp/tsconfig.app.json`'s `types` list is `["react", "react-dom/client", "vitest/globals"]`
— it deliberately excludes `"node"` even though `tests/` is included in the same project
(`@types/node` is a devDependency used only by `tsconfig.node.json` for `vite.config.ts`
etc.). This means a Vitest test file cannot `import` from `node:fs`/`node:path` and
type-check. `tests/v2-browser-storage-prohibition.test.ts`'s static scan uses
`import.meta.glob<string>(..., { eager: true, query: "?raw", import: "default" })` instead
(with `/// <reference types="vite/client" />` for the ambient `ImportMeta.glob` typing),
which stays within the same browser/bundler type boundary as the rest of `src/v2/` and
avoids the deprecated `as: "raw"` glob option (which still works but prints a runtime
deprecation warning).

A second environment quirk worth recording: in the Vitest+jsdom test environment, bytes
that round-trip through `Response`/`ReadableStream`/`CompressionStream` can come back as a
`Uint8Array` from a different realm than the plain `new TextEncoder().encode(...)` output
in a test file — same byte content, different prototype. Manual verification (a standalone
Node script, and a temporary debug test, both deleted after use) confirmed the actual
bytes were identical; `expect(...).toEqual(...)` on the raw typed arrays still fails because
Vitest's equality check considers the constructor. Tests compare `Array.from(...)` of both
sides where this arises, with a comment explaining why.

## Commands run

```bash
git checkout -b v2-phase7-repository-core

export NVM_DIR="$HOME/.nvm"; . "$NVM_DIR/nvm.sh"; nvm use   # webapp/.nvmrc pins 24.18.0

cd webapp
npm ci                    # package-lock.json was already committed; installed cleanly, no lockfile changes
npm run format:check      # after `npm run format:write` to fix the new files' formatting
npm run typecheck         # tsc -b --pretty false
npm run lint              # eslint . --max-warnings=0
npm run stylelint
npm run test               # 298/298 passing (231 pre-existing + 67 new)
npm run test:coverage      # thresholds (60/60/60/60) met; src/v2 is 87.85% statements / 83.94% branches
npm run build
npm run test:browser       # Playwright, real Chrome — "Real Chrome Phase 17.10 workflows passed."
cd ..
./scripts/verify-no-remote-assets.sh webapp/dist   # no output = pass
```

## Results

- `npm run test`: **298 passed (298)**, 33 test files, 0 failures.
- `npm run test:coverage`: statements 81.77%, branches 79.69%, functions 87.44%, lines
  81.89% overall (threshold 60% each); `src/v2` specifically: 87.85% statements / 83.94%
  branches / 99.12% functions / 87.74% lines.
- `npm run typecheck`, `npm run lint` (`--max-warnings=0`), `npm run stylelint`
  (`--max-warnings=0`), `npm run format:check`: all clean, zero warnings.
- `npm run build`: succeeds; `dist/assets/index-*.js` is 266.61 kB (79.35 kB gzip).
- `npm run test:browser`: "Real Chrome Phase 17.10 workflows passed."
- `scripts/verify-no-remote-assets.sh webapp/dist`: passes (no remote `//` URLs).
- No `eslint-disable`, no suppressed warnings, no `|| true`, no first-party lint exception
  was added anywhere in this change.
- `webapp/package.json`/`webapp/package-lock.json` are unchanged — no new dependency.

## What is not covered by this change

- No wiring into `App.tsx`, routing, or any page component — that is explicitly deferred
  to the phases that come after Phase 7 per the migration map's stated ordering.
- No end-to-end validation against live firmware: Phase 5 (the actual `/api/v1/*` route
  implementations) is itself still open per
  `docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md`. Everything here was
  validated against the frozen Phase 1 contract layer and a mocked `fetch`
  (`webapp/tests/fakeFetch.ts`), not a physical device — consistent with CLAUDE.md's rule
  against claiming hardware validation from anything short of an actual device.
- The v1 `PackageSelectionPage.tsx`'s `localStorage` usage (recent package IDs) is
  untouched; see the V2-072 section above.
- `DELETE /api/v1/send`'s response body is consumed unvalidated pending a contract-layer
  guard; see the V2-075 section above.
