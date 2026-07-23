# Web Application Tests

Vitest is configured and `app.test.ts` currently verifies selected frontend limits
against the firmware specification.

Still required:

- API client and structured-error tests
- authentication and CSRF behavior
- route and session-expiry tests
- component interaction tests
- accessibility checks
- browser-level workflows against a device or faithful test server
- failure-state and cancellation regression tests

Run the current suite with:

```bash
cd webapp
npm test
```

No test may hide console errors, rejected requests, type failures, or lint warnings.
