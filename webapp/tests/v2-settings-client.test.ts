import { describe, expect, test } from "vitest";
import {
  changePassword,
  getSettings,
  updateSettings,
} from "../src/v2/settingsClient";
import { getFetchCalls, jsonResponse, planFetch } from "./fakeFetch";

const settingsBody = {
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

describe("v2 settings client", () => {
  test("getSettings reads GET /api/v1/settings", async () => {
    planFetch((call) => {
      expect(call.method).toBe("GET");
      expect(call.url).toBe("/api/v1/settings");
      return jsonResponse(settingsBody);
    });
    expect(await getSettings()).toEqual(settingsBody);
  });

  test("updateSettings PUTs a strict partial update and returns the sanitized result", async () => {
    planFetch((call) => {
      expect(call.method).toBe("PUT");
      expect(call.url).toBe("/api/v1/settings");
      expect(call.headers.get("Content-Type")).toBe("application/json");
      expect(call.body).toBe(JSON.stringify({ deviceName: "New name" }));
      return jsonResponse({
        settings: { ...settingsBody, deviceName: "New name" },
        restartRequired: false,
        reconnectRequired: false,
      });
    });
    const result = await updateSettings({ deviceName: "New name" });
    expect(result.settings.deviceName).toBe("New name");
    expect(result.restartRequired).toBe(false);
    expect(result.reconnectRequired).toBe(false);
  });

  test("updateSettings surfaces restartRequired/reconnectRequired for an access-point change", async () => {
    planFetch((call) => {
      expect(call.body).toBe(
        JSON.stringify({
          accessPoint: { ssid: "NewNetwork", passphrase: "new-passphrase" },
        }),
      );
      return jsonResponse({
        settings: { ...settingsBody, apSsid: "NewNetwork" },
        restartRequired: true,
        reconnectRequired: true,
      });
    });
    const result = await updateSettings({
      accessPoint: { ssid: "NewNetwork", passphrase: "new-passphrase" },
    });
    expect(result.restartRequired).toBe(true);
    expect(result.reconnectRequired).toBe(true);
  });

  test("changePassword POSTs the request and expects 204 with no body", async () => {
    planFetch((call) => {
      expect(call.method).toBe("POST");
      expect(call.url).toBe("/api/v1/settings/change-password");
      expect(call.body).toBe(
        JSON.stringify({
          currentPassword: "old-example-password",
          newPassword: "new-example-password",
        }),
      );
      return new Response(null, { status: 204 });
    });
    await expect(
      changePassword({
        currentPassword: "old-example-password",
        newPassword: "new-example-password",
      }),
    ).resolves.toBeUndefined();
  });

  test("changePassword rejects a non-204 success as invalid", async () => {
    planFetch(() => jsonResponse({}, 200));
    await expect(
      changePassword({
        currentPassword: "old-example-password",
        newPassword: "new-example-password",
      }),
    ).rejects.toThrow();
    // The 200 response above still consumed the one planned fetch call; no
    // further assertion needed beyond the rejection itself.
    expect(getFetchCalls()).toHaveLength(1);
  });
});
