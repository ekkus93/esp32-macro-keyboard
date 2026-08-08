import { describe, expect, test, vi } from "vitest";
import {
  cancelSend,
  isTerminalSendState,
  recoverSendState,
  sendMacro,
} from "../src/v2/sendClient";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
} from "./fakeFetch";

const request = { source: "make -j8{ENTER}", keyPressMs: 8, interKeyMs: 15 };
const sendId = "550e8400-e29b-41d4-a716-446655440000";

const accepted = {
  id: sendId,
  state: "running",
  actionCount: 9,
  estimatedDurationMs: 214,
};

function statusAt(
  state: string,
  actionIndex: number,
  overrides: Record<string, unknown> = {},
): Record<string, unknown> {
  return {
    id: sendId,
    state,
    actionIndex,
    actionCount: 9,
    estimatedDurationMs: 214,
    cancellationRequested: false,
    error: "",
    releaseError: "",
    ...overrides,
  };
}

describe("v2 send helper", () => {
  test("isTerminalSendState matches exactly the terminal states", () => {
    expect(isTerminalSendState("completed")).toBe(true);
    expect(isTerminalSendState("cancelled")).toBe(true);
    expect(isTerminalSendState("failed")).toBe(true);
    expect(isTerminalSendState("timed_out")).toBe(true);
    expect(isTerminalSendState("running")).toBe(false);
    expect(isTerminalSendState("awaiting_confirmation")).toBe(false);
  });

  test("posts once, polls no slower than once per second, and completes exactly once", async () => {
    vi.useFakeTimers();
    try {
      planJsonResponse(accepted, 202);
      const onStatus = vi.fn();
      const onComplete = vi.fn();

      const handlePromise = sendMacro(request, { onStatus, onComplete });
      await vi.advanceTimersByTimeAsync(0);
      const handle = await handlePromise;
      expect(handle.accepted).toEqual(accepted);

      planJsonResponse(statusAt("running", 3));
      await vi.advanceTimersByTimeAsync(1000);
      expect(onStatus).toHaveBeenCalledTimes(1);
      expect(onStatus).toHaveBeenLastCalledWith(statusAt("running", 3));

      // Unchanged state/progress: no additional onStatus call.
      planJsonResponse(statusAt("running", 3));
      await vi.advanceTimersByTimeAsync(1000);
      expect(onStatus).toHaveBeenCalledTimes(1);

      planJsonResponse(statusAt("completed", 9));
      await vi.advanceTimersByTimeAsync(1000);
      expect(onStatus).toHaveBeenCalledTimes(2);
      expect(onComplete).toHaveBeenCalledTimes(1);
      expect(onComplete).toHaveBeenCalledWith(statusAt("completed", 9));

      // No further polling after a terminal state.
      await vi.advanceTimersByTimeAsync(5000);
      expect(onComplete).toHaveBeenCalledTimes(1);

      const postCalls = getFetchCalls().filter(
        (call) => call.method === "POST",
      );
      expect(postCalls).toHaveLength(1);
    } finally {
      vi.useRealTimers();
    }
  });

  test("stop() halts polling without cancelling the send on the device", async () => {
    vi.useFakeTimers();
    try {
      planJsonResponse(accepted, 202);
      const onStatus = vi.fn();
      const handle = await sendMacro(request, { onStatus });

      handle.stop();
      await vi.advanceTimersByTimeAsync(5000);
      expect(onStatus).not.toHaveBeenCalled();
      expect(
        getFetchCalls().filter((call) => call.method === "GET"),
      ).toHaveLength(0);
    } finally {
      vi.useRealTimers();
    }
  });

  test("rejects without polling when the initial POST fails", async () => {
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
    expect(
      getFetchCalls().filter((call) => call.method === "GET"),
    ).toHaveLength(0);
  });

  test("handle.cancel() calls DELETE /api/v1/send", async () => {
    vi.useFakeTimers();
    try {
      planJsonResponse(accepted, 202);
      const handle = await sendMacro(request);
      handle.stop();

      planFetch((call) => {
        expect(call.method).toBe("DELETE");
        expect(call.url).toBe("/api/v1/send");
        return jsonResponse({ id: sendId }, 202);
      });
      await handle.cancel();
    } finally {
      vi.useRealTimers();
    }
  });

  test("recoverSendState returns the current status", async () => {
    planJsonResponse(statusAt("running", 2));
    const status = await recoverSendState();
    expect(status).toEqual(statusAt("running", 2));
  });

  test("recoverSendState returns null when no send exists since boot (404)", async () => {
    planJsonResponse(
      { error: { code: "not_found", message: "No send since boot." } },
      404,
    );
    const status = await recoverSendState();
    expect(status).toBeNull();
  });

  test("cancelSend calls DELETE /api/v1/send", async () => {
    planFetch((call) => {
      expect(call.method).toBe("DELETE");
      expect(call.url).toBe("/api/v1/send");
      return jsonResponse({ id: sendId }, 202);
    });
    await cancelSend();
  });
});
