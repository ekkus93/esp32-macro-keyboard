import { useEffect, useRef, useState } from "react";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import type { DeviceActionKind } from "./DeviceReconnectScreen";
import { UnsavedChangesPrompt } from "../../shell/v2/UnsavedChangesPrompt";
import { forceSessionEnded } from "../../../v2/apiClient";
import { isSettingsUpdateRequest } from "../../../v2/apiContracts";
import {
  factoryResetDevice as defaultFactoryResetDevice,
  resetDeviceSettings as defaultResetDeviceSettings,
  restartDevice as defaultRestartDevice,
  signOut as defaultSignOut,
} from "../../../v2/deviceActionsClient";
import { v2Limits } from "../../../v2/limits";
import { utf8ByteLength } from "../../../v2/repository";
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

// SPEC_V2 §13.4/§13.9 device-name examples are all well under this bound;
// `apiRequestGuards.ts`'s `isOptionalDeviceSettings`/`isSetupIdentity` are the
// actual enforced gate (1..32 UTF-8 bytes) — this constant is presentation
// only, for the live byte-count helper text.
const deviceNameMaxBytes = 32;
const ssidMaxBytes = 32;
const passphraseMinBytes = 8;
const passphraseMaxBytes = 63;

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

function saveBytesAsFile(
  bytes: Uint8Array,
  filename: string,
  mimeType: string,
): void {
  const blob = new Blob([bytes], { type: mimeType });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = filename;
  document.body.append(anchor);
  anchor.click();
  anchor.remove();
  URL.revokeObjectURL(url);
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

function byteCountLabel(value: string, max: number): string {
  return `${String(utf8ByteLength(value))}/${String(max)} bytes`;
}

interface IdentityFormProps {
  settings: SettingsResponse;
  busy: boolean;
  onSubmit: (request: SettingsUpdateRequest) => void;
}

function IdentityForm({
  settings,
  busy,
  onSubmit,
}: IdentityFormProps): React.JSX.Element {
  const [deviceName, setDeviceName] = useState(settings.deviceName);
  const [requireSerialConfirmation, setRequireSerialConfirmation] = useState(
    settings.requireSerialConfirmation,
  );
  const [sendMode, setSendMode] = useState(settings.sendMode);
  const [snapshotRetentionTarget, setSnapshotRetentionTarget] = useState(
    String(settings.snapshotRetentionTarget),
  );
  const [showMacroSourcePreviews, setShowMacroSourcePreviews] = useState(
    settings.showMacroSourcePreviews,
  );

  useEffect(() => {
    setDeviceName(settings.deviceName);
    setRequireSerialConfirmation(settings.requireSerialConfirmation);
    setSendMode(settings.sendMode);
    setSnapshotRetentionTarget(String(settings.snapshotRetentionTarget));
    setShowMacroSourcePreviews(settings.showMacroSourcePreviews);
  }, [settings]);

  const retentionTargetNumber = Number(snapshotRetentionTarget);
  const request: SettingsUpdateRequest = {
    deviceName,
    requireSerialConfirmation,
    sendMode,
    snapshotRetentionTarget: retentionTargetNumber,
    showMacroSourcePreviews,
  };
  const valid = isSettingsUpdateRequest(request);

  return (
    <form
      className="form-stack"
      onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        if (!valid) {
          return;
        }
        onSubmit(request);
      }}
    >
      <label htmlFor="settings-device-name">
        Device name
        <input
          disabled={busy}
          id="settings-device-name"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setDeviceName(event.currentTarget.value);
          }}
          value={deviceName}
        />
      </label>
      <p className="field-help">
        {byteCountLabel(deviceName, deviceNameMaxBytes)}
      </p>

      <label className="checkbox-row" htmlFor="settings-serial-confirmation">
        <input
          checked={requireSerialConfirmation}
          disabled={busy}
          id="settings-serial-confirmation"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setRequireSerialConfirmation(event.currentTarget.checked);
          }}
          type="checkbox"
        />
        Require physical confirmation before typing
      </label>

      <fieldset disabled={busy}>
        <legend>Sending behavior</legend>
        <label className="checkbox-row" htmlFor="settings-send-mode-quick">
          <input
            checked={sendMode === "quick"}
            id="settings-send-mode-quick"
            name="settings-send-mode"
            onChange={() => {
              setSendMode("quick");
            }}
            type="radio"
          />
          Quick Send
        </label>
        <label className="checkbox-row" htmlFor="settings-send-mode-preview">
          <input
            checked={sendMode === "preview"}
            id="settings-send-mode-preview"
            name="settings-send-mode"
            onChange={() => {
              setSendMode("preview");
            }}
            type="radio"
          />
          Always Preview
        </label>
      </fieldset>

      <label htmlFor="settings-retention-target">
        Advisory snapshot retention target
        <input
          disabled={busy}
          id="settings-retention-target"
          max={v2Limits.snapshotRetentionTargetMax}
          min={0}
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setSnapshotRetentionTarget(event.currentTarget.value);
          }}
          type="number"
          value={snapshotRetentionTarget}
        />
      </label>

      <label className="checkbox-row" htmlFor="settings-source-previews">
        <input
          checked={showMacroSourcePreviews}
          disabled={busy}
          id="settings-source-previews"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setShowMacroSourcePreviews(event.currentTarget.checked);
          }}
          type="checkbox"
        />
        Show macro source previews
      </label>
      <p className="field-help">
        Off by default — macro source can contain passwords or private commands.
      </p>

      <div className="form-actions">
        <button className="primary" disabled={busy || !valid} type="submit">
          {busy ? "Saving…" : "Save"}
        </button>
      </div>
    </form>
  );
}

