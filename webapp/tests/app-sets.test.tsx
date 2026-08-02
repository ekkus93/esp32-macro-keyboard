import { describe, expect, test } from "vitest";
import App from "../src/App";
import {
  macroSet,
  planAuthenticatedBootstrap,
  setId,
  settings,
} from "./appFixtures";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";
import { setHashSilently } from "./fakeLocation";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setCheckboxChecked,
  setInputValue,
} from "./render";

const secondSet = {
  ...macroSet,
  id: "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa",
  name: "Workshop desktop workflow",
};

describe("server-backed set selection", () => {
  test("shows live metadata and filters by search", async () => {
    setHashSilently("/sets");
    planAuthenticatedBootstrap({
      activeSetId: null,
      sets: [macroSet, secondSet],
    });
    const view = await render(<App />);
    await flushReact();

    expect(document.body.textContent).toContain("Lab bench workflow");
    expect(document.body.textContent).toContain("Workshop desktop workflow");

    await setInputValue(
      requiredElement("#set-search", HTMLInputElement),
      "Workshop",
    );
    expect(document.body.textContent).not.toContain("Lab bench workflow");
    expect(document.body.textContent).toContain("Workshop desktop workflow");
    await view.unmount();
  });

  // SPEC 24.5 item: set switching

  // SPEC 24.5 item: active-set visibility

  test("selects a set with the settings revision and updates the header", async () => {
    setHashSilently("/sets");
    planAuthenticatedBootstrap({ activeSetId: null });
    const view = await render(<App />);
    await flushReact();

    planJsonResponse({
      ok: true,
      data: {
        ...settings,
        revision: settings.revision + 1,
        activeSetId: setId,
      },
    });
    await click(buttonWithText("Use this set"));
    await flushReact();

    const call = getFetchCalls()[5];
    expect(call?.url).toBe(`/api/v1/sets/${setId}/select`);
    expect(call?.method).toBe("POST");
    expect(call?.body).toBe(
      JSON.stringify({ expectedRevision: settings.revision }),
    );
    expect(document.querySelector(".app-header")?.textContent).toContain(
      "Lab bench workflow",
    );
    expect(
      window.localStorage.getItem("esp32-macro-keyboard.recent-set-ids"),
    ).toContain(setId);
    await view.unmount();
  });

  // SPEC 24.5 item: stale-edit conflict UI

  test("shows revision conflicts instead of silently accepting selection", async () => {
    setHashSilently("/sets");
    planAuthenticatedBootstrap({ activeSetId: null });
    const view = await render(<App />);
    await flushReact();

    planJsonResponse(
      {
        ok: false,
        error: {
          code: "conflict",
          message: "Settings revision is stale.",
        },
      },
      409,
    );
    await click(buttonWithText("Use this set"));
    await flushReact();

    expect(
      requiredElement("[role='alert']", HTMLElement).textContent,
    ).toContain("conflict: Settings revision is stale.");
    expect(document.querySelector(".app-header")?.textContent).toContain(
      "No active macro set",
    );
    await view.unmount();
  });

  test("persists real settings updates", async () => {
    setHashSilently("/settings");
    planAuthenticatedBootstrap();
    const view = await render(<App />);
    await flushReact();

    const checkbox = requiredElement(
      "input[type='checkbox']",
      HTMLInputElement,
    );
    await setCheckboxChecked(checkbox, false);
    planJsonResponse({
      ok: true,
      data: {
        ...settings,
        revision: settings.revision + 1,
        alwaysSelectSet: false,
      },
    });
    await click(buttonWithText("Save settings"));
    await flushReact();

    const call = getFetchCalls()[5];
    expect(call?.url).toBe("/api/v1/settings");
    expect(call?.method).toBe("PUT");
    /* No activeSetId: the active set is repository state (SPEC 12.3) and moves
       only through the select route, so a settings PUT must not carry it. */
    expect(call?.body).toBe(
      JSON.stringify({
        expectedRevision: settings.revision,
        requirePhysicalConfirmation: settings.requirePhysicalConfirmation,
        alwaysSelectSet: false,
      }),
    );
    await view.unmount();
  });
});
