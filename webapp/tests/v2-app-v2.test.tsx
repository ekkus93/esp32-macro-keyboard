import { act } from "react";
import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import AppV2, { AuthenticatedShell } from "../src/AppV2";
import { landscapePhoneMediaQuery } from "../src/features/shell/v2/useLandscapePhoneBlock";
import { v2Limits } from "../src/v2/limits";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { FetchCall } from "./fakeFetch";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";
import { installFakeMatchMedia } from "./fakeMatchMedia";
import {
  buttonWithText,
  click,
  render,
  requiredElement,
  setInputValue,
} from "./render";
import type { RenderResult } from "./render";

/**
 * V2-090 integration coverage: this is the actual composition wired into
 * `main.tsx` (RepositoryStartupScreen's `onReady` handed into a running
 * application shell — the item Phase 8 deliberately left open, and the
 * unblocking work TODO_V2 names explicitly). Exercises the real, unmocked
 * v2 API clients end to end via the fake-fetch harness, the same
 * request/response contract a browser would see, rather than injecting
 * fakes at the page level. Only the "no stored blobs yet" startup path is
 * covered here — it is the only path that involves no gzip/libuv work,
 * which keeps this test compatible with fully faked timers throughout.
 * `RepositoryStartupScreen`'s own test suite already covers the blob-load
 * paths in isolation.
 */

async function tick(ms = 0): Promise<void> {
  await act(async () => {
    await vi.advanceTimersByTimeAsync(ms);
  });
}

function bodyIncludes(text: string): boolean {
  return document.body.textContent?.includes(text) === true;
}

/**
 * Polls by repeatedly flushing microtasks until `predicate` is true, rather
 * than guessing a fixed number of flushes. A React state update landing
 * inside a resolved promise chain can take more than one `advanceTimersByTimeAsync(0)`
 * round to fully commit and re-run a newly mounted component's own effects
 * (for example `FirstRunSetupPage`'s independent load of `/api/v1/setup`).
 */
async function waitUntil(
  predicate: () => boolean,
  description: string,
): Promise<void> {
  for (let attempt = 0; attempt < 30; attempt += 1) {
    if (predicate()) {
      return;
    }
    await tick(0);
  }
  const recentFetches = getFetchCalls()
    .slice(-10)
    .map((call) => `${call.method} ${call.url}`)
    .join(", ");
  throw new Error(
    `Timed out waiting for: ${description}; body=${document.body.textContent ?? ""}; recentFetches=${recentFetches}`,
  );
}

const settingsBody = {
  deviceName: "Desk Macro Keyboard",
  requireSerialConfirmation: false,
  sendMode: "quick",
  snapshotRetentionTarget: 5,
  showMacroSourcePreviews: false,
  lastSelectedPackageId: null,
  apSsid: "MacroKeyboard",
  stationConfigured: false,
  stationSsid: null,
};

const statusBody = {
  provisioned: true,
  deviceName: "Desk Macro Keyboard",
  firmwareVersion: "0.2.0",
  buildId: "abc123",
  uptimeMs: 1000,
  usb: { state: "ready" },
  accessPoint: { state: "started", ssid: "MacroKeyboard", clientCount: 0 },
  station: { configured: false, state: "idle", ssid: null, ipv4: null },
  storage: {
    state: "healthy",
    totalBytes: 100,
    usedBytes: 0,
    remainingBytes: 100,
    blobCount: 0,
  },
  send: { present: false, state: null },
};

const validSession = {
  authenticated: true,
  idleExpiresInSeconds: v2Limits.sessionIdleLifetimeSeconds,
  absoluteExpiresInSeconds: v2Limits.sessionAbsoluteLifetimeSeconds,
};

/**
 * Responds to whichever of a known set of GETs arrives next, without
 * assuming a fixed order between concurrently mounted effects (the app
 * shell's own settings fetch and the Macros page's device-status poll both
 * fire from the same React commit; their real request order is an
 * implementation detail, not a contract this test should pin down).
 */
function planRace(byUrl: Record<string, () => Response>, times: number): void {
  for (let i = 0; i < times; i += 1) {
    planFetch((call: FetchCall) => {
      const handler = byUrl[call.url];
      if (handler === undefined) {
        throw new Error(
          `Unplanned concurrent call: ${call.method} ${call.url}`,
        );
      }
      return handler();
    });
  }
}

