import { useEffect, useState } from "react";
import { CheckboxRow } from "../../../components/CheckboxRow";
import { FormActions } from "../../../components/FormActions";
import { isSettingsUpdateRequest } from "../../../v2/apiContracts";
import type {
  SettingsResponse,
  SettingsUpdateRequest,
} from "../../../v2/apiTypes";

interface UsbConfirmationFormProps {
  settings: SettingsResponse;
  busy: boolean;
  onSubmit: (request: SettingsUpdateRequest) => void;
}

export function UsbConfirmationForm({
  settings,
  busy,
  onSubmit,
}: UsbConfirmationFormProps): React.JSX.Element {
  const [requireSerialConfirmation, setRequireSerialConfirmation] = useState(
    settings.requireSerialConfirmation,
  );

  useEffect(() => {
    setRequireSerialConfirmation(settings.requireSerialConfirmation);
  }, [settings.requireSerialConfirmation]);

  const request: SettingsUpdateRequest = { requireSerialConfirmation };
  const valid = isSettingsUpdateRequest(request);

  return (
    <form
      className="grid gap-[0.85rem]"
      onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        if (valid) {
          onSubmit(request);
        }
      }}
    >
      <CheckboxRow htmlFor="settings-serial-confirmation">
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
      </CheckboxRow>
      <FormActions>
        <button className="primary" disabled={busy || !valid} type="submit">
          {busy ? "Saving…" : "Save"}
        </button>
      </FormActions>
    </form>
  );
}
