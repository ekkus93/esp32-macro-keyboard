import { useEffect, useState } from "react";
import { FieldHelp } from "../../../components/FieldHelp";
import { FormActions } from "../../../components/FormActions";
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
  const [snapshotRetentionTarget, setSnapshotRetentionTarget] = useState(
    String(settings.snapshotRetentionTarget),
  );

  useEffect(() => {
    setDeviceName(settings.deviceName);
    setSnapshotRetentionTarget(String(settings.snapshotRetentionTarget));
  }, [settings.deviceName, settings.snapshotRetentionTarget]);

  const retentionTargetNumber = Number(snapshotRetentionTarget);
  const request: SettingsUpdateRequest = {
    deviceName,
    snapshotRetentionTarget: retentionTargetNumber,
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
      <FieldHelp as="p">
        {byteCountLabel(deviceName, deviceNameMaxBytes)}
      </FieldHelp>

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

      <FormActions>
        <button className="primary" disabled={busy || !valid} type="submit">
          {busy ? "Saving…" : "Save"}
        </button>
      </FormActions>
    </form>
  );
}
