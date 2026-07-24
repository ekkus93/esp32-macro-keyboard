import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import App from "../src/App";
import { getFetchCalls, planFetch, planJsonResponse } from "./fakeFetch";
import { setHashSilently } from "./fakeLocation";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
} from "./render";

function executionStatus(
  state: "running" | "completed" | "cancelled" | "failed",
  actionIndex = 2,
  actionCount = 5,
): object {
  return {
    state,
    error: state === "failed" ? "press_failed" : "",
    releaseError: "",
    actionIndex,
    actionCount,
  };
}

async function renderExecution(
  state: "running" | "completed" | "cancelled" | "failed" = "running",
): Promise<Awaited<ReturnType<typeof render>>> {
  setHashSilently("/execution");
  planJsonResponse({ ok: true, data: executionStatus(state) });
  const view = await render(<App />);
  await flushReact();
  return view;
}

describe("execution workflow", () => {
  test("polls immediately and displays running progress", async () => {
    const view = await renderExecution();
    expect(getFetchCalls()).toHaveLength(1);
    expect(getFetchCalls()[0]?.url).toBe("/api/v1/executions/current");
    expect(document.body.textContent).toContain("2 / 5");
    await view.unmount();
  });

  test.each([
    ["completed", "Macro finished"],
    ["cancelled", "Macro finished"],
    ["failed", "Macro failed"],
  ] as const)("navigates after %s", async (state, expectedTitle) => {
    vi.useFakeTimers();
    const view = await renderExecution(state);
    await act(async () => {
      window.dispatchEvent(new HashChangeEvent("hashchange"));
      await Promise.resolve();
    });
    expect(document.body.textContent).toContain(expectedTitle);
    await vi.advanceTimersByTimeAsync(1_000);
    expect(getFetchCalls()).toHaveLength(1);
    await view.unmount();
  });

  test("keeps polling failures visible without synthesizing completion", async () => {
    setHashSilently("/execution");
    planFetch(() => Promise.reject(new Error("poll unavailable")));
    const view = await render(<App />);
    await flushReact();
    expect(
      requiredElement("[role='alert']", HTMLElement).textContent,
    ).toContain("poll unavailable");
    expect(document.body.textContent).toContain("Typing macro");
    expect(document.body.textContent).not.toContain("Macro finished");
    await view.unmount();
  });

  test("stops polling after unmount", async () => {
    vi.useFakeTimers();
    const view = await renderExecution();
    expect(getFetchCalls()).toHaveLength(1);
    await view.unmount();
    await vi.advanceTimersByTimeAsync(1_500);
    expect(getFetchCalls()).toHaveLength(1);
  });

  test("posts cancellation without claiming completion", async () => {
    const view = await renderExecution();
    planJsonResponse({ ok: true, data: { cancelRequested: true } });
    await click(buttonWithText("Cancel and release keys"));
    await flushReact();

    const cancelCall = getFetchCalls()[1];
    expect(cancelCall?.url).toBe("/api/v1/executions/current/cancel");
    expect(cancelCall?.method).toBe("POST");
    expect(document.body.textContent).toContain("Typing macro");
    expect(document.body.textContent).not.toContain("Macro finished");
    await view.unmount();
  });

  test("shows cancellation failures", async () => {
    const view = await renderExecution();
    planJsonResponse(
      {
        ok: false,
        error: { code: "cancel_failed", message: "Cancellation was rejected." },
      },
      409,
    );
    await click(buttonWithText("Cancel and release keys"));
    await flushReact();
    expect(
      requiredElement("[role='alert']", HTMLElement).textContent,
    ).toContain("cancel_failed: Cancellation was rejected.");
    await view.unmount();
  });
});
