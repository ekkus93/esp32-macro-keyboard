# SPEC → test traceability

Generated from `docs/SPEC.md` and `tests/host/test_*.c`. Regenerate with
`scripts/generate-spec-traceability.py`. Do not edit by hand.

## Why this exists

The host suite has hundreds of test functions and passes. That number says
nothing about whether the specification is covered, because the tests were
written **after** the code they test, in the same pass — so they encode what the
implementation does rather than what the specification requires.

That is not hypothetical. `POST /api/v1/package/{packageId}/select` required an
`expectedRevision` field that the handler parsed and never used. Six tests
asserted that requirement because the handler had it. Not one asked what §12.3
actually says, which is nothing about a revision on selection. All six passed
for months. Hardware found it in a minute by sending `{}`.

Consensus among tests derived from the same source is worth nothing. This
document exists so a requirement can be checked against a test, in that
direction.

## How to read `Status`

- **referenced** — at least one test cites this section. That is a *weak* signal:
  it means someone had the section in mind, not that this particular sentence is
  covered. Verify before trusting it.
- **gate-enforced** — no test cites it, but a `scripts/check-*.sh` that runs on
  every `check-all.sh` does. Some prohibitions are properties of the tree, not
  behaviours of a function: "MUST NOT fetch remote resources" and "MUST NOT use
  warning suppression" cannot be unit-tested, and a script that fails the build
  is the stronger enforcement. Listing them as unmapped understated coverage;
  calling them tests would overstate it.
- **UNMAPPED** — nothing anywhere cites this section. Certainly not deliberately
  covered.

None of these is a coverage measurement. This is a worklist, not a score.

## Totals

| | Statements | Unmapped |
| --- | --- | --- |
| MUST NOT | 0 | 0 |
| MUST | 0 | 0 |
| **Total** | **0** | **0** |

## Prohibitions (`MUST NOT`) — do these first

A prohibition has no happy path, so nothing covers it by accident. These are the
cheapest place to find real gaps.

| Section | SPEC line | Requirement | Status | Referencing test / enforcer |
| --- | --- | --- | --- | --- |

## Requirements (`MUST`)

| Section | SPEC line | Requirement | Status | Referencing test / enforcer |
| --- | --- | --- | --- | --- |
