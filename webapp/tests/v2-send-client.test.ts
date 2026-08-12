import { describe, expect, test, vi } from "vitest";
import {
  cancelSend,
  isTerminalSendState,
  recoverSendState,
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

  test("awaiting confirmation transitions to running by polling without a second POST", async () => {
    vi.useFakeTimers();
    try {
      const awaitingAccepted = {
        ...accepted,
        state: "awaiting_confirmation" as const,
      };
      planJsonResponse(awaitingAccepted, 202);
      const onStatus = vi.fn();
      const handle = await sendMacro(request, { onStatus });
      expect(handle.accepted).toEqual(awaitingAccepted);

      planJsonResponse(statusAt("awaiting_confirmation", 0));
      await vi.advanceTimersByTimeAsync(1000);
      planJsonResponse(statusAt("running", 0));
      await vi.advanceTimersByTimeAsync(1000);

      expect(onStatus).toHaveBeenLastCalledWith(statusAt("running", 0));
      expect(
        getFetchCalls().filter(
          (call) => call.method === "POST" && call.url === "/api/v1/send",
        ),
      ).toHaveLength(1);
      handle.stop();
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

  test("trackSend resumes polling a recovered send without posting", async () => {
    vi.useFakeTimers();
    try {
      const seed = statusAt("running", 3);
      const onStatus = vi.fn();
      const onComplete = vi.fn();

      trackSend(seed as unknown as SendStatusResponse, {
        onStatus,
        onComplete,
      });

      // No POST is ever made by trackSend.
      expect(
        getFetchCalls().filter((call) => call.method === "POST"),
      ).toHaveLength(0);

      planJsonResponse(statusAt("running", 3));
      await vi.advanceTimersByTimeAsync(1000);
      // Unchanged from the seed: no onStatus call yet.
      expect(onStatus).not.toHaveBeenCalled();

      planJsonResponse(statusAt("completed", 9));
      await vi.advanceTimersByTimeAsync(1000);
      expect(onStatus).toHaveBeenCalledTimes(1);
      expect(onComplete).toHaveBeenCalledTimes(1);
      expect(onComplete).toHaveBeenCalledWith(statusAt("completed", 9));

      await vi.advanceTimersByTimeAsync(5000);
      expect(onComplete).toHaveBeenCalledTimes(1);
    } finally {
      vi.useRealTimers();
    }
  });

  test("trackSend never polls or completes when seeded with an already-terminal status", async () => {
    vi.useFakeTimers();
    try {
      const seed = statusAt("failed", 4, { error: "usb_not_ready" });
      const onComplete = vi.fn();
      const handle = trackSend(seed as unknown as SendStatusResponse, {
        onComplete,
      });

      await vi.advanceTimersByTimeAsync(5000);
      expect(onComplete).not.toHaveBeenCalled();
      expect(
        getFetchCalls().filter((call) => call.method === "GET"),
      ).toHaveLength(0);
      handle.stop();
    } finally {
      vi.useRealTimers();
    }
  });

  test("persistent transient poll failures surface and stop tracking", async () => {
    vi.useFakeTimers();
    try {
      const seed = statusAt("running", 0);
      const onError = vi.fn();
      for (let attempt = 0; attempt < 3; attempt += 1) {
        planJsonResponse(
          {
            error: {
              code: "temporarily_unavailable",
              message: "Send status is temporarily unavailable.",
            },
          },
          503,
        );
      }

      trackSend(seed as unknown as SendStatusResponse, { onError });
      await vi.advanceTimersByTimeAsync(3000);

      expect(onError).toHaveBeenCalledOnce();
      expect(onError).toHaveBeenCalledWith(
        expect.objectContaining({ status: 503 }),
      );
      expect(
        getFetchCalls().filter((call) => call.method === "GET"),
      ).toHaveLength(3);

      await vi.advanceTimersByTimeAsync(5000);
      expect(onError).toHaveBeenCalledOnce();
      expect(
        getFetchCalls().filter((call) => call.method === "GET"),
      ).toHaveLength(3);
    } finally {
      vi.useRealTimers();
    }
  });

  test("non-transient poll failure surfaces without retrying", async () => {
    vi.useFakeTimers();
    try {
      const seed = statusAt("running", 0);
      const onError = vi.fn();
      planJsonResponse(
        { error: { code: "bad_request", message: "Invalid send status." } },
        400,
      );

      trackSend(seed as unknown as SendStatusResponse, { onError });
      await vi.advanceTimersByTimeAsync(1000);

      expect(onError).toHaveBeenCalledOnce();
      expect(onError).toHaveBeenCalledWith(
        expect.objectContaining({ status: 400 }),
      );
      await vi.advanceTimersByTimeAsync(5000);
      expect(
        getFetchCalls().filter((call) => call.method === "GET"),
      ).toHaveLength(1);
    } finally {
      vi.useRealTimers();
    }
  });

  test("trackSend.stop() halts polling", async () => {
    vi.useFakeTimers();
    try {
      const seed = statusAt("running", 0);
      const onStatus = vi.fn();
      const handle = trackSend(seed as unknown as SendStatusResponse, {
        onStatus,
      });
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
});
