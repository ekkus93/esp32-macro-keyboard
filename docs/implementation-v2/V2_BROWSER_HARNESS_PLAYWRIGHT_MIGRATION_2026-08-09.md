# Browser test harness migration — hand-rolled CDP to real Playwright

**Track:** Infrastructure (not a numbered V2-xxx task; no TODO_V2.md
checkbox is claimed complete or newly checked by this work).
**Commit:** `b0fd4e3` (worktree branch `worktree-agent-a77ed32c0868d8126`),
built on `master` tip `0c9cc47` ("Merge Track M: Phase 12 settings,
diagnostics, and destructive operations UI").
**Date:** 2026-08-09.

## Why

`webapp/tests/browser/run-browser-tests.mjs` drove a system Chrome/Chromium
binary through a hand-built Chrome DevTools Protocol (CDP) client: manual
`which google-chrome`/`chromium`/... binary discovery (`commandPath()`), a
raw `new WebSocket(...)` connection to the browser's DevTools endpoint
(`devToolsUrl()`/`connectBrowser()`/`connectPage()`), and a hand-rolled
`class Cdp` speaking the CDP JSON-RPC protocol directly
(`Runtime.evaluate`, `Page.enable`/`Page.reload`,
`Page.javascriptDialogOpening`/`Page.handleJavaScriptDialog`,
`Browser.setDownloadBehavior`/`Browser.downloadProgress`,
`DOM.setFileInputFiles`, `Emulation.setDeviceMetricsOverride`). It had no
Playwright dependency at all (`grep -i playwright webapp/package.json`
returned nothing before this change), despite `CLAUDE.md`'s test-location
table already calling this suite "Browser (Playwright)" — an
accidentally-correct label describing an aspiration, not the actual code.

The user's direction: replace the hand-rolled protocol layer with the real
`playwright` package, since it buys nothing over a standard, well-supported
tool.

## What changed

### Migrated (the browser-driving layer)

| Old (hand-rolled CDP) | New (Playwright) |
| --- | --- |
| `commandPath()` (`which` scan for a system Chrome/Chromium binary) | `chromium.launch()` — Playwright's own bundled, pinned Chromium |
| `devToolsUrl()`, `connectBrowser()`, `connectPage()`, `class Cdp` | `browser.newContext()` / `context.newPage()` |
| `evaluate(cdp, expressionString)` (`Runtime.evaluate` over the raw socket) | `page.evaluate(fn)` — real closures, not stringified JS |
| `waitFor(cdp, expressionString, message, timeoutMs)` (manual 50ms poll loop) | `page.waitForFunction(fn, ..., { timeout, polling: 50 })`, same message-on-timeout contract |
| `buttonExpression`/`buttonByAriaLabelExpression`/`clickExpression` (`querySelectorAll('button').find(...)` + manual retry loop) | `page.getByRole("button", { name, exact: true }).first().click()` / `page.getByLabel(label, { exact: true }).first().click()` — Playwright's locators auto-wait/retry, so the manual retry loop is gone |
| `dispatchKey(cdp, "Enter")` after a manual `.focus()` | `locator.press("Enter")` (focuses and presses in one call) |
| `setFileInputFiles()` (`DOM.setFileInputFiles` over raw CDP) | `page.setInputFiles('input[type="file"]', path)` |
| `waitForDownloadEvent()` (`Browser.setDownloadBehavior` + polling `Browser.downloadProgress`, plus a 20-attempt retry loop worked around a snap-Chromium/AppArmor mount-namespace bug) | `page.waitForEvent("download")` + `download.saveAs(path)` — Playwright's own (non-snap) bundled Chromium doesn't hit that bug, so the workaround and its retry loop are gone entirely |
| Manual `Page.javascriptDialogOpening` interception, auto-accepting every dialog | `page.on("dialog", (dialog) => dialog.accept())` |
| Manual `.value` setter + `dispatchEvent(new Event(...))` for React-controlled inputs (device-name field, delete-confirmation field) | `locator.fill(value)` — Playwright's own React-safe fill implementation |
| `stopChrome()` (manual SIGTERM/SIGKILL of the spawned process) | `browser.close()` |

### Kept as a legitimate Playwright API, not hand-rolled protocol

`assertResponsiveLayout()` needs to toggle real Chrome device-metrics
emulation (`deviceScaleFactor`, `mobile`) mid-test — not just resize the
viewport — per the existing SPEC 9/24.5 comment on why a real mobile
emulation profile matters and a plain `page.setViewportSize()` (width/height
only) would not exercise. Playwright has no higher-level API for changing
`deviceScaleFactor`/`mobile` after page creation, so this uses
`page.context().newCDPSession(page)` — Playwright's own documented,
supported escape hatch for cases its high-level API doesn't cover. This is
categorically different from the code it replaces: it's a single, narrow,
Playwright-mediated CDP call for one specific capability gap, not a
hand-built WebSocket JSON-RPC transport implementing the whole protocol.

### Explicitly not migrated (out of scope, and correctly so)

The `node:http` fixture server (`startApplicationServer`, `createServer`
from `node:http`) that fakes the firmware's `/api/v1/*` responses and serves
the built `webapp/dist` is untouched. It's a test fixture, not a
browser-automation mechanism, and switching it to Playwright's
`page.route()` request interception would be a bigger, unrelated
architectural change nobody asked for.

## The framework decision: plain `playwright`, not `@playwright/test`

Investigated both options seriously before choosing.

**Option A — adopt `@playwright/test`:** the full test-runner framework,
with its own `test()`/`describe()` blocks, `playwright.config.ts`, HTML
reporter, and trace viewer.

**Option B — the plain `playwright` package**, driving `chromium.launch()`/
`Page` directly inside the existing single-script structure, keeping the
`console.log("...passed.")`-per-scenario convention.

**Chose B.** Reasoning:

- `check-webapp.sh`'s chain runs `npm run test:browser`, which is defined as
  `npm run build && node tests/browser/run-browser-tests.mjs` — a plain
  script invocation, once, producing exactly three log lines
  (`Real Chrome v2 Macros page/Quick Send workflows passed.` /
  `Real Chrome v2 Snapshots/import-export workflows passed.` /
  `Real Chrome v2 Settings/Diagnostics workflows passed.`) that this repo's
  gates already parse. `@playwright/test` invokes via its own `playwright
  test` CLI, discovers `*.spec.ts` files, and reports through its own
  runner/reporter — none of that is a drop-in replacement for
  `node script.mjs`; adopting it means changing the invocation contract
  those gates depend on.
- `@playwright/test` brings a `playwright.config.ts` this repo doesn't have,
  and (by default) HTML-reporter/trace-viewer artifacts this repo has no
  convention for storing, uploading, or gitignoring. That's new
  infrastructure with no behavioral payoff: the three scenarios already run
  correctly, in order, in one process, sharing one fixture server and one
  browser context — exactly what `@playwright/test`'s parallel-by-default,
  isolated-per-test model would have to be configured *against* (serial
  mode, one worker, shared fixtures) to reproduce.
- The three "workflows" are not independent tests in the `@playwright/test`
  sense — `runSnapshotsWorkflows` depends on `runBrowserWorkflows` having
  already reordered/sent macros, and `runSettingsWorkflows` runs against
  whatever state Snapshots left behind. Forcing them into `@playwright/test`
  would mean either three independently-runnable-but-fragile test cases (a
  worse design) or one giant `test()` block containing the entire sequence
  (which gains nothing over the plain script).
- Either option results in genuinely using Playwright's `Browser`/`Page`
  API and locators — the requirement that it be "recognizable as using
  Playwright" is satisfied by both. The plain-package route gets there with
  a strictly smaller diff, no new config surface, and zero risk to the
  `check-webapp.sh`/CI output contract.

If this suite grows into many independent, parallelizable scenarios in the
future, `@playwright/test` becomes worth revisiting — that tradeoff didn't
exist here.

## `.github/workflows/browser-tests.yml`

Added one step:

```yaml
- name: Install Playwright Chromium
  working-directory: webapp
  run: npx playwright install --with-deps chromium
```

placed after `npm ci --ignore-scripts --no-audit --no-fund` and before the
frontend build. It's required because `--ignore-scripts` means no
postinstall hook fetches the browser (the `playwright`/`playwright-core`
packages at the pinned version have no `postinstall` script of their own —
verified directly against the installed `node_modules/playwright/package.json`
and `node_modules/playwright-core/package.json`, neither defines a
`scripts` object), and the old implicit dependency on the runner's
pre-installed system Chrome no longer exists. `--with-deps` additionally
installs the apt packages Chromium needs to actually launch, using the
passwordless sudo GitHub-hosted runners have.

## A local-dev/`check-webapp.sh` precondition, documented rather than papered over

`scripts/check-webapp.sh` was not changed (out of the assigned file
surface) and still does not install the Playwright browser itself — the
same was true before this migration for the system Chrome/Chromium binary
it used to require on `PATH`. This is a precondition swap, not a
regression: a machine that runs `check-webapp.sh` needs
`npx playwright install [--with-deps] chromium` to have been run once
(exactly as it previously needed `google-chrome`/`chromium` pre-installed).
This machine already had a cached Chromium under `~/.cache/ms-playwright`
from prior use, so this was not exercised as a cold-start path in this
session; a genuinely fresh machine needs that one-time install step, which
is documented here rather than silently assumed.

A `"postinstall": "playwright install chromium"` script in
`webapp/package.json` was considered and rejected: `webapp/package.json` is
also `npm ci`'d by `quality.yml` (lint/typecheck/etc., no browser tests) and
`scripts/check-v2-contracts.sh`, and unconditionally downloading a full
Chromium build for jobs that never run browser tests is unwanted bloat with
no matching task instruction to add it.

## Commands run and results

All from `webapp/` unless noted, Node `v24.18.0` (`.nvmrc`), via `nvm use
24.18.0`.

```text
npm install --save-exact --save-dev playwright@1.62.1     # pinned, no ^/~
npx playwright install chromium                            # no-op: already cached
```

- `node tests/browser/run-browser-tests.mjs` — **10 consecutive standalone
  runs, all pass** (5 before `prettier --write` reformatted the new file, 5
  after, confirming the reformat changed no behavior):

  ```text
  Real Chrome v2 Macros page/Quick Send workflows passed.
  Real Chrome v2 Snapshots/import-export workflows passed.
  Real Chrome v2 Settings/Diagnostics workflows passed.
  ```

- `./scripts/check-webapp.sh` — **2 full runs, both exit 0** (`npm ci` →
  `format:check` → `typecheck` → `lint` → `stylelint` → `test` →
  `test:coverage` → `build` → `test:browser` →
  `verify-no-remote-assets.sh`), each including one more `test:browser`
  pass (12 total green `test:browser` runs across this session).
- `npm run test` (Vitest) — **577/577 passed**, 59 test files, unaffected by
  this change (it exercises none of `tests/browser/`).
- `npm run typecheck` — clean.
- `npm run lint` (`--max-warnings=0`) — clean.
- `npm run format:check` — clean (after one `format:write` pass reformatted
  the new file to this repo's Prettier config; re-verified with a second
  `format:check` and a full re-run of the browser suite afterward).
- `npm run stylelint` — clean (untouched by this change; ran as part of the
  `check-webapp.sh` chain).
- `git diff --stat` for this commit: `.github/workflows/browser-tests.yml`
  (+4), `docs/TODO_V2.md` (+3/-2, the one CDP-specific evidence line),
  `webapp/package-lock.json` (+48, the pinned `playwright`/`playwright-core`
  entries), `webapp/package.json` (+1), and
  `webapp/tests/browser/run-browser-tests.mjs` (net -186 lines: the
  hand-rolled CDP transport and its dialog/download/emulation workarounds
  are gone; the fixture server and scenario logic are unchanged).
- `actionlint .github/workflows/browser-tests.yml` — clean.

Not run in this session (out of scope / requires hardware or a from-scratch
environment this session doesn't have): firmware build, host tests, a
from-cold-cache CI run of `browser-tests.yml` itself (GitHub Actions is
where `--with-deps`'s apt install path is actually exercised; this sandbox
already had Chromium cached and no interactive sudo, so that specific path
was reasoned about, not executed, here).

## Files touched

- `webapp/tests/browser/run-browser-tests.mjs` — the migration itself.
- `webapp/package.json` / `webapp/package-lock.json` — `playwright@1.62.1`,
  pinned exact, `devDependencies`.
- `.github/workflows/browser-tests.yml` — added the
  `npx playwright install --with-deps chromium` step.
- `docs/TODO_V2.md` — corrected the one V2-115 evidence line that named the
  old CDP mechanism (`DOM.setFileInputFiles`) specifically; no checkbox
  state changed.
- `CLAUDE.md` — not edited: its test-location table already read "Browser
  (Playwright)", which was an inaccurate label before this commit and is an
  accurate one now, with no wording change needed.
