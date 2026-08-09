import { describe, expect, test } from "vitest";
import { subscribeUnauthorized } from "../src/v2/apiClient";
import {
  factoryResetDevice,
  resetDeviceSettings,
  restartDevice,
  signOut,
} from "../src/v2/deviceActionsClient";
import { jsonResponse, planFetch } from "./fakeFetch";

describe("v2 device actions client", () => {
  test("restartDevice POSTs with no body and no Content-Type header", async () => {
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.url).toBe("/api/v1/device/restart");
      expect(call.headers.has("Content-Type")).toBe(false);
      expect(call.body).toBeUndefined();
      return jsonResponse({
        accepted: true,
        connectionWillClose: true,
        reprovisioningRequired: false,
      });
    });
    const result = await restartDevice();
    expect(result).toEqual({
      accepted: true,
      connectionWillClose: true,
      reprovisioningRequired: false,
    });
  });

  test("resetDeviceSettings sends the exact confirmation phrase and parses the accepted shape", async () => {
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.url).toBe("/api/v1/device/reset-settings");
      expect(call.body).toBe(
        JSON.stringify({ confirmation: "RESET SETTINGS" }),
      );
      return jsonResponse({
        accepted: true,
        connectionWillClose: true,
        reprovisioningRequired: false,
        repositoryBlobsPreserved: true,
      });
    });
    const result = await resetDeviceSettings();
    expect(result.repositoryBlobsPreserved).toBe(true);
    expect(result.reprovisioningRequired).toBe(false);
  });

  test("factoryResetDevice sends the admin password and exact confirmation phrase", async () => {
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.url).toBe("/api/v1/device/factory-reset");
      expect(call.body).toBe(
        JSON.stringify({
          adminPassword: "correct horse battery staple",
          confirmation: "FACTORY RESET",
        }),
      );
      return jsonResponse({
        accepted: true,
        connectionWillClose: true,
        reprovisioningRequired: true,
        repositoryBlobsPreserved: false,
      });
    });
    const result = await factoryResetDevice("correct horse battery staple");
    expect(result.repositoryBlobsPreserved).toBe(false);
    expect(result.reprovisioningRequired).toBe(true);
  });

  test("factoryResetDevice rejects a too-short admin password before ever calling fetch", async () => {
    await expect(factoryResetDevice("short")).rejects.toThrow(
      "Invalid factory-reset request.",
    );
  });

  test("signOut POSTs to /api/v1/auth/logout with no body", async () => {
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.url).toBe("/api/v1/auth/logout");
      expect(call.headers.has("Content-Type")).toBe(false);
      return new Response(null, { status: 204 });
    });
    await expect(signOut()).resolves.toBeUndefined();
  });

  test("signOut still notifies unauthorized listeners on a 401 (session already gone)", async () => {
    planFetch(() =>
      jsonResponse(
        { error: { code: "unauthorized", message: "Session expired." } },
        401,
      ),
    );
    let notified = false;
    const unsubscribe = subscribeUnauthorized(() => {
      notified = true;
    });
    await expect(signOut()).rejects.toThrow();
    unsubscribe();
    expect(notified).toBe(true);
  });
});
