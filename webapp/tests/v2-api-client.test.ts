import { describe, expect, test } from "vitest";
import {
  isBlobCreatedResponse,
  isBlobListResponse,
  isSettingsResponse,
} from "../src/v2/apiContracts";
import {
  V2ApiError,
  forceSessionEnded,
  subscribeUnauthorized,
  v2DeleteNoContent,
  v2DeleteWithUnvalidatedJson,
  v2GetBinary,
  v2GetJson,
  v2PostBinary,
  v2PostJson,
  v2PostNoContent,
  v2PutJson,
} from "../src/v2/apiClient";
import {
  assertNoPendingFetchPlans,
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
  textResponse,
} from "./fakeFetch";

const settingsExample = {
  deviceName: "Desk Macro Keyboard",
  requireSerialConfirmation: false,
  sendMode: "quick",
  snapshotRetentionTarget: 5,
  showMacroSourcePreviews: false,
  lastSelectedPackageId: null,
  apSsid: "MacroKeyboard",
  stationConfigured: false,
  stationSsid: null,
};

describe("v2 API client", () => {
  test("v2GetJson resolves the validated payload", async () => {
    planJsonResponse(settingsExample);
    const settings = await v2GetJson("/api/v1/settings", isSettingsResponse);
    expect(settings).toEqual(settingsExample);
    const [call] = getFetchCalls();
    expect(call?.method).toBe("GET");
    expect(call?.url).toBe("/api/v1/settings");
    expect(call?.credentials).toBe("same-origin");
  });

  test("v2GetJson rejects a path outside /api/", async () => {
    await expect(v2GetJson("/settings", isSettingsResponse)).rejects.toThrow(
      "same-origin /api/ paths",
    );
    assertNoPendingFetchPlans();
  });

  test("v2GetJson throws V2ApiError with the parsed error detail on failure", async () => {
    planJsonResponse(
      { error: { code: "not_found", message: "No such settings." } },
      404,
    );
    await expect(
      v2GetJson("/api/v1/settings", isSettingsResponse),
    ).rejects.toMatchObject({
      name: "V2ApiError",
      status: 404,
      code: "not_found",
      message: "No such settings.",
    });
  });

  test("v2GetJson throws when the payload fails the guard", async () => {
    planJsonResponse({ unexpected: true });
    await expect(
      v2GetJson("/api/v1/settings", isSettingsResponse),
    ).rejects.toMatchObject({ code: "invalid_response" });
  });

  test("v2GetJson throws on a non-JSON content type", async () => {
    planFetch(() => textResponse("not json"));
    await expect(
      v2GetJson("/api/v1/settings", isSettingsResponse),
    ).rejects.toMatchObject({ code: "invalid_response" });
  });

  test("401 notifies unauthorized listeners by default", async () => {
    let notified = 0;
    const unsubscribe = subscribeUnauthorized(() => {
      notified += 1;
    });
    planJsonResponse(
      { error: { code: "unauthorized", message: "Session expired." } },
      401,
    );
    await expect(
      v2GetJson("/api/v1/settings", isSettingsResponse),
    ).rejects.toMatchObject({ status: 401 });
    expect(notified).toBe(1);
    unsubscribe();
  });

  test("401 does not notify when notifyOnUnauthorized is false", async () => {
    let notified = 0;
    const unsubscribe = subscribeUnauthorized(() => {
      notified += 1;
    });
    planJsonResponse(
      { error: { code: "unauthorized", message: "Session expired." } },
      401,
    );
    await expect(
      v2GetJson("/api/v1/settings", isSettingsResponse, {
        notifyOnUnauthorized: false,
      }),
    ).rejects.toMatchObject({ status: 401 });
    expect(notified).toBe(0);
    unsubscribe();
  });

  test("v2PostJson sends a JSON body and validates the response", async () => {
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.headers.get("Content-Type")).toBe("application/json");
      expect(call.body).toBe(JSON.stringify({ lastSelectedPackageId: null }));
      return jsonResponse(
        {
          settings: settingsExample,
          restartRequired: false,
          reconnectRequired: false,
        },
        200,
      );
    });
    const result = await v2PostJson(
      "/api/v1/settings",
      { lastSelectedPackageId: null },
      (value): value is { settings: unknown } =>
        typeof value === "object" && value !== null && "settings" in value,
    );
    expect(result).toEqual({
      settings: settingsExample,
      restartRequired: false,
      reconnectRequired: false,
    });
  });

  test("v2PostJson with an undefined body sends no body and no Content-Type header (bodyless routes, e.g. restart)", async () => {
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.headers.has("Content-Type")).toBe(false);
      expect(call.body).toBeUndefined();
      return jsonResponse({
        accepted: true,
        connectionWillClose: true,
        reprovisioningRequired: false,
      });
    });
    const result = await v2PostJson(
      "/api/v1/device/restart",
      undefined,
      (value): value is unknown => value !== undefined,
    );
    expect(result).toEqual({
      accepted: true,
      connectionWillClose: true,
      reprovisioningRequired: false,
    });
  });

  test("v2PostNoContent with a body sends JSON and expects 204", async () => {
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.headers.get("Content-Type")).toBe("application/json");
      expect(call.body).toBe(JSON.stringify({ a: 1 }));
      return new Response(null, { status: 204 });
    });
    await expect(
      v2PostNoContent("/api/v1/settings/change-password", { a: 1 }),
    ).resolves.toBeUndefined();
  });

  test("v2PostNoContent with an undefined body sends no body and no Content-Type header (e.g. sign-out)", async () => {
    planFetch((call) => {
      expect(call.headers.has("Content-Type")).toBe(false);
      expect(call.body).toBeUndefined();
      return new Response(null, { status: 204 });
    });
    await expect(
      v2PostNoContent("/api/v1/auth/logout", undefined),
    ).resolves.toBeUndefined();
  });

  test("v2PostNoContent throws when the status is not 204", async () => {
    planFetch(() => jsonResponse({}, 200));
    await expect(
      v2PostNoContent("/api/v1/auth/logout", undefined),
    ).rejects.toMatchObject({ code: "invalid_response" });
  });

  test("v2PostNoContent surfaces the error envelope on failure", async () => {
    planJsonResponse(
      { error: { code: "unauthorized", message: "Session expired." } },
      401,
    );
    await expect(
      v2PostNoContent("/api/v1/auth/logout", undefined),
    ).rejects.toBeInstanceOf(V2ApiError);
  });

  test("forceSessionEnded runs every subscribeUnauthorized listener", () => {
    let notified = 0;
    const unsubscribe = subscribeUnauthorized(() => {
      notified += 1;
    });
    forceSessionEnded();
    expect(notified).toBe(1);
    unsubscribe();
  });

  test("v2PutJson issues a PUT request", async () => {
    planFetch((call) => {
      expect(call.method).toBe("PUT");
      return jsonResponse(
        {
          settings: settingsExample,
          restartRequired: false,
          reconnectRequired: false,
        },
        200,
      );
    });
    await v2PutJson(
      "/api/v1/settings",
      { deviceName: "New name" },
      (value): value is unknown => value !== undefined,
    );
  });

  test("v2DeleteNoContent succeeds on 204", async () => {
    planFetch((call) => {
      expect(call.method).toBe("DELETE");
      return new Response(null, { status: 204 });
    });
    await expect(v2DeleteNoContent("/api/v1/blob/3")).resolves.toBeUndefined();
  });

  test("v2DeleteNoContent throws when the status is not 204", async () => {
    planFetch(() => new Response(null, { status: 200 }));
    await expect(v2DeleteNoContent("/api/v1/blob/3")).rejects.toMatchObject({
      code: "invalid_response",
    });
  });

  test("v2DeleteNoContent surfaces the error envelope on failure", async () => {
    planJsonResponse(
      { error: { code: "not_found", message: "No such blob." } },
      404,
    );
    await expect(v2DeleteNoContent("/api/v1/blob/9")).rejects.toBeInstanceOf(
      V2ApiError,
    );
  });

  test("v2DeleteWithUnvalidatedJson returns the raw parsed value", async () => {
    planFetch(() => jsonResponse({ id: "abc" }, 202));
    const value = await v2DeleteWithUnvalidatedJson("/api/v1/send");
    expect(value).toEqual({ id: "abc" });
  });

  test("v2PostBinary uploads bytes with the gzip content type", async () => {
    const bytes = new Uint8Array([1, 2, 3]);
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.headers.get("Content-Type")).toBe("application/gzip");
      return jsonResponse({ id: "4", sizeBytes: 3 }, 201);
    });
    const created = await v2PostBinary(
      "/api/v1/blob",
      bytes,
      "application/gzip",
      isBlobCreatedResponse,
    );
    expect(created).toEqual({ id: "4", sizeBytes: 3 });
  });

  test("v2PostBinary requires exactly 201, per SPEC_V2 §10.3 / TODO_V2 V2-110 'Mark saved only after 201 Created'", async () => {
    const bytes = new Uint8Array([1, 2, 3]);
    planFetch(() => jsonResponse({ id: "4", sizeBytes: 3 }, 200));
    await expect(
      v2PostBinary(
        "/api/v1/blob",
        bytes,
        "application/gzip",
        isBlobCreatedResponse,
      ),
    ).rejects.toMatchObject({ code: "invalid_response" });
  });

  test("v2GetBinary returns raw bytes and checks the content type", async () => {
    const bytes = new Uint8Array([9, 9, 9]);
    planFetch(
      () =>
        new Response(bytes, {
          status: 200,
          headers: { "Content-Type": "application/gzip" },
        }),
    );
    const result = await v2GetBinary("/api/v1/blob/1", "application/gzip");
    expect(result).toEqual(bytes);
  });

  test("v2GetBinary rejects an unexpected content type", async () => {
    planFetch(
      () =>
        new Response(new Uint8Array([1]), {
          status: 200,
          headers: { "Content-Type": "text/plain" },
        }),
    );
    await expect(
      v2GetBinary("/api/v1/blob/1", "application/gzip"),
    ).rejects.toMatchObject({ code: "invalid_response" });
  });

  test("blob list guard validates the whole response shape", async () => {
    planJsonResponse({
      blobs: [
        { id: "3", sizeBytes: 10 },
        { id: "2", sizeBytes: 20 },
      ],
      usedBytes: 30,
      remainingBytes: 100,
    });
    const list = await v2GetJson("/api/v1/blob", isBlobListResponse);
    expect(list.blobs).toHaveLength(2);
  });
});
