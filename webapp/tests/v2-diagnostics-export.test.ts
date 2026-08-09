import { describe, expect, test } from "vitest";
import { buildDiagnosticsExportText } from "../src/v2/diagnosticsExport";
import type { DiagnosticsResponse } from "../src/v2/apiTypes";

const diagnostics: DiagnosticsResponse = {
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
    invalidNames: ["bad name.gz"],
    temporaryFiles: ["3.gz.tmp"],
  },
  send: { present: true, state: "completed" },
  subsystems: [{ name: "storage", state: "healthy" }],
};

describe("buildDiagnosticsExportText (TODO_V2 V2-122)", () => {
  test("includes every field the fixed diagnostics schema defines", () => {
    const parsed = JSON.parse(
      buildDiagnosticsExportText(diagnostics),
    ) as unknown;
    expect(parsed).toEqual(diagnostics);
  });

  test("never carries a field beyond the fixed diagnostics schema, even if the input somehow had one", () => {
    const withExtra = {
      ...diagnostics,
      packageName: "sensitive package name",
      macroSource: "a password{ENTER}",
    } as DiagnosticsResponse;
    const parsed = JSON.parse(buildDiagnosticsExportText(withExtra)) as Record<
      string,
      unknown
    >;
    expect(Object.keys(parsed).sort()).toEqual(
      [
        "buildId",
        "firmwareVersion",
        "memory",
        "resetReason",
        "send",
        "storage",
        "subsystems",
        "uptimeMs",
        "usb",
        "wifi",
      ].sort(),
    );
    expect(JSON.stringify(parsed)).not.toContain("sensitive package name");
    expect(JSON.stringify(parsed)).not.toContain("a password");
  });
});