interface AccessPointFormProps {
  apSsid: string;
  busy: boolean;
  onSubmit: (request: SettingsUpdateRequest) => void;
}

function AccessPointForm({
  apSsid,
  busy,
  onSubmit,
}: AccessPointFormProps): React.JSX.Element {
  const [ssid, setSsid] = useState(apSsid);
  const [passphrase, setPassphrase] = useState("");
  const request: SettingsUpdateRequest = { accessPoint: { ssid, passphrase } };
  const valid = isSettingsUpdateRequest(request);

  return (
    <form
      className="form-stack"
      onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        if (!valid) {
          return;
        }
        onSubmit(request);
        setPassphrase("");
      }}
    >
      <label htmlFor="settings-ap-ssid">
        Access-point network name (SSID)
        <input
          disabled={busy}
          id="settings-ap-ssid"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setSsid(event.currentTarget.value);
          }}
          value={ssid}
        />
      </label>
      <p className="field-help">{byteCountLabel(ssid, ssidMaxBytes)}</p>

      <label htmlFor="settings-ap-passphrase">
        New access-point passphrase
        <input
          autoComplete="new-password"
          disabled={busy}
          id="settings-ap-passphrase"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setPassphrase(event.currentTarget.value);
          }}
          type="password"
          value={passphrase}
        />
      </label>
      <p className="field-help">
        {byteCountLabel(passphrase, passphraseMaxBytes)} (
        {String(passphraseMinBytes)}-{String(passphraseMaxBytes)} required). The
        current passphrase cannot be shown — enter a new one to change the
        network name, too.
      </p>

      <div className="form-actions">
        <button className="primary" disabled={busy || !valid} type="submit">
          {busy ? "Saving…" : "Update access point"}
        </button>
      </div>
    </form>
  );
}

interface StationFormProps {
  stationConfigured: boolean;
  stationSsid: string | null;
  busy: boolean;
  onSubmit: (request: SettingsUpdateRequest) => void;
  onRemove: () => void;
}

