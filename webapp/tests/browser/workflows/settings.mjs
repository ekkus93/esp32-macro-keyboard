import { startStartupFixtureServer } from "../fixtures/startupFixtureServer.mjs";
import { runAccessibilityScan } from "../lib/accessibility.mjs";
import { assert } from "../lib/http.mjs";
import {
  clickButton,
  evaluate,
  openDiagnostics,
  openSettingsGroup,
  openSettingsSection,
  waitFor,
} from "../lib/page.mjs";

export async function runSettingsWorkflows(page) {
  await openSettingsGroup(page, "device");
  await waitFor(
    page,
    () => document.querySelector("#settings-device-name") !== null,
    "The Device settings did not render.",
  );

  await page
    .locator("#settings-device-name")
    .fill("Bench Macro Keyboard (renamed)");
  await clickButton(page, "Save");
  await waitFor(
    page,
    // `textContent`, not `innerText`: the header renders the device name
    // inside `Eyebrow.tsx`, which is visually `text-transform: uppercase` —
    // `innerText` reflects that rendered casing, `textContent` does not.
    () => document.body.textContent.includes("Bench Macro Keyboard (renamed)"),
    "Saving the device name did not update the shell header.",
  );

  await clickButton(page, "Back to settings");
  await openSettingsGroup(page, "usb");
  await waitFor(
    page,
    () =>
      document.body.innerText.includes(
        "USB identity is fixed by the device firmware.",
      ) && document.querySelector("#settings-serial-confirmation") !== null,
    "The USB and sending settings did not render.",
  );
  assert(
    (await page.locator("main").innerText()).includes(
      "ESP32 Macro Keyboard Project",
    ),
    "The fixed USB manufacturer identity is not shown.",
  );

  await clickButton(page, "Back to settings");
  await waitFor(
    page,
    () => document.body.innerText.includes("USB identity and confirmation"),
    "Back did not return to the Settings list.",
  );
  await openDiagnostics(page);
  await waitFor(
    page,
    () => document.body.innerText.includes("browser-fixture-abc123"),
    "The Diagnostics page did not render the fixed diagnostics schema.",
  );
  assert(
    !(await evaluate(page, () =>
      document.body.innerText.includes("Lab bench workflow"),
    )),
    "Diagnostics rendered package/macro data, which SPEC_V2 §13.13 forbids.",
  );
  await clickButton(page, "Back to Settings");
  await waitFor(
    page,
    () => document.body.innerText.includes("USB identity and confirmation"),
    "Back to Settings did not return to the Settings list.",
  );

  await page.locator('select[name="theme-preference"]').selectOption("dark");
  await waitFor(
    page,
    () => document.documentElement.dataset.theme === "dark",
    "Choosing Dark did not switch the app theme.",
  );
  assert(
    (await page.evaluate(() =>
      window.localStorage.getItem("esp32-macro-keyboard.theme"),
    )) === "dark",
    "The explicit Dark choice was not saved as a UI preference.",
  );
  await page.locator('select[name="theme-preference"]').selectOption("system");
  await waitFor(
    page,
    () => document.documentElement.dataset.theme === "light",
    "Choosing System did not return to the current light device appearance.",
  );

  await openSettingsSection(page, "reset", "Restart");
  await clickButton(page, "Restart");
  await waitFor(
    page,
    () => document.querySelector('[role="alertdialog"]') !== null,
    "The restart confirmation did not open.",
  );
  await runAccessibilityScan(page, "Restart confirmation dialog");
  await page.keyboard.press("Escape");
  await waitFor(
    page,
    () => document.querySelector('[role="alertdialog"]') === null,
    "Escape did not dismiss the restart confirmation.",
  );

  await clickButton(page, "Back to restart and reset");
  await waitFor(
    page,
    () => document.body.innerText.includes("Restart and reset"),
    "Back did not return to the reset actions.",
  );
  await clickButton(page, "Factory reset");
  await waitFor(
    page,
    () => location.hash.includes("section=factory-reset"),
    "Factory reset action screen did not open.",
  );
  await clickButton(page, "Factory reset");
  await waitFor(
    page,
    () => document.querySelector('[role="alertdialog"]') !== null,
    "The factory-reset confirmation did not open.",
  );
  await runAccessibilityScan(page, "Unsaved-work prompt before factory reset");
  await page.keyboard.press("Escape");
  await waitFor(
    page,
    () => document.querySelector('[role="alertdialog"]') === null,
    "Escape did not dismiss the factory-reset confirmation.",
  );
}

/**
 * TODO_V2 Phase 9 exit gate's "USB unavailable" scenario. Runs against its
 * own fixture (not the shared one above, which is fixed at
 * `usb.state: "ready"`) started with a non-`ready` USB state from the very
 * first `GET /api/v1/status` response -- `useDeviceStatus`'s poll fires
 * immediately on mount (before its 5-second interval even starts), so there
 * is no need to wait out a poll cycle to "flip" state mid-test; the
 * not-ready state is simply the fixture's starting condition. Also proves
 * the *recovery* direction (USB becoming ready re-enables Send) does
 * exercise that 5-second poll for real, by mutating `fixture.state.usbState`
 * after load and waiting for the next poll to pick it up.
 */
export async function runUsbUnavailableWorkflow(browser) {
  const fixture = await startStartupFixtureServer({ usbState: "disconnected" });
  const context = await browser.newContext({
    viewport: { width: 390, height: 844 },
  });
  const page = await context.newPage();
  try {
    await page.goto(fixture.baseUrl);
    await waitFor(
      page,
      () => document.body?.innerText.includes("Lab bench workflow") ?? false,
      "USB-unavailable fixture did not reach the Macros page.",
    );
    await waitFor(
      page,
      () => document.body.innerText.includes("USB disconnected"),
      "The shell header did not show the non-ready USB state.",
    );
    const sendDisabledWhileUnavailable = await evaluate(page, () => {
      const button = document.querySelector(
        '[aria-label="Send Open terminal"]',
      );
      return button instanceof HTMLButtonElement ? button.disabled : null;
    });
    assert(
      sendDisabledWhileUnavailable === true,
      "Send remained enabled while USB was not ready.",
    );

    // Recovery: once USB becomes ready device-side, the app's own 5-second
    // status poll (not a page reload) must pick it up and re-enable Send.
    fixture.state.usbState = "ready";
    await waitFor(
      page,
      () => document.body.innerText.includes("USB ready"),
      "USB becoming ready was not reflected in the header within one poll cycle.",
      8_000,
    );
    await waitFor(
      page,
      () => {
        const button = document.querySelector(
          '[aria-label="Send Open terminal"]',
        );
        return button instanceof HTMLButtonElement && !button.disabled;
      },
      "Send did not re-enable once USB became ready.",
      8_000,
    );
  } finally {
    await context.close();
    await fixture.close();
  }
}

/**
 * TODO_V2 Phase 8 exit gate: "first phone" -- an unconfigured device's
 * entire first-ever launch, from First-Run Setup (V2-080) through Sign In
 * (V2-081) to Create Your First Repository (V2-083), all against a real
 * browser and a fixture that starts genuinely unprovisioned and blob-less.
 */
