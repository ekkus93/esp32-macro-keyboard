import { describe, expect, test, vi } from "vitest";
import { DiagnosticsPage } from "../src/features/settings/v2/DiagnosticsPage";
import type { DiagnosticsPageDependencies } from "../src/features/settings/v2/DiagnosticsPage";
import type { DiagnosticsResponse } from "../src/v2/apiTypes";
import { buttonWithText, click, render } from "./render";

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
  send: { present: false, state: null },
  subsystems: [{ name: "storage", state: "healthy" }],
};

function deps(
  overrides: Partial<DiagnosticsPageDependencies> = {},
): DiagnosticsPageDependencies {
  return {
    getDiagnostics: vi.fn().mockResolvedValue(diagnostics),
    copyToClipboard: vi.fn().mockResolvedValue(undefined),
    saveAsFile: vi.fn(),
    ...overrides,
  };
}

describe("DiagnosticsPage (TODO_V2 V2-122)", () => {
  test("renders every field of the fixed schema and nothing else", async () => {
    const { container, unmount } = await render(
      <DiagnosticsPage dependencies={deps()} onBack={vi.fn()} />,
    );
    await Promise.resolve();
    await Promise.resolve();
    expect(container.textContent).toContain("0.2.0");
    expect(container.textContent).toContain("git-abcdef0");
    expect(container.textContent).toContain("power_on");
    expect(container.textContent).toContain("200000 bytes");
    expect(container.textContent).toContain("ready");
    expect(container.textContent).toContain("running");
    expect(container.textContent).toContain("bad name.gz");
    expect(container.textContent).toContain("3.gz.tmp");
    expect(container.textContent).toContain("No send has run since boot.");
    expect(container.textContent).toContain("storage: healthy");
    await unmount();
  });

  test("shows a retry control on load failure and recovers", async () => {
    const getDiagnostics = vi
      .fn()
      .mockRejectedValueOnce(new Error("device unreachable"))
      .mockResolvedValueOnce(diagnostics);
    const { container, unmount } = await render(
      <DiagnosticsPage
        dependencies={deps({ getDiagnostics })}
        onBack={vi.fn()}
      />,
    );
    await Promise.resolve();
    await Promise.resolve();
    expect(container.textContent).toContain("device unreachable");
    await click(buttonWithText("Retry"));
    await Promise.resolve();
    await Promise.resolve();
    expect(container.textContent).toContain("0.2.0");
    await unmount();
  });

  test("Copy diagnostics calls copyToClipboard with the filtered export text", async () => {
    const copyToClipboard = vi.fn().mockResolvedValue(undefined);
    const { unmount } = await render(
      <DiagnosticsPage
        dependencies={deps({ copyToClipboard })}
        onBack={vi.fn()}
      />,
    );
    await Promise.resolve();
    await Promise.resolve();
    await click(buttonWithText("Copy diagnostics"));
    await Promise.resolve();
    expect(copyToClipboard).toHaveBeenCalledOnce();
    const [text] = copyToClipboard.mock.calls[0] as [string];
    expect(JSON.parse(text)).toEqual(diagnostics);
    await unmount();
  });

  test("Download diagnostics calls saveAsFile with a JSON payload matching the schema", async () => {
    const saveAsFile = vi.fn();
    const { unmount } = await render(
      <DiagnosticsPage dependencies={deps({ saveAsFile })} onBack={vi.fn()} />,
    );
    await Promise.resolve();
    await Promise.resolve();
    await click(buttonWithText("Download diagnostics"));
    expect(saveAsFile).toHaveBeenCalledOnce();
    const [bytes, filename, mimeType] = saveAsFile.mock.calls[0] as [
      Uint8Array,
      string,
      string,
    ];
    expect(filename).toBe("diagnostics.json");
    expect(mimeType).toBe("application/json");
    expect(JSON.parse(new TextDecoder().decode(bytes))).toEqual(diagnostics);
    await unmount();
  });

  test("Back to Settings calls onBack", async () => {
    const onBack = vi.fn();
    const { unmount } = await render(
      <DiagnosticsPage dependencies={deps()} onBack={onBack} />,
    );
    await Promise.resolve();
    await Promise.resolve();
    await click(buttonWithText("Back to Settings"));
    expect(onBack).toHaveBeenCalledOnce();
    await unmount();
  });
});