/**
 * Dispatches the current form's submit event and flushes every microtask it
 * triggers (not just one) — `planFetch` responses queued beforehand are
 * consumed in call order regardless of how many awaits separate them, so
 * every fetch a given user action will cause is planned before the action
 * fires, then flushed thoroughly here.
 */
async function submitForm(): Promise<void> {
  await act(async () => {
    requiredElement("form", HTMLFormElement).dispatchEvent(
      new SubmitEvent("submit", { bubbles: true, cancelable: true }),
    );
    await vi.advanceTimersByTimeAsync(0);
  });
}

/**
 * Returns the `render` result so callers that need to avoid leaving a
 * background `useDeviceStatus` poll interval running into later tests (the
 * Phase 12 device-action tests below, which drive fake timers across a real
 * reconnect sequence) can `unmount()` when done. Existing callers that don't
 * need this simply ignore the return value.
 */
interface SignInOptions {
  firstPackagePersistenceError?: Error;
}

async function signIn(options: SignInOptions = {}): Promise<RenderResult> {
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/setup");
    expect(call.method).toBe("GET");
    return jsonResponse(
      { error: { code: "not_found", message: "Already provisioned." } },
      404,
    );
  });
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/auth/session");
    expect(call.method).toBe("GET");
    return jsonResponse(
      { error: { code: "unauthorized", message: "Sign in required." } },
      401,
    );
  });
  const view = await render(<AppV2 />);
  await waitUntil(() => bodyIncludes("Sign in"), "the Sign In form");

  // Planned upfront, in the exact sequential order the login submission
  // will trigger: POST login, then RepositoryStartupScreen's own settings
  // and blob-list GETs once it mounts.
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/auth/login");
    expect(call.method).toBe("POST");
    return jsonResponse(validSession);
  });
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/settings");
    expect(call.method).toBe("GET");
    return jsonResponse(settingsBody);
  });
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/blob");
    expect(call.method).toBe("GET");
    return jsonResponse({ blobs: [], usedBytes: 0, remainingBytes: 100 });
  });
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/send");
    expect(call.method).toBe("GET");
    return jsonResponse(
      { error: { code: "not_found", message: "No send exists." } },
      404,
    );
  });
  await setInputValue(
    requiredElement("#admin-password", HTMLInputElement),
    "correct horse battery staple",
  );
  await submitForm();
  await waitUntil(
    () => bodyIncludes("Create Your First Repository"),
    "Create Your First Repository",
  );

  await setInputValue(
    requiredElement("#first-package-name", HTMLInputElement),
    "My First Package",
  );
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/settings");
    expect(call.method).toBe("PUT");
    if (options.firstPackagePersistenceError !== undefined) {
      throw options.firstPackagePersistenceError;
    }
    return jsonResponse({
      settings: settingsBody,
      restartRequired: false,
      reconnectRequired: false,
    });
  });
  planRace(
    {
      "/api/v1/settings": () => jsonResponse(settingsBody),
      "/api/v1/status": () => jsonResponse(statusBody),
    },
    2,
  );
  await submitForm();
  await waitUntil(
    () => bodyIncludes("My First Package"),
    "the authenticated shell showing the new package",
  );
  return view;
}

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

