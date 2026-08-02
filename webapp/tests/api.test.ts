import { describe, expect, test, vi } from "vitest";
import { ApiError, apiRequest, subscribeUnauthorized } from "../src/api/client";
import { isEmptyRecord, isRecord } from "../src/api/guards";
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

function isValueRecord(value: unknown): value is { value: number | string } {
  return (
    isRecord(value) &&
    Object.keys(value).length === 1 &&
    (typeof value.value === "number" || typeof value.value === "string")
  );
}

describe("apiRequest", () => {
  test("rejects non-API paths before fetch", async () => {
    await expect(apiRequest("/settings", {}, isEmptyRecord)).rejects.toThrow(
      "API requests must use same-origin /api/ paths.",
    );
    expect(getFetchCalls()).toHaveLength(0);
  });

  test("uses same-origin credentials and required JSON headers", async () => {
    planJsonResponse(success({ value: 1 }));
    await expect(
      apiRequest(
        "/api/v1/status",
        { headers: { "X-Trace": "trace-1" } },
        isValueRecord,
      ),
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
    await apiRequest(
      "/api/v1/items",
      { method: "POST", body: "{}" },
      isEmptyRecord,
    );
    expect(getFetchCalls()[0]?.headers.get("Content-Type")).toBe(
      "application/json",
    );

    planJsonResponse(success({}));
    await apiRequest(
      "/api/v1/items",
      {
        method: "POST",
        body: "raw",
        headers: { "Content-Type": "application/octet-stream" },
      },
      isEmptyRecord,
    );
    expect(getFetchCalls()[1]?.headers.get("Content-Type")).toBe(
      "application/octet-stream",
    );
  });

  test("runtime-validates successful response data", async () => {
    planJsonResponse(success({ value: "ok" }));
    await expect(
      apiRequest("/api/v1/status", {}, isValueRecord),
    ).resolves.toEqual({ value: "ok" });

    planJsonResponse(success({ unexpected: true }));
    await expect(
      apiRequest("/api/v1/status", {}, isValueRecord),
    ).rejects.toMatchObject({
      status: 200,
      body: {
        code: "invalid_response",
        message: "The device returned an invalid response payload.",
      },
    });
  });

  test("uses an API failure body for HTTP failures", async () => {
    planJsonResponse(
      {
        ok: false,
        error: { code: "bad_request", message: "Invalid request." },
      },
      400,
    );
    await expect(
      apiRequest("/api/v1/status", {}, isEmptyRecord),
    ).rejects.toMatchObject({
      status: 400,
      body: { code: "bad_request", message: "Invalid request." },
    });
  });

  test("maps an HTTP failure with a success envelope to http_error", async () => {
    planJsonResponse(success({ value: "ignored" }), 503);
    await expect(
      apiRequest("/api/v1/status", {}, isValueRecord),
    ).rejects.toMatchObject({
      status: 503,
      body: {
        code: "http_error",
        message: "Request failed with status 503.",
      },
    });
  });

  test("notifies listeners on 401", async () => {
    let notifications = 0;
    const unsubscribe = subscribeUnauthorized(() => {
      notifications += 1;
    });
    planJsonResponse(
      {
        ok: false,
        error: { code: "auth_required", message: "Sign in." },
      },
      401,
    );
    await expect(
      apiRequest("/api/v1/private", {}, isEmptyRecord),
    ).rejects.toBeInstanceOf(ApiError);
    expect(notifications).toBe(1);

    planJsonResponse(success({}));
    await apiRequest("/api/v1/items", { method: "POST" }, isEmptyRecord);
    unsubscribe();
  });

  test("supports a non-notifying setup-mode probe", async () => {
    let notifications = 0;
    const unsubscribe = subscribeUnauthorized(() => {
      notifications += 1;
    });
    planJsonResponse(
      {
        ok: false,
        error: { code: "auth_required", message: "Normal mode." },
      },
      401,
    );
    await expect(
      apiRequest("/api/v1/setup-state", {}, isEmptyRecord, {
        notifyOnUnauthorized: false,
      }),
    ).rejects.toBeInstanceOf(ApiError);
    expect(notifications).toBe(0);
    unsubscribe();
  });

  test("captures an integer Retry-After header", async () => {
    planFetch(
      () =>
        new Response(
          JSON.stringify({
            ok: false,
            error: { code: "rate_limited", message: "Wait." },
          }),
          {
            status: 429,
            headers: {
              "Content-Type": "application/json",
              "Retry-After": "17",
            },
          },
        ),
    );
    await expect(
      apiRequest("/api/v1/auth/login", {}, isEmptyRecord),
    ).rejects.toMatchObject({ retryAfterSeconds: 17 });
  });

  test("rejects non-JSON and malformed JSON responses", async () => {
    planTextResponse("not json");
    await expect(
      apiRequest("/api/v1/status", {}, isEmptyRecord),
    ).rejects.toMatchObject({
      body: {
        code: "invalid_response",
        message: "The device returned a non-JSON response.",
      },
    });

    planFetch(
      () =>
        new Response("{", {
          status: 200,
          headers: { "Content-Type": "application/json" },
        }),
    );
    await expect(
      apiRequest("/api/v1/status", {}, isEmptyRecord),
    ).rejects.toMatchObject({
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
    {
      ok: true,
      data: {},
      extra: true,
    },
  ])("rejects invalid envelope shape %#", async (value: unknown) => {
    planJsonResponse(value);
    await expect(
      apiRequest("/api/v1/status", {}, isEmptyRecord),
    ).rejects.toBeInstanceOf(ApiError);
  });

  test("propagates network failures and clears its timeout", async () => {
    vi.useFakeTimers();
    const failure = new Error("network offline");
    planFetch(() => Promise.reject(failure));
    await expect(apiRequest("/api/v1/status", {}, isEmptyRecord)).rejects.toBe(
      failure,
    );
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
            reject(
              new DOMException("The operation was aborted.", "AbortError"),
            );
          };
          if (signal.aborted) {
            rejectAbort();
          } else {
            signal.addEventListener("abort", rejectAbort, { once: true });
          }
        }),
    );

    const request = apiRequest("/api/v1/status", {}, isEmptyRecord);
    const rejection = expect(request).rejects.toMatchObject({
      name: "AbortError",
    });
    await vi.advanceTimersByTimeAsync(10_000);
    await rejection;
    expect(getFetchCalls()[0]?.signal?.aborted).toBe(true);
    expect(vi.getTimerCount()).toBe(0);
  });

  test("rejects caller-provided abort signals before fetch", async () => {
    const controller = new AbortController();
    await expect(
      apiRequest(
        "/api/v1/status",
        { signal: controller.signal },
        isEmptyRecord,
      ),
    ).rejects.toThrow("Caller-provided abort signals are not supported.");
    expect(getFetchCalls()).toHaveLength(0);
  });

  test("clears the timeout after a successful response", async () => {
    vi.useFakeTimers();
    planFetch(() => jsonResponse(success({ value: 1 })));
    await apiRequest("/api/v1/status", {}, isValueRecord);
    expect(vi.getTimerCount()).toBe(0);
  });
});
