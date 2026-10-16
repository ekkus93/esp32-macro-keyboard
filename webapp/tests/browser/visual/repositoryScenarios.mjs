import {
  STANDARD_VIEWPORTS,
  captureScenario,
  chooseMenuItem,
  clickButton,
  clickButtonByAriaLabel,
  importFixturePath,
  moveMacroViaMenu,
  scenario,
  waitFor,
  withApp,
} from "./scenarioSupport.mjs";

export const SCENARIOS = [
  scenario("packages", STANDARD_VIEWPORTS, (browser, viewport) =>
    withApp(browser, viewport, async (page) => {
      await clickButton(page, "Packages");
      await waitFor(
        page,
        () => document.body.innerText.includes("New package"),
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
          () => document.body.innerText.includes("New package"),
          "The Packages page did not render.",
        );
        await clickButton(page, "New package");
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
        () => document.body.innerText.includes("New package"),
        "The Packages page did not render.",
      );
      await chooseMenuItem(
        page,
        "Lab bench workflow",
        "Rename Lab bench workflow",
      );
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
        () => document.body.innerText.includes("New package"),
        "The Packages page did not render.",
      );
      await chooseMenuItem(
        page,
        "Lab bench workflow",
        "Delete Lab bench workflow",
      );
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
        () => document.body.innerText.toUpperCase().includes("STORED"),
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
          () => document.body.innerText.toUpperCase().includes("STORED"),
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
        () => document.body.innerText.toUpperCase().includes("STORED"),
        "The Snapshots page did not render.",
      );
      await chooseMenuItem(
        page,
        "snapshot 1",
        "Replace snapshot 1 with current working copy",
      );
      await waitFor(
        page,
        () => document.body.innerText.includes("Replace snapshot?"),
        "The replace confirmation did not open.",
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
          () => document.body.innerText.toUpperCase().includes("STORED"),
          "The Snapshots page did not render.",
        );
        await chooseMenuItem(page, "snapshot 1", "Delete snapshot 1");
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
        () => document.body.innerText.toUpperCase().includes("STORED"),
        "The Snapshots page did not render.",
      );
      await clickButton(page, "Import or export");
      await waitFor(
        page,
        () => document.body.innerText.includes("Export working copy"),
        "The Import and export screen did not open.",
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
      await moveMacroViaMenu(page, "Open terminal", "down");
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
];
