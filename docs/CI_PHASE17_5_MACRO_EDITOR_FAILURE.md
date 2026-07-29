# Phase 17.5 macro editor failure

- Transform: success
- Cleanup: success
- Node setup: success
- Frontend dependencies: success
- Format: success
- Frontend validation: failure
- Host dependencies: skipped
- ESP-IDF install: skipped
- Authoritative gate: skipped

## phase17-5-macro-editor-transform.log

```text

Phase 17.5 macro editor applied
```

## phase17-5-macro-editor-cleanup.log

```text

```

## phase17-5-macro-editor-frontend-deps.log

```text

npm warn deprecated whatwg-encoding@3.1.1: Use @exodus/bytes instead for a more spec-conformant and faster implementation
npm warn deprecated glob@10.5.0: Old versions of glob are not supported, and contain widely publicized security vulnerabilities, which have been fixed in the current version. Please update. Support for old versions may be purchased (at exorbitant rates) by contacting i@izs.me

added 471 packages, and audited 472 packages in 54s

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

## phase17-5-macro-editor-format.log

```text
10:[90msrc/api/errors.ts[39m 3ms (unchanged)
16:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
55:[90mtests/error-banner.test.tsx[39m 4ms (unchanged)
75:[90msrc/api/errors.ts[39m 5ms (unchanged)
81:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
120:[90mtests/error-banner.test.tsx[39m 3ms (unchanged)


> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 46ms (unchanged)
[90mindex.html[39m 28ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 27ms (unchanged)
[90msrc/api/client.ts[39m 85ms (unchanged)
[90msrc/api/errors.ts[39m 3ms (unchanged)
[90msrc/api/guards.ts[39m 37ms (unchanged)
[90msrc/api/README.md[39m 2ms (unchanged)
[90msrc/api/routes.ts[39m 21ms (unchanged)
[90msrc/App.tsx[39m 27ms (unchanged)
[90msrc/components/AppShell.tsx[39m 11ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 1ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 7ms (unchanged)
[90msrc/features/auth/README.md[39m 3ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 12ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 14ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 7ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 2ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 6ms (unchanged)
[90msrc/features/execution/README.md[39m 4ms (unchanged)
src/features/macros/macroDraft.ts 14ms
src/features/macros/MacroEditorPage.tsx 46ms
src/features/macros/MacroLibraryPage.tsx 14ms
[90msrc/features/macros/README.md[39m 8ms (unchanged)
[90msrc/features/procedures/README.md[39m 3ms (unchanged)
[90msrc/features/README.md[39m 2ms (unchanged)
[90msrc/features/sets/README.md[39m 3ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 17ms (unchanged)
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 6ms (unchanged)
[90msrc/main.tsx[39m 3ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 2ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 7ms (unchanged)
[90msrc/styles.css[39m 39ms (unchanged)
[90msrc/types/limits.ts[39m 1ms (unchanged)
[90msrc/types/models.ts[39m 5ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 1ms (unchanged)
[90mtests/api.test.ts[39m 22ms (unchanged)
[90mtests/app-auth.test.tsx[39m 15ms (unchanged)
[90mtests/app-execution.test.tsx[39m 15ms (unchanged)
tests/app-macros.test.tsx 15ms
[90mtests/app-routing.test.tsx[39m 7ms (unchanged)
[90mtests/app-sets.test.tsx[39m 7ms (unchanged)
[90mtests/app.test.ts[39m 1ms (unchanged)
[90mtests/appFixtures.ts[39m 6ms (unchanged)
[90mtests/error-banner.test.tsx[39m 4ms (unchanged)
[90mtests/fakeFetch.ts[39m 9ms (unchanged)
[90mtests/fakeLocation.ts[39m 3ms (unchanged)
[90mtests/guards.test.ts[39m 7ms (unchanged)
[90mtests/README.md[39m 3ms (unchanged)
[90mtests/render.tsx[39m 7ms (unchanged)
[90mtests/setup.ts[39m 4ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)

> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 34ms (unchanged)
[90mindex.html[39m 20ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 20ms (unchanged)
[90msrc/api/client.ts[39m 68ms (unchanged)
[90msrc/api/errors.ts[39m 5ms (unchanged)
[90msrc/api/guards.ts[39m 41ms (unchanged)
[90msrc/api/README.md[39m 2ms (unchanged)
[90msrc/api/routes.ts[39m 15ms (unchanged)
[90msrc/App.tsx[39m 26ms (unchanged)
[90msrc/components/AppShell.tsx[39m 10ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 1ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 1ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 8ms (unchanged)
[90msrc/features/auth/README.md[39m 3ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 12ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 12ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 6ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 2ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 4ms (unchanged)
[90msrc/features/execution/README.md[39m 4ms (unchanged)
[90msrc/features/macros/macroDraft.ts[39m 15ms (unchanged)
[90msrc/features/macros/MacroEditorPage.tsx[39m 44ms (unchanged)
[90msrc/features/macros/MacroLibraryPage.tsx[39m 13ms (unchanged)
[90msrc/features/macros/README.md[39m 7ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 3ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 15ms (unchanged)
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 6ms (unchanged)
[90msrc/main.tsx[39m 2ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 1ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 7ms (unchanged)
[90msrc/styles.css[39m 36ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 6ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 1ms (unchanged)
[90mtests/api.test.ts[39m 19ms (unchanged)
[90mtests/app-auth.test.tsx[39m 15ms (unchanged)
[90mtests/app-execution.test.tsx[39m 18ms (unchanged)
[90mtests/app-macros.test.tsx[39m 17ms (unchanged)
[90mtests/app-routing.test.tsx[39m 6ms (unchanged)
[90mtests/app-sets.test.tsx[39m 9ms (unchanged)
[90mtests/app.test.ts[39m 1ms (unchanged)
[90mtests/appFixtures.ts[39m 5ms (unchanged)
[90mtests/error-banner.test.tsx[39m 3ms (unchanged)
[90mtests/fakeFetch.ts[39m 9ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
[90mtests/guards.test.ts[39m 5ms (unchanged)
[90mtests/README.md[39m 3ms (unchanged)
[90mtests/render.tsx[39m 9ms (unchanged)
[90mtests/setup.ts[39m 5ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)
```

## phase17-5-macro-editor-frontend.log

```text
5:src/App.tsx(217,48): error TS2304: Cannot find name 'routeHash'.
6:src/features/macros/MacroEditorPage.tsx(26,3): error TS6133: 'utf8ByteLength' is declared but its value is never read.
7:src/features/macros/MacroEditorPage.tsx(276,31): error TS18047: 'activeSet' is possibly 'null'.


> esp32-macro-keyboard-webapp@0.1.0 typecheck
> tsc -b --pretty false

src/App.tsx(217,48): error TS2304: Cannot find name 'routeHash'.
src/features/macros/MacroEditorPage.tsx(26,3): error TS6133: 'utf8ByteLength' is declared but its value is never read.
src/features/macros/MacroEditorPage.tsx(276,31): error TS18047: 'activeSet' is possibly 'null'.
```
