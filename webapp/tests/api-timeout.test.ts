import { describe, expect, test, vi } from "vitest";
import { apiRequest } from "../src/api/client";
import { isEmptyRecord } from "../src/api/guards";
import { getFetchCalls, planFetch } from "./fakeFetch";

function planPendingRequest(): void {
  planFetch(
    (call) =>
      new Promise<Response>((_resolve, reject) => {
        const signal = call.signal;
        if (signal === null) {
          reject(new Error("Missing abort signal."));
          return;
        }
        const rejectAbort = (): void => {
          reject(new DOMException("The operation was aborted.", "AbortError"));
        };
        if (signal.aborted) {
          rejectAbort();
        } else {
          signal.addEventListener("abort", rejectAbort, { once: true });
        }
      }),
  );
}

describe("apiRequest timeout policy", () => {
  test("honors a bounded 25-second confirmation timeout", async () => {
    vi.useFakeTimers();
    planPendingRequest();

    const request = apiRequest(
      "/api/v1/executions",
      { method: "POST", body: "{}" },
      isEmptyRecord,
      { timeoutMs: 25_000 },
    );
    const rejection = expect(request).rejects.toMatchObject({
      name: "AbortError",
    });

    await vi.advanceTimersByTimeAsync(10_000);
    expect(getFetchCalls()[0]?.signal?.aborted).toBe(false);
    await vi.advanceTimersByTimeAsync(15_000);
    await rejection;
    expect(getFetchCalls()[0]?.signal?.aborted).toBe(true);
    expect(vi.getTimerCount()).toBe(0);
  });

  test.each([0, -1, 1.5, 60_001])(
    "rejects invalid timeout %s before fetch",
    async (timeoutMs) => {
      await expect(
        apiRequest("/api/v1/status", {}, isEmptyRecord, { timeoutMs }),
      ).rejects.toThrow(
        "API request timeout must be an integer from 1 through 60000 milliseconds.",
      );
      expect(getFetchCalls()).toHaveLength(0);
    },
  );
});
