import { describe, expect, test } from "vitest";
import { getDiagnostics } from "../src/v2/diagnosticsClient";
import { jsonResponse, planFetch } from "./fakeFetch";

const diagnostics = {
  firmwareVersion: "0.2.0",
  buildId: "git-abcdef0",
  resetReason: "power_on",
  uptimeMs: 123456,
  memory: {
    freeHeapBytes: 200000,
    minimumFreeHeapBytes: 180000,
    largestFreeBlockBytes: 120000,
  },
  usb: { state: "ready" },
  wifi: { accessPointState: "running", stationState: "disabled" },
  storage: {
    state: "ready",
    webfsTotalBytes: 1048576,
    webfsUsedBytes: 500000,
    userdataTotalBytes: 524288,
    userdataUsedBytes: 4096,
    blobCount: 3,
    invalidNames: [],
    temporaryFiles: [],
  },
  send: { present: false, state: null },
  subsystems: [],
};

describe("v2 diagnostics client", () => {
  test("getDiagnostics reads GET /api/v1/diagnostics", async () => {
    planFetch((call) => {
      expect(call.method).toBe("GET");
      expect(call.url).toBe("/api/v1/diagnostics");
      return jsonResponse(diagnostics);
    });
    expect(await getDiagnostics()).toEqual(diagnostics);
  });

  test("getDiagnostics rejects a response carrying an unexpected field", async () => {
    planFetch(() =>
      jsonResponse({
        ...diagnostics,
        packageName: "should never appear here",
      }),
    );
    await expect(getDiagnostics()).rejects.toThrow();
  });
});
