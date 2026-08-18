import { vi } from "vitest";

import { DiagnosticsPage } from "../src/features/settings/v2/DiagnosticsPage";
import type { DiagnosticsPageDependencies } from "../src/features/settings/v2/DiagnosticsPage";
import { SettingsPage } from "../src/features/settings/v2/SettingsPage";
import type { SettingsPageDependencies } from "../src/features/settings/v2/SettingsPage";
import { PackageManagementPage } from "../src/features/macros/v2/PackageManagementPage";
import type { DiagnosticsResponse, SettingsResponse } from "../src/v2/apiTypes";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import { renderMacrosPage } from "./macrosPageHarness";
import { render } from "./render";
import type { RenderResult } from "./render";

/**
 * Four structurally distinct, fully populated pages, rendered through their
 * own existing dependency-injection patterns (no fetch mocking, no `dist/`
 * dependency -- React renders straight from source in jsdom). Extracted from
 * `no-rendered-property-collisions.test.tsx` (T2-2) so
 * `no-descendant-variant-nesting.test.tsx` (T3-2) can drive the same four
 * renders without duplicating this setup.
 */

export async function renderMacros(): Promise<RenderResult> {
  return renderMacrosPage();
}

export async function renderDiagnostics(): Promise<RenderResult> {
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
  return view;
}

export async function renderSettings(): Promise<RenderResult> {
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
  return render(
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
}

export async function renderPackageManagement(): Promise<RenderResult> {
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
  return render(
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
}

export const REPRESENTATIVE_PAGES = [
  { name: "MacrosPage", render: renderMacros },
  { name: "DiagnosticsPage", render: renderDiagnostics },
  { name: "SettingsPage", render: renderSettings },
  { name: "PackageManagementPage", render: renderPackageManagement },
] as const;
