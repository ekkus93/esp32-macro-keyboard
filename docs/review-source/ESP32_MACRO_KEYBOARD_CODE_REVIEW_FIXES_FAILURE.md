# Code-review fixes validation failure

**Stage:** `intentional dependency upgrades`

**Exit status:** `1`

The production changes were not published. The one-shot workflow, runner, and verified payload remain on `master` for deterministic correction.

## Log tail

```text
npm error code ERESOLVE
npm error ERESOLVE could not resolve
npm error
npm error While resolving: esp32-macro-keyboard-webapp@0.1.0
npm error Found: @eslint/js@9.31.0
npm error node_modules/@eslint/js
npm error   dev @eslint/js@"10.0.1" from the root project
npm error   @eslint/js@"9.31.0" from eslint@9.31.0
npm error   node_modules/eslint
npm error     dev eslint@"10.8.0" from the root project
npm error     peer eslint@"^6.0.0 || ^7.0.0 || >=8.0.0" from @eslint-community/eslint-utils@4.10.1
npm error     node_modules/@eslint-community/eslint-utils
npm error       @eslint-community/eslint-utils@"^4.7.0" from @typescript-eslint/utils@8.38.0
npm error       node_modules/@typescript-eslint/utils
npm error         @typescript-eslint/utils@"8.38.0" from @typescript-eslint/eslint-plugin@8.38.0
npm error         node_modules/@typescript-eslint/eslint-plugin
npm error         2 more (@typescript-eslint/type-utils, typescript-eslint)
npm error       1 more (eslint)
npm error     7 more (@typescript-eslint/eslint-plugin, ...)
npm error
npm error Could not resolve dependency:
npm error dev @eslint/js@"10.0.1" from the root project
npm error
npm error Conflicting peer dependency: eslint@10.8.0
npm error node_modules/eslint
npm error   peerOptional eslint@"^10.0.0" from @eslint/js@10.0.1
npm error   node_modules/@eslint/js
npm error     dev @eslint/js@"10.0.1" from the root project
npm error
npm error Fix the upstream dependency conflict, or retry this command with --force or --legacy-peer-deps to accept an incorrect (and potentially broken) dependency resolution.
npm error
npm error
npm error For a full report see:
npm error /home/runner/.npm/_logs/2026-07-29T06_09_25_632Z-eresolve-report.txt
npm error A complete log of this run can be found in: /home/runner/.npm/_logs/2026-07-29T06_09_25_632Z-debug-0.log
From https://github.com/ekkus93/esp32-macro-keyboard
 * branch            master     -> FETCH_HEAD
HEAD is now at 9851369 ci(code-review-fixes): resolve dependency graph from exact manifest
Removing scripts/check-schema-byte-limits.sh
Removing tests/scripts/test-check-schema-byte-limits.sh
Removing webapp/tests/api-execution-submit.test.ts

```
