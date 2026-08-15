import { useState } from "react";
import { isSettingsUpdateRequest } from "../../../v2/apiContracts";
import {
  byteCountLabel,
  passphraseMaxBytes,
  passphraseMinBytes,
  ssidMaxBytes,
} from "./settingsFieldLimits";
import type { SettingsUpdateRequest } from "../../../v2/apiTypes";

interface StationFormProps {
  stationConfigured: boolean;
  stationSsid: string | null;
  busy: boolean;
  onSubmit: (request: SettingsUpdateRequest) => void;
  onRemove: () => void;
}

export function StationForm({
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
