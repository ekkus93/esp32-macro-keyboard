import { useEffect, useState } from "react";
import { CheckboxRow } from "../../../components/CheckboxRow";
import { FieldHelp } from "../../../components/FieldHelp";
import { FormActions } from "../../../components/FormActions";
import { StandaloneScreen } from "../../../components/StandaloneScreen";
import { V2ApiError, v2GetJson, v2PostJson } from "../../../v2/apiClient";
import { isSetupAccepted, isSetupStateResponse } from "../../../v2/apiGuards";
import { isSetupRequest } from "../../../v2/apiRequestGuards";
import { v2Limits } from "../../../v2/limits";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { v2ErrorText } from "./v2ErrorText";

export interface FirstRunSetupPageProps {
  /**
   * Called when the user dismisses the post-setup reconnect guidance and
   * chooses to proceed. UI_UX_SPEC_V2 §3.1 step 8 is "proceed to Sign In";
   * this component does not render Sign In itself or navigate — the caller
   * owns composing the next screen.
   */
  onSetupComplete: () => void;
}

type Phase = "loading" | "unavailable" | "form" | "review" | "complete";

/**
 * V2-080 — First-run setup screens (UI_UX_SPEC_V2 §3.1).
 *
 * Loads unauthenticated `GET /api/v1/setup` state, walks the user through
 * device identification/setup code, device name, AP credentials,
 * administrator password, and the optional physical-confirmation
 * preference, shows a review step that never displays the passphrase or
 * administrator password back to the user, submits `POST /api/v1/setup`,
 * and then shows restart/reconnect guidance before handing off to Sign In.
 * Repository creation is intentionally out of scope here (SPEC_V2 §12.3).
 */
