import { blobId, repositoryGzip } from "../fixtures/data.mjs";
import { startStartupFixtureServer } from "../fixtures/startupFixtureServer.mjs";
import { assert } from "../lib/http.mjs";
import { clickButton, clickButtonByAriaLabel, waitFor } from "../lib/page.mjs";

async function runStartupFirstPhoneScenario(browser) {
  const fixture = await startStartupFixtureServer({
    provisioned: false,
    authenticated: false,
    blobs: [],
  });
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
  });
  const page = await context.newPage();
  try {
    await page.goto(fixture.baseUrl);
    await waitFor(
      page,
      () => document.body.innerText.includes("First-run setup"),
      "An unprovisioned device did not open First-Run Setup.",
    );
    await page.locator("#setup-code").fill("12345678");
    await page.locator("#device-name").fill("Bench Macro Keyboard");
    await page.locator("#ap-ssid").fill("MacroKeyboard");
    await page.locator("#ap-passphrase").fill("bench-ap-passphrase-1");
    await page.locator("#admin-password").fill("bench-fixture-admin-pw-1");
    await clickButton(page, "Review setup");
    await waitFor(
      page,
      () => document.body.innerText.includes("Review setup"),
      "Submitting the setup form did not reach the review step.",
    );
    await clickButton(page, "Apply setup");
    await waitFor(
      page,
      () => document.body.innerText.includes("Setup complete"),
      "Applying setup did not reach the Setup complete screen.",
    );
    await clickButton(page, "Continue to Sign In");
    await waitFor(
      page,
      () => document.body.innerText.includes("Sign in"),
      "Setup completion did not proceed to Sign In.",
    );
    await page.locator("#admin-password").fill("bench-fixture-admin-pw-1");
    await clickButton(page, "Sign in");
    await waitFor(
      page,
      () => document.body.innerText.includes("Create Your First Repository"),
      "A first sign-in with no stored blobs did not offer to create the first repository.",
    );
    await page.locator("#first-package-name").fill("First bench package");
    await clickButton(page, "Create package");
    await waitFor(
      page,
      () =>
        document.body.innerText.includes("Add macro") &&
        document.body.innerText.includes("0 macros"),
      "Creating the first package did not open its empty Macros page.",
    );
    await waitFor(
      page,
      () => document.body.innerText.includes("Unsaved changes"),
      "The freshly created first repository was not shown as unsaved.",
    );
    assert(
      fixture.state.blobs.length === 0,
      "Creating the first repository must not upload a blob before an explicit Save snapshot.",
    );
  } finally {
    await context.close();
    await fixture.close();
  }
}

/**
 * TODO_V2 Phase 8 exit gate: "refresh" and "send recovery", together --
 * both are proven by the same mechanism (a real `page.reload()`), and the
 * exit gate names them as a pair. Starts a send, reloads mid-send, and
 * asserts two distinct things Phase 9's own reload scenario does not:
 * (1) the *entire* startup fetch sequence genuinely re-runs after a real
 * reload (React state is wiped; nothing survives it) and (2) the recovered
 * send status is produced by that startup sequence's own `GET /api/v1/send`
 * step (UI_UX_SPEC_V2 §3.4 step 8), not merely eventually visible.
 */