describe("AppV2 — Phase 10 wiring (macro editor and Packages, TODO_V2 V2-100/V2-101/V2-102/V2-103)", () => {
  test("Add macro -> Save changes returns to the Macros page, shows the new macro, and marks the header dirty", async () => {
    const view = await signIn();
    // Creating the first package (inside signIn()) is itself a repository
    // content change, so the working copy is already dirty here (SPEC_V2
    // §8.6) — the assertion below confirms the header keeps showing that,
    // not that this specific edit is what caused it.
    expect(bodyIncludes("Unsaved changes")).toBe(true);

    await click(buttonWithText("Add macro"));
    await waitUntil(() => bodyIncludes("Create macro"), "the macro editor");

    await setInputValue(
      requiredElement("#macro-editor-name", HTMLInputElement),
      "Open terminal",
    );
    await setInputValue(
      requiredElement("#macro-editor-source", HTMLTextAreaElement),
      "a",
    );
    await click(buttonWithText("Save changes"));

    await waitUntil(
      () => bodyIncludes("Open terminal"),
      "the Macros page showing the new macro",
    );
    expect(bodyIncludes("Unsaved changes")).toBe(true);
    expect(bodyIncludes("Save snapshot")).toBe(true);
    await view.unmount();
  });

  test("Packages -> Open switches the selected package, persists the change, and does not add a further dirty transition", async () => {
    const view = await signIn();

    await click(buttonWithText("Packages"));
    await waitUntil(() => bodyIncludes("Create package"), "the Packages page");
    expect(bodyIncludes("My First Package")).toBe(true);

    // A second package to actually switch to — opening the package that is
    // already selected would never call the settings PUT at all
    // (`persistSelectedPackageId` short-circuits on a no-op selection).
    await setInputValue(
      requiredElement("#package-management-create-name", HTMLInputElement),
      "Second Package",
    );
    await click(buttonWithText("Create package"));
    await waitUntil(
      () => bodyIncludes("Second Package"),
      "the newly created second package",
    );
    // Creating a package is itself a content change (SPEC_V2 §8.6), on top
    // of the already-dirty working copy from creating the first package.
    expect(bodyIncludes("Unsaved changes")).toBe(true);

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/settings");
      expect(call.method).toBe("PUT");
      return jsonResponse({
        settings: settingsBody,
        restartRequired: false,
        reconnectRequired: false,
      });
    });
    await click(
      requiredElement('[aria-label="Open Second Package"]', HTMLButtonElement),
    );

    await waitUntil(
      () => bodyIncludes("Second Package") && bodyIncludes("0 macros"),
      "back on the Macros page for the opened package",
    );
    // The header still shows the dirty state from the two content changes
    // above through a real package switch and re-render — confirming this
    // end-to-end wiring, on top of `PackageManagementPage`'s own unit test
    // ("Open ... does not dirty the repository") which proves the no-op
    // case from a clean baseline.
    expect(bodyIncludes("Unsaved changes")).toBe(true);
    await view.unmount();
  });
});

