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

added 471 packages, and audited 472 packages in 1m

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
10:[90msrc/api/errors.ts[39m 4ms (unchanged)
16:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
51:[90mtests/error-banner.test.tsx[39m 6ms (unchanged)
71:[90msrc/api/errors.ts[39m 3ms (unchanged)
77:[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
112:[90mtests/error-banner.test.tsx[39m 4ms (unchanged)


> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 48ms (unchanged)
[90mindex.html[39m 27ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 26ms (unchanged)
src/api/client.ts 94ms
[90msrc/api/errors.ts[39m 4ms (unchanged)
src/api/guards.ts 30ms
[90msrc/api/README.md[39m 2ms (unchanged)
src/api/routes.ts 11ms
src/App.tsx 23ms
[90msrc/components/AppShell.tsx[39m 10ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 1ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
src/features/auth/LoginPage.tsx 8ms
[90msrc/features/auth/README.md[39m 4ms (unchanged)
src/features/auth/SessionBoundary.tsx 13ms
src/features/auth/SetupPage.tsx 12ms
src/features/execution/ExecutionPage.tsx 7ms
src/features/execution/executionResult.ts 2ms
src/features/execution/ExecutionResultPage.tsx 4ms
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/README.md[39m 3ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 2ms (unchanged)
src/features/sets/SetSelectionPage.tsx 13ms
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 8ms (unchanged)
[90msrc/main.tsx[39m 2ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 1ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 3ms (unchanged)
[90msrc/styles.css[39m 39ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 6ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 1ms (unchanged)
tests/api.test.ts 28ms
tests/app-auth.test.tsx 12ms
tests/app-execution.test.tsx 15ms
tests/app-routing.test.tsx 11ms
tests/app-sets.test.tsx 9ms
[90mtests/app.test.ts[39m 2ms (unchanged)
[90mtests/appFixtures.ts[39m 5ms (unchanged)
[90mtests/error-banner.test.tsx[39m 6ms (unchanged)
[90mtests/fakeFetch.ts[39m 10ms (unchanged)
[90mtests/fakeLocation.ts[39m 3ms (unchanged)
tests/guards.test.ts 4ms
[90mtests/README.md[39m 5ms (unchanged)
[90mtests/render.tsx[39m 12ms (unchanged)
tests/setup.ts 5ms
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)

> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 40ms (unchanged)
[90mindex.html[39m 26ms (unchanged)
[90mpackage.json[39m 3ms (unchanged)
[90mREADME.md[39m 22ms (unchanged)
[90msrc/api/client.ts[39m 78ms (unchanged)
[90msrc/api/errors.ts[39m 3ms (unchanged)
[90msrc/api/guards.ts[39m 30ms (unchanged)
[90msrc/api/README.md[39m 2ms (unchanged)
[90msrc/api/routes.ts[39m 19ms (unchanged)
[90msrc/App.tsx[39m 42ms (unchanged)
[90msrc/components/AppShell.tsx[39m 9ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 2ms (unchanged)
[90msrc/components/README.md[39m 1ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 2ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 9ms (unchanged)
[90msrc/features/auth/README.md[39m 4ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 16ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 14ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 8ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 3ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 5ms (unchanged)
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/README.md[39m 3ms (unchanged)
[90msrc/features/procedures/README.md[39m 2ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 2ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 13ms (unchanged)
[90msrc/features/settings/README.md[39m 2ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 9ms (unchanged)
[90msrc/main.tsx[39m 2ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 1ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 3ms (unchanged)
[90msrc/styles.css[39m 34ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 5ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 2ms (unchanged)
[90mtests/api.test.ts[39m 20ms (unchanged)
[90mtests/app-auth.test.tsx[39m 13ms (unchanged)
[90mtests/app-execution.test.tsx[39m 21ms (unchanged)
[90mtests/app-routing.test.tsx[39m 9ms (unchanged)
[90mtests/app-sets.test.tsx[39m 13ms (unchanged)
[90mtests/app.test.ts[39m 2ms (unchanged)
[90mtests/appFixtures.ts[39m 5ms (unchanged)
[90mtests/error-banner.test.tsx[39m 4ms (unchanged)
[90mtests/fakeFetch.ts[39m 8ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
[90mtests/guards.test.ts[39m 5ms (unchanged)
[90mtests/README.md[39m 5ms (unchanged)
[90mtests/render.tsx[39m 12ms (unchanged)
[90mtests/setup.ts[39m 6ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)
```

## phase17-foundation-frontend.log

```text
34:[31m     → expected undefined to be '/api/v1/settings' // Object.is equality[39m
36: [32m✓[39m tests/error-banner.test.tsx [2m([22m[2m3 tests[22m[2m)[22m[32m 46[2mms[22m[39m
44:[31m[1mAssertionError[22m: expected undefined to be '/api/v1/settings' // Object.is equality[39m
63:[31m[1mError[22m: 1 planned fetch call(s) were not consumed.[39m
67:    [90m 95| [39m    [35mthrow[39m [35mnew[39m [33mError[39m(
82:::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/app-sets.test.tsx,title=tests/app-sets.test.tsx > server-backed set selection > persists real settings updates,line=138,column=23::AssertionError: expected undefined to be '/api/v1/settings' // Object.is equality%0A%0A- Expected: %0A"/api/v1/settings"%0A%0A+ Received: %0Aundefined%0A%0A ❯ tests/app-sets.test.tsx:138:23%0A%0A
84:::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/fakeFetch.ts,title=tests/app-sets.test.tsx > server-backed set selection > persists real settings updates,line=95,column=11::Error: 1 planned fetch call(s) were not consumed.%0A ❯ assertNoPendingFetchPlans tests/fakeFetch.ts:95:11%0A ❯ tests/setup.ts:45:5%0A%0A


> esp32-macro-keyboard-webapp@0.1.0 typecheck
> tsc -b --pretty false


> esp32-macro-keyboard-webapp@0.1.0 lint
> eslint . --max-warnings=0


> esp32-macro-keyboard-webapp@0.1.0 stylelint
> stylelint 'src/**/*.css' --max-warnings=0


> esp32-macro-keyboard-webapp@0.1.0 format:check
> prettier --check .

Checking formatting...
All matched files use Prettier code style!

> esp32-macro-keyboard-webapp@0.1.0 test
> vitest run


[1m[46m RUN [49m[22m [36mv3.2.4 [39m[90m/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp[39m

 [32m✓[39m tests/api.test.ts [2m([22m[2m20 tests[22m[2m)[22m[32m 51[2mms[22m[39m
 [32m✓[39m tests/app-auth.test.tsx [2m([22m[2m7 tests[22m[2m)[22m[32m 132[2mms[22m[39m
 [32m✓[39m tests/app-execution.test.tsx [2m([22m[2m11 tests[22m[2m)[22m[32m 216[2mms[22m[39m
 [31m❯[39m tests/app-sets.test.tsx [2m([22m[2m4 tests[22m[2m | [22m[31m1 failed[39m[2m)[22m[32m 113[2mms[22m[39m
   [32m✓[39m server-backed set selection[2m > [22mshows live metadata and filters by search[32m 63[2mms[22m[39m
   [32m✓[39m server-backed set selection[2m > [22mselects a set with the settings revision and updates the header[32m 18[2mms[22m[39m
   [32m✓[39m server-backed set selection[2m > [22mshows revision conflicts instead of silently accepting selection[32m 14[2mms[22m[39m
[31m   [31m×[31m server-backed set selection[2m > [22mpersists real settings updates[39m[32m 16[2mms[22m[39m
[31m     → expected undefined to be '/api/v1/settings' // Object.is equality[39m
[31m     → 1 planned fetch call(s) were not consumed.[39m
 [32m✓[39m tests/error-banner.test.tsx [2m([22m[2m3 tests[22m[2m)[22m[32m 46[2mms[22m[39m
 [32m✓[39m tests/app-routing.test.tsx [2m([22m[2m21 tests[22m[2m)[22m[32m 255[2mms[22m[39m
 [32m✓[39m tests/guards.test.ts [2m([22m[2m3 tests[22m[2m)[22m[32m 11[2mms[22m[39m
 [32m✓[39m tests/app.test.ts [2m([22m[2m1 test[22m[2m)[22m[32m 9[2mms[22m[39m

[31m⎯⎯⎯⎯⎯⎯⎯[39m[1m[41m Failed Tests 1 [49m[22m[31m⎯⎯⎯⎯⎯⎯⎯[39m

[41m[1m FAIL [22m[49m tests/app-sets.test.tsx[2m > [22mserver-backed set selection[2m > [22mpersists real settings updates
[31m[1mAssertionError[22m: expected undefined to be '/api/v1/settings' // Object.is equality[39m

[32m- Expected:[39m
"/api/v1/settings"

[31m+ Received:[39m
undefined

[36m [2m❯[22m tests/app-sets.test.tsx:[2m138:23[22m[39m
    [90m136| [39m
    [90m137| [39m    [35mconst[39m call [33m=[39m [34mgetFetchCalls[39m()[[34m5[39m][33m;[39m
    [90m138| [39m    [34mexpect[39m(call[33m?.[39murl)[33m.[39m[34mtoBe[39m([32m"/api/v1/settings"[39m)[33m;[39m
    [90m   | [39m                      [31m^[39m
    [90m139| [39m    [34mexpect[39m(call[33m?.[39mmethod)[33m.[39m[34mtoBe[39m([32m"PUT"[39m)[33m;[39m
    [90m140| [39m    [34mexpect[39m(call[33m?.[39mbody)[33m.[39m[34mtoBe[39m(

[31m[2m⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[1/2]⎯[22m[39m

[41m[1m FAIL [22m[49m tests/app-sets.test.tsx[2m > [22mserver-backed set selection[2m > [22mpersists real settings updates
[31m[1mError[22m: 1 planned fetch call(s) were not consumed.[39m
[36m [2m❯[22m assertNoPendingFetchPlans tests/fakeFetch.ts:[2m95:11[22m[39m
    [90m 93| [39m[35mexport[39m [35mfunction[39m [34massertNoPendingFetchPlans[39m()[33m:[39m [35mvoid[39m {
    [90m 94| [39m  [35mif[39m (handlers[33m.[39mlength [33m!==[39m [34m0[39m) {
    [90m 95| [39m    [35mthrow[39m [35mnew[39m [33mError[39m(
    [90m   | [39m          [31m^[39m
    [90m 96| [39m      `${String(handlers.length)} planned fetch call(s) were not consu…
    [90m 97| [39m    )[33m;[39m
[90m [2m❯[22m tests/setup.ts:[2m45:5[22m[39m

[31m[2m⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[2/2]⎯[22m[39m


[2m Test Files [22m [1m[31m1 failed[39m[22m[2m | [22m[1m[32m7 passed[39m[22m[90m (8)[39m
[2m      Tests [22m [1m[31m1 failed[39m[22m[2m | [22m[1m[32m69 passed[39m[22m[90m (70)[39m
[2m   Start at [22m 23:07:46
[2m   Duration [22m 2.68s[2m (transform 291ms, setup 195ms, collect 713ms, tests 834ms, environment 3.61s, prepare 667ms)[22m


::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/app-sets.test.tsx,title=tests/app-sets.test.tsx > server-backed set selection > persists real settings updates,line=138,column=23::AssertionError: expected undefined to be '/api/v1/settings' // Object.is equality%0A%0A- Expected: %0A"/api/v1/settings"%0A%0A+ Received: %0Aundefined%0A%0A ❯ tests/app-sets.test.tsx:138:23%0A%0A

::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/fakeFetch.ts,title=tests/app-sets.test.tsx > server-backed set selection > persists real settings updates,line=95,column=11::Error: 1 planned fetch call(s) were not consumed.%0A ❯ assertNoPendingFetchPlans tests/fakeFetch.ts:95:11%0A ❯ tests/setup.ts:45:5%0A%0A
```
