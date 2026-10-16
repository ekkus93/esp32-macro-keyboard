import { gzipSync } from "node:zlib";
import { mkdtemp, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";

import { startApplicationServer } from "../fixtures/applicationServer.mjs";
import { startStartupFixtureServer } from "../fixtures/startupFixtureServer.mjs";
import { startH4RecoveryFixture } from "../fixtures/h4RecoveryFixture.mjs";
import { repositoryGzip } from "../fixtures/data.mjs";
import { importFixtureRepository } from "../workflows/macroEditing.mjs";
import {
  chooseMenuItem,
  openDiagnostics,
  openSettingsSection,
  clickButton,
  clickButtonByAriaLabel,
  waitFor,
  moveMacroViaMenu,
} from "../lib/page.mjs";
import { completeSetupSteps } from "../workflows/startup.mjs";
import { captureScenario } from "./capture.mjs";

export {
  gzipSync,
  repositoryGzip,
  chooseMenuItem,
  openDiagnostics,
  openSettingsSection,
  clickButton,
  clickButtonByAriaLabel,
  waitFor,
  moveMacroViaMenu,
  completeSetupSteps,
  captureScenario,
  startApplicationServer,
  startH4RecoveryFixture,
};

/**
 * The two viewports every scenario runs at, per
 * WEBAPP_TAILWIND_SPEC_2026-08-18.md §10.3, unless a scenario overrides
 * `viewports` for a state that only exists at one size (the landscape
 * overlay).
 */
export const STANDARD_VIEWPORTS = [
  { name: "390x844", width: 390, height: 844 },
  { name: "1280x900", width: 1280, height: 900 },
];

let reviewColorScheme = "light";

export function setReviewColorScheme(colorScheme) {
  if (colorScheme !== "light" && colorScheme !== "dark") {
    throw new Error(`Unsupported review colour scheme: ${colorScheme}`);
  }
  reviewColorScheme = colorScheme;
}

/**
 * Boundary scenarios (T1-5, `*-boundary` names below) each define their own
 * three-viewport array -- the exact SPEC §5.3 threshold, one pixel below,
 * one pixel above -- rather than sharing one constant. Kept separate from
 * STANDARD_VIEWPORTS because most scenarios gain nothing from running at
 * six widths instead of two; only the element that actually carries a given
 * threshold needs its boundary swept, and every SPEC §5.3 threshold still in
 * use has exactly one boundary scenario covering it: 26rem
 * (`snapshots-storage-summary-boundary`), 32rem (`macros-32rem-boundary`),
 * 34rem (`macros-34rem-boundary`), 40rem (`dialog-restart-heading-boundary`),
 * 42rem (`macro-editor-42rem-boundary`), 60rem (`macros-60rem-boundary` for
 * the shell, `signin-60rem-boundary` for `StandaloneScreen`). The one
 * height-based threshold, 38rem (`macro-editor-short-viewport-boundary`),
 * was retired 2026-08-23 along with the editor's `short:` fallback -- the
 * macro editor no longer has a second mode to switch into at any height, so
 * there is no boundary left to sweep.
 */

let importFixturePathPromise = null;
export async function importFixturePath() {
  importFixturePathPromise ??= (async () => {
    const dir = await mkdtemp(join(tmpdir(), "visual-import-"));
    const path = join(dir, "import.gz");
    await writeFile(
      path,
      gzipSync(Buffer.from(JSON.stringify(importFixtureRepository), "utf8")),
    );
    return path;
  })();
  return importFixturePathPromise;
}

export async function newPage(browser, viewport, extra = {}) {
  const context = await browser.newContext({
    viewport: { width: viewport.width, height: viewport.height },
    colorScheme: reviewColorScheme,
    ...extra,
  });
  const page = await context.newPage();
  page.on("dialog", (dialog) => {
    dialog.accept().catch(() => {
      // Best-effort: ignore if the page is already navigating away.
    });
  });
  return { context, page };
}

/**
 * One scenario: a name, the viewports to run it at, and a `run(browser)`
 * function that drives the app to the target state, captures it, and
 * cleans up after itself. Independent of every other scenario -- each opens
 * its own fixture server and browser context -- so one scenario's failure
 * never corrupts another's result, and scenarios can be run selectively.
 *
 * `run` must throw (not resolve with a null capture) if it cannot reach the
 * target state; run-visual-tests.mjs treats a thrown scenario as a failed
 * run, never a silently skipped one (T1-1's "fail loudly" requirement).
 */
export function scenario(name, viewports, run) {
  return { name, viewports, run };
}

/** The application fixture (an already-provisioned, signed-in device). */
export async function withApp(browser, viewport, fn, contextOptions = {}) {
  const application = await startApplicationServer();
  const { context, page } = await newPage(browser, viewport, contextOptions);
  try {
    await page.goto(application.baseUrl);
    await waitFor(
      page,
      () => document.body?.innerText.includes("Lab bench workflow") ?? false,
      "The Macros page did not render.",
    );
    return await fn(page, application);
  } finally {
    await context.close();
    await application.close();
  }
}

/** The startup fixture (pre-authentication / pre-provisioning states). */
export async function withStartup(browser, viewport, options, marker, fn) {
  const fixture = await startStartupFixtureServer(options);
  const { context, page } = await newPage(browser, viewport);
  try {
    await page.goto(fixture.baseUrl);
    await waitFor(
      page,
      new Function(
        `return (document.body?.innerText ?? "").includes(${JSON.stringify(marker)})`,
      ),
      `The startup fixture did not reach the "${marker}" state.`,
    );
    return await fn(page, fixture);
  } finally {
    await context.close();
    await fixture.close();
  }
}
