import { packageId } from "../fixtures/data.mjs";
import { startStartupFixtureServer } from "../fixtures/startupFixtureServer.mjs";
import { assert } from "../lib/http.mjs";
import {
  clickButton,
  clickButtonByAriaLabel,
  evaluate,
  waitFor,
} from "../lib/page.mjs";

export async function runMacroEditingWorkflows(browser) {
  const fixture = await startStartupFixtureServer();
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
  });
  const page = await context.newPage();
  // Mirrors the dialog auto-accept registered on the shared context in
  // main(): package rename/duplicate/delete and macro add/edit all dirty
  // the working copy below, and this scenario deliberately reloads once
  // while dirty to prove that native dialog doesn't hang the page (the
  // exact defect V2_100_103_MACRO_EDITING_PACKAGE_MANAGEMENT_2026-08-09.md
  // found and fixed in this harness).
  page.on("dialog", (dialog) => {
    dialog.accept().catch(() => {
      // Best-effort: if the page is already navigating away, ignore it.
    });
  });
  try {
    await page.goto(fixture.baseUrl);
    await waitFor(
      page,
      () => document.body?.innerText.includes("Lab bench workflow") ?? false,
      "Macro-editing fixture did not reach the Macros page.",
    );

    // V2-100/101: Add macro -- name/timing fields, directive insertion (a
    // real focused-textarea keyboard/selection interaction jsdom cannot
    // exercise), live validation, and Save landing back on a dirtied Macros
    // page listing the new macro.
    await clickButton(page, "Add macro");
    await waitFor(
      page,
      () => document.querySelector("#macro-editor-name") !== null,
      "Add macro did not open the macro editor.",
    );
    await page.locator("#macro-editor-name").fill("Open the run dialog");
    await page.locator("#macro-editor-source").fill("r");
    await page.getByRole("button", { name: "ENTER", exact: true }).click();
    await waitFor(
      page,
      () => document.body.innerText.includes("Macro is valid."),
      "The new macro's source did not validate after inserting a directive.",
    );
    const sourceAfterInsert = await evaluate(page, () => {
      const source = document.querySelector("#macro-editor-source");
      return source instanceof HTMLTextAreaElement ? source.value : null;
    });
    assert(
      sourceAfterInsert === "r{ENTER}",
      `Inserting the ENTER directive did not append it to the macro source: ${String(sourceAfterInsert)}`,
    );
    await clickButton(page, "Advanced");
    await waitFor(
      page,
      () => document.querySelector("#macro-editor-key-press") !== null,
      "Advanced did not open the timing dialog.",
    );
    await page.locator("#macro-editor-key-press").fill("10");
    await page.locator("#macro-editor-inter-key").fill("20");
    await clickButton(page, "Done");
    await clickButton(page, "Save changes");
    await waitFor(
      page,
      () =>
        document.body.innerText.includes("Open the run dialog") &&
        document.body.innerText.includes("Unsaved changes"),
      "Saving the new macro did not return to a dirtied Macros page listing it.",
    );

    // V2-100: an invalid source shows its exact error location, and "Go to
    // error" moves real textarea focus/selection to it -- then correcting
    // and saving succeeds.
    await clickButtonByAriaLabel(page, "Edit Open the run dialog");
    await waitFor(
      page,
      () => document.querySelector("#macro-editor-source") !== null,
      "Edit did not reopen the macro editor.",
    );
    await page.locator("#macro-editor-source").fill("r{BADTOKEN}");
    await waitFor(
      page,
      () => document.body.innerText.includes("Line 1"),
      "An invalid macro source did not show its exact error location.",
    );
    await clickButton(page, "Go to error");
    const focusedSourceAfterGoToError = await evaluate(
      page,
      () =>
        document.activeElement ===
        document.querySelector("#macro-editor-source"),
    );
    assert(
      focusedSourceAfterGoToError === true,
      "Go to error did not focus the source textarea.",
    );
    await page.locator("#macro-editor-source").fill("r{ENTER}");
    await waitFor(
      page,
      () => document.body.innerText.includes("Macro is valid."),
      "The corrected macro source did not validate.",
    );
    await clickButton(page, "Save changes");
    await waitFor(
      page,
      () =>
        document.body.innerText.includes("Open the run dialog") &&
        !document.body.innerText.includes("Line 1"),
      "Saving the corrected macro did not return to the Macros page.",
    );

    // V2-100: Cancel discards the draft entirely.
    await clickButton(page, "Add macro");
    await waitFor(
      page,
      () => document.querySelector("#macro-editor-name") !== null,
      "Add macro (second) did not open the macro editor.",
    );
    await page.locator("#macro-editor-name").fill("Should not be saved");
    await clickButton(page, "Cancel");
    await waitFor(
      page,
      () => document.body.innerText.includes("Add macro"),
      "Cancel did not return to the Macros page.",
    );
    assert(
      !(await evaluate(page, () =>
        document.body.innerText.includes("Should not be saved"),
      )),
      "Cancel saved a macro it should have discarded.",
    );

    // V2-102: Package management -- create, rename, duplicate, reorder,
    // delete (name-bearing two-step confirm), and Open.
    await clickButton(page, "Packages");
    await waitFor(
      page,
      () => document.querySelector("#package-management-create-name") !== null,
      "The Packages page did not render.",
    );
    await page
      .locator("#package-management-create-name")
      .fill("Second bench package");
    await clickButton(page, "Create package");
    await waitFor(
      page,
      () => document.body.innerText.includes("Second bench package"),
      "Create package did not add a new package.",
    );

    await clickButtonByAriaLabel(page, "Rename Second bench package");
    await waitFor(
      page,
      () => document.querySelector('[id^="package-rename-"]') !== null,
      "Rename did not open its inline form.",
    );
    await page
      .locator('[id^="package-rename-"]')
      .fill("Second bench package (renamed)");
    await clickButton(page, "Save name");
    await waitFor(
      page,
      () => document.body.innerText.includes("Second bench package (renamed)"),
      "Renaming the package did not take effect.",
    );

    await clickButtonByAriaLabel(
      page,
      "Duplicate Second bench package (renamed)",
    );
    await waitFor(
      page,
      () =>
        document.body.innerText.includes("Second bench package (renamed) copy"),
      "Duplicate did not create a copy.",
    );

    await page
      .locator('[aria-label="Move Second bench package (renamed) copy up"]')
      .press("Enter");
    assert(
      await evaluate(page, () =>
        document.body.innerText.includes("Second bench package (renamed) copy"),
      ),
      "Reordering the duplicated package lost it from the list.",
    );

    await clickButtonByAriaLabel(
      page,
      "Delete Second bench package (renamed) copy",
    );
    await waitFor(
      page,
      () => document.querySelector('[role="alertdialog"]') !== null,
      "Delete did not show its name-bearing confirmation.",
    );
    await clickButton(page, "Confirm delete");
    await waitFor(
      page,
      () =>
        !document.body.innerText.includes(
          "Second bench package (renamed) copy",
        ),
      "Confirmed delete did not remove the duplicated package.",
    );

    await clickButtonByAriaLabel(page, "Open Second bench package (renamed)");
    await waitFor(
      page,
      () =>
        document.body.innerText.includes("Second bench package (renamed)") &&
        document.body.innerText.includes("Add macro"),
      "Open did not switch to and open the selected package's Macros page.",
    );

    // V2-103: a real, unstyleable native `beforeunload` dialog fires while
    // the working copy is dirty (every step above left it dirty); this
    // page's dialog handler (registered above) must not let the reload
    // hang.
    await page.reload();
    await waitFor(
      page,
      () => document.body?.innerText.includes("Lab bench workflow") ?? false,
      "Reloading a dirty working copy hung instead of completing (the beforeunload dialog was not handled).",
      15_000,
    );
  } finally {
    await context.close();
    await fixture.close();
  }
}

