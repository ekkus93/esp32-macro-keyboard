import { describe, expect, test, vi } from "vitest";
import { ExecutionRecoveryOverlay } from "../src/features/shell/v2/ExecutionRecoveryOverlay";
import {
  getExecutionRecoveryState,
  recoverSendState,
  retryExecutionRecovery,
  sendMacro,
  trackSend,
} from "../src/v2/sendClient";
import type { SendStatusResponse } from "../src/v2/apiTypes";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";
import { buttonWithText, click, flushReact, render } from "./render";

const request = { source: "a", keyPressMs: 8, interKeyMs: 15 };
const sendId = "123e4567-e89b-42d3-a456-426614174000";

function statusAt(state: string, actionIndex: number): SendStatusResponse {
  return {
    id: sendId,
    state: state as SendStatusResponse["state"],
    actionIndex,
    actionCount: 9,
    estimatedDurationMs: 214,
    cancellationRequested: false,
    error: "",
    releaseError: "",
  };
}

function unavailableResponse(): Record<string, unknown> {
  return {
    error: {
      code: "temporarily_unavailable",
      message: "Send status is temporarily unavailable.",
    },
  };
}

describe("H4 execution recovery", () => {
  test("one transient poll failure stays quiet and clears on the next successful refresh", async () => {
    vi.useFakeTimers();
    try {
      const onError = vi.fn();
      const onStatus = vi.fn();
      const seed = statusAt("running", 1);

      planJsonResponse(unavailableResponse(), 503);
      planJsonResponse(statusAt("running", 2));

      trackSend(seed, { onError, onStatus });
      await vi.advanceTimersByTimeAsync(1000);

      expect(getExecutionRecoveryState()).toEqual({ kind: "clear" });
      expect(onError).not.toHaveBeenCalled();
      expect(onStatus).not.toHaveBeenCalled();

      await vi.advanceTimersByTimeAsync(1000);

      expect(getExecutionRecoveryState()).toEqual({ kind: "clear" });
      expect(onError).not.toHaveBeenCalled();
      expect(onStatus).toHaveBeenCalledOnce();
      expect(onStatus).toHaveBeenCalledWith(statusAt("running", 2));
      expect(
        getFetchCalls().filter((call) => call.method === "POST"),
      ).toHaveLength(0);
    } finally {
      vi.useRealTimers();
    }
  });

  test("repeated unchanged successful polls announce once, not once per poll", async () => {
    // H4-042: "avoid live-region announcements on every unchanged successful
    // poll". onStatus is what drives the aria-live region in MacrosPage, so a
    // status that has not meaningfully changed must not re-invoke it. Four
    // identical successful polls therefore have to produce exactly one call.
    vi.useFakeTimers();
    try {
      const onError = vi.fn();
      const onStatus = vi.fn();
      const seed = statusAt("running", 1);
      const unchanged = statusAt("running", 2);

      // First poll differs from the seed and must announce; the next three are
      // byte-identical to it and must not.
      planJsonResponse(unchanged);
      planJsonResponse(unchanged);
      planJsonResponse(unchanged);
      planJsonResponse(unchanged);

      trackSend(seed, { onError, onStatus });
      for (let poll = 0; poll < 4; poll += 1) {
        await vi.advanceTimersByTimeAsync(1000);
      }

      expect(onError).not.toHaveBeenCalled();
      expect(onStatus).toHaveBeenCalledOnce();
      expect(onStatus).toHaveBeenCalledWith(unchanged);
      expect(getExecutionRecoveryState()).toEqual({ kind: "clear" });
    } finally {
      vi.useRealTimers();
    }
  });

  test("persistent tracking failure becomes explicit and GET-only retry resumes tracking", async () => {
    vi.useFakeTimers();
    try {
      const onStatus = vi.fn();
      const seed = statusAt("running", 1);
      for (let attempt = 0; attempt < 3; attempt += 1) {
        planJsonResponse(unavailableResponse(), 503);
      }

      trackSend(seed, { onStatus });
      await vi.advanceTimersByTimeAsync(3000);

      expect(getExecutionRecoveryState()).toMatchObject({
        kind: "unavailable",
        lastKnown: seed,
      });
      expect(
        getFetchCalls().filter((call) => call.method === "POST"),
      ).toHaveLength(0);

      planJsonResponse(statusAt("running", 2));
      await retryExecutionRecovery();
      expect(getExecutionRecoveryState()).toEqual({ kind: "clear" });

      planJsonResponse(statusAt("running", 3));
      await vi.advanceTimersByTimeAsync(1000);
      expect(onStatus).toHaveBeenLastCalledWith(statusAt("running", 3));
      expect(
        getFetchCalls().filter((call) => call.method === "POST"),
      ).toHaveLength(0);
    } finally {
      vi.useRealTimers();
    }
  });

  test("409 plus failed recovery blocks a second POST until GET reconciliation", async () => {
    const onStatus = vi.fn();
    planJsonResponse(
      {
        error: {
          code: "already_sending",
          message: "A send is already in progress.",
        },
      },
      409,
    );
    await expect(sendMacro(request, { onStatus })).rejects.toMatchObject({
      status: 409,
    });
    expect(getExecutionRecoveryState().kind).toBe("unavailable");

    planJsonResponse(unavailableResponse(), 503);
    await expect(recoverSendState()).rejects.toMatchObject({ status: 503 });

    await expect(sendMacro(request, { onStatus })).rejects.toThrow(
      "Execution state is unavailable",
    );
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(1);

    planJsonResponse(statusAt("running", 4));
    await retryExecutionRecovery();
    expect(getExecutionRecoveryState()).toEqual({ kind: "clear" });
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(1);
  });

  test("terminal reconciliation completes through the original callbacks and clears recovery", async () => {
    const onComplete = vi.fn();
    planJsonResponse(
      {
        error: {
          code: "already_sending",
          message: "A send is already in progress.",
        },
      },
      409,
    );
    await expect(sendMacro(request, { onComplete })).rejects.toMatchObject({
      status: 409,
    });

    const terminal = statusAt("cancelled", 2);
    planJsonResponse(terminal);
    await retryExecutionRecovery();

    expect(getExecutionRecoveryState()).toEqual({ kind: "clear" });
    expect(onComplete).toHaveBeenCalledOnce();
    expect(onComplete).toHaveBeenCalledWith(terminal);
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(1);
  });

  test("overlay keeps Retry and Cancel visible and reports cancel delivery failure", async () => {
    planJsonResponse(
      {
        error: {
          code: "already_sending",
          message: "A send is already in progress.",
        },
      },
      409,
    );
    await expect(sendMacro(request)).rejects.toMatchObject({ status: 409 });

    const view = await render(<ExecutionRecoveryOverlay />);
    expect(view.container.textContent).toContain("Execution state unavailable");
    expect(view.container.textContent).toContain("Retry execution status");
    expect(view.container.textContent).toContain("Cancel and release all keys");

    planFetch(() =>
      jsonResponse(
        { error: { code: "internal", message: "cancel transport failed" } },
        503,
      ),
    );
    await click(buttonWithText("Cancel and release all keys"));
    await flushReact();
    expect(view.container.textContent).toContain(
      "Cancel could not be delivered",
    );

    planJsonResponse(statusAt("cancelled", 2));
    await click(buttonWithText("Retry execution status"));
    await flushReact();
    expect(view.container.textContent).not.toContain(
      "Execution state unavailable",
    );
    expect(
      getFetchCalls().filter((call) => call.method === "POST"),
    ).toHaveLength(1);
    await view.unmount();
  });
});
