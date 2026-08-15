import { useState } from "react";
import { v2Limits } from "../../../v2/limits";
import { utf8ByteLength } from "../../../v2/repository";

interface PasswordFormProps {
  busy: boolean;
  onSubmit: (currentPassword: string, newPassword: string) => void;
}

export function PasswordForm({
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
