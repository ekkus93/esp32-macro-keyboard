import { useState } from "react";
import { errorText } from "../../api/errors";
import { restartAfterSetup, submitSetup } from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { SetupState } from "../../types/models";

interface SetupPageProps {
  initialState: SetupState;
}

export function SetupPage({ initialState }: SetupPageProps): React.JSX.Element {
  const [state, setState] = useState(initialState);
  const [setupCode, setSetupCode] = useState("");
  const [apSsid, setApSsid] = useState(initialState.apSsid);
  const [apPassphrase, setApPassphrase] = useState("");
  const [administratorPassword, setAdministratorPassword] = useState("");
  const [requirePhysicalConfirmation, setRequirePhysicalConfirmation] =
    useState(initialState.physicalConfirmationRequired);
  const [alwaysSelectPackage, setAlwaysSelectPackage] = useState(true);
  const [submitting, setSubmitting] = useState(false);
  const [restarting, setRestarting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const submit = async (
    event: React.FormEvent<HTMLFormElement>,
  ): Promise<void> => {
    event.preventDefault();
    setSubmitting(true);
    setError(null);
    try {
      const committed = await submitSetup({
        setupCode,
        apSsid,
        apPassphrase,
        administratorPassword,
        requirePhysicalConfirmation,
        alwaysSelectPackage,
      });
      setState(committed);
      setSetupCode("");
      setApPassphrase("");
      setAdministratorPassword("");
    } catch (submissionError: unknown) {
      setError(errorText(submissionError));
    } finally {
      setSubmitting(false);
    }
  };

  const restart = async (): Promise<void> => {
    setRestarting(true);
    setError(null);
    try {
      await restartAfterSetup();
    } catch (restartError: unknown) {
      setError(errorText(restartError));
      setRestarting(false);
    }
  };

  if (state.completed) {
    return (
      <section>
        <h1>Setup complete</h1>
        <p>
          Credentials were committed. Restart the device to enter normal
          operating mode.
        </p>
        <ErrorBanner message={error} />
        <button
          className="primary"
          disabled={restarting}
          onClick={() => {
            void restart();
          }}
          type="button"
        >
          {restarting ? "Restart scheduled…" : "Restart device"}
        </button>
      </section>
    );
  }

  return (
    <section>
      <h1>First-run setup</h1>
      <p>
        Device <strong>{state.deviceId}</strong> is in isolated setup mode.
      </p>
      {state.physicalConfirmationRequired ? (
        <p role="status">
          Send the confirm command on the device serial console when the
          firmware requests it.
        </p>
      ) : null}
      <form
        className="form-stack"
        onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
          void submit(event);
        }}
      >
        <label htmlFor="setup-code">Setup code</label>
        <input
          autoComplete="one-time-code"
          id="setup-code"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setSetupCode(event.target.value);
          }}
          required
          value={setupCode}
        />

        <label htmlFor="ap-ssid">Wi-Fi network name</label>
        <input
          id="ap-ssid"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setApSsid(event.target.value);
          }}
          required
          value={apSsid}
        />

        <label htmlFor="ap-passphrase">Wi-Fi passphrase</label>
        <input
          autoComplete="new-password"
          id="ap-passphrase"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setApPassphrase(event.target.value);
          }}
          required
          type="password"
          value={apPassphrase}
        />

        <label htmlFor="administrator-password">Administrator password</label>
        <input
          autoComplete="new-password"
          id="administrator-password"
          onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
            setAdministratorPassword(event.target.value);
          }}
          required
          type="password"
          value={administratorPassword}
        />

        <label className="checkbox-row">
          <input
            checked={requirePhysicalConfirmation}
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setRequirePhysicalConfirmation(event.target.checked);
            }}
            type="checkbox"
          />
          Require the device button before typing macros
        </label>

        <label className="checkbox-row">
          <input
            checked={alwaysSelectPackage}
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setAlwaysSelectPackage(event.target.checked);
            }}
            type="checkbox"
          />
          Always ask which macro package to use
        </label>

        <ErrorBanner message={error} />
        <button
          className="primary"
          disabled={
            submitting ||
            setupCode.length === 0 ||
            apSsid.length === 0 ||
            apPassphrase.length === 0 ||
            administratorPassword.length === 0
          }
          type="submit"
        >
          {submitting ? "Saving securely…" : "Complete setup"}
        </button>
      </form>
    </section>
  );
}
