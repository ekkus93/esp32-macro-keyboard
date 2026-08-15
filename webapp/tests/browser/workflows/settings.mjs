import { startStartupFixtureServer } from "../fixtures/startupFixtureServer.mjs";
import { assert } from "../lib/http.mjs";
import { clickButton, evaluate, waitFor } from "../lib/page.mjs";

export async function runSettingsWorkflows(page) {
  await clickButton(page, "Settings");
  await waitFor(
    page,
    () => document.querySelector("#settings-device-name") !== null,
    "The Settings page did not render.",
  );

  await page
    .locator("#settings-device-name")
    .fill("Bench Macro Keyboard (renamed)");
  await clickButton(page, "Save");
  await waitFor(
    page,
    // `textContent`, not `innerText`: the header renders the device name
    // inside `.eyebrow`, which is visually `text-transform: uppercase` —
    // `innerText` reflects that rendered casing, `textContent` does not.
    () => document.body.textContent.includes("Bench Macro Keyboard (renamed)"),
    "Saving the device name did not update the shell header.",
  );

  await clickButton(page, "View diagnostics");
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
    () => document.querySelector("#settings-device-name") !== null,
    "Back to Settings did not return to the Settings page.",
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