function StationForm({
  stationConfigured,
  stationSsid,
  busy,
  onSubmit,
  onRemove,
}: StationFormProps): React.JSX.Element {
  const [editing, setEditing] = useState(false);
  const [ssid, setSsid] = useState("");
  const [passphrase, setPassphrase] = useState("");
  const request: SettingsUpdateRequest = { station: { ssid, passphrase } };
  const valid = isSettingsUpdateRequest(request);

  if (stationConfigured && !editing) {
    return (
      <div className="form-stack">
        <p>Connected network: {stationSsid}</p>
        <div className="form-actions">
          <button
            disabled={busy}
            onClick={() => {
              setEditing(true);
            }}
            type="button"
          >
            Change network
          </button>
          <button
            className="danger"
            disabled={busy}
            onClick={onRemove}
            type="button"
          >
            Remove station network
          </button>
        </div>
      </div>
    );
  }

  return (
    <form
      className="form-stack"
      onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        if (!valid) {
          return;
        }
        onSubmit(request);
        setPassphrase("");
        setEditing(false);
      }}
    >
      {!stationConfigured ? <p>No station network is configured.</p> : null}
      <label htmlFor="settings-station-ssid">
        Network name (SSID)
        <input
          disabled={busy}
          id="settings-station-ssid"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setSsid(event.currentTarget.value);
          }}
          value={ssid}
        />
      </label>
      <p className="field-help">{byteCountLabel(ssid, ssidMaxBytes)}</p>

      <label htmlFor="settings-station-passphrase">
        Passphrase
        <input
          autoComplete="new-password"
          disabled={busy}
          id="settings-station-passphrase"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setPassphrase(event.currentTarget.value);
          }}
          type="password"
          value={passphrase}
        />
      </label>
      <p className="field-help">
        {byteCountLabel(passphrase, passphraseMaxBytes)} (
        {String(passphraseMinBytes)}-{String(passphraseMaxBytes)} required).
      </p>

      <div className="form-actions">
        <button className="primary" disabled={busy || !valid} type="submit">
          {busy ? "Saving…" : "Connect"}
        </button>
        {stationConfigured ? (
          <button
            disabled={busy}
            onClick={() => {
              setEditing(false);
            }}
            type="button"
          >
            Cancel
          </button>
        ) : null}
      </div>
    </form>
  );
}

interface PasswordFormProps {
  busy: boolean;
  onSubmit: (currentPassword: string, newPassword: string) => void;
}

function PasswordForm({
  busy,
  onSubmit,
}: PasswordFormProps): React.JSX.Element {
  const [currentPassword, setCurrentPassword] = useState("");
  const [newPassword, setNewPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const newPasswordBytes = utf8ByteLength(newPassword);
  const newPasswordValid =
    newPasswordBytes >= v2Limits.adminPasswordMinBytes &&
    newPasswordBytes <= v2Limits.adminPasswordMaxBytes;
  const currentPasswordBytes = utf8ByteLength(currentPassword);
  const currentPasswordValid =
    currentPasswordBytes >= v2Limits.adminPasswordMinBytes &&
    currentPasswordBytes <= v2Limits.adminPasswordMaxBytes;
  const matches = newPassword === confirmPassword;
  const valid = newPasswordValid && currentPasswordValid && matches;

  return (
    <form
      className="form-stack"
      onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        if (!valid) {
          return;
        }
        onSubmit(currentPassword, newPassword);
        setCurrentPassword("");
        setNewPassword("");
        setConfirmPassword("");
      }}
    >
      <label htmlFor="settings-current-password">
        Current password
        <input
          autoComplete="current-password"
          disabled={busy}
          id="settings-current-password"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setCurrentPassword(event.currentTarget.value);
          }}
          type="password"
          value={currentPassword}
        />
      </label>
      <label htmlFor="settings-new-password">
        New password
        <input
          autoComplete="new-password"
          disabled={busy}
          id="settings-new-password"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setNewPassword(event.currentTarget.value);
          }}
          type="password"
          value={newPassword}
        />
      </label>
      <p className="field-help">
        {String(v2Limits.adminPasswordMinBytes)}-
        {String(v2Limits.adminPasswordMaxBytes)} UTF-8 bytes.
      </p>
      <label htmlFor="settings-confirm-password">
        Confirm new password
        <input
          autoComplete="new-password"
          disabled={busy}
          id="settings-confirm-password"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setConfirmPassword(event.currentTarget.value);
          }}
          type="password"
          value={confirmPassword}
        />
      </label>
      {!matches && confirmPassword.length > 0 ? (
        <p role="alert">New password and confirmation do not match.</p>
      ) : null}
      <div className="form-actions">
        <button className="primary" disabled={busy || !valid} type="submit">
          {busy ? "Changing…" : "Change password"}
        </button>
      </div>
      <p className="field-help">
        Changing the password signs out every session, including this one.
      </p>
    </form>
  );
}

interface ConfirmPhraseDialogProps {
  kind: "reset-settings" | "factory-reset";
  busy: boolean;
  onCancel: () => void;
  onConfirm: (adminPassword: string) => void;
}

