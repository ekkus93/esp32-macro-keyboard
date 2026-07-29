# Web Application Tests

Vitest covers the runtime-validated API client, setup/login/session behavior,
set selection, execution status, the macro library/editor, and the persisted
procedure workflow.

Procedure tests verify list and progress loading, explicit not-started state,
current-step rendering, previous/next review, instruction completion,
checkpoint confirmation, confirmed skip, stale-progress reset, macro
Send/Resend navigation, and the invariant that no progress transition
automatically submits an execution.

Still required:

- accessibility audits beyond component semantics
- browser-level workflows against a device or faithful test server
- Phase 17.7 execution confirmation and submission
- Phase 18 package workflows
- Phase 19 diagnostics workflows

Run the current suite with:

```bash
cd webapp
npm test
```

No test may hide console errors, rejected requests, type failures, or lint warnings.
