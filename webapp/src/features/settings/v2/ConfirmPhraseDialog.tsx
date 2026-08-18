import { useRef, useState } from "react";
import { Dialog } from "../../../components/Dialog";
import { FormActions } from "../../../components/FormActions";
import { v2Limits } from "../../../v2/limits";
import { utf8ByteLength } from "../../../v2/repository";
import { useFocusTrap } from "../../shell/v2/useFocusTrap";

interface ConfirmPhraseDialogProps {
  kind: "reset-settings" | "factory-reset";
  busy: boolean;
  onCancel: () => void;
  onConfirm: (adminPassword: string) => void;
}

export function ConfirmPhraseDialog({
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
  const containerRef = useRef<HTMLDivElement>(null);
  useFocusTrap({ active: true, containerRef, onClose: onCancel });

  return (
    <Dialog
      aria-labelledby="confirm-phrase-title"
      containerRef={containerRef}
      heading={
        <h2 id="confirm-phrase-title">
          {kind === "reset-settings" ? "Reset settings" : "Factory reset"}
        </h2>
      }
      tone="danger"
    >
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
      <FormActions>
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
      </FormActions>
    </Dialog>
  );
}
