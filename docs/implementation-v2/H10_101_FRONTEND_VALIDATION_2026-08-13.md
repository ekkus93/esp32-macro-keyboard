# H10-101 Frontend Validation — 2026-08-13

## Scope and exact candidate

This report closes only **H10-101 — Frontend gates** from
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.
It does not claim the Phase H10 exit gate, device Unity execution, or the hardware
matrix.

The exact frontend-validation candidate was:

`d440be6c26174a26b5b62748161f59d8aa5c18c1`

Permanent Quality workflow run `31675479517`, job `94369022215`, executed the
authoritative aggregate check on that exact SHA. The retained failed-quality log
proves that `scripts/check-webapp.sh` completed in full before the aggregate check
later failed in the separate `scripts/check-scripts.sh` stage on `shfmt` diffs in
two shell regression tests. That downstream shell-format failure is recorded rather
than hidden and is not represented as a successful full Quality/check-all run.

## Authoritative frontend gate mapping

`scripts/check-webapp.sh` is fail-fast and runs, in order:

```text
npm ci
npm run format:check
npm run typecheck
npm run lint
npm run stylelint
npm run test
npm run test:coverage
npm run build
npm run test:browser
scripts/verify-no-remote-assets.sh webapp/dist
```

The same Quality log recorded the pinned toolchain as **Node.js v24.18.0**.

Results on the exact candidate:

- `npm ci`: PASS; 434 packages installed/audited and 0 vulnerabilities reported.
- Prettier `format:check`: PASS; all matched files used the configured style.
- TypeScript `typecheck`: PASS.
- ESLint: PASS with `--max-warnings=0`.
- stylelint: PASS with `--max-warnings=0`.
- full Vitest suite: **PASS — 47/47 test files, 528/528 tests**.
- frontend coverage: PASS; aggregate report was **87.47% statements**, **83.35% branches**, **91.43% functions**, and **87.55% lines**.
- production build: PASS with Vite 7.3.6; 77 modules transformed.
- local-only/static asset verification: PASS. `check-webapp.sh` is `set -euo pipefail`; the aggregate log proceeds from the completed browser suite into `check-scripts.sh`, so the intervening `verify-no-remote-assets.sh webapp/dist` command returned 0.
- real-Chrome suite: PASS. The log reports all scenario groups successful:
  - Macros page / Quick Send,
  - Snapshots / import-export,
  - Settings / Diagnostics,
  - axe-core accessibility scans,
  - USB-unavailable workflow,
  - startup workflows including first phone, refresh, expired session, no blobs, invalid newest blob, and send recovery,
  - macro-editing / package-management workflows.

These executable browser and Vitest results include the hardening scenarios already
wired into the permanent frontend suites; no source-inspection-only claim is used to
close H10-101.

## Downstream aggregate failure

After `scripts/check-webapp.sh` had completed, `scripts/check-scripts.sh` ran and
`shfmt` emitted diffs for:

- `tests/scripts/test-check-credential-logging.sh`;
- `tests/scripts/test-test-assert-redaction.sh`.

Therefore Quality run `31675479517` has an overall `failure` conclusion. H10-101 is
still complete because every command in its authoritative fail-fast frontend gate
finished successfully before that unrelated later stage. This evidence does **not**
mark the Phase H10 software exit gate complete and does not hide or downgrade the
shell-format failure.

## Disposition

**H10-101 COMPLETE** on exact candidate
`d440be6c26174a26b5b62748161f59d8aa5c18c1`.

H10-102 and H10-103 remain open for physical-device work. The Phase H10 exit gate
remains open until its separate software and hardware requirements are literally
satisfied.
