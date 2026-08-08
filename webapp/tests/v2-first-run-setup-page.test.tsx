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
  submit,
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

async function fillValidForm(): Promise<void> {
  await setInputValue(
    requiredElement("#setup-code", HTMLInputElement),
    validFields.setupCode,
  );
  await setInputValue(
    requiredElement("#device-name", HTMLInputElement),
    validFields.deviceName,
  );
  await setInputValue(
    requiredElement("#ap-ssid", HTMLInputElement),
    validFields.apSsid,
  );
  await setInputValue(
    requiredElement("#ap-passphrase", HTMLInputElement),
    validFields.apPassphrase,
  );
  await setInputValue(
    requiredElement("#admin-password", HTMLInputElement),
    validFields.adminPassword,
  );
}

async function reachReview(): Promise<void> {
  await fillValidForm();
  await submit(requiredElement("form", HTMLFormElement));
  await flushReact();
}

describe("v2 FirstRunSetupPage", () => {
  test("loads unauthenticated setup state and shows the form", async () => {
    const view = await renderSetupForm();
    expect(getFetchCalls()).toHaveLength(1);
    expect(document.body.textContent).toContain("First-run setup");
    expect(document.body.textContent).toContain("ESP32 Macro Keyboard");
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
    expect(document.body.textContent).not.toContain("First-run setup");
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

  test("keeps setup review disabled until every field is valid", async () => {
    const view = await renderSetupForm();
    const reviewButton = buttonWithText("Review setup");
    expect(reviewButton.disabled).toBe(true);

    await setInputValue(
      requiredElement("#setup-code", HTMLInputElement),
      "123",
    );
    expect(reviewButton.disabled).toBe(true);

    await fillValidForm();
    expect(reviewButton.disabled).toBe(false);
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
    expect(buttonWithText("Edit")).toBeDefined();

    await click(buttonWithText("Edit"));
    expect(requiredElement("#device-name", HTMLInputElement).value).toBe(
      validFields.deviceName,
    );
    await view.unmount();
  });
});