export const importFixtureRepository = {
  format: "esp32-macro-keyboard-repository",
  schemaVersion: 1,
  packages: [
    {
      // Deliberately the same package ID the fixture device already has
      // selected, so importing resolves straight to the Macros page
      // (UI_UX_SPEC_V2 §3.6) instead of the Package chooser — this test
      // targets import replacement, not package resolution.
      id: packageId,
      name: "Imported bench",
      macros: [
        {
          id: "88888888-8888-4888-8888-888888888888",
          name: "Imported macro",
          source: "b",
          keyPressMs: 8,
          interKeyMs: 15,
        },
      ],
    },
  ],
};

/*
 * TODO_V2 V2-133/UI_UX_SPEC_V2 §14 "Accessibility requirements ... pass
 * automated ... checks". Added as its own standalone block (not folded into
 * `runBrowserWorkflows`/`runSettingsWorkflows` above) so it stays a narrow,
 * independently reviewable addition rather than restructuring the existing
 * workflow functions. Real axe-core (`@axe-core/playwright`), injected into
 * and run inside the real Chrome page this harness already drives — this is
 * genuine automated a11y scanning, not a hand-rolled substitute for it.
 *
 * Scoped to `serious`/`critical` impact rather than every finding axe-core
 * can report. As of this writing (2026-08-09) a full unfiltered scan of all
 * three pages below finds zero violations at any impact level, so this
 * filter currently excludes nothing — but it is deliberate policy, not
 * incidental: `moderate`/`minor` findings on a real app are disproportionately
 * color-contrast warnings against whatever palette is in place, which is a
 * visual-design concern this phase does not own (no dedicated design pass
 * has happened). Gating the build on those later, once any exist, would
 * block unrelated work. `serious`/`critical` cover the structural defects
 * this task is actually about: missing accessible names, broken focus
 * order, invalid ARIA usage, and similar.
 */
