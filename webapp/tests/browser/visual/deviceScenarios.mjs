import {
  STANDARD_VIEWPORTS,
  captureScenario,
  clickButton,
  clickButtonByAriaLabel,
  openDiagnostics,
  openSettingsSection,
  scenario,
  newPage,
  startApplicationServer,
  waitFor,
  withApp,
  withStartup,
} from "./scenarioSupport.mjs";

export const SCENARIOS = [
  scenario("settings", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Settings");
      await waitFor(
        page,
        () => document.body.innerText.includes("USB identity and confirmation"),
        "The Settings list did not render.",
      );
      return captureScenario(page);
    }),
  ),
  scenario("dialog-restart", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await openSettingsSection(page, "reset", "Restart");
      await clickButton(page, "Restart");
      await waitFor(
        page,
        () => document.body.innerText.includes("Restart the device?"),
        "The restart dialog did not open.",
      );
      return captureScenario(page);
    }),
  ),
  // --- DeviceReconnectScreen (T2-4) -----------------------------------------
  // The one StandaloneScreen call site the migration never visually
  // verified -- neither fixture implemented `POST /api/v1/device/restart`
  // until this task added it to startStartupFixtureServer, which already
  // supports the fully-authenticated app state (it shares its repository
  // fixture data with startApplicationServer -- both reach "Lab bench
  // workflow") and so needs no separate fixture module.
  scenario("reconnect-waiting", STANDARD_VIEWPORTS, (browser, viewport) =>
    withStartup(browser, viewport, {}, "Lab bench workflow", async (page) => {
      await openSettingsSection(page, "reset", "Restart");
      await clickButton(page, "Restart");
      await waitFor(
        page,
        () => document.body.innerText.includes("Restart the device?"),
        "The restart dialog did not open.",
      );
      await clickButton(page, "Restart now");
      await waitFor(
        page,
        () => document.body.innerText.includes("Reconnecting"),
        "The reconnect screen did not render.",
      );
      return captureScenario(page);
    }),
  ),
  // No `reconnect-needs-reauth` scenario: AppV2.tsx's own comment confirms
  // why. A 401 on the reconnect poll ALSO fires the shared
  // `subscribeUnauthorized` mechanism, which drops the top-level app to
  // Sign In directly -- "this shell unmounts on its own once that happens".
  // `DeviceReconnectScreen`'s `phase === "needs-reauth"` render branch loses
  // that race in every case this fixture can drive (confirmed: the fixture
  // reaches Sign In, never the reconnect screen's own "needs-reauth" copy).
  // Whether that branch is reachable by *any* real sequence, or is
  // effectively dead code, is a question for the app itself, not something
  // a visual scenario should paper over by racing timings until one passes.
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
        await openSettingsSection(page, "reset", "Restart");
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
        await openSettingsSection(page, "reset", "Reset settings");
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
      await openDiagnostics(page);

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
];
