import { moveMacroViaMenu } from "./lib/page.mjs";
import { chromium } from "playwright";

import { startH4RecoveryFixture } from "./fixtures/h4RecoveryFixture.mjs";
import { assert } from "./lib/http.mjs";

async function waitFor(page, predicate, message, timeout = 12_000) {
  await page.waitForFunction(predicate, undefined, { timeout }).catch(() => {
    throw new Error(message);
  });
}

const fixture = await startH4RecoveryFixture();
const browser = await chromium.launch({ headless: true });
const context = await browser.newContext({
  viewport: { width: 390, height: 844 },
});
const page = await context.newPage();
page.on("dialog", (dialog) => {
  dialog.accept().catch(() => {
    // Cleanup-only race if the dirty-page dialog closes with the old document.
  });
});
try {
  await page.goto(fixture.baseUrl);
  await waitFor(
    page,
    () => document.body?.innerText.includes("H4 recovery package") ?? false,
    "H4 fixture did not reach the Macros page.",
  );

  await moveMacroViaMenu(page, "Recovery macro", "down");
  await waitFor(
    page,
    () => document.body?.innerText.includes("Unsaved changes") ?? false,
    "Reordering did not dirty the working copy before send recovery failed.",
  );

  await page.getByRole("button", { name: "Send Recovery macro" }).click();
  await page
    .getByRole("navigation", { name: "Primary navigation" })
    .getByRole("button", { name: "Packages", exact: true })
    .click();
  await page.getByRole("button", { name: "New package", exact: true }).click();
  const packageDialog = page.getByRole("dialog", { name: "New package" });
  await packageDialog.waitFor();
  await waitFor(
    page,
    () =>
      document.body?.innerText.includes("Execution state unavailable") ?? false,
    "Persistent status failures did not expose degraded execution state.",
  );
  assert(
    fixture.state.sendPostCount === 1,
    "The initial send must POST exactly once.",
  );
  assert(
    await page.getByText("Unsaved changes").isVisible(),
    "Execution degradation discarded the dirty working copy.",
  );
  const recovery = page.getByRole("alertdialog", {
    name: "Execution recovery",
  });
  assert(
    await packageDialog.evaluate((element) =>
      Boolean(element.closest("[inert]")),
    ),
    "The underlying package dialog stayed interactive during recovery.",
  );
  assert(
    await recovery
      .getByRole("button", { name: "Retry execution status" })
      .evaluate((button) => document.activeElement === button),
    "Execution recovery did not take keyboard focus from the lower dialog.",
  );
  assert(
    await recovery
      .getByRole("button", { name: "Cancel and release all keys" })
      .isVisible(),
    "Cancel was not visible while execution status was unavailable.",
  );

  await recovery
    .getByRole("button", { name: "Cancel and release all keys" })
    .click();
  await recovery
    .getByRole("button", { name: "Retry execution status" })
    .click();
  await waitFor(
    page,
    () =>
      !(
        document.body?.innerText.includes("Execution state unavailable") ?? true
      ),
    "Successful status reconciliation did not clear the degraded warning.",
  );
  assert(
    fixture.state.sendPostCount === 1,
    "Execution-status Retry must not POST the macro again.",
  );
  assert(
    await page.getByText("Unsaved changes").isVisible(),
    "Execution-status Retry discarded the dirty working copy.",
  );
  assert(
    await packageDialog.evaluate(
      (element) =>
        document.activeElement !== null &&
        element.contains(document.activeElement),
    ),
    "The package dialog did not regain focus after execution recovery cleared.",
  );
  await page.keyboard.press("Escape");

  // H4-043 reload case: force the first startup send-status recovery after a
  // real reload to fail. The page is intentionally dirty, so accept the
  // browser's beforeunload dialog; a reload naturally reconstructs the working
  // copy from the canonical device snapshot, while execution recovery must stay
  // explicit and must never create another send POST.
  fixture.state.completed = false;
  fixture.state.failFirstSendGetAfterNextDocument = true;
  await page.reload({ waitUntil: "domcontentloaded" });
  await waitFor(
    page,
    () => document.body?.innerText.includes("H4 recovery package") ?? false,
    "Reload did not restore the repository shell.",
  );
  await waitFor(
    page,
    () =>
      document.body?.innerText.includes("Execution state unavailable") ?? false,
    "A failed first recovery request after reload was represented as no send.",
  );
  assert(
    fixture.state.sendPostCount === 1,
    "Reload recovery must not POST the macro again.",
  );
  const reloadedRecovery = page.getByRole("alertdialog", {
    name: "Execution recovery",
  });
  assert(
    await reloadedRecovery
      .getByRole("button", { name: "Cancel and release all keys" })
      .isVisible(),
    "Cancel was not visible after reload recovery failed.",
  );

  // The global modal performs the GET and hands its recovered status to the
  // authenticated shell, which resumes tracking without a second POST.
  await reloadedRecovery
    .getByRole("button", { name: "Retry execution status" })
    .click();
  await waitFor(
    page,
    () =>
      !(
        document.body?.innerText.includes("Execution state unavailable") ?? true
      ),
    "Reload recovery Retry did not restore the active send.",
  );
  assert(
    fixture.state.sendPostCount === 1,
    "Reload recovery Retry must remain GET-only.",
  );
  await waitFor(
    page,
    () => document.body?.innerText.includes("Sending… action 0 of 1") ?? false,
    "Global recovery did not hand the active send back to the shell.",
  );

  console.log("Real Chrome H4 degraded-send recovery workflow passed.");
} finally {
  await context.close();
  await browser.close();
  await fixture.close();
}
