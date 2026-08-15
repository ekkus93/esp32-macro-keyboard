import { act } from "react";
import { afterEach, beforeEach, describe, expect, test, vi } from "vitest";
import { landscapePhoneMediaQuery } from "../src/features/shell/v2/useLandscapePhoneBlock";
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
  requiredElement,
  setInputValue,
} from "./render";
import { bodyIncludes, signIn, tick, waitUntil } from "./appV2Harness";

beforeEach(() => {
  vi.useFakeTimers();
});

afterEach(() => {
  vi.useRealTimers();
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
