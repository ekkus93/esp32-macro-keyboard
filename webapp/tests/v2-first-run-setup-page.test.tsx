import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { FirstRunSetupPage } from "../src/features/auth/v2/FirstRunSetupPage";
import { getFetchCalls, jsonResponse, planFetch } from "./fakeFetch";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setInputValue,
} from "./render";

function planSetupGet(body: unknown, status = 200): void {
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/setup");
    expect(call.method).toBe("GET");
    return jsonResponse(body, status);
  });
}

async function renderSetupForm(): Promise<Awaited<ReturnType<typeof render>>> {
  planSetupGet({ provisioned: false, deviceName: "ESP32 Macro Keyboard" });
  const view = await render(
    <FirstRunSetupPage
      onSetupComplete={() => {
        /* not exercised in this test */
      }}
    />,
  );
  await flushReact();
  return view;
}

const validFields = {
  setupCode: "12345678",
  deviceName: "Desk Macro Keyboard",
  apSsid: "MacroKeyboard",
  apPassphrase: "example-passphrase",
  adminPassword: "example-admin-password",
};

async function fillField(selector: string, value: string): Promise<void> {
  await setInputValue(requiredElement(selector, HTMLInputElement), value);
}

/** Fills step 1 (device) and moves on. */
async function completeDeviceStep(): Promise<void> {
  await fillField("#setup-code", validFields.setupCode);
  await fillField("#device-name", validFields.deviceName);
  await click(buttonWithText("Continue"));
}

/** Fills step 2 (Wi-Fi) and moves on. */
async function completeWifiStep(): Promise<void> {
  await fillField("#ap-ssid", validFields.apSsid);
  await fillField("#ap-passphrase", validFields.apPassphrase);
  await click(buttonWithText("Continue"));
}

/** Fills step 3 (access) and moves to review. */
async function completeAccessStep(): Promise<void> {
  await fillField("#admin-password", validFields.adminPassword);
  await click(buttonWithText("Review setup"));
}

async function reachReview(): Promise<void> {
  await completeDeviceStep();
  await completeWifiStep();
  await completeAccessStep();
  await flushReact();
}

