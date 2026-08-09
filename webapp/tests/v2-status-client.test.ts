import { describe, expect, test } from "vitest";
import { getStatus } from "../src/v2/statusClient";
import { planJsonResponse } from "./fakeFetch";

const status = {
  provisioned: true,
  deviceName: "Desk Macro Keyboard",
  firmwareVersion: "0.2.0",
  buildId: "abc123",
  uptimeMs: 1000,
  usb: { state: "ready" },
  accessPoint: { state: "started", ssid: "MacroKeyboard", clientCount: 1 },
  station: { configured: false, state: "idle", ssid: null, ipv4: null },
  storage: {
    state: "healthy",
    totalBytes: 100,
    usedBytes: 10,
    remainingBytes: 90,
    blobCount: 1,
  },
  send: { present: false, state: null },
};

describe("v2 status client", () => {
  test("getStatus reads GET /api/v1/status", async () => {
    planJsonResponse(status);
    const result = await getStatus();
    expect(result).toEqual(status);
  });
});
