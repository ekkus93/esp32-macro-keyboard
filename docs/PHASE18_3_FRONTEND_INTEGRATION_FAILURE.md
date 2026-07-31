# Phase 18.3 frontend integration failure

```text

> esp32-macro-keyboard-webapp@0.1.0 format:write
> prettier --write .

[90meslint.config.js[39m 53ms (unchanged)
[90mindex.html[39m 29ms (unchanged)
[90mpackage.json[39m 4ms (unchanged)
[90mREADME.md[39m 29ms (unchanged)
[90msrc/api/client.ts[39m 142ms (unchanged)
[90msrc/api/errors.ts[39m 4ms (unchanged)
[90msrc/api/executionGuards.ts[39m 5ms (unchanged)
[90msrc/api/guards.ts[39m 53ms (unchanged)
[90msrc/api/managementGuards.ts[39m 26ms (unchanged)
[90msrc/api/packages.ts[39m 5ms (unchanged)
[90msrc/api/README.md[39m 1ms (unchanged)
[90msrc/api/routes.ts[39m 31ms (unchanged)
[90msrc/App.tsx[39m 47ms (unchanged)
[90msrc/components/AccessibleDialog.tsx[39m 14ms (unchanged)
[90msrc/components/AppShell.tsx[39m 15ms (unchanged)
[90msrc/components/ConnectivityBanner.tsx[39m 8ms (unchanged)
[90msrc/components/ErrorBanner.tsx[39m 4ms (unchanged)
[90msrc/components/README.md[39m 3ms (unchanged)
[90msrc/components/StatusBadge.tsx[39m 6ms (unchanged)
[90msrc/features/auth/LoginPage.tsx[39m 9ms (unchanged)
[90msrc/features/auth/README.md[39m 5ms (unchanged)
[90msrc/features/auth/SessionBoundary.tsx[39m 15ms (unchanged)
[90msrc/features/auth/SetupPage.tsx[39m 21ms (unchanged)
[90msrc/features/execution/ConfirmExecutionPage.css[39m 43ms (unchanged)
[90msrc/features/execution/ConfirmExecutionPage.tsx[39m 63ms (unchanged)
[90msrc/features/execution/ExecutionPage.tsx[39m 12ms (unchanged)
[90msrc/features/execution/executionResult.ts[39m 5ms (unchanged)
[90msrc/features/execution/ExecutionResultPage.tsx[39m 4ms (unchanged)
[90msrc/features/execution/README.md[39m 3ms (unchanged)
[90msrc/features/macros/macroDraft.ts[39m 14ms (unchanged)
[90msrc/features/macros/MacroEditorPage.tsx[39m 36ms (unchanged)
[90msrc/features/macros/MacroLibraryPage.tsx[39m 15ms (unchanged)
[90msrc/features/macros/README.md[39m 7ms (unchanged)
[90msrc/features/procedures/ProcedureLibraryPage.tsx[39m 17ms (unchanged)
[90msrc/features/procedures/procedureState.ts[39m 15ms (unchanged)
[90msrc/features/procedures/ProcedureWorkflowPage.tsx[39m 56ms (unchanged)
[90msrc/features/procedures/README.md[39m 4ms (unchanged)
[90msrc/features/README.md[39m 1ms (unchanged)
[90msrc/features/sets/README.md[39m 2ms (unchanged)
[90msrc/features/sets/SetManagementPage.tsx[39m 52ms (unchanged)
[90msrc/features/sets/SetSelectionPage.tsx[39m 14ms (unchanged)
[90msrc/features/settings/DiagnosticsPage.tsx[39m 15ms (unchanged)
src/features/settings/PackageOperationsPage.tsx 16ms
[90msrc/features/settings/README.md[39m 3ms (unchanged)
[90msrc/features/settings/SettingsPage.tsx[39m 21ms (unchanged)
[90msrc/main.tsx[39m 1ms (unchanged)
[90msrc/management.css[39m 16ms (unchanged)
[90msrc/pages/DeferredPage.tsx[39m 2ms (unchanged)
[90msrc/pages/README.md[39m 2ms (unchanged)
[90msrc/README.md[39m 1ms (unchanged)
[90msrc/routing.ts[39m 11ms (unchanged)
[90msrc/styles.css[39m 23ms (unchanged)
[90msrc/types/limits.ts[39m 2ms (unchanged)
[90msrc/types/models.ts[39m 8ms (unchanged)
[90msrc/types/README.md[39m 1ms (unchanged)
[90mstylelint.config.mjs[39m 1ms (unchanged)
[90mtests/api-execution-submit.test.ts[39m 8ms (unchanged)
[90mtests/api-timeout.test.ts[39m 7ms (unchanged)
[90mtests/api.test.ts[39m 24ms (unchanged)
[90mtests/app-auth.test.tsx[39m 13ms (unchanged)
[90mtests/app-execution.test.tsx[39m 14ms (unchanged)
[90mtests/app-macros.test.tsx[39m 17ms (unchanged)
[90mtests/app-procedures.test.tsx[39m 20ms (unchanged)
[90mtests/app-routing.test.tsx[39m 7ms (unchanged)
[90mtests/app-sets.test.tsx[39m 7ms (unchanged)
[90mtests/app.test.ts[39m 1ms (unchanged)
[90mtests/appFixtures.ts[39m 8ms (unchanged)
[90mtests/browser/run-browser-tests.mjs[39m 47ms (unchanged)
[90mtests/error-banner.test.tsx[39m 5ms (unchanged)
[90mtests/execution-confirmation.test.tsx[39m 19ms (unchanged)
[90mtests/execution-identity.test.tsx[39m 4ms (unchanged)
[90mtests/fakeFetch.ts[39m 7ms (unchanged)
[90mtests/fakeLocation.ts[39m 2ms (unchanged)
[90mtests/guards.test.ts[39m 9ms (unchanged)
[90mtests/management-api.test.ts[39m 14ms (unchanged)
tests/management-screens.test.tsx 21ms
[90mtests/README.md[39m 3ms (unchanged)
[90mtests/render.tsx[39m 11ms (unchanged)
[90mtests/routing-confirmation.test.ts[39m 5ms (unchanged)
[90mtests/set-management.test.tsx[39m 18ms (unchanged)
[90mtests/setup.ts[39m 5ms (unchanged)
[90mtsconfig.app.json[39m 2ms (unchanged)
[90mtsconfig.json[39m 1ms (unchanged)
[90mtsconfig.node.json[39m 1ms (unchanged)
[90mvite.config.ts[39m 2ms (unchanged)

> esp32-macro-keyboard-webapp@0.1.0 typecheck
> tsc -b --pretty false


> esp32-macro-keyboard-webapp@0.1.0 lint
> eslint . --max-warnings=0


> esp32-macro-keyboard-webapp@0.1.0 stylelint
> stylelint 'src/**/*.css' --max-warnings=0


> esp32-macro-keyboard-webapp@0.1.0 test
> vitest run


[1m[30m[46m RUN [49m[39m[22m [36mv4.1.10 [39m[90m/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp[39m

 [32m✓[39m tests/api.test.ts [2m([22m[2m20 tests[22m[2m)[22m[32m 60[2mms[22m[39m
 [32m✓[39m tests/app-procedures.test.tsx [2m([22m[2m8 tests[22m[2m)[22m[32m 182[2mms[22m[39m
 [31m❯[39m tests/management-screens.test.tsx [2m([22m[2m7 tests[22m[2m | [22m[31m1 failed[39m[2m)[22m[32m 185[2mms[22m[39m
     [32m✓[39m shows live redacted storage and quarantine data[32m 63[2mms[22m[39m
     [32m✓[39m enables only transactional replacement on the import screen[32m 30[2mms[22m[39m
[31m     [31m×[31m validates and confirms a transactional set replacement[39m[32m 29[2mms[22m[39m
     [32m✓[39m downloads a strictly validated raw set package[32m 15[2mms[22m[39m
     [32m✓[39m shows physical-confirmation state for restart[32m 26[2mms[22m[39m
     [32m✓[39m requires an exact destructive factory-reset phrase[32m 16[2mms[22m[39m
     [32m✓[39m announces offline and reconnect state and triggers a live refresh[32m 4[2mms[22m[39m
 [32m✓[39m tests/execution-confirmation.test.tsx [2m([22m[2m5 tests[22m[2m)[22m[32m 114[2mms[22m[39m
 [32m✓[39m tests/set-management.test.tsx [2m([22m[2m4 tests[22m[2m)[22m[32m 206[2mms[22m[39m
 [32m✓[39m tests/app-macros.test.tsx [2m([22m[2m5 tests[22m[2m)[22m[32m 256[2mms[22m[39m
 [32m✓[39m tests/app-auth.test.tsx [2m([22m[2m7 tests[22m[2m)[22m[32m 190[2mms[22m[39m
 [32m✓[39m tests/management-api.test.ts [2m([22m[2m4 tests[22m[2m)[22m[32m 35[2mms[22m[39m
 [32m✓[39m tests/app-execution.test.tsx [2m([22m[2m11 tests[22m[2m)[22m[32m 192[2mms[22m[39m
 [32m✓[39m tests/api-execution-submit.test.ts [2m([22m[2m8 tests[22m[2m)[22m[32m 25[2mms[22m[39m
 [32m✓[39m tests/app-sets.test.tsx [2m([22m[2m4 tests[22m[2m)[22m[32m 189[2mms[22m[39m
 [32m✓[39m tests/app-routing.test.tsx [2m([22m[2m21 tests[22m[2m)[22m[33m 322[2mms[22m[39m
 [32m✓[39m tests/guards.test.ts [2m([22m[2m3 tests[22m[2m)[22m[32m 14[2mms[22m[39m
 [32m✓[39m tests/execution-identity.test.tsx [2m([22m[2m2 tests[22m[2m)[22m[32m 67[2mms[22m[39m
 [32m✓[39m tests/routing-confirmation.test.ts [2m([22m[2m10 tests[22m[2m)[22m[32m 27[2mms[22m[39m
 [32m✓[39m tests/api-timeout.test.ts [2m([22m[2m5 tests[22m[2m)[22m[32m 36[2mms[22m[39m
 [32m✓[39m tests/error-banner.test.tsx [2m([22m[2m3 tests[22m[2m)[22m[32m 33[2mms[22m[39m
 [32m✓[39m tests/app.test.ts [2m([22m[2m1 test[22m[2m)[22m[32m 10[2mms[22m[39m

[31m⎯⎯⎯⎯⎯⎯⎯[39m[1m[41m Failed Tests 1 [49m[22m[31m⎯⎯⎯⎯⎯⎯⎯[39m

[41m[1m FAIL [22m[49m tests/management-screens.test.tsx[2m > [22mmanagement screens[2m > [22mvalidates and confirms a transactional set replacement
[31m[1mAssertionError[22m: expected true to be false // Object.is equality[39m

[32m- Expected[39m
[31m+ Received[39m

[32m- false[39m
[31m+ true[39m

[36m [2m❯[22m tests/management-screens.test.tsx:[2m130:61[22m[39m
    [90m128|[39m     })[33m;[39m
    [90m129|[39m     [35mawait[39m [34mflushReact[39m()[33m;[39m
    [90m130|[39m     expect(buttonWithText("Replace selected set").disabled).toBe(false…
    [90m   |[39m                                                             [31m^[39m
    [90m131|[39m
    [90m132|[39m     [35mawait[39m [34mclick[39m([34mbuttonWithText[39m([32m"Replace selected set"[39m))[33m;[39m

[31m[2m⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[1/1]⎯[22m[39m

[31m⎯⎯⎯⎯⎯⎯[39m[1m[41m Unhandled Errors [49m[22m[31m⎯⎯⎯⎯⎯⎯[39m
[31m[1m
Vitest caught 1 unhandled error during the test run.
This might cause false positive tests. Resolve unhandled errors to make sure your tests are not affected.[22m[39m

[31m⎯⎯⎯⎯[39m[1m[41m Unhandled Rejection [49m[22m[31m⎯⎯⎯⎯⎯[39m
[31m[1mTypeError[22m: event.target.files?.item is not a function[39m
[36m [2m❯[22m selectReplacement src/features/settings/PackageOperationsPage.tsx:[2m112:38[22m[39m
    [90m110|[39m     [34msetReplacementPackage[39m([35mnull[39m)[33m;[39m
    [90m111|[39m     [34msetReplacementFilename[39m([35mnull[39m)[33m;[39m
    [90m112|[39m     [35mconst[39m file [33m=[39m event[33m.[39mtarget[33m.[39mfiles[33m?.[39m[34mitem[39m([34m0[39m)[33m;[39m
    [90m   |[39m                                      [31m^[39m
    [90m113|[39m     [35mif[39m (file [33m===[39m undefined [33m||[39m file [33m===[39m [35mnull[39m) {
    [90m114|[39m       [35mreturn[39m[33m;[39m
[90m [2m❯[22m onChange src/features/settings/PackageOperationsPage.tsx:[2m241:22[22m[39m
[90m [2m❯[22m executeDispatch node_modules/react-dom/cjs/react-dom-client.development.js:[2m16368:9[22m[39m
[90m [2m❯[22m runWithFiberInDEV node_modules/react-dom/cjs/react-dom-client.development.js:[2m1522:13[22m[39m
[90m [2m❯[22m processDispatchQueue node_modules/react-dom/cjs/react-dom-client.development.js:[2m16418:19[22m[39m
[90m [2m❯[22m node_modules/react-dom/cjs/react-dom-client.development.js:[2m17016:9[22m[39m
[90m [2m❯[22m batchedUpdates$1 node_modules/react-dom/cjs/react-dom-client.development.js:[2m3262:40[22m[39m
[90m [2m❯[22m dispatchEventForPluginEventSystem node_modules/react-dom/cjs/react-dom-client.development.js:[2m16572:7[22m[39m
[90m [2m❯[22m dispatchEvent node_modules/react-dom/cjs/react-dom-client.development.js:[2m20658:11[22m[39m
[90m [2m❯[22m dispatchDiscreteEvent node_modules/react-dom/cjs/react-dom-client.development.js:[2m20626:11[22m[39m

[31mThis error originated in "[1mtests/management-screens.test.tsx[22m" test file. It doesn't mean the error was thrown inside the file itself, but while it was running.[39m
[31mThe latest test that might've caused the error is "[1mvalidates and confirms a transactional set replacement[22m". It might mean one of the following:
- The error was thrown, while Vitest was running this test.
- If the error occurred after the test had been completed, this was the last documented test before it was thrown.[39m
[31m⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯⎯[39m


[2m Test Files [22m [1m[31m1 failed[39m[22m[2m | [22m[1m[32m17 passed[39m[22m[90m (18)[39m
[2m      Tests [22m [1m[31m1 failed[39m[22m[2m | [22m[1m[32m127 passed[39m[22m[90m (128)[39m
[2m     Errors [22m [1m[31m1 error[39m[22m
[2m   Start at [22m 02:38:09
[2m   Duration [22m 5.44s[2m (transform 891ms, setup 458ms, import 1.53s, tests 2.14s, environment 8.58s)[22m


::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/src/features/settings/PackageOperationsPage.tsx,title=Unhandled error,line=112,column=38::TypeError: event.target.files?.item is not a function%0A ❯ selectReplacement src/features/settings/PackageOperationsPage.tsx:112:38%0A ❯ onChange src/features/settings/PackageOperationsPage.tsx:241:22%0A ❯ executeDispatch node_modules/react-dom/cjs/react-dom-client.development.js:16368:9%0A ❯ runWithFiberInDEV node_modules/react-dom/cjs/react-dom-client.development.js:1522:13%0A ❯ processDispatchQueue node_modules/react-dom/cjs/react-dom-client.development.js:16418:19%0A ❯ node_modules/react-dom/cjs/react-dom-client.development.js:17016:9%0A ❯ batchedUpdates$1 node_modules/react-dom/cjs/react-dom-client.development.js:3262:40%0A ❯ dispatchEventForPluginEventSystem node_modules/react-dom/cjs/react-dom-client.development.js:16572:7%0A ❯ dispatchEvent node_modules/react-dom/cjs/react-dom-client.development.js:20658:11%0A ❯ dispatchDiscreteEvent node_modules/react-dom/cjs/react-dom-client.development.js:20626:11%0A%0AThis error originated in "tests/management-screens.test.tsx" test file. It doesn't mean the error was thrown inside the file itself, but while it was running.%0AThe latest test that might've caused the error is "validates and confirms a transactional set replacement". It might mean one of the following:%0A- The error was thrown, while Vitest was running this test.%0A- If the error occurred after the test had been completed, this was the last documented test before it was thrown.%0A

::error file=/home/runner/work/esp32-macro-keyboard/esp32-macro-keyboard/webapp/tests/management-screens.test.tsx,title=tests/management-screens.test.tsx > management screens > validates and confirms a transactional set replacement,line=130,column=61::AssertionError: expected true to be false // Object.is equality%0A%0A- Expected%0A+ Received%0A%0A- false%0A+ true%0A%0A ❯ tests/management-screens.test.tsx:130:61%0A%0A
```

