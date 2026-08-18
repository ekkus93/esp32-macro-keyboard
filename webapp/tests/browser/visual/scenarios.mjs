import { gzipSync } from "node:zlib";
import { mkdtemp, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";

import { startApplicationServer } from "../fixtures/applicationServer.mjs";
import { startStartupFixtureServer } from "../fixtures/startupFixtureServer.mjs";
import { startH4RecoveryFixture } from "../fixtures/h4RecoveryFixture.mjs";
import { repositoryGzip } from "../fixtures/data.mjs";
import { importFixtureRepository } from "../workflows/macroEditing.mjs";
import { clickButton, clickButtonByAriaLabel, waitFor } from "../lib/page.mjs";
import { captureScenario } from "./capture.mjs";

/**
 * The two viewports every scenario runs at, per
 * WEBAPP_TAILWIND_SPEC_2026-08-18.md §10.3, unless a scenario overrides
 * `viewports` for a state that only exists at one size (the landscape
 * overlay, the short-viewport editor fallback).
 */
export const STANDARD_VIEWPORTS = [
  { name: "390x844", width: 390, height: 844 },
  { name: "1280x900", width: 1280, height: 900 },
];

/**
 * Boundary scenarios (T1-5, `*-boundary` names below) each define their own
 * three-viewport array -- the exact SPEC §5.3 threshold, one pixel below,
 * one pixel above -- rather than sharing one constant. Kept separate from
 * STANDARD_VIEWPORTS because most scenarios gain nothing from running at
 * six widths instead of two; only the element that actually carries a given
 * threshold needs its boundary swept, and every SPEC §5.3 threshold has
 * exactly one boundary scenario covering it: 26rem
 * (`snapshots-storage-summary-boundary`), 32rem (`macros-32rem-boundary`),
 * 34rem (`macros-34rem-boundary`), 40rem (`dialog-restart-heading-boundary`),
 * 42rem (`macro-editor-42rem-boundary`), 60rem (`macros-60rem-boundary` for
 * the shell, `signin-60rem-boundary` for `StandaloneScreen`), and the one
 * height-based threshold, 38rem
 * (`macro-editor-short-viewport-boundary`).
 */

let importFixturePathPromise = null;
async function importFixturePath() {
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

async function newPage(browser, viewport, extra = {}) {
  const context = await browser.newContext({
    viewport: { width: viewport.width, height: viewport.height },
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
function scenario(name, viewports, run) {
  return { name, viewports, run };
}

/** The application fixture (an already-provisioned, signed-in device). */
async function withApp(browser, viewport, fn, contextOptions = {}) {
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
async function withStartup(browser, viewport, options, marker, fn) {
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

export const SCENARIOS = [
  // --- The ordinary authenticated application -----------------------------
  scenario("macros", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, (page) => captureScenario(page)),
  ),
  // Two of SPEC §5.3's thresholds land on this one page at once: 32rem
  // (<=, HeaderActions' narrow-screen justify-start, and each macro row's
  // action cluster going full-width) and 34rem (>=, Card's two-column
  // grid -- every macro row is a Card with two children).
  scenario(
    "macros-32rem-boundary",
    [
      { name: "511x900", width: 511, height: 900 }, // 32rem - 1px
      { name: "512x900", width: 512, height: 900 }, // 32rem
      { name: "513x900", width: 513, height: 900 }, // 32rem + 1px
    ],
    (browser, viewport) =>
      withApp(browser, viewport, (page) => captureScenario(page)),
  ),
  scenario(
    "macros-34rem-boundary",
    [
      { name: "543x900", width: 543, height: 900 }, // 34rem - 1px
      { name: "544x900", width: 544, height: 900 }, // 34rem
      { name: "545x900", width: 545, height: 900 }, // 34rem + 1px
    ],
    (browser, viewport) =>
      withApp(browser, viewport, (page) => captureScenario(page)),
  ),
  // 60rem (>=): the shell-chrome half of this pair -- see
  // signin-60rem-boundary for StandaloneScreen's identical formula.
  scenario(
    "macros-60rem-boundary",
    [
      { name: "959x900", width: 959, height: 900 }, // 60rem - 1px
      { name: "960x900", width: 960, height: 900 }, // 60rem
      { name: "961x900", width: 961, height: 900 }, // 60rem + 1px
    ],
    (browser, viewport) =>
      withApp(browser, viewport, (page) => captureScenario(page)),
  ),
  scenario("macros-overflow-menu", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButtonByAriaLabel(page, "More actions for Open terminal");
      await waitFor(
        page,
        () =>
          document.querySelector('[aria-label="Actions for Open terminal"]') !==
          null,
        "The overflow menu did not open.",
      );
      return captureScenario(page);
    }),
  ),
  scenario(
    "macros-overflow-delete-confirm",
    STANDARD_VIEWPORTS,
    (browser, viewport) =>
      withApp(browser, viewport, async (page) => {
        await clickButtonByAriaLabel(page, "More actions for Open terminal");
        await waitFor(
          page,
          () =>
            document.querySelector(
              '[aria-label="Actions for Open terminal"]',
            ) !== null,
          "The overflow menu did not open.",
        );
        await clickButtonByAriaLabel(page, "Delete Open terminal");
        await waitFor(
          page,
          () => document.body.innerText.includes("This cannot be undone once"),
          "The overflow delete confirmation did not open.",
        );
        return captureScenario(page);
      }),
  ),
  scenario("macros-dirty", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await page
        .locator('[aria-label="Move Open terminal down"]')
        .press("Enter");
      await waitFor(
        page,
        () => document.body.innerText.includes("Unsaved changes"),
        "Reordering did not dirty the working copy.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("macro-preview", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButtonByAriaLabel(page, "More actions for Open terminal");
      await waitFor(
        page,
        () =>
          document.querySelector('[aria-label="Actions for Open terminal"]') !==
          null,
        "The overflow menu did not open.",
      );
      await clickButtonByAriaLabel(page, "Preview and send Open terminal");
      await waitFor(
        page,
        () => document.body.innerText.includes("Preview"),
        "The macro preview page did not render.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("macro-editor", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButtonByAriaLabel(page, "Edit Open terminal");
      await waitFor(
        page,
        () => document.querySelector("#macro-editor-name") !== null,
        "The macro editor did not open.",
      );
      return captureScenario(page);
    }),
  ),
  // The editor's short-viewport fallback (`short:` variant, SPEC §5.3) --
  // Its own viewport set, not the standard pair: the split it toggles is
  // driven by viewport *height*, not width, and this is the 38rem (<=)
  // height boundary itself (SPEC §5.3's one height-based threshold),
  // 607/608/609px -- 390px wide throughout since width is not what varies.
  scenario(
    "macro-editor-short-viewport-boundary",
    [
      { name: "390x607", width: 390, height: 607 }, // 38rem - 1px
      { name: "390x608", width: 390, height: 608 }, // 38rem
      { name: "390x609", width: 390, height: 609 }, // 38rem + 1px
    ],
    (browser, viewport) =>
      withApp(browser, viewport, async (page) => {
        await clickButtonByAriaLabel(page, "Edit Open terminal");
        await waitFor(
          page,
          () => document.querySelector("#macro-editor-name") !== null,
          "The macro editor did not open.",
        );
        return captureScenario(page);
      }),
  ),
  // 42rem (>=): the editor's toolbar/timing controls go two-column.
  scenario(
    "macro-editor-42rem-boundary",
    [
      { name: "671x900", width: 671, height: 900 }, // 42rem - 1px
      { name: "672x900", width: 672, height: 900 }, // 42rem
      { name: "673x900", width: 673, height: 900 }, // 42rem + 1px
    ],
    (browser, viewport) =>
      withApp(browser, viewport, async (page) => {
        await clickButtonByAriaLabel(page, "Edit Open terminal");
        await waitFor(
          page,
          () => document.querySelector("#macro-editor-name") !== null,
          "The macro editor did not open.",
        );
        return captureScenario(page);
      }),
  ),
  scenario(
    "macro-editor-source-over-limit",
    STANDARD_VIEWPORTS,
    (browser, viewport) =>
      withApp(browser, viewport, async (page) => {
        await clickButtonByAriaLabel(page, "Edit Open terminal");
        await waitFor(
          page,
          () => document.querySelector("#macro-editor-name") !== null,
          "The macro editor did not open.",
        );
        await page.locator("#macro-editor-source").fill("a".repeat(4097));
        await waitFor(
          page,
          () => document.body.innerText.includes("4097"),
          "The editor did not go over its byte limit.",
        );
        return captureScenario(page);
      }),
  ),
  scenario("packages", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Packages");
      await waitFor(
        page,
        () => document.body.innerText.includes("Create package"),
        "The Packages page did not render.",
      );
      return captureScenario(page);
    }),
  ),
  scenario(
    "packages-name-over-limit",
    STANDARD_VIEWPORTS,
    (browser, viewport) =>
      withApp(browser, viewport, async (page) => {
        await clickButton(page, "Packages");
        await waitFor(
          page,
          () => document.body.innerText.includes("Create package"),
          "The Packages page did not render.",
        );
        await page
          .locator("#package-management-create-name")
          .fill("x".repeat(65));
        await waitFor(
          page,
          () => document.body.innerText.includes("65 / 64"),
          "The package name did not exceed its limit.",
        );
        return captureScenario(page);
      }),
  ),
  scenario("packages-rename", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Packages");
      await waitFor(
        page,
        () => document.body.innerText.includes("Create package"),
        "The Packages page did not render.",
      );
      await clickButtonByAriaLabel(page, "Rename Lab bench workflow");
      await waitFor(
        page,
        () => document.querySelector("form input") !== null,
        "The rename form did not open.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("packages-delete-confirm", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Packages");
      await waitFor(
        page,
        () => document.body.innerText.includes("Create package"),
        "The Packages page did not render.",
      );
      await clickButtonByAriaLabel(page, "Delete Lab bench workflow");
      await waitFor(
        page,
        () => document.body.innerText.includes("and all"),
        "The package delete confirmation did not open.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("snapshots", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Snapshots");
      await waitFor(
        page,
        () => document.body.innerText.includes("Save current snapshot"),
        "The Snapshots page did not render.",
      );
      return captureScenario(page);
    }),
  ),
  // storage-summary's own two breakpoints (26rem, both directions) --
  // T1-5's boundary set for this element.
  scenario(
    "snapshots-storage-summary-boundary",
    [
      { name: "415x900", width: 415, height: 900 }, // 26rem - 1px
      { name: "416x900", width: 416, height: 900 }, // 26rem
      { name: "417x900", width: 417, height: 900 }, // 26rem + 1px
    ],
    (browser, viewport) =>
      withApp(browser, viewport, async (page) => {
        await clickButton(page, "Snapshots");
        await waitFor(
          page,
          () => document.body.innerText.includes("Save current snapshot"),
          "The Snapshots page did not render.",
        );
        return captureScenario(page);
      }),
  ),
  scenario("snapshots-advanced", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Snapshots");
      await waitFor(
        page,
        () => document.body.innerText.includes("Save current snapshot"),
        "The Snapshots page did not render.",
      );
      await clickButtonByAriaLabel(
        page,
        "Show advanced options for snapshot 1",
      );
      await waitFor(
        page,
        () => document.body.innerText.includes("Replace"),
        "The Advanced panel did not open.",
      );
      return captureScenario(page);
    }),
  ),
  scenario(
    "snapshots-delete-confirm",
    STANDARD_VIEWPORTS,
    (browser, viewport) =>
      withApp(browser, viewport, async (page) => {
        await clickButton(page, "Snapshots");
        await waitFor(
          page,
          () => document.body.innerText.includes("Save current snapshot"),
          "The Snapshots page did not render.",
        );
        await clickButtonByAriaLabel(page, "Delete snapshot 1");
        await waitFor(
          page,
          () => document.body.innerText.includes("This permanently deletes"),
          "The snapshot delete confirmation did not open.",
        );
        return captureScenario(page);
      }),
  ),
  scenario("snapshots-import-ready", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Snapshots");
      await waitFor(
        page,
        () => document.body.innerText.includes("Save current snapshot"),
        "The Snapshots page did not render.",
      );
      const path = await importFixturePath();
      await page.setInputFiles('input[type="file"]', path);
      await waitFor(
        page,
        () => document.body.innerText.includes("packages and"),
        "The import confirmation did not open.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("unsaved-changes-prompt", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await page
        .locator('[aria-label="Move Open terminal down"]')
        .press("Enter");
      await waitFor(
        page,
        () => document.body.innerText.includes("Unsaved changes"),
        "Reordering did not dirty the working copy.",
      );
      await clickButton(page, "Snapshots");
      await waitFor(
        page,
        () => document.body.innerText.includes("Snapshot 1"),
        "The Snapshots page did not render.",
      );
      await clickButtonByAriaLabel(page, "Load snapshot 1");
      await waitFor(
        page,
        () => document.body.innerText.includes("You have unsaved changes"),
        "The unsaved-changes prompt did not open.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("settings", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Settings");
      await waitFor(
        page,
        () => document.querySelector("#settings-device-name") !== null,
        "The Settings page did not render.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("dialog-restart", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Settings");
      await waitFor(
        page,
        () => document.querySelector("#settings-device-name") !== null,
        "The Settings page did not render.",
      );
      await clickButton(page, "Restart");
      await waitFor(
        page,
        () => document.body.innerText.includes("Restart the device?"),
        "The restart dialog did not open.",
      );
      return captureScenario(page);
    }),
  ),
  // The dialog heading's own breakpoint (40rem, `<=`) -- T1-5.
  scenario(
    "dialog-restart-heading-boundary",
    [
      { name: "639x900", width: 639, height: 900 }, // 40rem - 1px
      { name: "640x900", width: 640, height: 900 }, // 40rem
      { name: "641x900", width: 641, height: 900 }, // 40rem + 1px
    ],
    (browser, viewport) =>
      withApp(browser, viewport, async (page) => {
        await clickButton(page, "Settings");
        await waitFor(
          page,
          () => document.querySelector("#settings-device-name") !== null,
          "The Settings page did not render.",
        );
        await clickButton(page, "Restart");
        await waitFor(
          page,
          () => document.body.innerText.includes("Restart the device?"),
          "The restart dialog did not open.",
        );
        return captureScenario(page);
      }),
  ),
  scenario(
    "dialog-danger-reset-settings",
    STANDARD_VIEWPORTS,
    (browser, viewport) =>
      withApp(browser, viewport, async (page) => {
        await clickButton(page, "Settings");
        await waitFor(
          page,
          () => document.querySelector("#settings-device-name") !== null,
          "The Settings page did not render.",
        );
        await clickButton(page, "Reset settings");
        await waitFor(
          page,
          () => document.body.innerText.includes("to confirm"),
          "The reset-settings dialog did not open.",
        );
        return captureScenario(page);
      }),
  ),
  scenario("diagnostics", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Settings");
      await waitFor(
        page,
        () => document.querySelector("#settings-device-name") !== null,
        "The Settings page did not render.",
      );
      await clickButton(page, "View diagnostics");
      await waitFor(
        page,
        () => document.body.innerText.includes("browser-fixture-abc123"),
        "The Diagnostics page did not render.",
      );
      return captureScenario(page);
    }),
  ),

  // --- Send banners --------------------------------------------------------
  // Waits for the "active" lifecycle state (the Cancel button), not the
  // "Sending X..." text: that text is the "starting" state, which exists
  // only until the very first status poll resolves and is inherently racy
  // to capture. "active" holds for a full poll interval (sendClient.ts's
  // pollIntervalMs, 1000ms against this fixture) before advancing to
  // "completed", which is comfortably longer than a capture takes.
  scenario("send-in-flight", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButtonByAriaLabel(page, "Send Open terminal");
      await waitFor(
        page,
        () => document.body.innerText.includes("Cancel and release all keys"),
        "The active send state was not reached.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("send-complete", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButtonByAriaLabel(page, "Send Open terminal");
      await waitFor(
        page,
        () => document.body.innerText.includes("Sent Open terminal."),
        "The send did not complete.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("send-failure", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButtonByAriaLabel(page, "Send Trigger failure");
      await waitFor(
        page,
        () => document.body.innerText.includes("failed: simulated_failure"),
        "The failure banner did not appear.",
      );
      return captureScenario(page);
    }),
  ),

  // --- The landscape orientation surface -----------------------------------
  // `hasTouch: true` supplies `pointer: coarse`, one of the three conditions
  // in `landscapePhoneMediaQuery`. Its own viewport, not the standard pair.
  scenario(
    "landscape-block-idle",
    [{ name: "844x390", width: 844, height: 390 }],
    // Not withApp(): that helper waits for the Macros page's own marker
    // text first, which this state deliberately never shows -- the overlay
    // covers it from the very first render.
    async (browser, viewport) => {
      const application = await startApplicationServer();
      const { context, page } = await newPage(browser, viewport, {
        hasTouch: true,
      });
      try {
        await page.goto(application.baseUrl);
        await waitFor(
          page,
          () => document.body.innerText.includes("Rotate your phone"),
          "The landscape block did not render.",
        );
        return await captureScenario(page);
      } finally {
        await context.close();
        await application.close();
      }
    },
  ),
  scenario(
    "landscape-block-sending",
    [{ name: "844x390", width: 844, height: 390 }],
    async (browser, viewport) => {
      const context = await browser.newContext({
        viewport: { width: viewport.width, height: viewport.height },
        hasTouch: true,
      });
      const application = await startApplicationServer();
      const page = await context.newPage();
      try {
        await page.goto(application.baseUrl);
        await waitFor(
          page,
          () => document.body.innerText.includes("Rotate your phone"),
          "The landscape block did not render.",
        );
        // Rotate to portrait to interact (the overlay hides the shell), start
        // a confirmation-gated send, then rotate back.
        await page.setViewportSize({ width: 390, height: 844 });
        await waitFor(
          page,
          () => document.body.innerText.includes("Lab bench workflow"),
          "Rotating to portrait did not restore the Macros page.",
        );
        await clickButtonByAriaLabel(page, "Send Confirm before typing");
        await waitFor(
          page,
          () => document.body.innerText.includes("Waiting for confirmation"),
          "The confirmation-wait state was not reached.",
        );
        await page.setViewportSize({ width: 844, height: 390 });
        await waitFor(
          page,
          () => document.body.innerText.includes("Rotate your phone"),
          "The landscape block did not return.",
        );
        return await captureScenario(page);
      } finally {
        await context.close();
        await application.close();
      }
    },
  ),

  // --- Screens only reachable pre-authentication / pre-provisioning -------
  scenario("signin", STANDARD_VIEWPORTS, (browser, viewport) =>
    withStartup(
      browser,
      viewport,
      { authenticated: false },
      "Sign in",
      (page) => captureScenario(page),
    ),
  ),
  // 60rem (>=): StandaloneScreen's column widens from 48rem to 64rem cap.
  // AppShellV2 shares the identical formula (SPEC §5.3) -- see
  // macros-60rem-boundary below for the shell-chrome half of this pair.
  scenario(
    "signin-60rem-boundary",
    [
      { name: "959x900", width: 959, height: 900 }, // 60rem - 1px
      { name: "960x900", width: 960, height: 900 }, // 60rem
      { name: "961x900", width: 961, height: 900 }, // 60rem + 1px
    ],
    (browser, viewport) =>
      withStartup(
        browser,
        viewport,
        { authenticated: false },
        "Sign in",
        (page) => captureScenario(page),
      ),
  ),
  scenario("signin-error", STANDARD_VIEWPORTS, (browser, viewport) =>
    withStartup(
      browser,
      viewport,
      { authenticated: false },
      "Sign in",
      async (page) => {
        await page.locator('input[type="password"]').fill("wrong-password-x");
        await clickButton(page, "Sign in");
        await waitFor(
          page,
          () => document.querySelector('[role="alert"]') !== null,
          "The failed sign-in did not surface an error banner.",
        );
        return captureScenario(page);
      },
    ),
  ),
  scenario("first-run-setup", STANDARD_VIEWPORTS, (browser, viewport) =>
    withStartup(
      browser,
      viewport,
      { provisioned: false },
      "isolated setup mode",
      (page) => captureScenario(page),
    ),
  ),
  scenario("first-run-review", STANDARD_VIEWPORTS, (browser, viewport) =>
    withStartup(
      browser,
      viewport,
      { provisioned: false },
      "isolated setup mode",
      async (page) => {
        await page.locator("#setup-code").fill("12345678");
        await page.locator("#device-name").fill("Bench device");
        await page.locator("#ap-ssid").fill("bench-ap");
        await page.locator("#ap-passphrase").fill("bench-fixture-passphrase");
        await page.locator("#admin-password").fill("bench-fixture-admin-1");
        await clickButton(page, "Review setup");
        await waitFor(
          page,
          () => document.body.innerText.includes("Review setup"),
          "The review step did not render.",
        );
        return captureScenario(page);
      },
    ),
  ),
  scenario("startup-no-blobs", STANDARD_VIEWPORTS, (browser, viewport) =>
    withStartup(browser, viewport, { blobs: [] }, "Save snapshot", (page) =>
      captureScenario(page),
    ),
  ),
  scenario(
    "startup-snapshot-recovery",
    STANDARD_VIEWPORTS,
    (browser, viewport) =>
      withStartup(
        browser,
        viewport,
        {
          blobs: [
            { id: "1", bytes: repositoryGzip },
            { id: "2", bytes: Buffer.from([0, 1, 2, 3, 4]) },
          ],
        },
        "Snapshot recovery",
        (page) => captureScenario(page),
      ),
  ),
  scenario("startup-empty-package", STANDARD_VIEWPORTS, (browser, viewport) => {
    const emptyRepository = {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [
        {
          id: "11111111-1111-4111-8111-111111111111",
          name: "Empty bench package",
          macros: [],
        },
      ],
    };
    return withStartup(
      browser,
      viewport,
      {
        blobs: [
          {
            id: "1",
            bytes: gzipSync(
              Buffer.from(JSON.stringify(emptyRepository), "utf8"),
            ),
          },
        ],
      },
      "Empty bench package",
      (page) => captureScenario(page),
    );
  }),

  // --- USB badge states ------------------------------------------------
  ...["ready", "suspended", "error", "disconnected"].map((usbState) =>
    scenario(`usb-badge-${usbState}`, STANDARD_VIEWPORTS, (browser, viewport) =>
      withStartup(
        browser,
        viewport,
        { usbState },
        "Lab bench workflow",
        (page) => captureScenario(page),
      ),
    ),
  ),

  // --- Execution recovery overlay (SPEC §10.4) -----------------------------
  // Its own fixture (h4RecoveryFixture.mjs, shared with
  // run-h4-recovery-tests.mjs): three persistent status-poll failures after
  // an initial send is what exposes the degraded "Execution state
  // unavailable" state this overlay reports.
  scenario(
    "execution-recovery-overlay",
    STANDARD_VIEWPORTS,
    async (browser, viewport) => {
      const fixture = await startH4RecoveryFixture();
      const { context, page } = await newPage(browser, viewport);
      try {
        await page.goto(fixture.baseUrl);
        await waitFor(
          page,
          () =>
            document.body?.innerText.includes("H4 recovery package") ?? false,
          "The H4 fixture did not reach the Macros page.",
        );
        await page
          .locator('[aria-label="Move Recovery macro down"]')
          .press("Enter");
        await waitFor(
          page,
          () => document.body.innerText.includes("Unsaved changes"),
          "Reordering did not dirty the working copy.",
        );
        await page.getByRole("button", { name: "Send Recovery macro" }).click();
        await waitFor(
          page,
          () => document.body.innerText.includes("Execution state unavailable"),
          "Persistent status failures did not expose the degraded execution state.",
        );
        return await captureScenario(page);
      } finally {
        await context.close();
        await fixture.close();
      }
    },
  ),
];
