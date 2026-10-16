import { useEffect, useState } from "react";
import { CheckboxRow } from "../../../components/CheckboxRow";
import { FieldHelp } from "../../../components/FieldHelp";
import { StepIndicator } from "../../../components/StepIndicator";
import { FormActions } from "../../../components/FormActions";
import { StandaloneScreen } from "../../../components/StandaloneScreen";
import { V2ApiError, v2GetJson, v2PostJson } from "../../../v2/apiClient";
import { isSetupAccepted, isSetupStateResponse } from "../../../v2/apiGuards";
import { isSetupRequest, setupFieldGuards } from "../../../v2/apiRequestGuards";
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
 * The input steps, grouped by the decision being made (UI_UX_SPEC_V2 §3.1
 * steps 1-5). Review is the fourth stop of the same progress indicator. Each
 * step holds at most two fields plus one option so the step's fields and its
 * Continue button fit a 320x568 screen without scrolling
 * (UI_UX_SPEC_MATERIAL_3_EXPRESSIVE §4.2).
 */
const STEP_LABELS = ["Device", "Wi-Fi", "Access", "Review"] as const;

type SetupField =
  | "setupCode"
  | "deviceName"
  | "apSsid"
  | "apPassphrase"
  | "adminPassword";

/** Which fields belong to which input step. */
const STEP_FIELDS: readonly (readonly SetupField[])[] = [
  ["setupCode", "deviceName"],
  ["apSsid", "apPassphrase"],
  ["adminPassword"],
];

/**
 * V2-080 — First-run setup screens (UI_UX_SPEC_V2 §3.1), presented as a
 * short stepper (UXR-030): device, Wi-Fi, access, then review.
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

  const [step, setStep] = useState(0);
  const [touched, setTouched] = useState<ReadonlySet<SetupField>>(new Set());

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

  const fieldValid = (field: SetupField): boolean =>
    setupFieldGuards[field](requestBody[field]);
  const stepValid = (index: number): boolean =>
    (STEP_FIELDS[index] ?? []).every(fieldValid);
  /** A field is flagged only after the user has left it once. */
  const fieldFlagged = (field: SetupField): boolean =>
    touched.has(field) && !fieldValid(field);
  const touch = (field: SetupField): void => {
    setTouched((current) => new Set(current).add(field));
  };

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
          className="primary"
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
          Reconnect to <strong>{apSsid}</strong> with the passphrase you just
          set, then sign in.
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
        <StepIndicator current={3} labels={STEP_LABELS} />
        <h1 className="mt-4">Review setup</h1>
        <dl className="mb-4">
          <dt>Device name</dt>
          <dd>{deviceName}</dd>
          <dt>Wi-Fi network</dt>
          <dd>{apSsid}</dd>
          <dt>Device button required before typing</dt>
          <dd>{requireSerialConfirmation ? "Yes" : "No"}</dd>
        </dl>
        <p>
          Applying restarts the device&apos;s Wi-Fi. Your passphrase, password
          and setup code are not shown again.
        </p>
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
            Back
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

  const lastInputStep = STEP_FIELDS.length - 1;
  const advance = (): void => {
    if (!stepValid(step)) {
      for (const field of STEP_FIELDS[step] ?? []) {
        touch(field);
      }
      return;
    }
    if (step === lastInputStep) {
      setPhase("review");
      return;
    }
    setStep(step + 1);
  };

  return (
    <StandaloneScreen>
      <StepIndicator current={step} labels={STEP_LABELS} />
      <h1 className="mt-4">
        {step === 0
          ? "Set up your keyboard"
          : step === 1
            ? "Create its Wi-Fi network"
            : "Protect access"}
      </h1>
      <form
        className="grid gap-4"
        key={step}
        onSubmit={(event: React.FormEvent<HTMLFormElement>) => {
          event.preventDefault();
          advance();
        }}
      >
        {step === 0 ? (
          <>
            <div>
              <label htmlFor="setup-code">Setup code</label>
              <input
                aria-invalid={fieldFlagged("setupCode")}
                autoComplete="one-time-code"
                autoFocus
                id="setup-code"
                inputMode="numeric"
                onBlur={() => {
                  touch("setupCode");
                }}
                onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                  setSetupCode(event.target.value);
                }}
                required
                value={setupCode}
              />
              <FieldHelp exceeded={fieldFlagged("setupCode")}>
                8 digits, shown in the device serial console.
              </FieldHelp>
            </div>
            <div>
              <label htmlFor="device-name">Device name</label>
              <input
                aria-invalid={fieldFlagged("deviceName")}
                id="device-name"
                onBlur={() => {
                  touch("deviceName");
                }}
                onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                  setDeviceName(event.target.value);
                }}
                required
                value={deviceName}
              />
              <FieldHelp exceeded={fieldFlagged("deviceName")}>
                1 to 32 bytes.
              </FieldHelp>
            </div>
          </>
        ) : null}
        {step === 1 ? (
          <>
            <div>
              <label htmlFor="ap-ssid">Wi-Fi network name</label>
              <input
                aria-invalid={fieldFlagged("apSsid")}
                autoFocus
                id="ap-ssid"
                onBlur={() => {
                  touch("apSsid");
                }}
                onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                  setApSsid(event.target.value);
                }}
                required
                value={apSsid}
              />
              <FieldHelp exceeded={fieldFlagged("apSsid")}>
                1 to 32 bytes.
              </FieldHelp>
            </div>
            <div>
              <label htmlFor="ap-passphrase">Wi-Fi passphrase</label>
              <input
                aria-invalid={fieldFlagged("apPassphrase")}
                autoComplete="new-password"
                id="ap-passphrase"
                onBlur={() => {
                  touch("apPassphrase");
                }}
                onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                  setApPassphrase(event.target.value);
                }}
                required
                type="password"
                value={apPassphrase}
              />
              <FieldHelp exceeded={fieldFlagged("apPassphrase")}>
                8 to 63 bytes.
              </FieldHelp>
            </div>
          </>
        ) : null}
        {step === 2 ? (
          <>
            <div>
              <label htmlFor="admin-password">Administrator password</label>
              <input
                aria-invalid={fieldFlagged("adminPassword")}
                autoComplete="new-password"
                autoFocus
                id="admin-password"
                onBlur={() => {
                  touch("adminPassword");
                }}
                onChange={(event: React.ChangeEvent<HTMLInputElement>) => {
                  setAdminPassword(event.target.value);
                }}
                required
                type="password"
                value={adminPassword}
              />
              <FieldHelp exceeded={fieldFlagged("adminPassword")}>
                {v2Limits.adminPasswordMinBytes} to{" "}
                {v2Limits.adminPasswordMaxBytes} bytes.
              </FieldHelp>
            </div>
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
          </>
        ) : null}
        <FormActions>
          {step > 0 ? (
            <button
              onClick={() => {
                setStep(step - 1);
              }}
              type="button"
            >
              Back
            </button>
          ) : null}
          <button className="primary" disabled={!stepValid(step)} type="submit">
            {step === lastInputStep ? "Review setup" : "Continue"}
          </button>
        </FormActions>
      </form>
    </StandaloneScreen>
  );
}
