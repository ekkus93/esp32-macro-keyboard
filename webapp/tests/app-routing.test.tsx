import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import App from "../src/App";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";
import { navigateHash, setHashSilently } from "./fakeLocation";
import { flushReact, render } from "./render";

const runningStatus = {
  state: "running",
  error: "",
  releaseError: "",
  actionIndex: 1,
  actionCount: 3,
} as const;

describe("application routing", () => {
  test.each(["", "/unknown"])("routes %j to login", async (hash: string) => {
    setHashSilently(hash);
    const view = await render(<App />);
    expect(document.body.textContent).toContain("Administrator password");
    await view.unmount();
  });

  test.each([
    ["/setup", "First-run setup"],
    ["/login", "Administrator password"],
    ["/sets", "Choose a macro set"],
    ["/procedures", "Procedures"],
    ["/procedure", "ChromeOS to Debian 13"],
    ["/instruction", "Disconnect the battery"],
    ["/procedure-editor", "Edit procedure"],
    ["/macros", "Macros"],
    ["/macro-editor", "Macro editor"],
    ["/confirm", "Confirm send"],
    ["/execution", "Typing macro"],
    ["/result", "Macro finished"],
    ["/manage-sets", "Manage macro sets"],
    ["/set-editor", "Create macro set"],
    ["/import", "Import macro set"],
    ["/export", "Export macro set"],
    ["/delete-set", "Delete macro set"],
    ["/settings", "Settings"],
    ["/diagnostics", "Diagnostics"],
  ])("renders route %s", async (hash: string, expectedText: string) => {
    if (hash === "/execution") {
      planJsonResponse({ ok: true, data: runningStatus });
    }
    setHashSilently(hash);
    const view = await render(<App />);
    await flushReact();
    expect(document.body.textContent).toContain(expectedText);
    await view.unmount();
  });

  test("removes the hash listener on unmount", async () => {
    const addListener = vi.spyOn(window, "addEventListener");
    const removeListener = vi.spyOn(window, "removeEventListener");
    setHashSilently("/sets");
    const view = await render(<App />);

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
    planJsonResponse({ ok: true, data: runningStatus });
    setHashSilently("/execution");
    const view = await render(<App />);
    await flushReact();
    expect(getFetchCalls()).toHaveLength(1);

    await act(async () => {
      navigateHash("/sets");
      await Promise.resolve();
    });
    await vi.advanceTimersByTimeAsync(1_500);
    expect(getFetchCalls()).toHaveLength(1);
    expect(document.body.textContent).toContain("Choose a macro set");
    await view.unmount();
  });
});
