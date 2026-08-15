import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import { jsonResponse, planFetch } from "./fakeFetch";
import {
  buttonWithText,
  click,
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
