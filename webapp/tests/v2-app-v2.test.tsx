import { act } from "react";
import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import AppV2 from "../src/AppV2";
import { v2Limits } from "../src/v2/limits";
import type { FetchCall } from "./fakeFetch";
import { getFetchCalls, jsonResponse, planFetch } from "./fakeFetch";
import {
  buttonWithText,
  click,
  render,
  requiredElement,
  setInputValue,
} from "./render";

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
  throw new Error(`Timed out waiting for: ${description}`);
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

async function signIn(): Promise<void> {
  planFetch((call) => {
    expect(call.url).toBe("/api/v1/setup");
    expect(call.method).toBe("GET");
    return jsonResponse(
      { error: { code: "not_found", message: "Already provisioned." } },
      404,
    );
  });
  await render(<AppV2 />);
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
}

beforeEach(() => {
  vi.useFakeTimers();
});

afterEach(() => {
  vi.useRealTimers();
});

describe("AppV2 — wiring RepositoryStartupScreen into the running app shell", () => {
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
    await render(<AppV2 />);
    await waitUntil(() => bodyIncludes("First-run setup"), "First-run setup");
  });

  test("device unreachable at the setup check shows a retry screen", async () => {
    planFetch(() => {
      throw new TypeError("Failed to fetch");
    });
    await render(<AppV2 />);
    await waitUntil(
      () => bodyIncludes("Device unreachable"),
      "device unreachable",
    );
  });

  test("Sign In -> repository startup -> the Macros page, with no dedicated progress route", async () => {
    await signIn();
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
  });

  test("session expiry drops back to Sign In without discarding the working copy", async () => {
    await signIn();
    const blobCallsBefore = getFetchCalls().filter(
      (call) => call.url === "/api/v1/blob",
    ).length;

    planFetch((call) => {
      expect(call.url).toBe("/api/v1/status");
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
  });
});

describe("AppV2 — Phase 10 wiring (macro editor and Packages, TODO_V2 V2-100/V2-101/V2-102/V2-103)", () => {
  test("Add macro -> Save changes returns to the Macros page, shows the new macro, and marks the header dirty", async () => {
    await signIn();
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
  });

  test("Packages -> Open switches the selected package, persists the change, and does not add a further dirty transition", async () => {
    await signIn();

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
  });
});
