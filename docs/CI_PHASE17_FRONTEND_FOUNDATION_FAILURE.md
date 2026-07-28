# Phase 17 frontend foundation failure

- Transform: success
- Cleanup: success
- Node setup: success
- Frontend dependencies: success
- Format: success
- Frontend validation: failure
- Host dependencies: skipped
- Firmware formatting/tests: skipped
- ESP-IDF install: skipped
- Authoritative gate: skipped

## phase17-foundation-transform.log

```text

Phase 17 backend session foundation applied
Phase 17 documentation evidence applied
Phase 17 authenticated frontend foundation applied (payload 7)
```

## phase17-foundation-cleanup.log

```text

```

## phase17-foundation-frontend-deps.log

```text

npm warn deprecated whatwg-encoding@3.1.1: Use @exodus/bytes instead for a more spec-conformant and faster implementation
npm warn deprecated glob@10.5.0: Old versions of glob are not supported, and contain widely publicized security vulnerabilities, which have been fixed in the current version. Please update. Support for old versions may be purchased (at exorbitant rates) by contacting i@izs.me

added 471 packages, and audited 472 packages in 58s

18 vulnerabilities (16 high, 2 critical)

To address issues that do not require attention, run:
  npm audit fix

To address all issues (including breaking changes), run:
  npm audit fix --force

Run `npm audit` for details.
npm warn allow-scripts 2 packages have install scripts not yet covered by allowScripts:
npm warn allow-scripts   @tailwindcss/oxide@4.1.11 (postinstall: node ./scripts/install.js)
npm warn allow-scripts   esbuild@0.25.12 (postinstall: node install.js)
npm warn allow-scripts
npm warn allow-scripts Run `npm approve-scripts --allow-scripts-pending` to review, or `npm approve-scripts <pkg>` to allow.
```

## phase17-foundation-format.log

```text
10:[90msrc/api/errors.ts[39m 3ms (unchanged)
16:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
51:[90mtests/error-banner.test.tsx[39m 3ms (unchanged)


> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 52ms (unchanged)
[90mindex.html[39m 28ms (unchanged)
[90mpackage.json[39m 4ms (unchanged)
[90mREADME.md[39m 30ms (unchanged)
src/api/client.ts 94ms
[90msrc/api/errors.ts[39m 3ms (unchanged)
src/api/guards.ts 29ms
[90msrc/api/README.md[39m 2ms (unchanged)
src/api/routes.ts 10ms
src/App.tsx 28ms
[90msrc/components/AppShell.tsx[39m 9ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 2ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
src/features/auth/LoginPage.tsx 9ms
[90msrc/features/auth/README.md[39m 3ms (unchanged)
src/features/auth/SessionBoundary.tsx 12ms
src/features/auth/SetupPage.tsx 14ms
src/features/execution/ExecutionPage.tsx 8ms
src/features/execution/executionResult.ts 2ms
src/features/execution/ExecutionResultPage.tsx 4ms
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/README.md[39m 2ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 2ms (unchanged)
src/features/sets/SetSelectionPage.tsx 13ms
[90msrc/features/settings/README.md[39m 3ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 12ms (unchanged)
[90msrc/main.tsx[39m 2ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 2ms (unchanged)
[90msrc/pages/README.md[39m 3ms (unchanged)
[90msrc/README.md[39m 2ms (unchanged)
[90msrc/routing.ts[39m 7ms (unchanged)
[90msrc/styles.css[39m 42ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 6ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 1ms (unchanged)
tests/api.test.ts 29ms
tests/app-auth.test.tsx 15ms
tests/app-execution.test.tsx 19ms
tests/app-routing.test.tsx 14ms
tests/app-sets.test.tsx 13ms
[90mtests/app.test.ts[39m 2ms (unchanged)
[90mtests/appFixtures.ts[39m 6ms (unchanged)
[90mtests/error-banner.test.tsx[39m 3ms (unchanged)
[90mtests/fakeFetch.ts[39m 9ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
tests/guards.test.ts 4ms
[90mtests/README.md[39m 5ms (unchanged)
[90mtests/render.tsx[39m 12ms (unchanged)
tests/setup.ts 8ms
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)
```

## phase17-foundation-frontend.log

```text
17:[31m✖[39m 1 problem ([31m1 error[39m, [33m0 warnings[39m)
18:  1 error potentially fixable with the "--fix" option.


> esp32-macro-keyboard-webapp@0.1.0 typecheck
> tsc -b --pretty false


> esp32-macro-keyboard-webapp@0.1.0 lint
> eslint . --max-warnings=0


> esp32-macro-keyboard-webapp@0.1.0 stylelint
> stylelint 'src/**/*.css' --max-warnings=0


src/styles.css
  [2m239:8[22m  [31m[31m✖[39m  Expected "context" media feature range notation  [2mmedia-feature-range-notation[22m

[31m✖[39m 1 problem ([31m1 error[39m, [33m0 warnings[39m)
  1 error potentially fixable with the "--fix" option.

```
