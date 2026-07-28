import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import App from "../src/App";
import { executionStatus, planAuthenticatedBootstrap } from "./appFixtures";
import { getFetchCalls, planFetch, planJsonResponse } from "./fakeFetch";
import { setHashSilently } from "./fakeLocation";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
} from "./render";

async function renderExecution(
  state:
    | "running"
    | "completed"
    | "cancelled"
    | "failed"
    | "timed_out" = "running",
): Promise<Awaited<ReturnType<typeof render>>> {
  setHashSilently("/execution");
  planAuthenticatedBootstrap();
  planJsonResponse({
    ok: true,
    data: executionStatus(state),
  });
  const view = await render(<App />);
  await flushReact();
  return view;
}

describe("execution workflow", () => {
  test("polls immediately and displays running progress", async () => {
    const view = await renderExecution();
    expect(getFetchCalls()).toHaveLength(6);
    expect(getFetchCalls()[5]?.url).toBe("/api/v1/executions/current");
    expect(document.body.textContent).toContain("2 / 5");
    await view.unmount();
  });

  test.each([
    ["completed", "Macro completed"],
    ["cancelled", "Macro cancelled"],
    ["failed", "Macro failed"],
    ["timed_out", "Macro timed out"],
  ] as const)(
    "labels the %s terminal state exactly",
    async (state, expectedTitle) => {
      vi.useFakeTimers();
      const view = await renderExecution("running");
      planJsonResponse({
        ok: true,
        data: executionStatus(state),
      });

      await act(async () => {
        await vi.advanceTimersByTimeAsync(500);
        window.dispatchEvent(new HashChangeEvent("hashchange"));
        await Promise.resolve();
      });

      expect(document.body.textContent).toContain(expectedTitle);
      await act(async () => {
        await vi.advanceTimersByTimeAsync(1_000);
      });
      expect(getFetchCalls()).toHaveLength(7);
      await view.unmount();
    },
  );

  test("prioritizes key-release failure labeling", async () => {
    vi.useFakeTimers();
    const view = await renderExecution("running");
    planJsonResponse({
      ok: true,
      data: {
        ...executionStatus("completed"),
        releaseError: "release_failed",
      },
    });

    await act(async () => {
      await vi.advanceTimersByTimeAsync(500);
      window.dispatchEvent(new HashChangeEvent("hashchange"));
      await Promise.resolve();
    });

    expect(document.body.textContent).toContain(
      "Macro ended with a key-release error",
    );
    await view.unmount();
  });

  test("keeps polling failures visible without synthesizing completion", async () => {
    setHashSilently("/execution");
    planAuthenticatedBootstrap();
    planFetch(() => Promise.reject(new Error("poll unavailable")));
    const view = await render(<App />);
    await flushReact();
    expect(
      requiredElement("[role='alert']", HTMLElement).textContent,
    ).toContain("poll unavailable");
    expect(document.body.textContent).toContain("Typing macro");
    expect(document.body.textContent).not.toContain("Macro completed");
    await view.unmount();
  });

  test("stops polling after unmount", async () => {
    vi.useFakeTimers();
    const view = await renderExecution();
    expect(getFetchCalls()).toHaveLength(6);
    await view.unmount();
    await vi.advanceTimersByTimeAsync(1_500);
    expect(getFetchCalls()).toHaveLength(6);
  });

  test("posts cancellation without claiming completion", async () => {
    const view = await renderExecution();
    planJsonResponse({
      ok: true,
      data: { cancelRequested: true },
    });
    await click(buttonWithText("Cancel and release keys"));
    await flushReact();

    const cancelCall = getFetchCalls()[6];
    expect(cancelCall?.url).toBe("/api/v1/executions/current/cancel");
    expect(cancelCall?.method).toBe("POST");
    expect(document.body.textContent).toContain("Typing macro");
    expect(document.body.textContent).not.toContain("Macro completed");
    await view.unmount();
  });

  test("shows cancellation failures", async () => {
    const view = await renderExecution();
    planJsonResponse(
      {
        ok: false,
        error: {
          code: "conflict",
          message: "Cancellation was rejected.",
        },
      },
      409,
    );
    await click(buttonWithText("Cancel and release keys"));
    await flushReact();
    expect(
      requiredElement("[role='alert']", HTMLElement).textContent,
    ).toContain("conflict: Cancellation was rejected.");
    await view.unmount();
  });

  test("returns to login and stops polling after session expiry", async () => {
    vi.useFakeTimers();
    const view = await renderExecution();
    planJsonResponse(
      {
        ok: false,
        error: {
          code: "auth_required",
          message: "Session expired.",
        },
      },
      401,
    );

    await act(async () => {
      await vi.advanceTimersByTimeAsync(500);
      await Promise.resolve();
    });
    expect(document.body.textContent).toContain("Administrator password");

    await act(async () => {
      await vi.advanceTimersByTimeAsync(6_000);
    });
    expect(getFetchCalls()).toHaveLength(7);
    await view.unmount();
  });
});