describe("AppV2 — Phase 12 wiring (Settings, Diagnostics, Sign Out, restart reconnect; TODO_V2 V2-120/V2-121/V2-122/V2-103)", () => {
  test("Settings -> Diagnostics -> Back to Settings", async () => {
    const view = await signIn();
    await click(buttonWithText("Settings"));
    await waitUntil(
      () => document.querySelector("#settings-device-name") !== null,
      "the Settings page",
    );

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/diagnostics");
      expect(call.method).toBe("GET");
      return jsonResponse({
        firmwareVersion: "0.2.0",
        buildId: "git-abcdef0",
        resetReason: "power_on",
        uptimeMs: 123456,
        memory: {
          freeHeapBytes: 200000,
          minimumFreeHeapBytes: 180000,
          largestFreeBlockBytes: 120000,
        },
        usb: { state: "ready" },
        wifi: { accessPointState: "running", stationState: "disabled" },
        storage: {
          state: "ready",
          webfsTotalBytes: 1048576,
          webfsUsedBytes: 500000,
          userdataTotalBytes: 524288,
          userdataUsedBytes: 4096,
          blobCount: 1,
          invalidNames: [],
          temporaryFiles: [],
        },
        send: { present: false, state: null },
        subsystems: [],
      });
    });
    await click(buttonWithText("View diagnostics"));
    await waitUntil(() => bodyIncludes("git-abcdef0"), "the diagnostics page");
    // Diagnostics is reachable only from Settings, not its own bottom-nav
    // destination (UI_UX_SPEC_V2 §4/§11) — the fixed nav still shows exactly
    // the four destinations.
    const navButtons = document.querySelectorAll(
      'nav[aria-label="Primary navigation"] button',
    );
    expect(Array.from(navButtons).map((button) => button.textContent)).toEqual([
      "Macros",
      "Packages",
      "Snapshots",
      "Settings",
    ]);

    await click(buttonWithText("Back to Settings"));
    await waitUntil(
      () => document.querySelector("#settings-device-name") !== null,
      "back on the Settings page",
    );
    await view.unmount();
  });

  test("Sign out (V2-103's fifth warning trigger) warns first because signIn() leaves the working copy dirty; Discard changes proceeds, ends the session, and a fresh sign-in recovers cleanly", async () => {
    // Deliberately exercises Discard, not Save snapshot: `saveWorkingCopyAsSnapshot`
    // compresses with the real `CompressionStream` API (`v2/gzip.ts`), which
    // this file's fake timers (needed for the restart-reconnect test below)
    // cannot reliably drive to completion under load — `v2-snapshots-page.test.tsx`
    // and `v2-snapshot-client.test.ts` already cover that save path under
    // real timers, and `SettingsPage`'s own unit test
    // (`v2-settings-page.test.tsx`) covers Save-snapshot-then-sign-out with
    // an injected fake `signOut`. This path also proves the stronger claim
    // SPEC_V2 §8.7 makes: "the UI MUST NOT claim that a closed dirty working
    // copy can be recovered."
    const view = await signIn();
    // signIn() creates the first package, which is itself a repository
    // content change (SPEC_V2 §8.6) — the working copy is already dirty
    // here, exactly the case V2-103's Sign Out warning trigger must cover.
    expect(bodyIncludes("Unsaved changes")).toBe(true);

    await click(buttonWithText("Settings"));
    await waitUntil(
      () => document.querySelector("#settings-device-name") !== null,
      "the Settings page",
    );

    await click(buttonWithText("Sign out"));
    await waitUntil(
      () => bodyIncludes("Continuing to sign out"),
      "the unsaved-changes warning before sign-out",
    );

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/auth/logout");
      expect(call.method).toBe("POST");
      return new Response(null, { status: 204 });
    });
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/auth/session");
      expect(call.method).toBe("GET");
      return jsonResponse(
        { error: { code: "unauthorized", message: "Sign in required." } },
        401,
      );
    });
    await click(buttonWithText("Discard changes"));
    await waitUntil(() => bodyIncludes("Sign in"), "Sign In after sign-out");

    // A fresh sign-in afterward succeeds and lands back in the authenticated
    // shell — the session truly ended and the application recovers cleanly
    // rather than getting stuck, resuming the same in-tab `ready` (no
    // RepositoryStartupScreen/blob-list re-run, exactly session expiry's
    // path per SPEC_V2 §7.3).
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
      () => document.querySelector("#settings-device-name") !== null,
      "back in the authenticated shell (still on the Settings route) after sign-out",
    );
    expect(bodyIncludes("Sign in")).toBe(false);
    await view.unmount();
  });

  test("Restart shows a real reconnecting screen, then Sign In once the device answers unauthorized, resuming the same working copy", async () => {
    const view = await signIn();

    await click(buttonWithText("Settings"));
    await waitUntil(
      () => document.querySelector("#settings-device-name") !== null,
      "the Settings page",
    );
    await click(buttonWithText("Restart"));
    await waitUntil(
      () => bodyIncludes("Restart the device?"),
      "the restart confirmation",
    );

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/device/restart");
      expect(call.method).toBe("POST");
      return jsonResponse({
        accepted: true,
        connectionWillClose: true,
        reprovisioningRequired: false,
      });
    });
    // The device's AP briefly drops for the reboot: the first reachability
    // poll fails at the network layer, exactly as a real `fetch` would.
    planFetch(() => {
      throw new TypeError("Failed to fetch");
    });
    await click(buttonWithText("Restart now"));
    await waitUntil(() => bodyIncludes("Reconnecting"), "the reconnect screen");
    expect(bodyIncludes("restarting")).toBe(true);

    // The device is back, but its RAM-only session is gone (a reboot always
    // clears it) — SPEC_V2 §7.3's "session expiry does not discard the
    // in-memory working copy" is exactly the path this now takes.
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/status");
      return jsonResponse(
        { error: { code: "unauthorized", message: "Session expired." } },
        401,
      );
    });
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/auth/session");
      expect(call.method).toBe("GET");
      return jsonResponse(
        { error: { code: "unauthorized", message: "Sign in required." } },
        401,
      );
    });
    await tick(1000);
    await waitUntil(() => bodyIncludes("Sign in"), "Sign In after reconnect");

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
      "the shell resuming with the preserved working copy after restart",
    );
    await view.unmount();
  });
});

