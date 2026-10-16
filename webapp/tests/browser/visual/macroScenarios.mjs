import {
  STANDARD_VIEWPORTS,
  captureScenario,
  clickButtonByAriaLabel,
  moveMacroViaMenu,
  scenario,
  waitFor,
  withApp,
} from "./scenarioSupport.mjs";

export const SCENARIOS = [
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
      await moveMacroViaMenu(page, "Open terminal", "down");
      await waitFor(
        page,
        () => document.body.innerText.includes("Unsaved changes"),
        "Reordering did not dirty the working copy.",
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
];
