import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { ConnectivityBanner } from "../src/components/ConnectivityBanner";
import { DiagnosticsPage } from "../src/features/settings/DiagnosticsPage";
import { PackageOperationsPage } from "../src/features/settings/PackageOperationsPage";
import { SettingsPage } from "../src/features/settings/SettingsPage";
import { macroPackage, settings } from "./appFixtures";
import {
  getFetchCalls,
  jsonResponse,
  planFetch,
  planJsonResponse,
  planTextResponse,
} from "./fakeFetch";
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
  test("shows live redacted storage data", async () => {
    planJsonResponse(
      success({
        verified: false,
        webMounted: true,
        dataMounted: true,
        usedBytes: 20480,
        totalBytes: 491520,
        remainingBytes: 471040,
        packageFileMaxBytes: 32768,
        temporariesRemovedAtBoot: 0,
        discardedObjectCount: 0,
        discardedObjects: [],
      }),
    );
    const view = await render(<DiagnosticsPage />);
    await flushReact();

    expect(document.body.textContent).toContain("Required filesystems mounted");
    expect(buttonWithText("Run full storage verification").disabled).toBe(true);
    expect(buttonWithText("Load full diagnostics").disabled).toBe(false);
    /* Quarantine was removed (SPEC 13.6): the screen must not request it. */
    expect(getFetchCalls().map((call) => call.url)).toEqual([
      "/api/v1/diagnostics/storage",
    ]);
    await view.unmount();
  });

  // SPEC 24.5 item: storage error UI
  test("surfaces a storage read failure instead of showing nothing", async () => {
    /* The success path above is the easy one. This is the path that matters:
       storage diagnostics is the screen a user opens precisely when the device
       is misbehaving, so a failure to read storage health has to say so. The
       failure modes it must not have are showing an empty screen that looks
       healthy, or leaving the verification button live as though the numbers
       behind it were real. */
    planJsonResponse(
      {
        ok: false,
        error: { code: "storage_unavailable", message: "storage unavailable" },
      },
      503,
    );
    const view = await render(<DiagnosticsPage />);
    await flushReact();

    const alert = document.querySelector<HTMLElement>("[role='alert']");
    expect(alert).not.toBeNull();
    expect(alert?.textContent).toContain("storage unavailable");

    /* No health was read, so nothing may claim the filesystems are mounted, and
       the whole storage-health panel stays absent rather than rendering with
       empty or zeroed figures. An action whose meaning depends on numbers the
       page does not have is not offered at all -- stronger than offering it
       disabled, and the behaviour this asserts. */
    expect(document.body.textContent).not.toContain(
      "Required filesystems mounted",
    );
    expect(
      Array.from(document.querySelectorAll("button")).find(
        (button) =>
          button.textContent?.trim() === "Run full storage verification",
      ),
    ).toBeUndefined();
    await view.unmount();
  });

  test("loads full subsystem diagnostics on demand", async () => {
    planJsonResponse(
      success({
        verified: false,
        webMounted: true,
        dataMounted: true,
        usedBytes: 20480,
        totalBytes: 491520,
        remainingBytes: 471040,
        packageFileMaxBytes: 32768,
        temporariesRemovedAtBoot: 0,
        discardedObjectCount: 0,
        discardedObjects: [],
      }),
    );
    planJsonResponse(
      success({
        buildId: "abcdef0123456789",
        firmwareVersion: "1.2.3",
        schemaVersion: 1,
        resetReason: "power-on",
        uptimeMs: 60000,
        freeHeapBytes: 200000,
        minFreeHeapBytes: 150000,
        stack: { controlsWords: 512, executorWords: 1024 },
        webfs: { ok: true, totalBytes: 1000000, usedBytes: 400000 },
        userdata: { ok: true, totalBytes: 2000000, usedBytes: 900000 },
        executionState: "idle",
        subsystems: [
          { name: "app_core", state: "healthy" },
          { name: "storage", state: "healthy" },
          { name: "repository", state: "healthy" },
          { name: "auth", state: "healthy" },
          { name: "usb", state: "healthy" },
          { name: "executor", state: "healthy" },
          { name: "controls", state: "degraded" },
          { name: "wifi", state: "healthy" },
          { name: "http", state: "healthy" },
        ],
      }),
    );
    const view = await render(<DiagnosticsPage />);
    await flushReact();

    await click(buttonWithText("Load full diagnostics"));
    await flushReact();

    expect(document.body.textContent).toContain("abcdef0123456789");
    expect(document.body.textContent).toContain("controls: degraded");
    expect(getFetchCalls().map((call) => call.url)).toEqual([
      "/api/v1/diagnostics/storage",
      "/api/v1/diagnostics",
    ]);
    await view.unmount();
  });

  test("enables transactional replacement and full restore", async () => {
    const view = await render(
      <PackageOperationsPage
        activePackage={macroPackage}
        initialSection="import"
      />,
    );

    expect(document.body.textContent).toContain(
      "all-or-nothing restore are available",
    );
    expect(buttonWithText("Import as new package").disabled).toBe(true);
    expect(buttonWithText("Replace selected package").disabled).toBe(true);
    expect(buttonWithText("Restore full backup").disabled).toBe(true);
    expect(
      requiredElement("#replacement-package", HTMLInputElement).disabled,
    ).toBe(false);
    expect(
      requiredElement("#restore-backup-package", HTMLInputElement).disabled,
    ).toBe(false);
    expect(getFetchCalls()).toHaveLength(0);
    await view.unmount();
  });

  test("validates and confirms a transactional package replacement", async () => {
    const replacement = {
      ...macroPackage,
      revision: macroPackage.revision + 1,
      name: "Imported Replacement",
    };
    const packageDocument = {
      schema_version: 1,
      package_type: "package",
      packages: [replacement],
      macros: [],
    } as const;
    const packageText = JSON.stringify(packageDocument);
    const file = new File([packageText], "replacement.json", {
      type: "application/json",
    });
    Object.defineProperty(file, "text", {
      configurable: true,
      value: () => Promise.resolve(packageText),
    });
    const onPackageReplaced = vi.fn();
    const view = await render(
      <PackageOperationsPage
        activePackage={macroPackage}
        initialSection="import"
        onPackageReplaced={onPackageReplaced}
      />,
    );
    const input = requiredElement("#replacement-package", HTMLInputElement);
    Object.defineProperty(input, "files", {
      configurable: true,
      value: [file],
    });
    await act(async () => {
      input.dispatchEvent(new Event("change", { bubbles: true }));
      await Promise.resolve();
    });
    await flushReact();
    expect(buttonWithText("Replace selected package").disabled).toBe(false);

    await click(buttonWithText("Replace selected package"));
    await setInputValue(
      requiredElement("#replacement-confirmation", HTMLInputElement),
      `REPLACE ${macroPackage.name}`,
    );
    planJsonResponse(success(replacement));
    await click(buttonWithText("Confirm replacement"));
    await flushReact();

    expect(getFetchCalls()).toHaveLength(1);
    const call = getFetchCalls()[0];
    expect(call?.url).toBe("/api/v1/package/import");
    expect(call?.method).toBe("POST");
    await view.unmount();
  });

  test("validates and confirms a full backup restore", async () => {
    const backupDocument = {
      schema_version: 1,
      package_type: "backup",
      packages: [macroPackage],
      macros: [],
    } as const;
    const backupText = JSON.stringify(backupDocument);
    const file = new File([backupText], "full-backup.json", {
      type: "application/json",
    });
    Object.defineProperty(file, "text", {
      configurable: true,
      value: () => Promise.resolve(backupText),
    });
    const onBackupRestored = vi.fn();
    const view = await render(
      <PackageOperationsPage
        activePackage={macroPackage}
        initialSection="import"
        onBackupRestored={onBackupRestored}
      />,
    );
    const input = requiredElement("#restore-backup-package", HTMLInputElement);
    Object.defineProperty(input, "files", {
      configurable: true,
      value: [file],
    });
    await act(async () => {
      input.dispatchEvent(new Event("change", { bubbles: true }));
      await Promise.resolve();
    });
    await flushReact();
    expect(buttonWithText("Restore full backup").disabled).toBe(false);

    await click(buttonWithText("Restore full backup"));
    expect(buttonWithText("Confirm full restore").disabled).toBe(true);
    await setInputValue(
      requiredElement("#restore-confirmation", HTMLInputElement),
      "RESTORE FULL BACKUP",
    );
    planJsonResponse(
      success({
        restored: true,
        reloadRequired: true,
        setsRestored: 1,
        setsFailed: 0,
        packages: [{ packageId: macroPackage.id, restored: true }],
      }),
    );
    await click(buttonWithText("Confirm full restore"));
    await flushReact();

    expect(getFetchCalls()).toHaveLength(1);
    const call = getFetchCalls()[0];
    expect(call?.url).toBe("/api/v1/restore");
    expect(call?.method).toBe("POST");
    await view.unmount();
  });

  test("downloads a strictly validated raw package package", async () => {
    const packageText = JSON.stringify({
      schema_version: 1,
      package_type: "package",
      packages: [macroPackage],
      macros: [],
    });
    planTextResponse(packageText, 200, "application/json");
    const saveFile = vi.fn<(filename: string, text: string) => void>();
    const view = await render(
      <PackageOperationsPage
        activePackage={macroPackage}
        initialSection="export"
        saveFile={saveFile}
      />,
    );

    await click(buttonWithText("Export selected package"));
    await flushReact();

    expect(getFetchCalls()).toHaveLength(1);
    expect(getFetchCalls()[0]?.url).toBe(
      `/api/v1/package/${macroPackage.id}/export`,
    );
    expect(getFetchCalls()[0]?.method).toBe("GET");
    expect(getFetchCalls()[0]?.credentials).toBe("same-origin");
    expect(saveFile).toHaveBeenCalledWith(
      `macro-package-${macroPackage.id}.json`,
      packageText,
    );
    expect(document.body.textContent).toContain(
      `Exported ${macroPackage.name} as ${String(new Blob([packageText]).size)} bytes.`,
    );
    await view.unmount();
  });

  test("downloads a strictly validated raw full backup", async () => {
    const backupText = JSON.stringify({
      schema_version: 1,
      package_type: "backup",
      packages: [macroPackage],
      macros: [],
    });
    planTextResponse(backupText, 200, "application/json");
    const saveFile = vi.fn<(filename: string, text: string) => void>();
    const view = await render(
      <PackageOperationsPage
        activePackage={macroPackage}
        initialSection="export"
        saveFile={saveFile}
      />,
    );

    await click(buttonWithText("Create full backup"));
    await flushReact();

    expect(getFetchCalls()).toHaveLength(1);
    expect(getFetchCalls()[0]?.url).toBe("/api/v1/backup");
    expect(getFetchCalls()[0]?.method).toBe("GET");
    expect(getFetchCalls()[0]?.credentials).toBe("same-origin");
    expect(saveFile).toHaveBeenCalledWith(
      "macro-keyboard-backup.json",
      backupText,
    );
    expect(document.body.textContent).toContain(
      `Created full backup as ${String(new Blob([backupText]).size)} bytes.`,
    );
    await view.unmount();
  });

  test("shows physical-confirmation state for restart", async () => {
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
      "Send the confirm command on the device serial console.",
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