export function FirstRunSetupPage({
  onSetupComplete,
}: FirstRunSetupPageProps): React.JSX.Element {
  const [phase, setPhase] = useState<Phase>("loading");
  const [loadError, setLoadError] = useState<V2ApiError | null>(null);
  const [currentDeviceName, setCurrentDeviceName] = useState("");

  const [setupCode, setSetupCode] = useState("");
  const [deviceName, setDeviceName] = useState("");
  const [apSsid, setApSsid] = useState("");
  const [apPassphrase, setApPassphrase] = useState("");
  const [adminPassword, setAdminPassword] = useState("");
  const [requireSerialConfirmation, setRequireSerialConfirmation] =
    useState(false);

  const [submitting, setSubmitting] = useState(false);
  const [submitError, setSubmitError] = useState<string | null>(null);

  useEffect(() => {
    let active = true;
    const loadSetupState = async (): Promise<void> => {
      try {
        const state = await v2GetJson("/api/v1/setup", isSetupStateResponse, {
          notifyOnUnauthorized: false,
        });
        if (!active) {
          return;
        }
        setCurrentDeviceName(state.deviceName);
        setDeviceName(state.deviceName);
        setPhase("form");
      } catch (error: unknown) {
        if (!active) {
          return;
        }
        setLoadError(error instanceof V2ApiError ? error : null);
        setPhase("unavailable");
      }
    };
    void loadSetupState();
    return () => {
      active = false;
    };
  }, []);

  const requestBody = {
    setupCode,
    deviceName,
    apSsid,
    apPassphrase,
    adminPassword,
    requireSerialConfirmation,
  };
  const requestValid = isSetupRequest(requestBody);

  const submit = async (): Promise<void> => {
    setSubmitting(true);
    setSubmitError(null);
    try {
      await v2PostJson("/api/v1/setup", requestBody, isSetupAccepted, {
        notifyOnUnauthorized: false,
      });
      setSetupCode("");
      setApPassphrase("");
      setAdminPassword("");
      setPhase("complete");
    } catch (error: unknown) {
      setSubmitError(v2ErrorText(error));
    } finally {
      setSubmitting(false);
    }
  };

  if (phase === "loading") {
    return (
      <StandaloneScreen aria-busy="true">
        <p role="status">Loading setup information…</p>
      </StandaloneScreen>
    );
  }

  if (phase === "unavailable") {
    const alreadyProvisioned = loadError?.status === 404;
    return (
      <StandaloneScreen>
        <h1>Setup unavailable</h1>
        <p>
          {alreadyProvisioned
            ? "This device has already completed setup."
            : "The device did not return valid setup information."}
        </p>
        <ErrorBanner
          message={loadError === null ? null : v2ErrorText(loadError)}
        />
        <button
          onClick={() => {
            window.location.reload();
          }}
          type="button"
        >
          {alreadyProvisioned ? "Continue" : "Reload"}
        </button>
      </StandaloneScreen>
    );
  }

  if (phase === "complete") {
    return (
      <StandaloneScreen>
        <h1>Setup complete</h1>
        <p>
          The device applied your configuration and is restarting its Wi-Fi
          network.
        </p>
        <p>
          Reconnect to <strong>{apSsid}</strong> using the passphrase you just
          set, then continue.
        </p>
        <button className="primary" onClick={onSetupComplete} type="button">
          Continue to Sign In
        </button>
      </StandaloneScreen>
    );
  }

  if (phase === "review") {
    return (
      <StandaloneScreen>
        <h1>Review setup</h1>
        <p>
          Your Wi-Fi passphrase, administrator password, and setup code are
          never shown again after you enter them.
        </p>
        <dl className="mt-3 text-[0.85rem]">
          <dt>Device name</dt>
          <dd>{deviceName}</dd>
          <dt>Wi-Fi network name</dt>
          <dd>{apSsid}</dd>
          <dt>Require the device button before typing macros</dt>
          <dd>{requireSerialConfirmation ? "Yes" : "No"}</dd>
        </dl>
        <ErrorBanner message={submitError} />
        <FormActions>
          <button
            disabled={submitting}
            onClick={() => {
              setSubmitError(null);
              setPhase("form");
            }}
            type="button"
          >
            Edit
          </button>
          <button
            className="primary"
            disabled={submitting || !requestValid}
            onClick={() => {
              void submit();
            }}
            type="button"
          >
            {submitting ? "Applying setup…" : "Apply setup"}
          </button>
        </FormActions>
      </StandaloneScreen>
    );
  }

  return (
    <StandaloneScreen>
      <section>
        <h1>First-run setup</h1>
        <p>
          Device <strong>{currentDeviceName}</strong> is in isolated setup mode.
        </p>
        <form
          className="grid gap-[0.85rem]"
          onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
            event.preventDefault();
            if (requestValid) {
              setPhase("review");
            }
          }}
        >
          <label htmlFor="setup-code">
            Setup code (from the device serial console)
          </label>
          <input
            autoComplete="one-time-code"
            id="setup-code"
            inputMode="numeric"
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setSetupCode(event.target.value);
            }}
            required
            value={setupCode}
          />
          <FieldHelp>Exactly 8 digits.</FieldHelp>

          <label htmlFor="device-name">Device name</label>
          <input
            id="device-name"
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setDeviceName(event.target.value);
            }}
            required
            value={deviceName}
          />
          <FieldHelp>1 through 32 UTF-8 bytes.</FieldHelp>

          <label htmlFor="ap-ssid">Wi-Fi network name (access point)</label>
          <input
            id="ap-ssid"
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setApSsid(event.target.value);
            }}
            required
            value={apSsid}
          />
          <FieldHelp>1 through 32 UTF-8 bytes.</FieldHelp>

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
          <FieldHelp>8 through 63 UTF-8 bytes.</FieldHelp>

          <label htmlFor="admin-password">Administrator password</label>
          <input
            autoComplete="new-password"
            id="admin-password"
            onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
              setAdminPassword(event.target.value);
            }}
            required
            type="password"
            value={adminPassword}
          />
          <FieldHelp>
            {v2Limits.adminPasswordMinBytes} through{" "}
            {v2Limits.adminPasswordMaxBytes} UTF-8 bytes.
          </FieldHelp>

          <CheckboxRow>
            <input
              checked={requireSerialConfirmation}
              onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                setRequireSerialConfirmation(event.target.checked);
              }}
              type="checkbox"
            />
            Require the device button before typing macros
          </CheckboxRow>

          <ErrorBanner message={submitError} />
          <button className="primary" disabled={!requestValid} type="submit">
            Review setup
          </button>
        </form>
      </section>
    </StandaloneScreen>
  );
}
