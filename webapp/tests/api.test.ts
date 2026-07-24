import { describe, expect, test, vi } from "vitest";
import { ApiError, apiRequest, setCsrfToken } from "../src/api/client";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
  planTextResponse,
} from "./fakeFetch";

function success<T>(data: T): { ok: true; data: T } {
  return { ok: true, data };
}

describe("apiRequest", () => {
  test("rejects non-API paths before fetch", async () => {
    await expect(apiRequest("/settings")).rejects.toThrow(
      "API requests must use same-origin /api/ paths.",
    );
    expect(getFetchCalls()).toHaveLength(0);
  });

  test("uses same-origin credentials and required JSON headers", async () => {
    planJsonResponse(success({ value: 1 }));
    await expect(
      apiRequest<{ value: number }>("/api/v1/status", {
        headers: { "X-Trace": "trace-1" },
      }),
    ).resolves.toEqual({ value: 1 });

    const call = getFetchCalls()[0];
    expect(call?.credentials).toBe("same-origin");
    expect(call?.method).toBe("GET");
    expect(call?.headers.get("Accept")).toBe("application/json");
    expect(call?.headers.get("X-Trace")).toBe("trace-1");
    expect(call?.headers.has("Content-Type")).toBe(false);
  });

  test("adds Content-Type only for request bodies and preserves an explicit value", async () => {
    planJsonResponse(success({}));
    await apiRequest("/api/v1/items", {
      method: "POST",
      body: "{}",
    });
    expect(getFetchCalls()[0]?.headers.get("Content-Type")).toBe("application/json");

    planJsonResponse(success({}));
    await apiRequest("/api/v1/items", {
      method: "POST",
      body: "raw",
      headers: { "Content-Type": "application/octet-stream" },
    });
    expect(getFetchCalls()[1]?.headers.get("Content-Type")).toBe(
      "application/octet-stream",
    );
  });

  test("adds CSRF only to mutation methods", async () => {
    setCsrfToken("csrf-token");
    for (const method of ["POST", "put", "PATCH", "delete"]) {
      planJsonResponse(success({}));
      await apiRequest("/api/v1/items", { method });
    }
    planJsonResponse(success({}));
    await apiRequest("/api/v1/items", { method: "GET" });
    planJsonResponse(success({}));
    await apiRequest("/api/v1/items", { method: "HEAD" });

    for (const call of getFetchCalls().slice(0, 4)) {
      expect(call.headers.get("X-CSRF-Token")).toBe("csrf-token");
    }
    expect(getFetchCalls()[4]?.headers.has("X-CSRF-Token")).toBe(false);
    expect(getFetchCalls()[5]?.headers.has("X-CSRF-Token")).toBe(false);
  });

  test("returns successful envelopes including JSON with a charset", async () => {
    planFetch(() =>
      new Response(JSON.stringify(success({ value: "ok" })), {
        status: 200,
        headers: { "Content-Type": "application/json; charset=utf-8" },
      }),
    );
    await expect(apiRequest("/api/v1/status")).resolves.toEqual({ value: "ok" });
  });

  test("uses an API failure body for HTTP failures", async () => {
    planJsonResponse(
      {
        ok: false,
        error: { code: "bad_request", message: "Invalid request." },
      },
      400,
    );
    await expect(apiRequest("/api/v1/status")).rejects.toMatchObject({
      status: 400,
      body: { code: "bad_request", message: "Invalid request." },
    });
  });

  test("maps an HTTP failure with a success envelope to http_error", async () => {
    planJsonResponse(success({ value: "ignored" }), 503);
    await expect(apiRequest("/api/v1/status")).rejects.toMatchObject({
      status: 503,
      body: {
        code: "http_error",
        message: "Request failed with status 503.",
      },
    });
  });

  test("rejects a failure envelope even when HTTP succeeded", async () => {
    planJsonResponse({
      ok: false,
      error: { code: "busy", message: "Device is busy." },
    });
    await expect(apiRequest("/api/v1/status")).rejects.toMatchObject({
      status: 200,
      body: { code: "busy", message: "Device is busy." },
    });
  });

  test("rejects non-JSON and malformed JSON responses", async () => {
    planTextResponse("not json");
    await expect(apiRequest("/api/v1/status")).rejects.toMatchObject({
      status: 200,
      body: {
        code: "invalid_response",
        message: "The device returned a non-JSON response.",
      },
    });

    planFetch(() =>
      new Response("{", {
        status: 200,
        headers: { "Content-Type": "application/json" },
      }),
    );
    await expect(apiRequest("/api/v1/status")).rejects.toMatchObject({
      status: 200,
      body: {
        code: "invalid_response",
        message: "The device returned malformed JSON.",
      },
    });
  });

  test.each([
    {},
    { ok: true },
    { ok: false },
    { ok: false, error: { code: 1, message: "bad" } },
  ])("rejects invalid envelope shape %#", async (value: unknown) => {
    planJsonResponse(value);
    await expect(apiRequest("/api/v1/status")).rejects.toBeInstanceOf(ApiError);
  });

  test("propagates network failures and clears its timeout", async () => {
    vi.useFakeTimers();
    const failure = new Error("network offline");
    planFetch(() => Promise.reject(failure));
    await expect(apiRequest("/api/v1/status")).rejects.toBe(failure);
    expect(vi.getTimerCount()).toBe(0);
  });

  test("aborts after ten seconds and clears its timeout", async () => {
    vi.useFakeTimers();
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

    const request = apiRequest("/api/v1/status");
    const rejection = expect(request).rejects.toMatchObject({ name: "AbortError" });
    await vi.advanceTimersByTimeAsync(10_000);
    await rejection;
    expect(getFetchCalls()[0]?.signal?.aborted).toBe(true);
    expect(vi.getTimerCount()).toBe(0);
  });

  test("rejects caller-provided abort signals before fetch", async () => {
    const controller = new AbortController();
    await expect(
      apiRequest("/api/v1/status", { signal: controller.signal }),
    ).rejects.toThrow("Caller-provided abort signals are not supported.");
    expect(getFetchCalls()).toHaveLength(0);
  });

  test("clears the timeout after a successful response", async () => {
    vi.useFakeTimers();
    planFetch(() => jsonResponse(success({ value: 1 })));
    await apiRequest("/api/v1/status");
    expect(vi.getTimerCount()).toBe(0);
  });
});
