import { useRef, useState } from "react";
import { Card } from "../../../components/Card";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import type { DeviceActionKind } from "./DeviceReconnectScreen";
import { UnsavedChangesPrompt } from "../../shell/v2/UnsavedChangesPrompt";
import { forceSessionEnded } from "../../../v2/apiClient";
import { AccessPointForm } from "./AccessPointForm";
import { ConfirmPhraseDialog } from "./ConfirmPhraseDialog";
import { IdentityForm } from "./IdentityForm";
import { PasswordForm } from "./PasswordForm";
import { StationForm } from "./StationForm";
import {
  factoryResetDevice as defaultFactoryResetDevice,
  resetDeviceSettings as defaultResetDeviceSettings,
  restartDevice as defaultRestartDevice,
  signOut as defaultSignOut,
} from "../../../v2/deviceActionsClient";
import { saveBytesAsFile } from "../../../v2/saveFile";
import { useFocusTrap } from "../../shell/v2/useFocusTrap";
import {
  changePassword as defaultChangePassword,
  updateSettings as defaultUpdateSettings,
} from "../../../v2/settingsClient";
import { exportRepository as defaultExportRepository } from "../../../v2/snapshotClient";
import type {
  SettingsResponse,
  SettingsUpdateRequest,
} from "../../../v2/apiTypes";
import type { RepositoryWorkingCopyStore } from "../../../v2/repositoryWorkingCopy";

/**
 * Settings, per SPEC_V2 §11/§13.9 and UI_UX_SPEC_V2 §11 (TODO_V2 V2-120):
 * device name, serial-confirmation policy, access-point and optional
 * station configuration, administrator password change, Quick Send/Always
 * Preview, advisory retention target, source-preview preference, Sign Out,
 * and the restart/reset-settings/factory-reset destructive actions
 * (TODO_V2 V2-121). `lastSelectedPackageId` is deliberately not one of the
 * editable fields below — SPEC_V2 §11 calls it "not normally shown as an
 * editable text setting" and the UI manages it itself as an opaque
 * selection, not user-typed text. Every submit here is a device-settings
 * `PUT`/`POST`, never a `RepositoryWorkingCopyStore` mutation, so none of
 * these forms dirty the repository (SPEC_V2 §11.1/UI_UX_SPEC_V2 §7.2).
 */

export interface SettingsPageDependencies {
  updateSettings: typeof defaultUpdateSettings;
  changePassword: typeof defaultChangePassword;
  restartDevice: typeof defaultRestartDevice;
  resetDeviceSettings: typeof defaultResetDeviceSettings;
  factoryResetDevice: typeof defaultFactoryResetDevice;
  signOut: typeof defaultSignOut;
  exportRepository: typeof defaultExportRepository;
  saveAsFile: (bytes: Uint8Array, filename: string, mimeType: string) => void;
}

function defaultDependencies(): SettingsPageDependencies {
  return {
    updateSettings: defaultUpdateSettings,
    changePassword: defaultChangePassword,
    restartDevice: defaultRestartDevice,
    resetDeviceSettings: defaultResetDeviceSettings,
    factoryResetDevice: defaultFactoryResetDevice,
    signOut: defaultSignOut,
    exportRepository: defaultExportRepository,
    saveAsFile: saveBytesAsFile,
  };
}

export interface SettingsPageProps {
  store: RepositoryWorkingCopyStore;
  settings: SettingsResponse;
  onSettingsChanged: (settings: SettingsResponse) => void;
  /** The single shared Save snapshot handler (same one the header button uses). */
  onSaveSnapshot: () => Promise<void>;
  saving: boolean;
  saveError: string | null;
  onOpenDiagnostics: () => void;
  /**
   * Restart/reset-settings/factory-reset all report `connectionWillClose:
   * true` (SPEC_V2 §13.12). This hands control of the "the device is
   * rebooting, reconnect" experience to the caller (`AppV2.tsx`'s
   * authenticated shell), which is what replaces the whole screen — a
   * device reboot is disruptive to the entire application, not one page
   * (TODO_V2 V2-121).
   */
  onDeviceActionStarted: (kind: DeviceActionKind) => void;
  dependencies?: SettingsPageDependencies;
}

