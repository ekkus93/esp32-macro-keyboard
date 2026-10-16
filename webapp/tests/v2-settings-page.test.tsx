import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { subscribeUnauthorized, V2ApiError } from "../src/v2/apiClient";
import { SettingsPage } from "../src/features/settings/v2/SettingsPage";
import type { SettingsPageDependencies } from "../src/features/settings/v2/SettingsPage";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { RepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { SettingsResponse } from "../src/v2/apiTypes";
import type { SettingsGroup, SettingsSection } from "../src/v2/routingV2";
import {
  buttonContaining,
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setInputValue,
} from "./render";

const packageId = "550e8400-e29b-41d4-a716-446655440000";

function repository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [{ id: packageId, name: "Lab bench", macros: [] }],
  };
}

function settings(overrides: Partial<SettingsResponse> = {}): SettingsResponse {
  return {
    deviceName: "Desk Macro Keyboard",
    requireSerialConfirmation: false,
    snapshotRetentionTarget: 5,
    lastSelectedPackageId: null,
    apSsid: "MacroKeyboard",
    stationConfigured: false,
    stationSsid: null,
    ...overrides,
  };
}

function makeDependencies(
  overrides: Partial<SettingsPageDependencies> = {},
): SettingsPageDependencies {
  return {
    updateSettings: vi.fn().mockResolvedValue({
      settings: settings(),
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
    ...overrides,
  };
}

interface Overrides {
  store?: RepositoryWorkingCopyStore;
  settings?: SettingsResponse;
  deps?: Partial<SettingsPageDependencies>;
  saving?: boolean;
  saveError?: string | null;
  /** The Settings group to show; the list of groups when omitted. */
  group?: SettingsGroup | null;
  section?: SettingsSection | null;
}

async function renderPage(overrides: Overrides = {}) {
  const store =
    overrides.store ?? createRepositoryWorkingCopyStore(repository());
  const deps = makeDependencies(overrides.deps);
  const onSettingsChanged = vi.fn();
  const onSaveSnapshot = vi.fn().mockResolvedValue(undefined);
  const onOpenDiagnostics = vi.fn();
  const onDeviceActionStarted = vi.fn();
  const onOpenGroup = vi.fn();
  const onOpenSection = vi.fn();

  const view = await render(
    <SettingsPage
      dependencies={deps}
      group={overrides.group ?? null}
      section={overrides.section ?? null}
      onOpenGroup={onOpenGroup}
      onOpenSection={onOpenSection}
      onDeviceActionStarted={onDeviceActionStarted}
      onOpenDiagnostics={onOpenDiagnostics}
      onSaveSnapshot={onSaveSnapshot}
      onSettingsChanged={onSettingsChanged}
      saveError={overrides.saveError ?? null}
      saving={overrides.saving ?? false}
      settings={overrides.settings ?? settings()}
      store={store}
    />,
  );
  await flushReact();
  return {
    ...view,
    store,
    deps,
    onSettingsChanged,
    onSaveSnapshot,
    onOpenDiagnostics,
    onDeviceActionStarted,
    onOpenGroup,
    onOpenSection,
  };
}

describe("SettingsPage (TODO_V2 V2-120)", () => {
  test("pre-fills device identity fields from settings and never renders lastSelectedPackageId as editable text", async () => {
    const { container, unmount } = await renderPage({
      group: "device",
      settings: settings({ lastSelectedPackageId: packageId }),
    });
    expect(
      requiredElement("#settings-device-name", HTMLInputElement).value,
    ).toBe("Desk Macro Keyboard");
    expect(document.querySelector("#settings-serial-confirmation")).toBeNull();
    expect(
      requiredElement("#settings-retention-target", HTMLInputElement).value,
    ).toBe("5");
    // The opaque device UUID must never appear as literal text anywhere on
    // the page, and there is no input whose value is that UUID.
    expect(container.textContent).not.toContain(packageId);
    const allInputs = Array.from(container.querySelectorAll("input"));
    expect(allInputs.every((input) => input.value !== packageId)).toBe(true);
    await unmount();
  });

  test("submitting the identity form calls updateSettings with the edited fields and applies the sanitized result", async () => {
    const updateSettings = vi.fn().mockResolvedValue({
      settings: settings({ deviceName: "New name" }),
      restartRequired: false,
      reconnectRequired: false,
    });
    const { onSettingsChanged, store } = await renderPage({
      group: "device",
      deps: { updateSettings },
    });
    await setInputValue(
      requiredElement("#settings-device-name", HTMLInputElement),
      "New name",
    );
    await click(buttonWithText("Save"));
    await flushReact();
    expect(updateSettings).toHaveBeenCalledWith({
      deviceName: "New name",
      snapshotRetentionTarget: 5,
    });
    expect(onSettingsChanged).toHaveBeenCalledWith(
      settings({ deviceName: "New name" }),
    );
    // A device-settings write must never touch the repository working copy.
    expect(store.getIsDirty()).toBe(false);
  });

  test("the USB page shows the fixed descriptor and saves physical-confirmation policy separately", async () => {
    const updateSettings = vi.fn().mockResolvedValue({
      settings: settings({ requireSerialConfirmation: true }),
      restartRequired: false,
      reconnectRequired: false,
    });
    const { container, onSettingsChanged, store } = await renderPage({
      group: "usb",
      deps: { updateSettings },
    });
    expect(container.textContent).toContain("ESP32 Macro Keyboard Project");
    expect(container.textContent).toContain("ESP32 Macro Keyboard");
    expect(container.textContent).toContain(
      "USB identity is fixed by the device firmware.",
    );
    const confirmation = requiredElement(
      "#settings-serial-confirmation",
      HTMLInputElement,
    );
    expect(confirmation.checked).toBe(false);
    await click(confirmation);
    await click(buttonWithText("Save"));
    await flushReact();
    expect(updateSettings).toHaveBeenCalledWith({
      requireSerialConfirmation: true,
    });
    expect(onSettingsChanged).toHaveBeenCalledWith(
      settings({ requireSerialConfirmation: true }),
    );
    expect(store.getIsDirty()).toBe(false);
  });

  test("network settings open focused access-point and station screens", async () => {
    const { container, onOpenSection, unmount } = await renderPage({
      group: "network",
    });
    expect(container.querySelector("input")).toBeNull();
    await click(buttonContaining("Access point"));
    expect(onOpenSection).toHaveBeenCalledWith("access-point");
    await unmount();

    const { container: accessPoint, unmount: unmountAccessPoint } =
      await renderPage({ group: "network", section: "access-point" });
    expect(accessPoint.querySelector("#settings-ap-ssid")).not.toBeNull();
    expect(accessPoint.querySelector("#settings-station-ssid")).toBeNull();
    await unmountAccessPoint();

    const { container: station, unmount: unmountStation } = await renderPage({
      group: "network",
      section: "station",
    });
    expect(station.querySelector("#settings-station-ssid")).not.toBeNull();
    expect(station.querySelector("#settings-ap-ssid")).toBeNull();
    await unmountStation();
  });

  test("access-point update requires both fields and shows a restart notice when the server reports one is required", async () => {
    const updateSettings = vi.fn().mockResolvedValue({
      settings: settings({ apSsid: "NewNetwork" }),
      restartRequired: true,
      reconnectRequired: true,
    });
    await renderPage({
      group: "network",
      section: "access-point",
      deps: { updateSettings },
    });
    const updateButton = buttonWithText("Update access point");
    expect(updateButton.disabled).toBe(true);

    await setInputValue(
      requiredElement("#settings-ap-ssid", HTMLInputElement),
      "NewNetwork",
    );
    expect(updateButton.disabled).toBe(true);
    await setInputValue(
      requiredElement("#settings-ap-passphrase", HTMLInputElement),
      "new-passphrase",
    );
    expect(updateButton.disabled).toBe(false);

    await click(updateButton);
    await flushReact();
    expect(updateSettings).toHaveBeenCalledWith({
      accessPoint: { ssid: "NewNetwork", passphrase: "new-passphrase" },
    });
    expect(document.body.textContent).toContain("A restart is required");
    expect(document.body.textContent).toContain("NewNetwork");
  });

  test("station form offers Connect when unconfigured and Remove/Change when configured", async () => {
    const { container: unconfigured, unmount } = await renderPage({
      group: "network",
      section: "station",
      settings: settings({ stationConfigured: false, stationSsid: null }),
    });
    expect(unconfigured.textContent).toContain(
      "No station network is configured.",
    );
    expect(() => buttonWithText("Connect")).not.toThrow();
    await unmount();

    const removeStation = vi.fn().mockResolvedValue({
      settings: settings({ stationConfigured: false, stationSsid: null }),
      restartRequired: false,
      reconnectRequired: false,
    });
    const { container: configured } = await renderPage({
      group: "network",
      section: "station",
      settings: settings({
        stationConfigured: true,
        stationSsid: "OfficeWiFi",
      }),
      deps: { updateSettings: removeStation },
    });
    expect(configured.textContent).toContain("OfficeWiFi");
    await click(buttonWithText("Remove station network"));
    await flushReact();
    expect(removeStation).toHaveBeenCalledWith({ station: null });
  });

  test("password form requires matching confirmation and ends the session on success without discarding it", async () => {
    const changePassword = vi.fn().mockResolvedValue(undefined);
    await renderPage({
      group: "security",
      section: "password",
      deps: { changePassword },
    });

    await setInputValue(
      requiredElement("#settings-current-password", HTMLInputElement),
      "old-example-password",
    );
    await setInputValue(
      requiredElement("#settings-new-password", HTMLInputElement),
      "new-example-password",
    );
    await setInputValue(
      requiredElement("#settings-confirm-password", HTMLInputElement),
      "does-not-match",
    );
    expect(buttonWithText("Change password").disabled).toBe(true);
    expect(document.body.textContent).toContain("do not match");

    await setInputValue(
      requiredElement("#settings-confirm-password", HTMLInputElement),
      "new-example-password",
    );
    expect(buttonWithText("Change password").disabled).toBe(false);

    let notified = 0;
    const unsubscribe = subscribeUnauthorized(() => {
      notified += 1;
    });
    await click(buttonWithText("Change password"));
    await flushReact();
    unsubscribe();

    expect(changePassword).toHaveBeenCalledWith({
      currentPassword: "old-example-password",
      newPassword: "new-example-password",
    });
    expect(notified).toBe(1);
  });

  test("password partial commit tells the owner the new password is authoritative without pretending full success", async () => {
    const changePassword = vi.fn().mockRejectedValue(
      new V2ApiError(409, {
        code: "auth_state_incomplete",
        message:
          "password changed; session invalidation incomplete; sign in with the new password",
      }),
    );
    await renderPage({
      group: "security",
      section: "password",
      deps: { changePassword },
    });

    await setInputValue(
      requiredElement("#settings-current-password", HTMLInputElement),
      "old-example-password",
    );
    await setInputValue(
      requiredElement("#settings-new-password", HTMLInputElement),
      "new-example-password",
    );
    await setInputValue(
      requiredElement("#settings-confirm-password", HTMLInputElement),
      "new-example-password",
    );

    let notified = 0;
    const unsubscribe = subscribeUnauthorized(() => {
      notified += 1;
    });
    await click(buttonWithText("Change password"));
    await flushReact();
    unsubscribe();

    expect(document.body.textContent).toContain(
      "Your password changed, but not all active sessions could be cleared. Sign in with the new password.",
    );
    // The error remains visible long enough for the owner to know which
    // credential is authoritative. The server already cleared the cookie; a
    // subsequent request will drive the normal unauthorized transition.
    expect(notified).toBe(0);
  });

  test("Diagnostics is listed with the groups and opens its own screen", async () => {
    const { onOpenDiagnostics } = await renderPage();
    await click(buttonContaining("Diagnostics"));
    expect(onOpenDiagnostics).toHaveBeenCalledOnce();
  });

  test("the compact list shows each group and opens it on tap", async () => {
    const { container, onOpenGroup, unmount } = await renderPage({
      settings: settings({
        stationConfigured: true,
        stationSsid: "OfficeWiFi",
      }),
    });
    const headlines = Array.from(
      container.querySelectorAll("li button span.text-title-sm"),
    ).map((node) => node.textContent);
    expect(headlines).toEqual([
      "Device identity",
      "Network",
      "Security",
      "USB identity and confirmation",
      "Restart and reset",
      "Diagnostics",
    ]);
    expect(container.textContent).not.toContain("OfficeWiFi");
    // No form is on the list screen.
    expect(container.querySelector("input")).toBeNull();
    await click(buttonContaining("Network"));
    expect(onOpenGroup).toHaveBeenCalledWith("network");
    await unmount();
  });

  test("each group shows only its own controls", async () => {
    const groups: [SettingsGroup, SettingsSection | null, string, string[]][] =
      [
        [
          "device",
          null,
          "#settings-device-name",
          ["#settings-ap-ssid", "#settings-new-password"],
        ],
        [
          "network",
          "access-point",
          "#settings-ap-ssid",
          ["#settings-device-name", "#settings-new-password"],
        ],
        [
          "security",
          "password",
          "#settings-new-password",
          ["#settings-device-name", "#settings-ap-ssid"],
        ],
        [
          "usb",
          null,
          "#settings-serial-confirmation",
          [
            "#settings-device-name",
            "#settings-ap-ssid",
            "#settings-new-password",
          ],
        ],
      ];
    for (const [group, section, present, absent] of groups) {
      const { unmount } = await renderPage({ group, section });
      expect(
        document.querySelector(present),
        `${group}: ${present}`,
      ).not.toBeNull();
      for (const selector of absent) {
        expect(
          document.querySelector(selector),
          `${group}: ${selector}`,
        ).toBeNull();
      }
      await unmount();
    }
    const { container, unmount } = await renderPage({ group: "reset" });
    expect(container.textContent).toContain("Restart");
    expect(container.textContent).toContain("Reset settings");
    expect(container.textContent).toContain("Factory reset");
    expect(container.querySelector("input")).toBeNull();
    await unmount();
  });

  test("Sign out with a clean working copy calls signOut immediately and ends the session", async () => {
    const signOut = vi.fn().mockResolvedValue(undefined);
    await renderPage({
      group: "security",
      section: "session",
      deps: { signOut },
    });
    let notified = 0;
    const unsubscribe = subscribeUnauthorized(() => {
      notified += 1;
    });
    await click(buttonWithText("Sign out"));
    await flushReact();
    unsubscribe();
    expect(signOut).toHaveBeenCalledOnce();
    expect(notified).toBe(1);
  });

  test("Sign out while dirty warns first (V2-103) and only signs out after Discard changes", async () => {
    const signOut = vi.fn().mockResolvedValue(undefined);
    const store = createRepositoryWorkingCopyStore(repository());
    store.applyContentChange({
      ...repository(),
      packages: [{ id: packageId, name: "Renamed", macros: [] }],
    });
    expect(store.getIsDirty()).toBe(true);
    await renderPage({
      group: "security",
      section: "session",
      store,
      deps: { signOut },
    });

    await click(buttonWithText("Sign out"));
    await flushReact();
    expect(document.body.textContent).toContain("Unsaved changes");
    expect(signOut).not.toHaveBeenCalled();

    await click(buttonWithText("Discard changes"));
    await flushReact();
    expect(store.getIsDirty()).toBe(false);
    expect(signOut).toHaveBeenCalledOnce();
  });

  test("Restart shows a confirmation, then calls restartDevice and onDeviceActionStarted", async () => {
    const restartDevice = vi.fn().mockResolvedValue({
      accepted: true,
      connectionWillClose: true,
      reprovisioningRequired: false,
    });
    const { onDeviceActionStarted } = await renderPage({
      group: "reset",
      section: "restart",
      deps: { restartDevice },
    });
    await click(buttonWithText("Restart"));
    await flushReact();
    expect(document.body.textContent).toContain("Restart the device?");
    await click(buttonWithText("Restart now"));
    await flushReact();
    expect(restartDevice).toHaveBeenCalledOnce();
    expect(onDeviceActionStarted).toHaveBeenCalledWith("restart");
  });

  // TODO_V2 V2-133/UI_UX_SPEC_V2 §14 "Dialogs trap focus and restore it to
  // their invoking control". Both the Restart and the reset-settings/
  // factory-reset dialogs keep their trigger button mounted underneath
  // them (a sibling conditional block, not a ternary that swaps the
  // trigger away), so the trap's automatic capture is exactly right.
  test("the Restart confirmation traps focus and Escape cancels, restoring focus to Restart", async () => {
    const { unmount } = await renderPage({
      group: "reset",
      section: "restart",
    });
    const restartButton = buttonWithText("Restart");
    restartButton.focus();
    await click(restartButton);
    await flushReact();

    const dialog = requiredElement('[role="alertdialog"]', HTMLDivElement);
    expect(dialog.contains(document.activeElement)).toBe(true);

    await act(async () => {
      document.dispatchEvent(
        new KeyboardEvent("keydown", {
          key: "Escape",
          bubbles: true,
          cancelable: true,
        }),
      );
      await Promise.resolve();
    });
    expect(document.querySelector('[role="alertdialog"]')).toBeNull();
    expect(document.activeElement).toBe(restartButton);
    await unmount();
  });

  test("the reset-settings phrase dialog traps focus and Escape cancels, restoring focus to Reset settings", async () => {
    const { unmount } = await renderPage({
      group: "reset",
      section: "reset-settings",
    });
    const resetButton = buttonWithText("Reset settings");
    resetButton.focus();
    await click(resetButton);
    await flushReact();

    const dialog = requiredElement('[role="alertdialog"]', HTMLDivElement);
    expect(dialog.contains(document.activeElement)).toBe(true);

    await act(async () => {
      document.dispatchEvent(
        new KeyboardEvent("keydown", {
          key: "Escape",
          bubbles: true,
          cancelable: true,
        }),
      );
      await Promise.resolve();
    });
    expect(document.querySelector('[role="alertdialog"]')).toBeNull();
    expect(document.activeElement).toBe(resetButton);
    await unmount();
  });

  test("Reset settings while dirty warns first, then requires the exact typed phrase", async () => {
    const resetDeviceSettings = vi.fn().mockResolvedValue({
      accepted: true,
      connectionWillClose: true,
      reprovisioningRequired: false,
      repositoryBlobsPreserved: true,
    });
    const store = createRepositoryWorkingCopyStore(repository());
    store.applyContentChange({
      ...repository(),
      packages: [{ id: packageId, name: "Renamed", macros: [] }],
    });
    const { onDeviceActionStarted } = await renderPage({
      group: "reset",
      section: "reset-settings",
      store,
      deps: { resetDeviceSettings },
    });

    await click(buttonWithText("Reset settings"));
    await flushReact();
    expect(document.body.textContent).toContain("Unsaved changes");
    await click(buttonWithText("Discard changes"));
    await flushReact();

    expect(document.body.textContent).toContain(
      'Type "RESET SETTINGS" to confirm',
    );
    const confirmButton = buttonWithText("Confirm reset settings");
    expect(confirmButton.disabled).toBe(true);
    await setInputValue(
      requiredElement("#confirm-phrase-input", HTMLInputElement),
      "wrong phrase",
    );
    expect(confirmButton.disabled).toBe(true);
    await setInputValue(
      requiredElement("#confirm-phrase-input", HTMLInputElement),
      "RESET SETTINGS",
    );
    expect(confirmButton.disabled).toBe(false);
    await click(confirmButton);
    await flushReact();
    expect(resetDeviceSettings).toHaveBeenCalledOnce();
    expect(onDeviceActionStarted).toHaveBeenCalledWith("reset-settings");
  });

  test("Factory reset requires the admin password and the exact typed phrase, and is not dirty-blocked when clean", async () => {
    const factoryResetDevice = vi.fn().mockResolvedValue({
      accepted: true,
      connectionWillClose: true,
      reprovisioningRequired: true,
      repositoryBlobsPreserved: false,
    });
    const { onDeviceActionStarted } = await renderPage({
      group: "reset",
      section: "factory-reset",
      deps: { factoryResetDevice },
    });

    await click(buttonWithText("Factory reset"));
    await flushReact();
    // Not dirty: straight to the typed-confirmation dialog, no warning.
    expect(document.body.textContent).not.toContain("Unsaved changes");
    expect(document.body.textContent).toContain(
      'Type "FACTORY RESET" to confirm',
    );

    const confirmButton = buttonWithText("Erase everything");
    expect(confirmButton.disabled).toBe(true);
    await setInputValue(
      requiredElement("#confirm-phrase-input", HTMLInputElement),
      "FACTORY RESET",
    );
    expect(confirmButton.disabled).toBe(true); // password still empty
    await setInputValue(
      requiredElement("#confirm-phrase-password", HTMLInputElement),
      "correct horse battery staple",
    );
    expect(confirmButton.disabled).toBe(false);
    await click(confirmButton);
    await flushReact();
    expect(factoryResetDevice).toHaveBeenCalledWith(
      "correct horse battery staple",
    );
    expect(onDeviceActionStarted).toHaveBeenCalledWith("factory-reset");
  });

  test("Factory reset while dirty warns first, and only reaches the typed-confirmation dialog after Export working copy", async () => {
    const exportRepository = vi.fn().mockResolvedValue({
      bytes: new Uint8Array([1, 2, 3]),
      filename: "working-copy.emk-repository.json.gz",
      mimeType: "application/gzip",
    });
    const saveAsFile = vi.fn();
    const store = createRepositoryWorkingCopyStore(repository());
    store.applyContentChange({
      ...repository(),
      packages: [{ id: packageId, name: "Renamed", macros: [] }],
    });
    await renderPage({
      group: "reset",
      section: "factory-reset",
      store,
      deps: { exportRepository, saveAsFile },
    });

    await click(buttonWithText("Factory reset"));
    await flushReact();
    expect(document.body.textContent).toContain("Unsaved changes");
    expect(document.body.textContent).toContain("factory reset the device");
    expect(document.body.textContent).not.toContain(
      'Type "FACTORY RESET" to confirm',
    );

    await click(buttonWithText("Export working copy"));
    await flushReact();
    expect(exportRepository).toHaveBeenCalledWith(store.getRepository());
    expect(saveAsFile).toHaveBeenCalledOnce();
    // Exporting does not itself resolve the warning — the working copy is
    // still dirty and the prompt stays open until Save or Discard.
    expect(document.body.textContent).toContain("Unsaved changes");

    await click(buttonWithText("Discard changes"));
    await flushReact();
    expect(store.getIsDirty()).toBe(false);
    expect(document.body.textContent).toContain(
      'Type "FACTORY RESET" to confirm',
    );
  });

  test("a device-action failure surfaces an error and does not call onDeviceActionStarted", async () => {
    const restartDevice = vi.fn().mockRejectedValue(new Error("boom"));
    const { onDeviceActionStarted } = await renderPage({
      group: "reset",
      section: "restart",
      deps: { restartDevice },
    });
    await click(buttonWithText("Restart"));
    await flushReact();
    await click(buttonWithText("Restart now"));
    await flushReact();
    expect(document.body.textContent).toContain("boom");
    expect(onDeviceActionStarted).not.toHaveBeenCalled();
  });
});
