import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import App from "../src/App";
import {
  executionStatus,
  macro,
  planAuthenticatedBootstrap,
  planNormalUnauthenticatedBootstrap,
} from "./appFixtures";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";
import { navigateHash, setHashSilently } from "./fakeLocation";
import { flushReact, render } from "./render";

describe("application routing", () => {
  test("unknown routes use the set selector after authentication", async () => {
    setHashSilently("/unknown");
    planAuthenticatedBootstrap();
    const view = await render(<App />);
    await flushReact();
    expect(document.body.textContent).toContain("Choose a macro set");
    await view.unmount();
  });

  test("unknown routes still require a valid session", async () => {
    setHashSilently("/unknown");
    planNormalUnauthenticatedBootstrap();
    const view = await render(<App />);
    await flushReact();
    expect(document.body.textContent).toContain("Administrator password");
    await view.unmount();
  });

  test.each([
    ["/sets", "Choose a macro set"],
    ["/procedures", "Procedures"],
    ["/procedure", "Procedure"],
    ["/instruction", "Instruction"],
    ["/procedure-editor", "Edit procedure"],
    ["/macros", "Macros"],
    ["/macro-editor", "Create macro"],
    ["/confirm", "Confirm send"],
    ["/execution", "Typing macro"],
    ["/result", "No execution result"],
    ["/manage-sets", "Manage macro sets"],
    ["/set-editor", "Manage macro sets"],
    ["/import", "Import, export, and recovery"],
    ["/export", "Import, export, and recovery"],
    ["/delete-set", "Manage macro sets"],
    ["/settings", "Settings"],
    ["/diagnostics", "Storage diagnostics"],
  ])("renders authenticated route %s", async (hash, expectedText) => {
    setHashSilently(hash);
    planAuthenticatedBootstrap();
    if (hash === "/execution") {
      planJsonResponse({
        ok: true,
        data: executionStatus("running"),
      });
    }
    if (hash === "/macros") {
      planJsonResponse({ ok: true, data: [macro] });
    }
    if (hash === "/procedures") {
      planJsonResponse({ ok: true, data: [] });
    }
    if (hash === "/diagnostics") {
      planJsonResponse({
        ok: true,
        data: {
          verified: false,
          webMounted: true,
          dataMounted: true,
          quarantineCount: 0,
          damagedQuarantineCount: 0,
        },
      });
      planJsonResponse({
        ok: true,
        data: { damagedCount: 0, items: [] },
      });
    }
    const view = await render(<App />);
    await flushReact();
    expect(document.body.textContent).toContain(expectedText);
    await view.unmount();
  });

  test("removes the hash listener on unmount", async () => {
    const addListener = vi.spyOn(window, "addEventListener");
    const removeListener = vi.spyOn(window, "removeEventListener");
    setHashSilently("/sets");
    planAuthenticatedBootstrap();
    const view = await render(<App />);
    await flushReact();

    const hashRegistration = addListener.mock.calls.find(
      ([type]) => type === "hashchange",
    );
    expect(hashRegistration).toBeDefined();
    await view.unmount();
    expect(removeListener).toHaveBeenCalledWith(
      "hashchange",
      hashRegistration?.[1],
    );
  });

  test("clears execution polling after route change", async () => {
    vi.useFakeTimers();
    setHashSilently("/execution");
    planAuthenticatedBootstrap();
    planJsonResponse({
      ok: true,
      data: executionStatus("running"),
    });
    const view = await render(<App />);
    await flushReact();
    expect(getFetchCalls()).toHaveLength(6);

    await act(async () => {
      navigateHash("/sets");
      await Promise.resolve();
    });
    await vi.advanceTimersByTimeAsync(1_500);
    expect(getFetchCalls()).toHaveLength(6);
    expect(document.body.textContent).toContain("Choose a macro set");
    await view.unmount();
  });
});
