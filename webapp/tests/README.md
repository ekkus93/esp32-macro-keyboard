# Web Application Tests

Vitest (this directory) covers the v2 contract layer end to end: the runtime API
client and guards, setup/sign-in/session behavior, repository modeling and
validation, macro compilation, the Macros/Packages/Snapshots/Settings/Diagnostics
pages, and shared shell/hook behavior — against `AppV2`, the only application tree
that exists (the retired v1 `App.tsx` tree and its own test suite were deleted in
V2-140). `browser/` holds the real-Chrome Playwright suite
(`npm run test:browser`) that drives a built `AppV2` against a deterministic
same-origin fixture server.

Run the current suite with:

```bash
cd webapp
npm test
```

No test may hide console errors, rejected requests, type failures, or lint
warnings.
