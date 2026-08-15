import { readFile } from "node:fs/promises";
import { join } from "node:path";
import { gunzipSync } from "node:zlib";

import { assert } from "../lib/http.mjs";
import {
  clickButton,
  clickButtonByAriaLabel,
  evaluate,
  waitFor,
} from "../lib/page.mjs";

export async function runSnapshotsWorkflows(page, application, fixtures) {
  const { importFixturePath, downloadDirectory } = fixtures;

  await clickButton(page, "Snapshots");
  await waitFor(
    page,
    () => document.body.innerText.includes("Snapshot 1"),
    "The Snapshots page did not render the stored blob.",
  );
  assert(
    await evaluate(page, () =>
      document.body.innerText.includes("retention target 5"),
    ),
    "The configured advisory retention target was not shown.",
  );

  // Manual Save (V2-110): an explicit click, never automatic, and never
  // deletes an existing snapshot (V2-111/V2-112).
  await clickButton(page, "Save current snapshot");
  await waitFor(
    page,
    () => document.body.innerText.includes("Snapshot 2"),
    "Save current snapshot did not add a new blob.",
  );
  assert(
    application.state.blobs.length === 2,
    `Expected 2 stored blobs after one save, got ${String(application.state.blobs.length)}.`,
  );
  assert(
    application.state.blobs.some((blob) => blob.id === "1"),
    "Save current snapshot deleted the original blob — saves must be additive only.",
  );

  // Dirty-work protection during load (V2-113): dirty the working copy,
  // then confirm Load warns with all four spec'd choices before touching
  // anything, and that Discard changes and load both discards and loads.
  await clickButton(page, "Macros");
  await waitFor(
    page,
    () => document.body.innerText.includes("Add macro"),
    "Did not return to the Macros page.",
  );
  await page.locator('[aria-label="Move Open terminal down"]').press("Enter");
  await waitFor(
    page,
    () => document.body.innerText.includes("Unsaved changes"),
    "Reordering a macro did not dirty the working copy.",
  );

  await clickButton(page, "Snapshots");
  await waitFor(
    page,
    () => document.body.innerText.includes("Snapshot 1"),
    "The Snapshots page did not render after dirtying the working copy.",
  );
  await clickButtonByAriaLabel(page, "Load snapshot 1");
  await waitFor(
    page,
    () => document.body.innerText.includes("Discard changes and load"),
    "Loading while dirty did not show the unsaved-changes warning.",
  );
  for (const label of [
    "Cancel",
    "Export working copy",
    "Save snapshot",
    "Discard changes and load",
  ]) {
    assert(
      (await page.getByRole("button", { name: label, exact: true }).count()) >
        0,
      `The dirty-load warning was missing its "${label}" choice.`,
    );
  }
  await clickButton(page, "Discard changes and load");
  // "Lab bench workflow" alone is not a safe signal here: it is also the
  // selected-package name shown in the app header on every route, so it (and
  // the "Unsaved changes" clear) is satisfied the instant the synchronous
  // `store.discardChanges()` runs — before the async `performLoad()` it
  // kicks off has actually navigated away from the Snapshots route. Wait for
  // "Add macro", a Macros-page-only marker, so the subsequent Export step
  // does not race that in-flight navigation.
  await waitFor(
    page,
    () =>
      document.body.innerText.includes("Add macro") &&
      !document.body.innerText.includes("Unsaved changes"),
    "Discard changes and load did not restore a clean, loaded working copy.",
  );

  // Export (V2-115): a real Chrome download of the exact gzip bytes, via
  // Playwright's own `page.waitForEvent('download')` — replacing the manual
  // `Browser.setDownloadBehavior`/`Browser.downloadProgress` CDP plumbing
  // and its snap-AppArmor tmpdir workarounds, neither of which apply to
  // Playwright's own (non-snap) bundled Chromium.
  await clickButton(page, "Snapshots");
  await waitFor(
    page,
    () => document.body.innerText.includes("Snapshot 1"),
    "The Snapshots page did not render before export.",
  );
  await waitFor(
    page,
    () => document.body.innerText.includes("Import and export"),
    "The Import/export section did not render before export.",
  );
  const downloadPromise = page.waitForEvent("download");
  await clickButton(page, "Export working copy");
  const download = await downloadPromise;
  assert(
    download.suggestedFilename().endsWith(".emk-repository.json.gz"),
    `Exported filename did not match the spec'd suffix: ${download.suggestedFilename()}`,
  );
  const downloadedPath = join(downloadDirectory, download.suggestedFilename());
  await download.saveAs(downloadedPath);
  const downloadedBytes = await readFile(downloadedPath);
  const decompressed = JSON.parse(gunzipSync(downloadedBytes).toString("utf8"));
  assert(
    decompressed.format === "esp32-macro-keyboard-repository" &&
      decompressed.packages?.[0]?.name === "Lab bench workflow",
    "The exported file did not decompress to the current working copy.",
  );

  // Import (V2-115): a real file selection through Playwright's own
  // `page.setInputFiles()` — jsdom cannot exercise this at all, since
  // scripts cannot assign `HTMLInputElement.files` directly.
  await page.setInputFiles('input[type="file"]', importFixturePath);
  await waitFor(
    page,
    () =>
      document.body.innerText.includes("1 packages") &&
      document.body.innerText.includes("1 macros"),
    "Import did not show package/macro counts before confirmation.",
  );
  await clickButton(page, "Replace working copy with this import");
  // As with Discard changes and load above: "Imported bench" and "Unsaved
  // changes" both become true the instant the synchronous
  // `store.applyImport()` runs, before the async navigation back to Macros
  // that follows it completes. Also require "Add macro" (a Macros-page-only
  // marker) so the next step does not race that in-flight navigation.
  await waitFor(
    page,
    () =>
      document.body.innerText.includes("Imported bench") &&
      document.body.innerText.includes("Unsaved changes") &&
      document.body.innerText.includes("Add macro"),
    "Import did not replace the working copy, mark it dirty, and return to the Macros page.",
  );

  // Manual deletion (V2-111), confirmed by exact blob ID, never automatic.
  await clickButton(page, "Snapshots");
  await waitFor(
    page,
    () => document.body.innerText.includes("Snapshot 2"),
    "The Snapshots page did not render before delete.",
  );
  await clickButtonByAriaLabel(page, "Delete snapshot 2");
  await waitFor(
    page,
    () => document.querySelector("#snapshot-delete-confirm-2") !== null,
    "Delete did not show the exact-ID confirmation field.",
  );
  // `locator.fill()` is Playwright's own React-controlled-input-safe input
  // API: it sets the value through the native property setter and dispatches
  // real `input`/`change` events, exactly like the manual setter dispatch it
  // replaces.
  await page.locator("#snapshot-delete-confirm-2").fill("2");
  await clickButton(page, "Confirm delete");
  await waitFor(
    page,
    () => !document.body.innerText.includes("Snapshot 2"),
    "Confirmed delete did not remove the blob from the list.",
  );
  assert(
    application.state.blobs.length === 1 &&
      application.state.blobs[0]?.id === "1",
    "Delete removed the wrong blob, or left the store in an unexpected state.",
  );
}

/**
 * Settings and Diagnostics against a real Chrome (TODO_V2 V2-120/V2-122).
 * Deliberately scoped to non-destructive, order-independent coverage —
 * device-name edit and Diagnostics rendering — rather than the
 * restart/reset-settings/factory-reset reconnect flows: those need this
 * fixture server to simulate a genuine connection loss (the point of the
 * feature), which this Playwright harness has no way to do safely, and
 * `AppV2` (`tests/v2-app-v2.test.tsx`) already exercises that full
 * disruption-and-reconnect sequence end to end against the real,
 * unmocked v2 API clients. Sign Out is likewise left to that same jsdom
 * suite and `SettingsPage`'s own unit tests: exercising it here would
 * leave every following browser assertion running unauthenticated.
 */
