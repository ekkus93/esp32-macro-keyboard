import { mkdir, mkdtemp, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import process from "node:process";
import { gzipSync } from "node:zlib";
import { chromium } from "playwright";

import { startApplicationServer } from "./fixtures/applicationServer.mjs";
import { runAccessibilityScan } from "./lib/accessibility.mjs";
import { runBrowserWorkflows } from "./workflows/browser.mjs";
import {
  importFixtureRepository,
  runMacroEditingWorkflows,
} from "./workflows/macroEditing.mjs";
import {
  runSettingsWorkflows,
  runUsbUnavailableWorkflow,
} from "./workflows/settings.mjs";
import { runSnapshotsWorkflows } from "./workflows/snapshots.mjs";
import { runStartupWorkflows } from "./workflows/startup.mjs";

async function main() {
  const browserExchangeBase = await mkdtemp(
    join(tmpdir(), "esp32-macro-browser-tests-"),
  );
  await mkdir(browserExchangeBase, { recursive: true });
  const downloadDirectory = await mkdtemp(
    join(browserExchangeBase, "download-"),
  );
  const fixtureDirectory = await mkdtemp(join(browserExchangeBase, "fixture-"));
  const importFixturePath = join(fixtureDirectory, "import-fixture.gz");
  await writeFile(
    importFixturePath,
    gzipSync(Buffer.from(JSON.stringify(importFixtureRepository), "utf8")),
  );
  const application = await startApplicationServer();
  const browser = await chromium.launch({ headless: true });
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
    acceptDownloads: true,
  });
  const page = await context.newPage();
  // TODO_V2 V2-103 registers a real `beforeunload` listener while the
  // working copy is dirty, which fires a native, unstyleable browser dialog
  // on reload/navigation-away. Playwright auto-dismisses dialogs *unless* a
  // handler is registered, in which case the harness owns the decision —
  // auto-accept every dialog so scenarios that intentionally leave the store
  // dirty (e.g. reorder-then-reload) don't wedge the run; no current
  // scenario asserts on the dialog's own presence, only on app state
  // before/after it.
  page.on("dialog", (dialog) => {
    dialog.accept().catch(() => {
      // Best-effort: if the page is already navigating away, ignore it.
    });
  });

  try {
    await page.goto(application.baseUrl);
    await runBrowserWorkflows(page, application.state);
    console.log("Real Chrome v2 Macros page/Quick Send workflows passed.");
    // TODO_V2 V2-133 — see runAccessibilityScan's own doc comment.
    await runAccessibilityScan(page, "Macros page");
    await runSnapshotsWorkflows(page, application, {
      importFixturePath,
      downloadDirectory,
    });
    console.log("Real Chrome v2 Snapshots/import-export workflows passed.");
    await runAccessibilityScan(page, "Snapshots page");
    await runSettingsWorkflows(page);
    console.log("Real Chrome v2 Settings/Diagnostics workflows passed.");
    await runAccessibilityScan(page, "Settings page");
    console.log("Real Chrome axe-core accessibility scans passed.");

    await runUsbUnavailableWorkflow(browser);
    console.log("Real Chrome v2 USB-unavailable workflow passed.");
    await runStartupWorkflows(browser);
    console.log(
      "Real Chrome v2 startup workflows (first phone, refresh, expired " +
        "session, no blobs, invalid newest blob, send recovery) passed.",
    );
    await runMacroEditingWorkflows(browser);
    console.log(
      "Real Chrome v2 macro-editing/package-management workflows passed.",
    );
  } finally {
    await context.close();
    await browser.close();
    await application.close();
    await rm(browserExchangeBase, {
      recursive: true,
      force: true,
      maxRetries: 5,
      retryDelay: 100,
    });
  }
}

main().catch((error) => {
  console.error(error instanceof Error ? error.stack : error);
  process.exitCode = 1;
});
