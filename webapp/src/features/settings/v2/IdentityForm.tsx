import { useEffect, useState } from "react";
import { isSettingsUpdateRequest } from "../../../v2/apiContracts";
import { v2Limits } from "../../../v2/limits";
import { byteCountLabel, deviceNameMaxBytes } from "./settingsFieldLimits";
import type {
  SettingsResponse,
  SettingsUpdateRequest,
} from "../../../v2/apiTypes";

interface IdentityFormProps {
  settings: SettingsResponse;
  busy: boolean;
  onSubmit: (request: SettingsUpdateRequest) => void;
}

export function IdentityForm({
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
  }, [
    settings.deviceName,
    settings.requireSerialConfirmation,
    settings.sendMode,
    settings.snapshotRetentionTarget,
    settings.showMacroSourcePreviews,
  ]);

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
      className="grid gap-[0.85rem]"
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
