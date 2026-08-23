# v2 specification traceability

Generated from `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`, first-party
tests, and gate scripts. Regenerate with
`python3 scripts/generate-spec-traceability.py`. Do not edit by hand.

## Interpretation

This is a conservative source-level worklist. A source is marked
`referenced, not proven` only when a first-party test or gate script uses an
explicit citation such as `SPEC_V2 §13.7` or `UI_UX_SPEC_V2 §8.2`.
A citation does not prove every requirement in that section. An unmapped
source means no requirement in that source is being claimed covered by this
matrix.

The source fingerprints make the report fail closed when either authoritative
specification changes. The generator also fails if either source contains zero
normative requirement lines.

## Totals

| Metric | Count |
| --- | ---: |
| Authoritative v2 sources | 2 |
| Sources containing normative requirements | 2 |
| Sources with explicit v2 citations | 2 |
| Unmapped authoritative sources | 0 |

## Source-level mapping

| Source | Git blob SHA | Normative requirements | Status | Explicit section citations |
| --- | --- | --- | --- | --- |
| docs/SPEC_V2.md | `3fae0e5c6b2bed93547479716aa0ffbd44359323` | present | referenced, not proven | §5.3, §5.4, §7.3, §7.4, §7.11, §7.12, §8.6, §8.7, §9, §9.6, §10.2, §10.3, §10.5, §10.6, §11.1, §13.13, §14.4, §17 |
| docs/UI_UX_SPEC_V2.md | `6a74fa2b81e00dcb13ca4f0cc75b1c0a0b4dca42` | present | referenced, not proven | §3.3, §3.4, §3.6, §4, §5.5, §5.6, §8, §9.5, §12, §12.2, §12.3, §12.4, §14 |

## Next refinement

Phase 2 and later work should add explicit v2 section citations while tests are
rewritten against the authoritative specifications. This matrix must remain
conservative: citations are references, never automatic coverage claims.
