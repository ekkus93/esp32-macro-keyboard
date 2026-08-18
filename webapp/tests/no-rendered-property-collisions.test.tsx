import { describe, expect, test, vi } from "vitest";

import { DiagnosticsPage } from "../src/features/settings/v2/DiagnosticsPage";
import type { DiagnosticsPageDependencies } from "../src/features/settings/v2/DiagnosticsPage";
import { SettingsPage } from "../src/features/settings/v2/SettingsPage";
import type { SettingsPageDependencies } from "../src/features/settings/v2/SettingsPage";
import { PackageManagementPage } from "../src/features/macros/v2/PackageManagementPage";
import type { DiagnosticsResponse, SettingsResponse } from "../src/v2/apiTypes";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import { findPropertyCollisions } from "./tailwindClassCollisions";
import { renderMacrosPage } from "./macrosPageHarness";
import { render } from "./render";
import type { RenderResult } from "./render";

/**
 * WEBAPP_TAILWIND_TODO_2026-08-18.md T2-2: the SPEC §3 rule-4 invariant
 * ("no element carries two utilities that set the same CSS property"),
 * generalized from component-variant-maps.test.tsx's six known-risky maps
 * (T2-1) to every element on a real, populated page -- an executable check
 * standing in for the one-off Python/regex scan the migration review ran by
 * hand. jsdom is enough: this only reads `className` strings off rendered
 * elements, never anything jsdom cannot compute (layout, actual CSS).
 *
 * Four structurally distinct pages, covering the app's shapes that matter
 * for this check: `MacrosPage` (a list of `Card` rows, `StatusBadge`,
 * `SendStatus`), `DiagnosticsPage` (many `Card`s, dense `dl`/`dt`/`dd`
 * grids), `SettingsPage` (`Card`s, `FormActions`, `PageHeading`,
 * `HeaderActions`), `PackageManagementPage` (`Card` rows, a create form).
 * Extending this to another page component follows the identical pattern --
 * render it populated, call `assertNoRenderedPropertyCollisions`.
 */

function assertNoRenderedPropertyCollisions(view: RenderResult): void {
  const elements = view.container.querySelectorAll<HTMLElement>("*");
  const failures: string[] = [];
  for (const element of elements) {
    if (element.className === "" || typeof element.className !== "string") {
      continue;
    }
    const collisions = findPropertyCollisions(element.className);
    if (collisions.length > 0) {
      failures.push(
        `<${element.tagName} class="${element.className}">: ${collisions.join("; ")}`,
      );
    }
  }
  expect(failures, failures.join("\n")).toEqual([]);
}

describe("no rendered page has a same-property class collision", () => {
  test("MacrosPage", async () => {
    const view = await renderMacrosPage();
    assertNoRenderedPropertyCollisions(view);
    await view.unmount();
  });

  test("DiagnosticsPage", async () => {
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
    const dependencies: DiagnosticsPageDependencies = {
      getDiagnostics: vi.fn().mockResolvedValue(diagnostics),
      copyToClipboard: vi.fn().mockResolvedValue(undefined),
      saveAsFile: vi.fn(),
    };
    const view = await render(
      <DiagnosticsPage dependencies={dependencies} onBack={vi.fn()} />,
    );
    await Promise.resolve();
    await Promise.resolve();
    assertNoRenderedPropertyCollisions(view);
    await view.unmount();
  });

  test("SettingsPage", async () => {
    const packageId = "550e8400-e29b-41d4-a716-446655440000";
    const repository: Repository = {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [{ id: packageId, name: "Lab bench", macros: [] }],
    };
    const settings: SettingsResponse = {
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
    const dependencies: SettingsPageDependencies = {
      updateSettings: vi.fn().mockResolvedValue({
        settings,
        restartRequired: false,
        reconnectRequired: false,
      }),
      changePassword: vi.fn().mockResolvedValue(undefined),
      restartDevice: vi.fn().mockResolvedValue({
        accepted: true,
        connectionWillClose: true,
        reprovisioningRequired: false,
      }),
      resetDeviceSettings: vi.fn().mockResolvedValue({
        accepted: true,
        connectionWillClose: true,
        reprovisioningRequired: false,
        repositoryBlobsPreserved: true,
      }),
      factoryResetDevice: vi.fn().mockResolvedValue({
        accepted: true,
        connectionWillClose: true,
        reprovisioningRequired: true,
        repositoryBlobsPreserved: false,
      }),
      signOut: vi.fn().mockResolvedValue(undefined),
      exportRepository: vi.fn().mockResolvedValue({
        bytes: new Uint8Array([1, 2, 3]),
        filename: "working-copy.emk-repository.json.gz",
        mimeType: "application/gzip",
      }),
      saveAsFile: vi.fn(),
    };
    const view = await render(
      <SettingsPage
        dependencies={dependencies}
        onDeviceActionStarted={vi.fn()}
        onOpenDiagnostics={vi.fn()}
        onSaveSnapshot={vi.fn().mockResolvedValue(undefined)}
        onSettingsChanged={vi.fn()}
        saveError={null}
        saving={false}
        settings={settings}
        store={createRepositoryWorkingCopyStore(repository)}
      />,
    );
    assertNoRenderedPropertyCollisions(view);
    await view.unmount();
  });

  test("PackageManagementPage", async () => {
    const packageAId = "550e8400-e29b-41d4-a716-446655440000";
    const packageBId = "550e8400-e29b-41d4-a716-446655440001";
    const repository: Repository = {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [
        { id: packageAId, name: "Package A", macros: [] },
        { id: packageBId, name: "Package B", macros: [] },
      ],
    };
    const view = await render(
      <PackageManagementPage
        dependencies={{
          persistSelectedPackageId: vi.fn().mockResolvedValue(null),
        }}
        onOpenMacros={vi.fn()}
        onSelectionChange={vi.fn()}
        onSelectionPersistenceFailure={vi.fn()}
        onSelectionPersistenceSuccess={vi.fn()}
        persistedPackageId={packageAId}
        selectedPackageId={packageAId}
        store={createRepositoryWorkingCopyStore(repository)}
      />,
    );
    assertNoRenderedPropertyCollisions(view);
    await view.unmount();
  });
});
