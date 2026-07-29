# Code-review fixes validation failure

**Stage:** `audit report regeneration`

**Exit status:** `1`

The production changes were not published. The one-shot workflow and verified payload remain on `master` for deterministic correction.

## Log tail

```text
npm warn deprecated whatwg-encoding@3.1.1: Use @exodus/bytes instead for a more spec-conformant and faster implementation
npm warn deprecated glob@10.5.0: Old versions of glob are not supported, and contain widely publicized security vulnerabilities, which have been fixed in the current version. Please update. Support for old versions may be purchased (at exorbitant rates) by contacting i@izs.me

added 462 packages, and audited 463 packages in 14s

5 high severity vulnerabilities

To address all issues (including breaking changes), run:
  npm audit fix --force

Run `npm audit` for details.
npm warn allow-scripts 2 packages have install scripts not yet covered by allowScripts:
npm warn allow-scripts   @tailwindcss/oxide@4.1.11 (postinstall: node ./scripts/install.js)
npm warn allow-scripts   esbuild@0.28.1 (postinstall: node install.js)
npm warn allow-scripts
npm warn allow-scripts Run `npm approve-scripts --allow-scripts-pending` to review, or `npm approve-scripts <pkg>` to allow.

> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 54ms (unchanged)
[90mindex.html[39m 31ms (unchanged)
[90mpackage.json[39m 4ms (unchanged)
[90mREADME.md[39m 29ms (unchanged)
[90msrc/api/client.ts[39m 102ms (unchanged)
[90msrc/api/errors.ts[39m 4ms (unchanged)
[90msrc/api/guards.ts[39m 62ms (unchanged)
[90msrc/api/README.md[39m 3ms (unchanged)
[90msrc/api/routes.ts[39m 24ms (unchanged)
[90msrc/App.tsx[39m 39ms (unchanged)
[90msrc/components/AppShell.tsx[39m 14ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 3ms (unchanged)
[90msrc/components/README.md[39m 2ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 9ms (unchanged)
[90msrc/features/auth/README.md[39m 4ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 15ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 18ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 9ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 3ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 5ms (unchanged)
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/macroDraft.ts[39m 16ms (unchanged)
[90msrc/features/macros/MacroEditorPage.tsx[39m 55ms (unchanged)
[90msrc/features/macros/MacroLibraryPage.tsx[39m 19ms (unchanged)
[90msrc/features/macros/README.md[39m 9ms (unchanged)
[90msrc/features/procedures/ProcedureLibraryPage.tsx[39m 21ms (unchanged)
[90msrc/features/procedures/procedureState.ts[39m 13ms (unchanged)
[90msrc/features/procedures/ProcedureWorkflowPage.tsx[39m 66ms (unchanged)
[90msrc/features/procedures/README.md[39m 4ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 3ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 18ms (unchanged)
[90msrc/features/settings/README.md[39m 3ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 6ms (unchanged)
[90msrc/main.tsx[39m 2ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 2ms (unchanged)
[90msrc/pages/README.md[39m 3ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 14ms (unchanged)
[90msrc/styles.css[39m 58ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 11ms (unchanged)
[90msrc/types/README.md[39m 2ms (unchanged)
[90mstylelint.config.mjs[39m 2ms (unchanged)
[90mtests/api-execution-submit.test.ts[39m 7ms (unchanged)
[90mtests/api.test.ts[39m 23ms (unchanged)
[90mtests/app-auth.test.tsx[39m 15ms (unchanged)
[90mtests/app-execution.test.tsx[39m 14ms (unchanged)
[90mtests/app-macros.test.tsx[39m 24ms (unchanged)
[90mtests/app-procedures.test.tsx[39m 17ms (unchanged)
[90mtests/app-routing.test.tsx[39m 6ms (unchanged)
[90mtests/app-sets.test.tsx[39m 6ms (unchanged)
[90mtests/app.test.ts[39m 1ms (unchanged)
[90mtests/appFixtures.ts[39m 10ms (unchanged)
[90mtests/error-banner.test.tsx[39m 3ms (unchanged)
[90mtests/fakeFetch.ts[39m 7ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
[90mtests/guards.test.ts[39m 7ms (unchanged)
[90mtests/README.md[39m 5ms (unchanged)
[90mtests/render.tsx[39m 8ms (unchanged)
[90mtests/setup.ts[39m 6ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)
HEAD is now at 71e750c ci(code-review-fixes): correct transform root
Removing node_modules/
Removing tests/api-execution-submit.test.ts

```
