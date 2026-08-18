import { useState } from "react";
import { FieldHelp } from "../../../components/FieldHelp";
import { FormActions } from "../../../components/FormActions";
import { isSettingsUpdateRequest } from "../../../v2/apiContracts";
import {
  byteCountLabel,
  passphraseMaxBytes,
  passphraseMinBytes,
  ssidMaxBytes,
} from "./settingsFieldLimits";
import type { SettingsUpdateRequest } from "../../../v2/apiTypes";

interface AccessPointFormProps {
  apSsid: string;
  busy: boolean;
  onSubmit: (request: SettingsUpdateRequest) => void;
}

export function AccessPointForm({
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
      className="grid gap-[0.85rem]"
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
      <FieldHelp as="p">{byteCountLabel(ssid, ssidMaxBytes)}</FieldHelp>

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
      <FieldHelp as="p">
        {byteCountLabel(passphrase, passphraseMaxBytes)} (
        {String(passphraseMinBytes)}-{String(passphraseMaxBytes)} required). The
        current passphrase cannot be shown — enter a new one to change the
        network name, too.
      </FieldHelp>

      <FormActions>
        <button className="primary" disabled={busy || !valid} type="submit">
          {busy ? "Saving…" : "Update access point"}
        </button>
      </FormActions>
    </form>
  );
}
