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

  await page.locator('[aria-label="Move Recovery macro down"]').press("Enter");
  await waitFor(
    page,
    () => document.body?.innerText.includes("Unsaved changes") ?? false,
    "Reordering did not dirty the working copy before send recovery failed.",
  );

  await page.getByRole("button", { name: "Send Recovery macro" }).click();
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
  const recovery = page.getByLabel("Execution recovery");
  assert(
    await recovery
      .getByRole("button", { name: "Cancel and release all keys" })
      .isVisible(),
    "Cancel was not visible while execution status was unavailable.",
  );

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
  const reloadedRecovery = page.getByLabel("Execution recovery");
  assert(
    await reloadedRecovery
      .getByRole("button", { name: "Cancel and release all keys" })
      .isVisible(),
    "Cancel was not visible after reload recovery failed.",
  );

  // Use the authenticated shell's startup-recovery Retry. The global overlay
  // deliberately cannot hand a nonterminal startup send to a page that has not
  // adopted it yet, so the page-level control performs the GET and hands the
  // recovered status into MacrosPage without a POST.
  const retryButtons = page.getByRole("button", {
    name: "Retry execution status",
  });
  const retryCount = await retryButtons.count();
  assert(retryCount >= 2, "Expected both page and global recovery controls.");
  let clickedPageRetry = false;
  for (let index = 0; index < retryCount; index += 1) {
    const button = retryButtons.nth(index);
    const inOverlay = await button.evaluate(
      (element) =>
        element.closest('aside[aria-label="Execution recovery"]') !== null,
    );
    if (!inOverlay) {
      await button.click();
      clickedPageRetry = true;
      break;
    }
  }
  assert(
    clickedPageRetry,
    "The page-level execution recovery Retry was missing.",
  );
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

  console.log("Real Chrome H4 degraded-send recovery workflow passed.");
} finally {
  await context.close();
  await browser.close();
  await fixture.close();
}