async function runStartupRefreshAndSendRecoveryScenario(browser) {
  const fixture = await startStartupFixtureServer();
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
  });
  const page = await context.newPage();
  try {
    await page.goto(fixture.baseUrl);
    await waitFor(
      page,
      () => document.body?.innerText.includes("Lab bench workflow") ?? false,
      "Startup fixture: initial load did not reach the Macros page.",
    );

    await clickButtonByAriaLabel(page, "Send Open terminal");
    await waitFor(
      page,
      () => document.body.innerText.includes("Sending Open terminal"),
      "Progress did not show before reload.",
    );

    fixture.state.requestLog.length = 0;
    await page.reload();
    await waitFor(
      page,
      () => document.body?.innerText.includes("Lab bench workflow") ?? false,
      "Refresh did not return to the Macros page.",
      15_000,
    );
    await waitFor(
      page,
      () =>
        document.body.innerText.includes("Sending…") ||
        document.body.innerText.includes("Sent."),
      "Send state was not recovered as part of the startup sequence after reload.",
    );

    const paths = fixture.state.requestLog.map((entry) => entry.split(" ")[1]);
    for (const required of [
      "/api/v1/setup",
      "/api/v1/auth/session",
      "/api/v1/settings",
      "/api/v1/blob",
      `/api/v1/blob/${blobId}`,
      "/api/v1/send",
    ]) {
      assert(
        paths.includes(required),
        `Refresh did not re-run ${required} as part of the startup sequence.`,
      );
    }
  } finally {
    await context.close();
    await fixture.close();
  }
}

/**
 * TODO_V2 Phase 8 exit gate: "expired session". Dirties the working copy,
 * forces the fixture's session to end server-side, waits for the app's own
 * status poll to surface the resulting `401` and drop to Sign In (without
 * ever reloading the page), re-authenticates, and asserts the *same*
 * dirty in-memory working copy resumes -- proving UI_UX_SPEC_V2 §3.3/§7.3's
 * "a session expiry does not discard the in-memory working copy" against a
 * real browser: no new blob fetch happens on re-authentication.
 */
async function runStartupExpiredSessionScenario(browser) {
  const fixture = await startStartupFixtureServer();
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
  });
  const page = await context.newPage();
  try {
    await page.goto(fixture.baseUrl);
    await waitFor(
      page,
      () => document.body?.innerText.includes("Lab bench workflow") ?? false,
      "Startup fixture: initial load did not reach the Macros page.",
    );

    await page.locator('[aria-label="Move Open terminal down"]').press("Enter");
    await waitFor(
      page,
      () => document.body.innerText.includes("Unsaved changes"),
      "Reordering did not dirty the working copy before expiring the session.",
    );

    const blobFetchesBeforeExpiry = fixture.state.requestLog.filter((entry) =>
      entry.startsWith("GET /api/v1/blob/"),
    ).length;

    fixture.state.authenticated = false;
    await waitFor(
      page,
      () => document.body.innerText.includes("Sign in"),
      "Session expiry did not drop the app back to Sign In.",
      12_000,
    );

    await page.locator("#admin-password").fill(fixture.state.adminPassword);
    await clickButton(page, "Sign in");
    await waitFor(
      page,
      () =>
        (document.body?.innerText.includes("Lab bench workflow") ?? false) &&
        document.body.innerText.includes("Unsaved changes"),
      "Re-authenticating did not resume the same dirty working copy.",
    );

    const blobFetchesAfterExpiry = fixture.state.requestLog.filter((entry) =>
      entry.startsWith("GET /api/v1/blob/"),
    ).length;
    assert(
      blobFetchesAfterExpiry === blobFetchesBeforeExpiry,
      "Re-authentication re-fetched the repository blob instead of resuming the live in-memory working copy.",
    );
  } finally {
    await context.close();
    await fixture.close();
  }
}

/**
 * TODO_V2 Phase 8 exit gate: "no blobs" -- a signed-in device with zero
 * stored snapshots opens Create Your First Repository (UI_UX_SPEC_V2 §8),
 * and creating the first package must not upload anything until an
 * explicit Save snapshot.
 */
