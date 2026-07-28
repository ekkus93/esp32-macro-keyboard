import { describe, expect, test } from "vitest";
import App from "../src/App";
import { apiRequest } from "../src/api/client";
import { isEmptyRecord } from "../src/api/guards";
import {
  deviceStatus,
  planAuthenticatedBootstrap,
  planNormalUnauthenticatedBootstrap,
  planPostLoginBootstrap,
} from "./appFixtures";
import { getFetchCalls, planFetch, planJsonResponse } from "./fakeFetch";
import { setHashSilently } from "./fakeLocation";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setInputValue,
  submit,
} from "./render";

async function renderLogin(): Promise<Awaited<ReturnType<typeof render>>> {
  setHashSilently("/login");
  planNormalUnauthenticatedBootstrap();
  const view = await render(<App />);
  await flushReact();
  await setInputValue(
    requiredElement("#password", HTMLInputElement),
    "correct horse",
  );
  return view;
}

describe("application authentication", () => {
  test("redirects setup-mode devices to a real setup form", async () => {
    planJsonResponse({
      ok: true,
      data: {
        deviceId: "device-123",
        apSsid: "Macro Keyboard Setup",
        completed: false,
        physicalConfirmationRequired: true,
      },
    });
    const view = await render(<App />);
    await flushReact();

    expect(document.body.textContent).toContain("First-run setup");
    expect(document.body.textContent).toContain("device-123");
    expect(requiredElement("#setup-code", HTMLInputElement)).toBeDefined();
    expect(getFetchCalls()).toHaveLength(1);
    await view.unmount();
  });

  test("loads status and session before showing login", async () => {
    const view = await renderLogin();
    expect(getFetchCalls().map((call) => call.url)).toEqual([
      "/api/v1/setup-state",
      "/api/v1/status",
      "/api/v1/auth/session",
    ]);
    expect(document.body.textContent).toContain("Administrator password");
    await view.unmount();
  });

  test("submits the password, stores CSRF, and loads live sets", async () => {
    const view = await renderLogin();
    planPostLoginBootstrap();
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();

    const loginCall = getFetchCalls()[3];
    expect(loginCall?.url).toBe("/api/v1/auth/login");
    expect(loginCall?.method).toBe("POST");
    expect(loginCall?.body).toBe(JSON.stringify({ password: "correct horse" }));
    expect(document.body.textContent).toContain("Choose a macro set");
    expect(document.body.textContent).toContain("Lab Chromebook workflow");

    planJsonResponse({ ok: true, data: {} });
    await apiRequest(
      "/api/v1/test-mutation",
      { method: "POST", body: "{}" },
      isEmptyRecord,
    );
    expect(getFetchCalls().at(-1)?.headers.get("X-CSRF-Token")).toBe(
      "csrf-123",
    );
    await view.unmount();
  });

  test("displays rate-limit retry time", async () => {
    const view = await renderLogin();
    planFetch(
      () =>
        new Response(
          JSON.stringify({
            ok: false,
            error: {
              code: "rate_limited",
              message: "Too many attempts.",
            },
          }),
          {
            status: 429,
            headers: {
              "Content-Type": "application/json",
              "Retry-After": "12",
            },
          },
        ),
    );
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();

    expect(
      requiredElement("[role='alert']", HTMLElement).textContent,
    ).toContain("rate_limited: Too many attempts.");
    expect(document.body.textContent).toContain("Try again in 12 seconds.");
    await view.unmount();
  });

  test("logs out, clears the session boundary, and returns to login", async () => {
    setHashSilently("/sets");
    planAuthenticatedBootstrap();
    const view = await render(<App />);
    await flushReact();

    planJsonResponse({ ok: true, data: {} });
    await click(buttonWithText("Sign out"));
    await flushReact();

    const logoutCall = getFetchCalls().at(-1);
    expect(logoutCall?.url).toBe("/api/v1/auth/logout");
    expect(logoutCall?.method).toBe("POST");
    expect(logoutCall?.headers.get("X-CSRF-Token")).toBe("csrf-restored");
    expect(document.body.textContent).toContain("Administrator password");
    await view.unmount();
  });

  test("submits setup credentials and schedules restart", async () => {
    planJsonResponse({
      ok: true,
      data: {
        deviceId: "device-123",
        apSsid: "Macro Keyboard Setup",
        completed: false,
        physicalConfirmationRequired: false,
      },
    });
    const view = await render(<App />);
    await flushReact();

    await setInputValue(
      requiredElement("#setup-code", HTMLInputElement),
      "setup-code",
    );
    await setInputValue(
      requiredElement("#ap-passphrase", HTMLInputElement),
      "network-password",
    );
    await setInputValue(
      requiredElement("#administrator-password", HTMLInputElement),
      "administrator-password",
    );

    planJsonResponse(
      {
        ok: true,
        data: {
          deviceId: "device-123",
          apSsid: "Macro Keyboard Setup",
          completed: true,
          physicalConfirmationRequired: false,
        },
      },
      201,
    );
    await submit(requiredElement("form", HTMLFormElement));
    await flushReact();
    expect(document.body.textContent).toContain("Setup complete");

    planJsonResponse(
      {
        ok: true,
        data: { restartScheduled: true },
      },
      202,
    );
    await click(buttonWithText("Restart device"));
    await flushReact();
    expect(getFetchCalls().at(-1)?.url).toBe("/api/v1/setup/restart");
    await view.unmount();
  });

  test("shows device status after authentication", async () => {
    setHashSilently("/sets");
    planAuthenticatedBootstrap({ usbState: deviceStatus.usbState });
    const view = await render(<App />);
    await flushReact();
    expect(document.body.textContent).toContain("USB ready");
    await view.unmount();
  });
});
