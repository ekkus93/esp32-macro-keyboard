# Pages

Placeholder only. Route-level screen components currently live directly under
`src/features/<domain>/v2/` and are composed by `src/AppV2.tsx` (mounted from
`src/main.tsx`), using `src/v2/routingV2.ts` for hash routing — not here. The
retired v1 `App.tsx`, which used to centralize routing before V2-140 deleted
it, is gone; nothing in the current tree references it.

A route existing does not imply its backend workflow is complete. Check the
component itself, not just its route, before presenting a screen as
functional.