async function runStartupNoBlobsScenario(browser) {
  const fixture = await startStartupFixtureServer({ blobs: [] });
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
  });
  const page = await context.newPage();
  try {
    await page.goto(fixture.baseUrl);
    await waitFor(
      page,
      () => document.body.innerText.includes("Create Your First Repository"),
      "A device with no stored blobs did not offer to create the first repository.",
    );
    await page.locator("#first-package-name").fill("First bench package");
    await clickButton(page, "Create package");
    await waitFor(
      page,
      () =>
        document.body.innerText.includes("Add macro") &&
        document.body.innerText.includes("0 macros"),
      "First-repository creation did not open an empty Macros page.",
    );
    await waitFor(
      page,
      () => document.body.innerText.includes("Unsaved changes"),
      "The new in-memory repository was not marked unsaved.",
    );
    assert(
      fixture.state.blobs.length === 0,
      "Creating the first repository must not upload a blob before an explicit Save snapshot.",
    );
  } finally {
    await context.close();
    await fixture.close();
  }
}

/**
 * TODO_V2 Phase 8 exit gate: "invalid newest blob" -- the newest stored blob
 * is corrupt (not valid gzip), an older blob is valid. React must never
 * silently fall back to the older blob (UI_UX_SPEC_V2 §3.4): it shows
 * Snapshot recovery with the exact failure, and only loads the older blob
 * after an explicit user pick -- which must leave the corrupt blob in
 * storage, not delete it (SPEC_V2 §9.6).
 */
async function runStartupInvalidNewestBlobScenario(browser) {
  const corruptBytes = Buffer.from([0x00, 0x01, 0x02, 0x03, 0x04]);
  const fixture = await startStartupFixtureServer({
    blobs: [
      { id: "1", bytes: repositoryGzip },
      { id: "2", bytes: corruptBytes },
    ],
  });
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
  });
  const page = await context.newPage();
  try {
    await page.goto(fixture.baseUrl);
    await waitFor(
      page,
      () => document.body.innerText.includes("Snapshot recovery"),
      "An unreadable newest blob did not surface the Snapshot recovery screen.",
    );
    await waitFor(
      page,
      () =>
        document.body.innerText.includes(
          "Blob 2 could not be opened automatically.",
        ),
      "Snapshot recovery did not identify the corrupt blob by ID.",
    );
    await clickButton(
      page,
      `Load snapshot 1 (${String(repositoryGzip.byteLength)} bytes)`,
    );
    await waitFor(
      page,
      () => document.body?.innerText.includes("Lab bench workflow") ?? false,
      "Recovering via an older snapshot did not reach the Macros page.",
      15_000,
    );
    assert(
      fixture.state.blobs.some((blob) => blob.id === "2"),
      "Snapshot recovery must not delete the unreadable blob.",
    );
  } finally {
    await context.close();
    await fixture.close();
  }
}

/**
 * TODO_V2 Phase 8 exit gate: "Real-browser tests cover first phone, refresh,
 * expired session, no blobs, invalid newest blob, and send recovery." Each
 * scenario runs against its own fresh fixture server and browser context
 * (device state -- provisioning, session validity, blob contents -- differs
 * too much between them to share the fixture above), and each cleans up
 * fully before the next starts.
 */
export async function runStartupWorkflows(browser) {
  await runStartupFirstPhoneScenario(browser);
  await runStartupRefreshAndSendRecoveryScenario(browser);
  await runStartupExpiredSessionScenario(browser);
  await runStartupNoBlobsScenario(browser);
  await runStartupInvalidNewestBlobScenario(browser);
}

/**
 * TODO_V2 Phase 10 exit gate: "Editing and package-management unit and
 * browser tests pass" -- no browser (real-Chrome) scenario existed for the
 * macro editor or package-management page before this. Runs against its own
 * fresh fixture server/context so its assertions do not depend on, and are
 * not disturbed by, the Phase 9/11/12 workflows' own mutations of the shared
 * fixture's working copy. Focuses on the real-browser-only concerns the
 * existing Vitest coverage (`v2-macro-editor-page.test.tsx`,
 * `v2-package-management-page.test.tsx`) cannot reach: actual textarea
 * focus/selection for directive insertion and "Go to error", real hash-route
 * navigation, and the native `beforeunload` dialog while the working copy is
 * dirty.
 */