type DirtyGuardedAction = "sign-out" | "reset-settings" | "factory-reset";

export function SettingsPage({
  store,
  settings,
  onSettingsChanged,
  onSaveSnapshot,
  saving,
  saveError,
  onOpenDiagnostics,
  onDeviceActionStarted,
  dependencies,
}: SettingsPageProps): React.JSX.Element {
  const deps = dependencies ?? defaultDependencies();
  const depsRef = useRef(deps);
  depsRef.current = deps;

  const [identityBusy, setIdentityBusy] = useState(false);
  const [identityError, setIdentityError] = useState<string | null>(null);
  const [apBusy, setApBusy] = useState(false);
  const [apError, setApError] = useState<string | null>(null);
  const [apNotice, setApNotice] = useState<string | null>(null);
  const [stationBusy, setStationBusy] = useState(false);
  const [stationError, setStationError] = useState<string | null>(null);
  const [passwordBusy, setPasswordBusy] = useState(false);
  const [passwordError, setPasswordError] = useState<string | null>(null);

  const [pendingDirty, setPendingDirty] = useState<DirtyGuardedAction | null>(
    null,
  );
  const [exportingWorkingCopy, setExportingWorkingCopy] = useState(false);
  const [confirmPhrase, setConfirmPhrase] = useState<
    "reset-settings" | "factory-reset" | null
  >(null);
  const [restartConfirming, setRestartConfirming] = useState(false);
  const [actionBusy, setActionBusy] = useState<
    "sign-out" | "restart" | "reset-settings" | "factory-reset" | null
  >(null);
  const [actionError, setActionError] = useState<string | null>(null);
  const restartConfirmRef = useRef<HTMLDivElement>(null);
  useFocusTrap({
    active: restartConfirming,
    containerRef: restartConfirmRef,
    onClose: () => {
      setRestartConfirming(false);
    },
  });

  const applyUpdate = async (
    request: SettingsUpdateRequest,
    onBusy: (busy: boolean) => void,
    onError: (message: string | null) => void,
  ): Promise<void> => {
    onBusy(true);
    onError(null);
    try {
      const updated = await depsRef.current.updateSettings(request);
      onSettingsChanged(updated.settings);
      if (
        request.accessPoint !== undefined &&
        (updated.restartRequired || updated.reconnectRequired)
      ) {
        setApNotice(updated.settings.apSsid);
      }
    } catch (error: unknown) {
      onError(v2ErrorText(error));
    } finally {
      onBusy(false);
    }
  };

  const submitIdentity = (request: SettingsUpdateRequest): void => {
    void applyUpdate(request, setIdentityBusy, setIdentityError);
  };

  const submitAccessPoint = (request: SettingsUpdateRequest): void => {
    void applyUpdate(request, setApBusy, setApError);
  };

  const submitStation = (request: SettingsUpdateRequest): void => {
    void applyUpdate(request, setStationBusy, setStationError);
  };

  const removeStation = (): void => {
    void applyUpdate({ station: null }, setStationBusy, setStationError);
  };

  const submitPassword = async (
    currentPassword: string,
    newPassword: string,
  ): Promise<void> => {
    setPasswordBusy(true);
    setPasswordError(null);
    try {
      await depsRef.current.changePassword({ currentPassword, newPassword });
      forceSessionEnded();
    } catch (error: unknown) {
      setPasswordError(v2ErrorText(error));
    } finally {
      setPasswordBusy(false);
    }
  };

  const exportWorkingCopy = async (): Promise<void> => {
    setExportingWorkingCopy(true);
    try {
      const exported = await depsRef.current.exportRepository(
        store.getRepository(),
      );
      depsRef.current.saveAsFile(
        exported.bytes,
        exported.filename,
        exported.mimeType,
      );
    } finally {
      setExportingWorkingCopy(false);
    }
  };

  const doSignOut = async (): Promise<void> => {
    setActionBusy("sign-out");
    setActionError(null);
    try {
      await depsRef.current.signOut();
      forceSessionEnded();
    } catch (error: unknown) {
      setActionError(v2ErrorText(error));
    } finally {
      setActionBusy(null);
    }
  };

  const proceedAfterDirtyResolved = (kind: DirtyGuardedAction): void => {
    if (kind === "sign-out") {
      void doSignOut();
      return;
    }
    setConfirmPhrase(kind);
  };

  const beginDestructive = (kind: DirtyGuardedAction): void => {
    setActionError(null);
    if (store.getIsDirty()) {
      setPendingDirty(kind);
      return;
    }
    proceedAfterDirtyResolved(kind);
  };

  const saveThenProceed = async (): Promise<void> => {
    const kind = pendingDirty;
    if (kind === null) {
      return;
    }
    await onSaveSnapshot();
    if (!store.getIsDirty()) {
      setPendingDirty(null);
      proceedAfterDirtyResolved(kind);
    }
  };

  const doReset = async (): Promise<void> => {
    setActionBusy("reset-settings");
    setActionError(null);
    try {
      await depsRef.current.resetDeviceSettings();
      setConfirmPhrase(null);
      onDeviceActionStarted("reset-settings");
    } catch (error: unknown) {
      setActionError(v2ErrorText(error));
    } finally {
      setActionBusy(null);
    }
  };

  const doFactoryReset = async (adminPassword: string): Promise<void> => {
    setActionBusy("factory-reset");
    setActionError(null);
    try {
      await depsRef.current.factoryResetDevice(adminPassword);
      setConfirmPhrase(null);
      onDeviceActionStarted("factory-reset");
    } catch (error: unknown) {
      setActionError(v2ErrorText(error));
    } finally {
      setActionBusy(null);
    }
  };

  const doRestart = async (): Promise<void> => {
    setActionBusy("restart");
    setActionError(null);
    try {
      await depsRef.current.restartDevice();
      setRestartConfirming(false);
      onDeviceActionStarted("restart");
    } catch (error: unknown) {
      setActionError(v2ErrorText(error));
    } finally {
      setActionBusy(null);
    }
  };

  return (
    <section aria-labelledby="settings-title">
      <div className="page-heading">
        <h2 id="settings-title">Settings</h2>
      </div>

      <Card aria-labelledby="settings-identity-title">
        <h3 id="settings-identity-title">Device</h3>
        <ErrorBanner message={identityError} />
        <IdentityForm
          busy={identityBusy}
          onSubmit={submitIdentity}
          settings={settings}
        />
      </Card>

      <Card aria-labelledby="settings-ap-title">
        <h3 id="settings-ap-title">Access point network</h3>
        <ErrorBanner message={apError} />
        <AccessPointForm
          apSsid={settings.apSsid}
          busy={apBusy}
          onSubmit={submitAccessPoint}
        />
        {apNotice !== null ? (
          <p role="status">
            Saved. A restart is required to apply the new access point network
            &quot;{apNotice}&quot;. Use Restart below, then reconnect this
            device&apos;s Wi-Fi to the new network and sign in again.
          </p>
        ) : null}
      </Card>

      <Card aria-labelledby="settings-station-title">
        <h3 id="settings-station-title">Station network (optional)</h3>
        <ErrorBanner message={stationError} />
        <StationForm
          busy={stationBusy}
          onRemove={removeStation}
          onSubmit={submitStation}
          stationConfigured={settings.stationConfigured}
          stationSsid={settings.stationSsid}
        />
      </Card>

      <Card aria-labelledby="settings-password-title">
        <h3 id="settings-password-title">Administrator password</h3>
        <ErrorBanner message={passwordError} />
        <PasswordForm
          busy={passwordBusy}
          onSubmit={(currentPassword, newPassword) => {
            void submitPassword(currentPassword, newPassword);
          }}
        />
      </Card>

      <Card aria-labelledby="settings-diagnostics-title">
        <h3 id="settings-diagnostics-title">Diagnostics</h3>
        <button onClick={onOpenDiagnostics} type="button">
          View diagnostics
        </button>
      </Card>

      <Card aria-labelledby="settings-session-title">
        <h3 id="settings-session-title">Session</h3>
        <ErrorBanner message={actionError} />
        <button
          disabled={actionBusy !== null}
          onClick={() => {
            beginDestructive("sign-out");
          }}
          type="button"
        >
          {actionBusy === "sign-out" ? "Signing out…" : "Sign out"}
        </button>
      </Card>

      <Card aria-labelledby="settings-danger-title" variant="danger">
        <h3 id="settings-danger-title">Device actions</h3>
        <div className="form-actions">
          <button
            disabled={actionBusy !== null}
            onClick={() => {
              setRestartConfirming(true);
            }}
            type="button"
          >
            Restart
          </button>
          <button
            className="danger"
            disabled={actionBusy !== null}
            onClick={() => {
              beginDestructive("reset-settings");
            }}
            type="button"
          >
            Reset settings
          </button>
          <button
            className="danger"
            disabled={actionBusy !== null}
            onClick={() => {
              beginDestructive("factory-reset");
            }}
            type="button"
          >
            Factory reset
          </button>
        </div>
      </Card>

      {restartConfirming ? (
        <div className="dialog-backdrop" role="presentation">
          <div
            aria-labelledby="confirm-restart-title"
            className="dialog-panel"
            ref={restartConfirmRef}
            role="alertdialog"
            tabIndex={-1}
          >
            <div className="dialog-heading">
              <h2 id="confirm-restart-title">Restart the device?</h2>
            </div>
            <p>
              The device restarts and its Wi-Fi access point briefly drops. This
              tab reconnects automatically and, once the device is back, asks
              you to sign in again — your unsaved work, if any, is preserved.
            </p>
            <div className="form-actions">
              <button
                disabled={actionBusy !== null}
                onClick={() => {
                  setRestartConfirming(false);
                }}
                type="button"
              >
                Cancel
              </button>
              <button
                className="danger"
                disabled={actionBusy !== null}
                onClick={() => {
                  void doRestart();
                }}
                type="button"
              >
                {actionBusy === "restart" ? "Restarting…" : "Restart now"}
              </button>
            </div>
          </div>
        </div>
      ) : null}

      {confirmPhrase !== null ? (
        <ConfirmPhraseDialog
          busy={actionBusy === confirmPhrase}
          kind={confirmPhrase}
          onCancel={() => {
            setConfirmPhrase(null);
          }}
          onConfirm={(adminPassword) => {
            if (confirmPhrase === "reset-settings") {
              void doReset();
            } else {
              void doFactoryReset(adminPassword);
            }
          }}
        />
      ) : null}

      <ErrorBanner message={saveError} />

      {pendingDirty !== null ? (
        <UnsavedChangesPrompt
          actionLabel={
            pendingDirty === "sign-out"
              ? "sign out"
              : pendingDirty === "reset-settings"
                ? "reset settings"
                : "factory reset the device"
          }
          exporting={exportingWorkingCopy}
          onCancel={() => {
            setPendingDirty(null);
          }}
          onDiscard={() => {
            const kind = pendingDirty;
            store.discardChanges();
            setPendingDirty(null);
            proceedAfterDirtyResolved(kind);
          }}
          onExport={() => {
            void exportWorkingCopy();
          }}
          onSaveSnapshot={() => {
            void saveThenProceed();
          }}
          saving={saving}
        />
      ) : null}
    </section>
  );
}
