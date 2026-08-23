import { act } from "react";
import { expect, vi } from "vitest";
import AppV2 from "../src/AppV2";
import { v2Limits } from "../src/v2/limits";
import type { FetchCall } from "./fakeFetch";
import { getFetchCalls, jsonResponse, planFetch } from "./fakeFetch";
import { render, requiredElement, setInputValue } from "./render";
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

export async function tick(ms = 0): Promise<void> {
  await act(async () => {
    await vi.advanceTimersByTimeAsync(ms);
  });
}

export function bodyIncludes(text: string): boolean {
  return document.body.textContent?.includes(text) === true;
}

/**
 * Polls by repeatedly flushing microtasks until `predicate` is true, rather
 * than guessing a fixed number of flushes. A React state update landing
 * inside a resolved promise chain can take more than one `advanceTimersByTimeAsync(0)`
 * round to fully commit and re-run a newly mounted component's own effects
 * (for example `FirstRunSetupPage`'s independent load of `/api/v1/setup`).
 */
export async function waitUntil(
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

export const settingsBody = {
  deviceName: "Desk Macro Keyboard",
  requireSerialConfirmation: false,
  sendMode: "quick",
  snapshotRetentionTarget: 5,
  lastSelectedPackageId: null,
  apSsid: "MacroKeyboard",
  stationConfigured: false,
  stationSsid: null,
};

export const statusBody = {
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

export const validSession = {
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
export function planRace(
  byUrl: Record<string, () => Response>,
  times: number,
): void {
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
export async function submitForm(): Promise<void> {
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
export interface SignInOptions {
  firstPackagePersistenceError?: Error;
}

export async function signIn(
  options: SignInOptions = {},
): Promise<RenderResult> {
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
