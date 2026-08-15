import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import AppV2, { AuthenticatedShell } from "../src/AppV2";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import { getFetchCalls, jsonResponse, planFetch } from "./fakeFetch";
import {
  buttonWithText,
  click,
  render,
  requiredElement,
  setInputValue,
} from "./render";
import {
  bodyIncludes,
  planRace,
  settingsBody,
  signIn,
  statusBody,
  submitForm,
  tick,
  validSession,
  waitUntil,
} from "./appV2Harness";

beforeEach(() => {
  vi.useFakeTimers();
});

afterEach(() => {
  vi.useRealTimers();
});

describe("AppV2 — wiring RepositoryStartupScreen into the running app shell", () => {
  test("execution recovery failure stays visible and Retry resolves it without discarding the working copy", async () => {
    const packageId = "4a929c71-d7cc-40ef-982e-68cb00676bde";
    const store = createRepositoryWorkingCopyStore({
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [{ id: packageId, name: "Recovery Package", macros: [] }],
    });
    planRace(
      {
        "/api/v1/settings": () => jsonResponse(settingsBody),
        "/api/v1/status": () => jsonResponse(statusBody),
      },
      2,
    );
    const view = await render(
      <AuthenticatedShell
        ready={{
          store,
          packageId,
          sendRecovery: {
            kind: "unavailable",
            message: "send recovery network failure",
          },
          loadedBlobId: "7",
        }}
      />,
    );
    await waitUntil(
      () => bodyIncludes("Execution state unavailable."),
      "execution state warning",
    );
    expect(document.body.textContent).toContain("Recovery Package");
    expect(document.body.textContent).toContain(
      "An active send may still be running on the device.",
    );

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/send");
      expect(call.method).toBe("GET");
      return jsonResponse(
        { error: { code: "not_found", message: "No send exists." } },
        404,
      );
    });
    await click(buttonWithText("Retry execution status"));
    await waitUntil(
      () => !bodyIncludes("Execution state unavailable."),
      "execution state warning to clear",
    );
    expect(document.body.textContent).toContain("Recovery Package");
    await view.unmount();
  });

  test("stale USB readiness becomes visible, disables send, and recovers after a good poll", async () => {
    const packageId = "550e8400-e29b-41d4-a716-446655440000";
    const store = createRepositoryWorkingCopyStore({
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [
        {
          id: packageId,
          name: "USB Freshness Package",
          macros: [
            {
              id: "6ba7b810-9dad-41d1-80b4-00c04fd430c8",
              name: "Macro A",
              source: "a",
              keyPressMs: 8,
              interKeyMs: 15,
            },
          ],
        },
      ],
    });
    planRace(
      {
        "/api/v1/settings": () => jsonResponse(settingsBody),
        "/api/v1/status": () => jsonResponse(statusBody),
      },
      2,
    );
    const view = await render(
      <AuthenticatedShell
        ready={{
          store,
          packageId,
          sendRecovery: { kind: "none" },
          loadedBlobId: "7",
        }}
      />,
    );
    await waitUntil(() => bodyIncludes("Macro A"), "Macro A");
    expect(buttonWithText("Send").disabled).toBe(false);

    for (let failure = 0; failure < 3; failure += 1) {
      planFetch((call) => {
        expect(call.url).toBe("/api/v1/status");
        expect(call.method).toBe("GET");
        throw new TypeError("device status unavailable");
      });
      await tick(5000);
    }

    await waitUntil(
      () => bodyIncludes("USB status is stale."),
      "stale USB warning",
    );
    expect(document.body.textContent).toContain("Last known state: ready.");
    expect(buttonWithText("Send").disabled).toBe(true);

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/status");
      expect(call.method).toBe("GET");
      return jsonResponse(statusBody);
    });
    await tick(5000);
    await waitUntil(
      () => !bodyIncludes("USB status is stale."),
      "stale USB warning to clear",
    );
    expect(buttonWithText("Send").disabled).toBe(false);
    await view.unmount();
  });

  test("unprovisioned device opens First-run setup", async () => {
    // AppV2's own provisioning probe, then FirstRunSetupPage's independent
    // load of the same unauthenticated state (device name/setup-code entry).
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/setup");
      return jsonResponse({
        provisioned: false,
        deviceName: "Desk Macro Keyboard",
      });
    });
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/setup");
      return jsonResponse({
        provisioned: false,
        deviceName: "Desk Macro Keyboard",
      });
    });
    const view = await render(<AppV2 />);
    await waitUntil(() => bodyIncludes("First-run setup"), "First-run setup");
    await view.unmount();
  });

  test("device unreachable at the setup check shows a retry screen", async () => {
    planFetch(() => {
      throw new TypeError("Failed to fetch");
    });
    const view = await render(<AppV2 />);
    await waitUntil(
      () => bodyIncludes("Device unreachable"),
      "device unreachable",
    );
    await view.unmount();
  });

  test("Sign In -> repository startup -> the Macros page, with no dedicated progress route", async () => {
    const view = await signIn();
    expect(bodyIncludes("Macros")).toBe(true);
    expect(bodyIncludes("This selection may not survive a reload.")).toBe(
      false,
    );
    expect(bodyIncludes("0 macros")).toBe(true);
    // The bottom navigation is the real one; a standalone send-progress
    // route is not part of it (Phase 9 exit gate).
    const navButtons = document.querySelectorAll(
      'nav[aria-label="Primary navigation"] button',
    );
    expect(navButtons).toHaveLength(4);
    expect(Array.from(navButtons).map((button) => button.textContent)).toEqual([
      "Macros",
      "Packages",
      "Snapshots",
      "Settings",
    ]);
    // Every later test in this file that navigates by hash relies on there
    // being exactly one `AuthenticatedShell` listening for `hashchange` at a
    // time — an un-unmounted tree left running here would otherwise still
    // hold its own `useDeviceStatus` poll interval and `hashchange`
    // listener, competing for the same fake-fetch queue against tests that
    // render a fresh `AppV2` later.
    await view.unmount();
  });

  test("failed first-package selection persistence stays visible after local open and Retry clears it", async () => {
    const view = await signIn({
      firstPackagePersistenceError: new TypeError("settings write unavailable"),
    });

    expect(bodyIncludes("My First Package")).toBe(true);
    expect(bodyIncludes("This selection may not survive a reload.")).toBe(true);
    expect(bodyIncludes("settings write unavailable")).toBe(true);
    buttonWithText("Retry saving selection");

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/settings");
      expect(call.method).toBe("PUT");
      return jsonResponse({
        settings: settingsBody,
        restartRequired: false,
        reconnectRequired: false,
      });
    });
    await click(buttonWithText("Retry saving selection"));
    await waitUntil(
      () => !bodyIncludes("This selection may not survive a reload."),
      "selection persistence warning to clear",
    );

    expect(bodyIncludes("My First Package")).toBe(true);
    await view.unmount();
  });

  test("session expiry drops back to Sign In without discarding the working copy", async () => {
    const view = await signIn();
    const blobCallsBefore = getFetchCalls().filter(
      (call) => call.url === "/api/v1/blob",
    ).length;

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/status");
      planFetch((sessionCall) => {
        expect(sessionCall.url).toBe("/api/v1/auth/session");
        expect(sessionCall.method).toBe("GET");
        return jsonResponse(
          { error: { code: "unauthorized", message: "Sign in required." } },
          401,
        );
      });
      return jsonResponse(
        { error: { code: "unauthorized", message: "Session expired." } },
        401,
      );
    });
    await tick(5_000);
    await waitUntil(
      () => bodyIncludes("Sign in"),
      "Sign In after session expiry",
    );

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/auth/login");
      return jsonResponse(validSession);
    });
    planRace(
      {
        "/api/v1/settings": () => jsonResponse(settingsBody),
        "/api/v1/status": () => jsonResponse(statusBody),
      },
      2,
    );
    await setInputValue(
      requiredElement("#admin-password", HTMLInputElement),
      "correct horse battery staple",
    );
    await submitForm();
    await waitUntil(
      () => bodyIncludes("My First Package"),
      "the shell resuming with the preserved working copy",
    );

    expect(bodyIncludes("Create Your First Repository")).toBe(false);
    const blobCallsAfter = getFetchCalls().filter(
      (call) => call.url === "/api/v1/blob",
    ).length;
    expect(blobCallsAfter).toBe(blobCallsBefore);
    await view.unmount();
  });
});
