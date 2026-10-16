import {
  STANDARD_VIEWPORTS,
  captureScenario,
  clickButton,
  completeSetupSteps,
  gzipSync,
  moveMacroViaMenu,
  repositoryGzip,
  scenario,
  newPage,
  startH4RecoveryFixture,
  waitFor,
  withStartup,
} from "./scenarioSupport.mjs";

export const SCENARIOS = [
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
      "Set up your keyboard",
      (page) => captureScenario(page),
    ),
  ),
  scenario("first-run-review", STANDARD_VIEWPORTS, (browser, viewport) =>
    withStartup(
      browser,
      viewport,
      { provisioned: false },
      "Set up your keyboard",
      async (page) => {
        await completeSetupSteps(page, {
          setupCode: "12345678",
          deviceName: "Bench device",
          apSsid: "bench-ap",
          apPassphrase: "bench-fixture-passphrase",
          adminPassword: "bench-fixture-admin-1",
        });
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
    withStartup(
      browser,
      viewport,
      { blobs: [] },
      "Create Your First Repository",
      (page) => captureScenario(page),
    ),
  ),
  scenario(
    "startup-package-chooser",
    STANDARD_VIEWPORTS,
    (browser, viewport) => {
      const chooserRepository = {
        format: "esp32-macro-keyboard-repository",
        schemaVersion: 1,
        packages: [
          {
            id: "11111111-1111-4111-8111-111111111111",
            name: "Writing",
            macros: [],
          },
          {
            id: "22222222-2222-4222-8222-222222222222",
            name: "Build tasks",
            macros: [],
          },
        ],
      };
      return withStartup(
        browser,
        viewport,
        {
          lastSelectedPackageId: null,
          blobs: [
            {
              id: "1",
              bytes: gzipSync(
                Buffer.from(JSON.stringify(chooserRepository), "utf8"),
              ),
            },
          ],
        },
        "Choose a package",
        (page) => captureScenario(page),
      );
    },
  ),
  scenario("startup-settings-retry", STANDARD_VIEWPORTS, (browser, viewport) =>
    withStartup(
      browser,
      viewport,
      { settingsFailures: 10 },
      "Couldn't load device settings",
      (page) => captureScenario(page),
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
        await moveMacroViaMenu(page, "Recovery macro", "down");
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