function ConfirmPhraseDialog({
  kind,
  busy,
  onCancel,
  onConfirm,
}: ConfirmPhraseDialogProps): React.JSX.Element {
  const phrase = kind === "reset-settings" ? "RESET SETTINGS" : "FACTORY RESET";
  const [typed, setTyped] = useState("");
  const [adminPassword, setAdminPassword] = useState("");
  const passwordBytes = utf8ByteLength(adminPassword);
  const passwordValid =
    kind === "reset-settings" ||
    (passwordBytes >= v2Limits.adminPasswordMinBytes &&
      passwordBytes <= v2Limits.adminPasswordMaxBytes);
  const canConfirm = typed === phrase && passwordValid && !busy;

  return (
    <div className="dialog-backdrop" role="presentation">
      <div
        aria-labelledby="confirm-phrase-title"
        className="dialog-panel danger-zone"
        role="alertdialog"
      >
        <div className="dialog-heading">
          <h2 id="confirm-phrase-title">
            {kind === "reset-settings" ? "Reset settings" : "Factory reset"}
          </h2>
        </div>
        {kind === "reset-settings" ? (
          <p>
            This restores device name, serial-confirmation policy, sending
            behavior, retention target, and source-preview preference to their
            defaults, and removes the station network. The access-point network,
            administrator password, provisioning state, and every stored
            repository blob are preserved. All sessions are invalidated and the
            device restarts.
          </p>
        ) : (
          <p>
            This erases device configuration, the administrator password, all
            sessions, and <strong>every stored repository blob</strong>, then
            restarts unprovisioned. Firmware and web assets are preserved. This
            cannot be undone.
          </p>
        )}
        {kind === "factory-reset" ? (
          <label htmlFor="confirm-phrase-password">
            Administrator password
            <input
              autoComplete="current-password"
              disabled={busy}
              id="confirm-phrase-password"
              onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                setAdminPassword(event.currentTarget.value);
              }}
              type="password"
              value={adminPassword}
            />
          </label>
        ) : null}
        <label htmlFor="confirm-phrase-input">
          Type &quot;{phrase}&quot; to confirm
          <input
            disabled={busy}
            id="confirm-phrase-input"
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setTyped(event.currentTarget.value);
            }}
            value={typed}
          />
        </label>
        <div className="form-actions">
          <button disabled={busy} onClick={onCancel} type="button">
            Cancel
          </button>
          <button
            className="danger"
            disabled={!canConfirm}
            onClick={() => {
              onConfirm(adminPassword);
            }}
            type="button"
          >
            {busy
              ? "Working…"
              : kind === "reset-settings"
                ? "Confirm reset settings"
                : "Erase everything"}
          </button>
        </div>
      </div>
    </div>
  );
}

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

      <article className="card" aria-labelledby="settings-identity-title">
        <h3 id="settings-identity-title">Device</h3>
        <ErrorBanner message={identityError} />
        <IdentityForm
          busy={identityBusy}
          onSubmit={submitIdentity}
          settings={settings}
        />
      </article>

      <article className="card" aria-labelledby="settings-ap-title">
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
      </article>

      <article className="card" aria-labelledby="settings-station-title">
        <h3 id="settings-station-title">Station network (optional)</h3>
        <ErrorBanner message={stationError} />
        <StationForm
          busy={stationBusy}
          onRemove={removeStation}
          onSubmit={submitStation}
          stationConfigured={settings.stationConfigured}
          stationSsid={settings.stationSsid}
        />
      </article>

      <article className="card" aria-labelledby="settings-password-title">
        <h3 id="settings-password-title">Administrator password</h3>
        <ErrorBanner message={passwordError} />
        <PasswordForm
          busy={passwordBusy}
          onSubmit={(currentPassword, newPassword) => {
            void submitPassword(currentPassword, newPassword);
          }}
        />
      </article>

      <article className="card" aria-labelledby="settings-diagnostics-title">
        <h3 id="settings-diagnostics-title">Diagnostics</h3>
        <button onClick={onOpenDiagnostics} type="button">
          View diagnostics
        </button>
      </article>

      <article className="card" aria-labelledby="settings-session-title">
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
      </article>

      <article
        className="card danger-zone"
        aria-labelledby="settings-danger-title"
      >
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
      </article>

      {restartConfirming ? (
        <div className="dialog-backdrop" role="presentation">
          <div
            aria-labelledby="confirm-restart-title"
            className="dialog-panel"
            role="alertdialog"
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
