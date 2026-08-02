# Web Application Tests

Vitest covers the runtime-validated API client, setup/login/session behavior,
set selection, execution status, and the macro library/editor.

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
