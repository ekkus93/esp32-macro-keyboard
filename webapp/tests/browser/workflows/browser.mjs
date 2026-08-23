import { assert } from "../lib/http.mjs";
import {
  assertResponsiveLayout,
  assertTouchTargets,
  clickButton,
  clickButtonByAriaLabel,
  evaluate,
  waitFor,
} from "../lib/page.mjs";

export async function runBrowserWorkflows(page, serverState) {
  await waitFor(
    page,
    () => document.body?.innerText.includes("Lab bench workflow") ?? false,
    "The Macros page did not load the real gzip-decoded repository.",
  );
  await waitFor(
    page,
    () => document.body.innerText.includes("Open terminal"),
    "The macro list did not render.",
  );
  await assertTouchTargets(page);
  await assertResponsiveLayout(page);

  // Accessible reordering (V2-091): a real keyboard Enter press on the
  // "Move down" control — not just a mouse click — actually reorders.
  // `locator.press()` focuses the element itself before dispatching the key,
  // matching the harness's previous explicit focus()-then-key sequence.
  await page.locator('[aria-label="Move Open terminal down"]').press("Enter");
  await waitFor(
    page,
    () =>
      document.body.innerText.includes("Moved Open terminal to position 2."),
    "Keyboard Enter on Move down did not reorder the macro list.",
  );
  await page.locator('[aria-label="Move Open terminal up"]').press("Enter");
  await waitFor(
    page,
    () =>
      document.body.innerText.includes("Moved Open terminal to position 1."),
    "Keyboard Enter on Move up did not reorder the macro list back.",
  );

  // Macro source is always shown, with no reveal/hide control (V2-092).
  assert(
    !(await evaluate(page, () =>
      document.body.innerText.includes("Source hidden"),
    )),
    "Macro source was hidden.",
  );
  const openTerminalSource = await evaluate(
    page,
    () => document.querySelector("code")?.textContent ?? null,
  );
  assert(
    openTerminalSource === "a",
    `Macro source was not shown unconditionally: ${String(openTerminalSource)}`,
  );

  // Quick Send: progress, then a completion acknowledgement that clears
  // itself (V2-093).
  await clickButtonByAriaLabel(page, "Send Open terminal");
  await waitFor(
    page,
    () => document.body.innerText.includes("Sending Open terminal"),
    "Quick Send did not show inline progress.",
  );
  await waitFor(
    page,
    () => document.body.innerText.includes("Sent Open terminal."),
    "Quick Send did not reach a completion acknowledgement.",
  );
  await waitFor(
    page,
    () => !document.body.innerText.includes("Sent Open terminal."),
    "The completion acknowledgement did not clear itself.",
    8_000,
  );
  assert(
    serverState.sendPostCount === 1,
    `Expected exactly one POST /api/v1/send for the completed send, got ${String(serverState.sendPostCount)}.`,
  );

  // Rapid repeated input (TODO_V2 Phase 9 exit gate, V2-095 "Prevent
  // double-send on rapid taps"): three synchronous DOM `.click()` calls
  // inside a single `page.evaluate()` invoke React's `onClick` handler
  // three times in the same JS task, before any re-render -- the same-tick
  // double-dispatch race `startSend`'s `startingRef` guard is built to
  // withstand (see the updated comment above `runBrowserWorkflows`).
  const sendPostCountBeforeRapidTaps = serverState.sendPostCount;
  await evaluate(page, () => {
    const button = document.querySelector('[aria-label="Send Open terminal"]');
    if (!(button instanceof HTMLElement)) {
      throw new Error(
        "Send Open terminal button not found for the rapid-tap scenario.",
      );
    }
    button.click();
    button.click();
    button.click();
  });
  await waitFor(
    page,
    () => document.body.innerText.includes("Sending Open terminal"),
    "Rapid repeated Send taps did not start a send.",
  );
  await waitFor(
    page,
    () => document.body.innerText.includes("Sent Open terminal."),
    "The rapid-tap send did not reach completion.",
  );
  await waitFor(
    page,
    () => !document.body.innerText.includes("Sent Open terminal."),
    "The rapid-tap completion acknowledgement did not clear itself.",
    8_000,
  );
  assert(
    serverState.sendPostCount === sendPostCountBeforeRapidTaps + 1,
    `Three rapid Send taps produced ${String(serverState.sendPostCount - sendPostCountBeforeRapidTaps)} POST /api/v1/send calls instead of exactly one.`,
  );

  // Serial-confirmation waiting state, inline (UI_UX_SPEC_V2 §5.5).
  const sendPostCountBeforeConfirmation = serverState.sendPostCount;
  await clickButtonByAriaLabel(page, "Send Confirm before typing");
  await waitFor(
    page,
    () =>
      document.body.innerText.includes(
        "Waiting for confirmation on the device",
      ) &&
      document.body.innerText.includes(
        "Run confirm in the device serial console",
      ),
    "The confirmation-wait state or serial-confirm instruction was not shown inline.",
  );
  assert(
    (await page
      .getByRole("button", {
        name: "Cancel and release all keys",
        exact: true,
      })
      .count()) >= 1,
    "Cancel was not reachable while awaiting confirmation.",
  );
  await waitFor(
    page,
    () => document.body.innerText.includes("Sent Confirm before typing."),
    "The confirmed send did not complete.",
  );
  await waitFor(
    page,
    () => !document.body.innerText.includes("Sent Confirm before typing."),
    "The confirmation completion acknowledgement did not clear itself.",
    8_000,
  );
  assert(
    serverState.sendPostCount === sendPostCountBeforeConfirmation + 1,
    `Confirmation transition produced ${String(serverState.sendPostCount - sendPostCountBeforeConfirmation)} POST /api/v1/send calls instead of exactly one.`,
  );

  // Cancel and release all keys.
  await clickButtonByAriaLabel(page, "Send Open terminal");
  await waitFor(
    page,
    () => document.body.innerText.includes("Sending Open terminal"),
    "Progress did not show before cancelling.",
  );
  await clickButton(page, "Cancel and release all keys");
  await waitFor(
    page,
    () => document.body.innerText.includes("was cancelled."),
    "Cancellation did not produce a persistent acknowledgement.",
  );
  await clickButton(page, "Dismiss");
  await waitFor(
    page,
    () => !document.body.innerText.includes("was cancelled."),
    "Dismiss did not clear the cancellation banner.",
  );

  // Failure, with the exact error and a persistent banner until dismissed.
  await clickButtonByAriaLabel(page, "Send Trigger failure");
  await waitFor(
    page,
    () => document.body.innerText.includes("failed: simulated_failure"),
    "The failure state was not shown with its exact error.",
  );
  await clickButton(page, "Dismiss");
  await waitFor(
    page,
    () => !document.body.innerText.includes("failed: simulated_failure"),
    "Dismiss did not clear the failure banner.",
  );

  // Timeout, persistent until dismissed.
  await clickButtonByAriaLabel(page, "Send Trigger timeout");
  await waitFor(
    page,
    () => document.body.innerText.includes("timed out."),
    "The timeout state was not shown.",
  );
  await clickButton(page, "Dismiss");

  // Release error, reported separately from the completion it rode in on.
  await clickButtonByAriaLabel(page, "Send Trigger release error");
  await waitFor(
    page,
    () =>
      document.body.innerText.includes(
        "Key release failed: stuck_key_left_ctrl",
      ),
    "The release error was not reported.",
  );
  const releaseErrorButtons = await page
    .getByRole("button", { name: "Dismiss", exact: true })
    .count();
  assert(
    releaseErrorButtons >= 1,
    "The release-error banner did not offer its own Dismiss control.",
  );
  await clickButton(page, "Dismiss");
  // The release error is a separate, independently dismissible banner from
  // the completion acknowledgement it rode in on (UI_UX_SPEC_V2 §5.5); Send
  // controls stay disabled until that acknowledgement itself clears.
  await waitFor(
    page,
    () => !document.body.innerText.includes("Sent Trigger release error."),
    "The completion acknowledgement behind the release error did not clear.",
    8_000,
  );

  // Reload recovery (V2-095/UI_UX_SPEC_V2 §5.6): start a send, reload before
  // it finishes, and confirm React resumes tracking from `GET /api/v1/send`
  // instead of losing the in-progress state.
  await clickButtonByAriaLabel(page, "Send Open terminal");
  await waitFor(
    page,
    () => document.body.innerText.includes("Sending Open terminal"),
    "Progress did not show before reload.",
  );
  await page.reload();
  await waitFor(
    page,
    () => document.body?.innerText.includes("Lab bench workflow") ?? false,
    "The app did not reload back to the Macros page.",
    15_000,
  );
  await waitFor(
    page,
    () =>
      document.body.innerText.includes("Sending…") ||
      document.body.innerText.includes("Sent."),
    "Send state was not recovered after reload.",
  );
}

/**
 * TODO_V2 Phase 11 exit gate: "Snapshot and import/export browser tests
 * pass." Covered here against the real built app, real gzip
 * (`CompressionStream`/`DecompressionStream`, unavailable to jsdom), and a
 * real multi-blob fixture server: snapshot list rendering, manual Save,
 * dirty-work protection during Load (discard-and-load), exact-ID-confirmed
 * Delete, Export (a real file landing on disk via a real Playwright/Chrome
 * download), and Import (a real file selected via `page.setInputFiles()`,
 * since scripts cannot assign `HTMLInputElement.files` directly for security
 * reasons — this is the one thing the Vitest suite cannot exercise at all).
 */
