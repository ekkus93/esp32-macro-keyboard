# Frontend Test Expansion — Progress

Status: **Implemented and validated in pull-request CI**

Branch: `ralph/frontend-tests`

This document tracks the frontend slice of `docs/UNIT_TESTS1_TODO.md`. Validation claims are
limited to the automated checks that actually ran.

## Implemented

- Deterministic fetch fake with ordered plans, request recording, abort-signal visibility, and
  failure on unexpected calls.
- Deterministic hash-location helper.
- React DOM render, input, submit, click, rerender, flush, and unmount helpers without adding a
  second frontend test framework.
- Global test setup that resets CSRF, DOM, fetch plans, hash state, timers, console errors, and
  unhandled rejections.
- API-client tests for same-origin policy, headers, CSRF, envelopes, JSON validation, network
  failure, timeout abort, timer cleanup, and caller-signal policy.
- Application routing tests for every implemented route, hash-listener cleanup, and polling
  cleanup after navigation.
- Authentication tests for password submission, CSRF storage, navigation, structured failures,
  network failures, and in-flight submission disablement.
- Execution tests for immediate polling, progress, completed/cancelled/failed navigation,
  visible polling failures, timer cleanup, cancellation, and cancellation failures.
- Error-banner tests for hidden, cleared, long, and markup-like untrusted text.
- A committed npm lockfile and a reproducible `npm ci` pull-request job.

## Validated automated checks

- TypeScript strict type checking passes.
- ESLint passes with zero warnings and no first-party rule suppressions.
- Stylelint passes with zero warnings.
- Prettier reports no unformatted frontend files.
- Vitest passes the API, routing, authentication, execution, and error-banner suites.
- The native host suite continues to pass alongside the frontend job.
- Normal pull-request validation retains no frontend artifact.

## Validation boundary

This milestone does not claim physical ESP32-S3 execution, USB enumeration, real typing, radio
behavior, browser-to-device integration, power-interruption behavior, or hardware-in-the-loop
completion. Those require reviewed device output and `docs/HIL_TEST_PLAN.md`.