describe("v2 FirstRunSetupPage", () => {
  test("loads unauthenticated setup state and shows the form", async () => {
    const view = await renderSetupForm();
    expect(getFetchCalls()).toHaveLength(1);
    expect(document.body.textContent).toContain("Set up your keyboard");
    expect(document.body.textContent).toContain("Step 1 of 4: Device");
    expect(requiredElement("#setup-code", HTMLInputElement)).toBeDefined();
    expect(requiredElement("#device-name", HTMLInputElement).value).toBe(
      "ESP32 Macro Keyboard",
    );
    await view.unmount();
  });

  test("rejects a setup-state response with fields beyond provisioned and deviceName", async () => {
    planSetupGet({
      provisioned: false,
      deviceName: "ESP32 Macro Keyboard",
      apSsid: "unexpected",
    });
    const view = await render(
      <FirstRunSetupPage
        onSetupComplete={() => {
          /* not exercised in this test */
        }}
      />,
    );
    await flushReact();

    expect(document.body.textContent).toContain("Setup unavailable");
    expect(document.body.textContent).not.toContain("Set up your keyboard");
    await view.unmount();
  });

  test("shows a distinct message when the device is already provisioned", async () => {
    planFetch(() =>
      jsonResponse(
        { error: { code: "not_found", message: "Setup is unavailable." } },
        404,
      ),
    );
    const view = await render(
      <FirstRunSetupPage
        onSetupComplete={() => {
          /* not exercised in this test */
        }}
      />,
    );
    await flushReact();

    expect(document.body.textContent).toContain(
      "This device has already completed setup.",
    );
    await view.unmount();
  });

  test("keeps Continue disabled until the current step's fields are valid, one step at a time", async () => {
    const view = await renderSetupForm();
    const next = (): HTMLButtonElement => buttonWithText("Continue");
    expect(next().disabled).toBe(true);

    await fillField("#setup-code", "123");
    expect(next().disabled).toBe(true);

    await fillField("#setup-code", validFields.setupCode);
    // The device name arrives pre-filled from the device.
    expect(next().disabled).toBe(false);
    await click(next());
    expect(document.body.textContent).toContain("Step 2 of 4: Wi-Fi");

    expect(next().disabled).toBe(true);
    await fillField("#ap-ssid", validFields.apSsid);
    await fillField("#ap-passphrase", "short");
    expect(next().disabled).toBe(true);
    await fillField("#ap-passphrase", validFields.apPassphrase);
    await click(next());
    expect(document.body.textContent).toContain("Step 3 of 4: Access");

    expect(buttonWithText("Review setup").disabled).toBe(true);
    await fillField("#admin-password", validFields.adminPassword);
    expect(buttonWithText("Review setup").disabled).toBe(false);
    await view.unmount();
  });

  test("flags an invalid field beside it only after the user leaves it", async () => {
    const view = await renderSetupForm();
    const code = requiredElement("#setup-code", HTMLInputElement);
    await fillField("#setup-code", "123");
    expect(code.getAttribute("aria-invalid")).toBe("false");
    await act(async () => {
      code.dispatchEvent(new FocusEvent("focusout", { bubbles: true }));
      await Promise.resolve();
    });
    expect(code.getAttribute("aria-invalid")).toBe("true");
    await view.unmount();
  });

  test("Back keeps what was entered on the earlier step", async () => {
    const view = await renderSetupForm();
    await completeDeviceStep();
    await fillField("#ap-ssid", validFields.apSsid);
    await click(buttonWithText("Back"));
    expect(requiredElement("#setup-code", HTMLInputElement).value).toBe(
      validFields.setupCode,
    );
    await click(buttonWithText("Continue"));
    expect(requiredElement("#ap-ssid", HTMLInputElement).value).toBe(
      validFields.apSsid,
    );
    await view.unmount();
  });

  test("review step never displays the passphrase, admin password, or setup code", async () => {
    const view = await renderSetupForm();
    await reachReview();

    expect(document.body.textContent).toContain("Review setup");
    expect(document.body.textContent).toContain(validFields.apSsid);
    expect(document.body.textContent).toContain(validFields.deviceName);
    expect(document.body.textContent).not.toContain(validFields.apPassphrase);
    expect(document.body.textContent).not.toContain(validFields.adminPassword);
    expect(document.body.textContent).not.toContain(validFields.setupCode);
    await view.unmount();
  });

  test("applies setup, shows reconnect guidance, and hands off to Sign In", async () => {
    const onSetupComplete = vi.fn<() => void>();
    planSetupGet({ provisioned: false, deviceName: "ESP32 Macro Keyboard" });
    const view = await render(
      <FirstRunSetupPage onSetupComplete={onSetupComplete} />,
    );
    await flushReact();
    await reachReview();

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/setup");
      expect(call.method).toBe("POST");
      expect(call.body).toBe(
        JSON.stringify({
          setupCode: validFields.setupCode,
          deviceName: validFields.deviceName,
          apSsid: validFields.apSsid,
          apPassphrase: validFields.apPassphrase,
          adminPassword: validFields.adminPassword,
          requireSerialConfirmation: false,
        }),
      );
      return jsonResponse(
        {
          accepted: true,
          restartRequired: true,
          connectionWillClose: true,
          reprovisioningRequired: false,
        },
        202,
      );
    });
    await click(buttonWithText("Apply setup"));
    await flushReact();

    expect(document.body.textContent).toContain("Setup complete");
    expect(document.body.textContent).toContain(validFields.apSsid);
    expect(document.body.textContent).not.toContain(validFields.adminPassword);

    await click(buttonWithText("Continue to Sign In"));
    expect(onSetupComplete).toHaveBeenCalledTimes(1);
    await view.unmount();
  });

  test("keeps the working review on a submission failure and lets the user edit", async () => {
    const onSetupComplete = vi.fn<() => void>();
    planSetupGet({ provisioned: false, deviceName: "ESP32 Macro Keyboard" });
    const view = await render(
      <FirstRunSetupPage onSetupComplete={onSetupComplete} />,
    );
    await flushReact();
    await reachReview();

    planFetch(() =>
      jsonResponse(
        {
          error: {
            code: "invalid_field",
            message: "Device name exceeds 32 UTF-8 bytes.",
            field: "deviceName",
          },
        },
        422,
      ),
    );
    await click(buttonWithText("Apply setup"));
    await flushReact();

    expect(requiredElement("[role='alert']", HTMLElement).textContent).toBe(
      "invalid_field: Device name exceeds 32 UTF-8 bytes.",
    );
    expect(onSetupComplete).not.toHaveBeenCalled();
    expect(buttonWithText("Back")).toBeDefined();

    await click(buttonWithText("Back"));
    expect(requiredElement("#admin-password", HTMLInputElement).value).toBe(
      validFields.adminPassword,
    );
    await view.unmount();
  });
});