## Pending diff

```diff
diff --git a/docs/API.md b/docs/API.md
index 54e6b07..1599abb 100644
--- a/docs/API.md
+++ b/docs/API.md
@@ -59,9 +59,34 @@ session tokens, CSRF tokens, setup secrets, or encryption material.
 Set duplication requires a new UUID, name, and the source expected revision. The
 new set and all copied set-owned objects begin at revision 1. Progress is not
 copied. Set export returns the raw, validated Phase 18 package with its exact byte
-length. Set import remains an explicit `503 Service Unavailable` boundary until
-Phase 18.3 supplies transactional activation; the Phase 18.1 reader and validator
-never mutate repository state.
+length.
+
+Set replacement uses `POST /api/v1/sets/import` with an exact wrapper:
+
+```json
+{
+  "targetSetId": "11111111-1111-4111-8111-111111111111",
+  "expectedRevision": 3,
+  "package": {
+    "schema_version": 1,
+    "package_type": "set",
+    "sets": [],
+    "macros": [],
+    "global_macros": [],
+    "procedures": [],
+    "progress": []
+  }
+}
+```
+
+The package must contain exactly one set whose ID matches `targetSetId`. The
+current set revision must match `expectedRevision`, and the replacement revision
+comes from the validated package. Referenced global macros are dependencies: they
+must already exist with identical canonical content and are never modified by set
+replacement. The server writes a durable transaction manifest, stages and validates
+the complete replacement tree, atomically activates it, updates the set index, and
+recovers or rolls forward interrupted activation on startup. Physical confirmation
+is required before the request reaches the mutation handler.
 
 ### Macros
 
diff --git a/docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md b/docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md
index 3e907a9..aabe68d 100644
--- a/docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md
+++ b/docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md
@@ -703,3 +703,22 @@ Validation:
 - ESP-IDF v5.5.5 production and device-test builds with fail-closed clang-tidy;
 - macro, procedure, progress, active-set deletion, provisioning-settings, atomic-validator, and
   transaction-recovery host suites all execute as registered CTest targets.
+
+
+## Phase 18.3 completion evidence
+
+Phase 18.3 transactional macro-set replacement is complete.
+
+- Package replacement validates the bounded package and exact set identity before mutation.
+- Referenced global macros are verified as immutable dependencies.
+- A durable `PREPARED` manifest is written before staging begins.
+- The complete set tree is materialized, read back, and validated before activation.
+- Recovery rolls forward `STAGED` through `INDEXED` transactions and rolls back incomplete
+  `PREPARED` staging.
+- `POST /api/v1/sets/import` exposes optimistic-concurrency replacement behind CSRF and
+  physical confirmation.
+- The frontend validates the package, requires an exact typed confirmation, and refreshes the
+  committed set after success.
+- Host coverage includes invalid packages, revision conflicts, dependency mismatches, complete
+  activation, recovery phases, API envelopes, physical-confirmation policy, and frontend request
+  construction.
diff --git a/docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md b/docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md
index 57b78c0..7643817 100644
--- a/docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md
+++ b/docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md
@@ -2038,14 +2038,14 @@ HTTP response.
 
 Add a transaction type and recovery code that handles every phase.
 
-- [ ] stage complete replacement;
-- [ ] validate readback;
-- [ ] back up current set;
-- [ ] activate replacement;
-- [ ] update index;
-- [ ] validate active set;
-- [ ] remove backup and manifest;
-- [ ] recover after every interrupted phase.
+- [x] stage complete replacement;
+- [x] validate readback;
+- [x] back up current set;
+- [x] activate replacement;
+- [x] update index;
+- [x] validate active set;
+- [x] remove backup and manifest;
+- [x] recover after every interrupted phase.
 
 ### 18.4 Implement full backup and restore
 
diff --git a/docs/schemas/macro-set-package.schema.json b/docs/schemas/macro-set-package.schema.json
index 33ef7fc..ac268f2 100644
--- a/docs/schemas/macro-set-package.schema.json
+++ b/docs/schemas/macro-set-package.schema.json
@@ -2,27 +2,32 @@
   "$schema": "https://json-schema.org/draft/2020-12/schema",
   "$id": "https://local.invalid/schemas/macro-set-package.schema.json",
   "title": "ESP32 Macro Keyboard macro-set package",
-  "$comment": "JSON Schema maxLength counts Unicode code points. The executable Phase 18 import validator must additionally enforce every documented UTF-8 byte limit before any persistent mutation.",
+  "$comment": "The executable Phase 18 validator is authoritative and additionally enforces UTF-8 byte limits, UUID ownership, referential integrity, macro compilation, exact global dependencies, and the 512 KiB package limit before persistent mutation.",
   "type": "object",
   "additionalProperties": false,
   "required": [
-    "format",
-    "format_version",
-    "set",
+    "schema_version",
+    "package_type",
+    "sets",
     "macros",
+    "global_macros",
     "procedures",
-    "keyboard_layout",
-    "integrity"
+    "progress"
   ],
   "properties": {
-    "format": {
-      "const": "esp32-macro-keyboard-set"
-    },
-    "format_version": {
+    "schema_version": {
       "const": 1
     },
-    "set": {
-      "$ref": "#/$defs/set"
+    "package_type": {
+      "const": "set"
+    },
+    "sets": {
+      "type": "array",
+      "minItems": 1,
+      "maxItems": 1,
+      "items": {
+        "$ref": "#/$defs/set"
+      }
     },
     "macros": {
       "type": "array",
@@ -31,6 +36,13 @@
         "$ref": "#/$defs/macro"
       }
     },
+    "global_macros": {
+      "type": "array",
+      "maxItems": 100,
+      "items": {
+        "$ref": "#/$defs/macro"
+      }
+    },
     "procedures": {
       "type": "array",
       "maxItems": 50,
@@ -38,18 +50,12 @@
         "$ref": "#/$defs/procedure"
       }
     },
-    "keyboard_layout": {
-      "const": "en-US"
-    },
     "progress": {
       "type": "array",
       "maxItems": 50,
       "items": {
         "$ref": "#/$defs/progress"
       }
-    },
-    "integrity": {
-      "$ref": "#/$defs/integrity"
     }
   },
   "$defs": {
@@ -58,24 +64,6 @@
       "$comment": "Canonical UUIDs are exactly 36 ASCII characters and therefore 36 UTF-8 bytes.",
       "pattern": "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$"
     },
-    "integrity": {
-      "type": "object",
-      "additionalProperties": false,
-      "required": [
-        "algorithm",
-        "digest"
-      ],
-      "properties": {
-        "algorithm": {
-          "const": "sha256"
-        },
-        "digest": {
-          "type": "string",
-          "$comment": "The digest is exactly 64 lowercase ASCII characters and therefore 64 UTF-8 bytes.",
-          "pattern": "^[0-9a-f]{64}$"
-        }
-      }
-    },
     "set": {
       "type": "object",
       "additionalProperties": false,
@@ -343,7 +331,8 @@
                 "$ref": "#/$defs/manual_step"
               }
             ]
-          }
+          },
+          "minItems": 1
         },
         "sort_order": {
           "type": "integer"
diff --git a/webapp/src/App.tsx b/webapp/src/App.tsx
index 6e0ca15..f118e44 100644
--- a/webapp/src/App.tsx
+++ b/webapp/src/App.tsx
@@ -145,6 +145,16 @@ function AuthenticatedApp({
     setLoadVersion((version) => version + 1);
   }, []);
 
+  const onSetReplaced = useCallback((replacement: MacroSet): void => {
+    setSets((current) =>
+      current === null
+        ? current
+        : current.map((item) =>
+            item.id === replacement.id ? replacement : item,
+          ),
+    );
+  }, []);
+
   const signOut = async (): Promise<void> => {
     setSigningOut(true);
     setRuntimeError(null);
@@ -300,6 +310,7 @@ function AuthenticatedApp({
           <PackageOperationsPage
             activeSet={activeSet}
             initialSection="import"
+            onSetReplaced={onSetReplaced}
           />
         );
       case "export":
diff --git a/webapp/src/api/packages.ts b/webapp/src/api/packages.ts
index caa1a80..f72f8ff 100644
--- a/webapp/src/api/packages.ts
+++ b/webapp/src/api/packages.ts
@@ -1,5 +1,6 @@
-import { apiRawJsonRequest } from "./client";
-import { isRecord } from "./guards";
+import { apiRawJsonRequest, apiRequest } from "./client";
+import { isMacroSet, isRecord } from "./guards";
+import type { MacroSet } from "../types/models";
 
 export interface SetPackageDocument {
   schema_version: 1;
@@ -60,3 +61,23 @@ export async function exportSetPackage(
     byteLength: response.byteLength,
   };
 }
+
+export async function replaceSetPackage(
+  targetSetId: string,
+  expectedRevision: number,
+  packageDocument: SetPackageDocument,
+): Promise<MacroSet> {
+  return apiRequest(
+    "/api/v1/sets/import",
+    {
+      method: "POST",
+      body: JSON.stringify({
+        targetSetId,
+        expectedRevision,
+        package: packageDocument,
+      }),
+    },
+    isMacroSet,
+    { timeoutMs: 30_000 },
+  );
+}
diff --git a/webapp/src/features/settings/PackageOperationsPage.tsx b/webapp/src/features/settings/PackageOperationsPage.tsx
index 9c9b57a..a362605 100644
--- a/webapp/src/features/settings/PackageOperationsPage.tsx
+++ b/webapp/src/features/settings/PackageOperationsPage.tsx
@@ -1,13 +1,24 @@
 import { useState } from "react";
+import type { ChangeEvent } from "react";
 import { errorText } from "../../api/errors";
-import { exportSetPackage } from "../../api/packages";
+import { isRecord } from "../../api/guards";
+import {
+  exportSetPackage,
+  isSetPackageDocument,
+  replaceSetPackage,
+} from "../../api/packages";
+import type { SetPackageDocument } from "../../api/packages";
+import { AccessibleDialog } from "../../components/AccessibleDialog";
 import { ErrorBanner } from "../../components/ErrorBanner";
 import type { MacroSet } from "../../types/models";
 
+const SET_PACKAGE_MAX_BYTES = 512 * 1024;
+
 interface PackageOperationsPageProps {
   activeSet: MacroSet | null;
   initialSection: "import" | "export";
   saveFile?: (filename: string, text: string) => void;
+  onSetReplaced?: (replacement: MacroSet) => void;
 }
 
 interface OperationCardProps {
@@ -45,15 +56,32 @@ function downloadSetPackage(filename: string, text: string): void {
   URL.revokeObjectURL(url);
 }
 
+function packagedSetId(packageDocument: SetPackageDocument): string | null {
+  const set = packageDocument.sets[0];
+  return isRecord(set) && typeof set.id === "string" ? set.id : null;
+}
+
 export function PackageOperationsPage({
   activeSet,
   initialSection,
   saveFile = downloadSetPackage,
+  onSetReplaced = () => undefined,
 }: PackageOperationsPageProps): React.JSX.Element {
   const [exporting, setExporting] = useState(false);
+  const [replacing, setReplacing] = useState(false);
+  const [replacementPackage, setReplacementPackage] =
+    useState<SetPackageDocument | null>(null);
+  const [replacementFilename, setReplacementFilename] = useState<string | null>(
+    null,
+  );
+  const [confirmationOpen, setConfirmationOpen] = useState(false);
+  const [confirmation, setConfirmation] = useState("");
   const [error, setError] = useState<string | null>(null);
   const [message, setMessage] = useState<string | null>(null);
 
+  const confirmationPhrase =
+    activeSet === null ? "REPLACE" : `REPLACE ${activeSet.name}`;
+
   const performExport = async (): Promise<void> => {
     if (activeSet === null || exporting) {
       return;
@@ -74,6 +102,79 @@ export function PackageOperationsPage({
     }
   };
 
+  const selectReplacement = async (
+    event: ChangeEvent<HTMLInputElement>,
+  ): Promise<void> => {
+    setError(null);
+    setMessage(null);
+    setReplacementPackage(null);
+    setReplacementFilename(null);
+    const file = event.target.files?.item(0);
+    if (file === undefined || file === null) {
+      return;
+    }
+    if (activeSet === null) {
+      setError("Select an active set before choosing a replacement package.");
+      return;
+    }
+    if (file.size === 0 || file.size > SET_PACKAGE_MAX_BYTES) {
+      setError("The replacement package must be between 1 byte and 512 KiB.");
+      return;
+    }
+    try {
+      const parsed: unknown = JSON.parse(await file.text());
+      if (!isSetPackageDocument(parsed)) {
+        throw new Error("The file is not a supported macro-set package.");
+      }
+      if (packagedSetId(parsed) !== activeSet.id) {
+        throw new Error(
+          "The package set ID does not match the selected replacement target.",
+        );
+      }
+      setReplacementPackage(parsed);
+      setReplacementFilename(file.name);
+      setMessage(
+        `Validated ${file.name}. Review the replacement before continuing.`,
+      );
+    } catch (selectionError: unknown) {
+      setError(errorText(selectionError));
+    }
+  };
+
+  const performReplacement = async (): Promise<void> => {
+    if (
+      activeSet === null ||
+      replacementPackage === null ||
+      confirmation !== confirmationPhrase ||
+      replacing
+    ) {
+      return;
+    }
+    setReplacing(true);
+    setError(null);
+    setMessage("Press the confirmation button on the device.");
+    try {
+      const committed = await replaceSetPackage(
+        activeSet.id,
+        activeSet.revision,
+        replacementPackage,
+      );
+      onSetReplaced(committed);
+      setConfirmationOpen(false);
+      setConfirmation("");
+      setReplacementPackage(null);
+      setReplacementFilename(null);
+      setMessage(
+        `Replaced ${activeSet.name} with revision ${String(committed.revision)}.`,
+      );
+    } catch (replacementError: unknown) {
+      setError(errorText(replacementError));
+      setMessage(null);
+    } finally {
+      setReplacing(false);
+    }
+  };
+
   return (
     <section aria-labelledby="package-operations-title">
       <div className="page-heading">
@@ -88,9 +189,9 @@ export function PackageOperationsPage({
       </div>
 
       <div className="boundary-message" role="status">
-        Deterministic set export is available. Import, transactional
-        replacement, full backup, and restore remain disabled until their Phase
-        18 transaction services are complete.
+        Deterministic set export and transactional replacement are available.
+        Import-as-new, full backup, and full restore remain disabled until their
+        later Phase 18 services are complete.
       </div>
 
       <ErrorBanner message={error} />
@@ -124,19 +225,42 @@ export function PackageOperationsPage({
             description="Validate a complete package, assign a new identity, and create it without changing existing sets."
             explanation="transactional import-as-new is not implemented."
           />
-          <OperationCard
-            action="Replace selected set"
-            description={
-              activeSet === null
+          <article className="validation-card">
+            <h3>Replace selected set</h3>
+            <p>
+              {activeSet === null
                 ? "Select an active set before choosing a transactional replacement target."
-                : `Replace ${activeSet.name} only after staging and validating the entire package.`
-            }
-            explanation={
-              activeSet === null
-                ? "there is no active replacement target."
-                : "transactional replace and interrupted-operation recovery are not implemented."
-            }
-          />
+                : `Stage, validate, and atomically replace ${activeSet.name}, including local macros, procedures, ordering, and progress.`}
+            </p>
+            <label htmlFor="replacement-package">Replacement package</label>
+            <input
+              accept="application/json,.json"
+              disabled={activeSet === null || replacing}
+              id="replacement-package"
+              onChange={(event) => {
+                void selectReplacement(event);
+              }}
+              type="file"
+            />
+            <button
+              className="danger"
+              disabled={
+                activeSet === null || replacementPackage === null || replacing
+              }
+              onClick={() => {
+                setConfirmation("");
+                setConfirmationOpen(true);
+              }}
+              type="button"
+            >
+              Replace selected set
+            </button>
+            <p className="field-help">
+              {replacementFilename === null
+                ? "Referenced global macros must already exist on the device with identical content."
+                : `Ready to review ${replacementFilename}.`}
+            </p>
+          </article>
           <OperationCard
             action="Restore full backup"
             description="Restore all sets, global macros, procedures, and optional progress as one transaction."
@@ -175,6 +299,56 @@ export function PackageOperationsPage({
           />
         </div>
       )}
+
+      <AccessibleDialog
+        description={
+          activeSet === null
+            ? "No replacement target is selected."
+            : `This replaces ${activeSet.name} and its set-owned data. Interrupted activation is recovered from the durable transaction manifest.`
+        }
+        onClose={() => {
+          if (!replacing) {
+            setConfirmationOpen(false);
+            setConfirmation("");
+          }
+        }}
+        open={confirmationOpen}
+        title="Confirm transactional replacement"
+      >
+        <label htmlFor="replacement-confirmation">
+          Type <strong>{confirmationPhrase}</strong> to continue.
+        </label>
+        <input
+          autoComplete="off"
+          id="replacement-confirmation"
+          onChange={(event) => {
+            setConfirmation(event.target.value);
+          }}
+          value={confirmation}
+        />
+        <div className="dialog-actions">
+          <button
+            disabled={replacing}
+            onClick={() => {
+              setConfirmationOpen(false);
+              setConfirmation("");
+            }}
+            type="button"
+          >
+            Cancel
+          </button>
+          <button
+            className="danger"
+            disabled={confirmation !== confirmationPhrase || replacing}
+            onClick={() => {
+              void performReplacement();
+            }}
+            type="button"
+          >
+            {replacing ? "Replacing…" : "Confirm replacement"}
+          </button>
+        </div>
+      </AccessibleDialog>
     </section>
   );
 }
diff --git a/webapp/tests/management-screens.test.tsx b/webapp/tests/management-screens.test.tsx
index e47841a..f2e0157 100644
--- a/webapp/tests/management-screens.test.tsx
+++ b/webapp/tests/management-screens.test.tsx
@@ -67,21 +67,96 @@ describe("management screens", () => {
     await view.unmount();
   });
 
-  test("keeps deferred mutating package operations visibly disabled", async () => {
+  test("enables only transactional replacement on the import screen", async () => {
     const view = await render(
       <PackageOperationsPage activeSet={macroSet} initialSection="import" />,
     );
 
     expect(document.body.textContent).toContain(
-      "Deterministic set export is available",
+      "transactional replacement are available",
     );
     expect(buttonWithText("Import as new set").disabled).toBe(true);
     expect(buttonWithText("Replace selected set").disabled).toBe(true);
     expect(buttonWithText("Restore full backup").disabled).toBe(true);
+    expect(
+      requiredElement("#replacement-package", HTMLInputElement).disabled,
+    ).toBe(false);
+    expect(getFetchCalls()).toHaveLength(0);
+    await view.unmount();
+  });
+
+  test("validates and confirms a transactional set replacement", async () => {
+    setCsrfToken("csrf-replace");
+    const replacement = {
+      ...macroSet,
+      revision: macroSet.revision + 1,
+      name: "Imported Replacement",
+    };
+    const packageDocument = {
+      schema_version: 1,
+      package_type: "set",
+      sets: [replacement],
+      macros: [],
+      global_macros: [],
+      procedures: [],
+      progress: [],
+    } as const;
+    const packageText = JSON.stringify(packageDocument);
+    const file = new File([packageText], "replacement.json", {
+      type: "application/json",
+    });
+    Object.defineProperty(file, "text", {
+      configurable: true,
+      value: () => Promise.resolve(packageText),
+    });
+    const onSetReplaced = vi.fn();
+    const view = await render(
+      <PackageOperationsPage
+        activeSet={macroSet}
+        initialSection="import"
+        onSetReplaced={onSetReplaced}
+      />,
+    );
+    const input = requiredElement("#replacement-package", HTMLInputElement);
+    Object.defineProperty(input, "files", {
+      configurable: true,
+      value: [file],
+    });
+    await act(async () => {
+      input.dispatchEvent(new Event("change", { bubbles: true }));
+      await Promise.resolve();
+    });
+    await flushReact();
+    expect(buttonWithText("Replace selected set").disabled).toBe(false);
+
+    await click(buttonWithText("Replace selected set"));
+    await setInputValue(
+      requiredElement("#replacement-confirmation", HTMLInputElement),
+      `REPLACE ${macroSet.name}`,
+    );
+    planJsonResponse(success(replacement));
+    await click(buttonWithText("Confirm replacement"));
+    await flushReact();
+
+    expect(getFetchCalls()).toHaveLength(1);
+    const call = getFetchCalls()[0];
+    expect(call?.url).toBe("/api/v1/sets/import");
+    expect(call?.method).toBe("POST");
+    expect(call?.headers.get("X-CSRF-Token")).toBe("csrf-replace");
+    const requestBody = call?.body;
+    expect(typeof requestBody).toBe("string");
+    if (typeof requestBody !== "string") {
+      throw new Error("Replacement request body was not serialized JSON.");
+    }
+    expect(JSON.parse(requestBody)).toEqual({
+      targetSetId: macroSet.id,
+      expectedRevision: macroSet.revision,
+      package: packageDocument,
+    });
+    expect(onSetReplaced).toHaveBeenCalledWith(replacement);
     expect(document.body.textContent).toContain(
-      "transactional import-as-new is not implemented",
+      `Replaced ${macroSet.name} with revision ${String(replacement.revision)}.`,
     );
-    expect(getFetchCalls()).toHaveLength(0);
     await view.unmount();
   });
 
```