describe("AppV2 — V2-131/V2-132 phone-landscape orientation surface (UI_UX_SPEC_V2 §12)", () => {
  test("hides the app behind Rotate your phone in landscape, and restores the exact route and dirty state on return — no reload, no re-fetch, no lost draft", async () => {
    const fakeMedia = installFakeMatchMedia();
    const view = await signIn();
    // Creating the first package inside signIn() already dirties the
    // working copy (SPEC_V2 §8.6) — this is the "dirty working copy" state
    // UI_UX_SPEC_V2 §12.2 requires surviving the round trip.
    expect(bodyIncludes("Unsaved changes")).toBe(true);

    // Navigate off the default route so "restores the exact route" is
    // actually meaningful to check, not trivially true of the landing page.
    await click(buttonWithText("Snapshots"));
    await waitUntil(() => bodyIncludes("Snapshots"), "the Snapshots page");
    expect(bodyIncludes("Rotate your phone")).toBe(false);

    const blobGetsBeforeLandscape = getFetchCalls().filter(
      (call) => call.url === "/api/v1/blob" && call.method === "GET",
    ).length;

    act(() => {
      fakeMedia.set(landscapePhoneMediaQuery, true);
    });
    expect(bodyIncludes("Rotate your phone")).toBe(true);
    expect(
      bodyIncludes("ESP32 Macro Keyboard is designed for portrait mode."),
    ).toBe(true);
    // The ordinary shell is hidden, not discarded: it is still in the DOM
    // (an ancestor is `display: none`) — a reload or a fresh mount would
    // have removed it entirely, not merely hidden it.
    expect(document.querySelector(".app-shell")).not.toBeNull();

    act(() => {
      fakeMedia.set(landscapePhoneMediaQuery, false);
    });
    expect(bodyIncludes("Rotate your phone")).toBe(false);
    // Exact same route and dirty state as before the landscape excursion.
    expect(bodyIncludes("Snapshots")).toBe(true);
    expect(bodyIncludes("Unsaved changes")).toBe(true);
    // No re-fetch of the repository blob happened — nothing reloaded or
    // remounted `RepositoryStartupScreen`, which is the only place that GET
    // is issued from.
    const blobGetsAfterLandscape = getFetchCalls().filter(
      (call) => call.url === "/api/v1/blob" && call.method === "GET",
    ).length;
    expect(blobGetsAfterLandscape).toBe(blobGetsBeforeLandscape);
    await view.unmount();
  });

  test("an active send's macro name, progress, and Cancel remain accessible while landscape-blocked", async () => {
    const fakeMedia = installFakeMatchMedia();
    const view = await signIn();

    await click(buttonWithText("Add macro"));
    await waitUntil(() => bodyIncludes("Create macro"), "the macro editor");
    await setInputValue(
      requiredElement("#macro-editor-name", HTMLInputElement),
      "Open terminal",
    );
    await setInputValue(
      requiredElement("#macro-editor-source", HTMLTextAreaElement),
      "a",
    );
    await click(buttonWithText("Save changes"));
    await waitUntil(
      () => bodyIncludes("Open terminal"),
      "the Macros page showing the new macro",
    );

    const sendId = "22222222-2222-4222-8222-222222222222";
    planFetch((call) => {
      expect(call.url).toBe("/api/v1/send");
      expect(call.method).toBe("POST");
      return jsonResponse(
        {
          id: sendId,
          state: "running",
          actionCount: 1,
          estimatedDurationMs: 50,
        },
        202,
      );
    });
    await click(buttonWithText("Send"));
    await tick(0);
    expect(bodyIncludes("Sending Open terminal")).toBe(true);

    act(() => {
      fakeMedia.set(landscapePhoneMediaQuery, true);
    });
    expect(bodyIncludes("Rotate your phone")).toBe(true);
    // UI_UX_SPEC_V2 §12.3: macro name, progress, and Cancel and release all
    // keys remain visible and actionable on the orientation surface itself.
    expect(bodyIncludes("Sending Open terminal")).toBe(true);
    expect(bodyIncludes("Cancel and release all keys")).toBe(true);

    // Specifically the button rendered inside the orientation surface, not
    // whichever "Cancel and release all keys" button happens to come first
    // in DOM order (the ordinary, now-hidden Macros page has its own) — this
    // proves the control the user can actually see and reach in landscape
    // is real and wired, not merely that the text string exists somewhere
    // in a hidden part of the document.
    const overlay = requiredElement(".landscape-block", HTMLElement);
    const overlayCancelButton = Array.from(
      overlay.querySelectorAll("button"),
    ).find(
      (button) => button.textContent?.trim() === "Cancel and release all keys",
    );
    expect(overlayCancelButton).toBeDefined();

    planFetch((call) => {
      expect(call.method).toBe("DELETE");
      expect(call.url).toBe("/api/v1/send");
      return jsonResponse({ id: sendId }, 202);
    });
    if (overlayCancelButton === undefined) {
      throw new Error("unreachable: asserted above");
    }
    await click(overlayCancelButton);

    planJsonResponse(
      {
        id: sendId,
        state: "cancelled",
        actionIndex: 0,
        actionCount: 1,
        estimatedDurationMs: 50,
        cancellationRequested: true,
        error: "",
        releaseError: "",
      },
      200,
    );
    await tick(1000);
    expect(bodyIncludes("Send Open terminal was cancelled.")).toBe(true);
    await view.unmount();
  });
});
