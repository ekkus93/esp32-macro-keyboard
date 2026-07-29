import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { setCsrfToken } from "../src/api/client";
import { ConnectivityBanner } from "../src/components/ConnectivityBanner";
import { DiagnosticsPage } from "../src/features/settings/DiagnosticsPage";
import { PackageOperationsPage } from "../src/features/settings/PackageOperationsPage";
import { SettingsPage } from "../src/features/settings/SettingsPage";
import { macroSet, settings } from "./appFixtures";
import { getFetchCalls, jsonResponse, planFetch, planJsonResponse } from "./fakeFetch";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setInputValue,
} from "./render";

function success(data: unknown): object {
  return { ok: true, data };
}

describe("management screens", () => {
  test("shows live redacted storage and quarantine data", async () => {
    planJsonResponse(
      success({
        verified: false,
        webMounted: true,
        dataMounted: true,
        quarantineCount: 1,
        damagedQuarantineCount: 1,
      }),
    );
    planJsonResponse(
      success({
        damagedCount: 1,
        items: [
          {
            id: "99999999-9999-4999-8999-999999999999",
            sourcePath: "/data/sets/source.json",
            evidencePath: "/data/quarantine/evidence.json",
            reason: "checksum mismatch",
          },
        ],
      }),
    );
    const view = await render(<DiagnosticsPage />);
    await flushReact();

    expect(document.body.textContent).toContain("Required filesystems mounted");
    expect(document.body.textContent).toContain("checksum mismatch");
    expect(document.body.textContent).toContain(
      "1 quarantine record is damaged",
    );
    expect(buttonWithText("Run full storage verification").disabled).toBe(true);
    expect(buttonWithText("Load full diagnostics").disabled).toBe(true);
    expect(getFetchCalls().map((call) => call.url)).toEqual([
      "/api/v1/diagnostics/storage",
      "/api/v1/diagnostics/quarantine",
    ]);
    await view.unmount();
  });

  test("keeps deferred package operations visibly disabled", async () => {
    const view = await render(
      <PackageOperationsPage activeSet={macroSet} initialSection="import" />,
    );

    expect(document.body.textContent).toContain("503 Service Unavailable");
    expect(buttonWithText("Import as new set").disabled).toBe(true);
    expect(buttonWithText("Replace selected set").disabled).toBe(true);
    expect(buttonWithText("Restore full backup").disabled).toBe(true);
    expect(document.body.textContent).toContain(
      "all-object validation service",
    );
    expect(getFetchCalls()).toHaveLength(0);
    await view.unmount();
  });

  test("shows physical-confirmation state for restart", async () => {
    setCsrfToken("csrf-restart");
    const response = { resolve: null as ((response: Response) => void) | null };
    const view = await render(
      <SettingsPage
        navigate={() => undefined}
        onUpdated={() => undefined}
        settings={settings}
      />,
    );
    await click(buttonWithText("Restart device"));
    planFetch(
      () =>
        new Promise<Response>((resolve) => {
          response.resolve = resolve;
        }),
    );
    await click(buttonWithText("Confirm Restart device"));
    await flushReact();

    expect(document.body.textContent).toContain(
      "Press the confirmation button on the device.",
    );
    expect(getFetchCalls()[0]?.url).toBe("/api/v1/device/restart");
    expect(getFetchCalls()[0]?.method).toBe("POST");

    const resolveResponse = response.resolve;
    if (resolveResponse === null) {
      throw new Error("Restart request was not issued.");
    }
    await act(async () => {
      resolveResponse(jsonResponse(success({ restartScheduled: true }), 202));
      await Promise.resolve();
    });
    await flushReact();
    expect(document.body.textContent).toContain("Restart scheduled.");
    await view.unmount();
  });

  test("requires an exact destructive factory-reset phrase", async () => {
    const view = await render(
      <SettingsPage
        navigate={() => undefined}
        onUpdated={() => undefined}
        settings={settings}
      />,
    );
    await click(buttonWithText("Factory reset"));
    expect(buttonWithText("Confirm Factory reset device").disabled).toBe(true);
    await setInputValue(
      requiredElement("#factory-reset-confirmation", HTMLInputElement),
      "FACTORY RESET",
    );
    expect(buttonWithText("Confirm Factory reset device").disabled).toBe(false);
    await view.unmount();
  });

  test("announces offline and reconnect state and triggers a live refresh", async () => {
    const onReconnect = vi.fn<() => void>();
    const view = await render(<ConnectivityBanner onReconnect={onReconnect} />);

    await act(async () => {
      window.dispatchEvent(new Event("offline"));
      await Promise.resolve();
    });
    expect(document.body.textContent).toContain("Offline.");

    await act(async () => {
      window.dispatchEvent(new Event("online"));
      await Promise.resolve();
    });
    expect(document.body.textContent).toContain("Connection restored.");
    expect(onReconnect).toHaveBeenCalledOnce();
    await view.unmount();
  });
});
