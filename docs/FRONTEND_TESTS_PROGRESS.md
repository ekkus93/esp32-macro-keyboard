# Frontend Test Expansion — Progress

Status: **In progress**

Branch: `ralph/frontend-tests`

This document tracks the frontend slice of `docs/UNIT_TESTS1_TODO.md` without claiming completion before pull-request CI is green.

## Implemented in this branch

- Deterministic fetch fake with ordered plans, request recording, abort-signal visibility, and failure on unexpected calls.
- Deterministic hash-location helper.
- React DOM render, input, submit, click, rerender, flush, and unmount helpers without adding a new test framework dependency.
- Global test setup that resets CSRF, DOM, fetch plans, hash state, timers, console errors, and unhandled rejections.
- API-client tests for same-origin policy, headers, CSRF, envelopes, JSON validation, network failure, timeout abort, and caller-signal policy.
- Application routing tests for all implemented routes and polling cleanup.
- Authentication tests for password submission, CSRF storage, navigation, structured failures, network failures, and in-flight submission disablement.
- Execution tests for immediate polling, progress, terminal navigation, visible polling failures, cleanup, cancellation, and cancellation failures.
- Error-banner tests for hidden, cleared, long, and markup-like text.
- Pull-request CI job for type checking, ESLint, Stylelint, Prettier, and Vitest.

## Validation state

The files are committed and the branch is ready for pull-request CI. No frontend validation result is claimed until the new CI job completes successfully.
