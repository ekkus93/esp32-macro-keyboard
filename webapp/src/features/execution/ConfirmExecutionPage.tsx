import { useEffect, useMemo, useState } from "react";
import { ApiError } from "../../api/client";
import { errorText } from "../../api/errors";
import {
  getDeviceStatus,
  getSetMacro,
  getSettings,
  submitExecution,
  validateSetMacro,
} from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { ExecutionConfirmationTarget } from "../../routing";
import { navigate } from "../../routing";
import type {
  DeviceStatus,
  ExecutionAccepted,
  ExecutionSubmitRequest,
  Macro,
  MacroSet,
  MacroValidation,
  Settings,
} from "../../types/models";
import "./ConfirmExecutionPage.css";

type SubmissionStage =
  | "idle"
  | "preflight"
  | "physical-confirmation"
  | "submitting";

interface LoadedConfirmation {
  macro: Macro;
  validation: MacroValidation;
}

interface ConfirmExecutionPageProps {
  activeSet: MacroSet | null;
  onAccepted: (accepted: ExecutionAccepted, returnHash: string) => void;
  settings: Settings;
  status: DeviceStatus;
  target: ExecutionConfirmationTarget;
}

/* Every macro belongs to exactly one set (SPEC 7.2), so there is no
   second place to look when the set does not have it. */
function loadPersistedMacro(setId: string, macroId: string): Promise<Macro> {
  return getSetMacro(setId, macroId);
}

function validatePersistedMacro(
  activeSetId: string,
  macro: Macro,
): Promise<MacroValidation> {
  return validateSetMacro(activeSetId, macro);
}

function macroIdentityIssue(
  activeSetId: string,
  requestedMacroId: string,
  macro: Macro,
): string | null {
  if (macro.id !== requestedMacroId) {
    return "The device returned a different macro than the requested macro.";
  }
  if (macro.set_id !== activeSetId) {
    return "The requested macro does not belong to the active macro set.";
  }
  return null;
}

async function loadConfirmation(
  activeSet: MacroSet,
  target: Extract<ExecutionConfirmationTarget, { kind: "valid" }>,
): Promise<LoadedConfirmation> {
  const macro = await loadPersistedMacro(activeSet.id, target.macroId);
  const identityIssue = macroIdentityIssue(activeSet.id, target.macroId, macro);
  if (identityIssue !== null) {
    throw new Error(identityIssue);
  }

  const validation = await validatePersistedMacro(activeSet.id, macro);
  return { macro, validation };
}

function sameMacroSnapshot(left: Macro, right: Macro): boolean {
  return (
    left.id === right.id &&
    left.revision === right.revision &&
    left.set_id === right.set_id &&
    left.source === right.source &&
    left.key_press_ms === right.key_press_ms &&
    left.inter_key_ms === right.inter_key_ms
  );
}

function sameValidationSnapshot(
  left: MacroValidation,
  right: MacroValidation,
): boolean {
  return (
    left.actionCount === right.actionCount &&
    left.estimatedDurationMs === right.estimatedDurationMs
  );
}

function returnHash(): string {
  return "/macros";
}

function returnToSource(): void {
  navigate("macros");
}

function submissionError(error: unknown): string {
  if (error instanceof DOMException && error.name === "AbortError") {
    return "The device did not complete the confirmation request before the 25-second timeout.";
  }
  return errorText(error);
}

function statusLabel(value: string): string {
  return value.replaceAll("_", " ");
}

export function ConfirmExecutionPage({
  activeSet,
  onAccepted,
  settings,
  status,
  target,
}: ConfirmExecutionPageProps): React.JSX.Element {
  const [loaded, setLoaded] = useState<LoadedConfirmation | null>(null);
  const [currentSettings, setCurrentSettings] = useState(settings);
  const [currentStatus, setCurrentStatus] = useState(status);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [submitError, setSubmitError] = useState<string | null>(null);
  const [loadVersion, setLoadVersion] = useState(0);
  const [stale, setStale] = useState(false);
  const [stage, setStage] = useState<SubmissionStage>("idle");

  useEffect(() => {
    setCurrentSettings(settings);
  }, [settings]);

  useEffect(() => {
    setCurrentStatus(status);
  }, [status]);

  useEffect(() => {
    if (activeSet === null || target.kind !== "valid") {
      setLoaded(null);
      setLoadError(null);
      setSubmitError(null);
      setStale(false);
      setStage("idle");
      return;
    }

    let active = true;
    setLoaded(null);
    setLoadError(null);
    setSubmitError(null);
    setStale(false);
    setStage("idle");
    void loadConfirmation(activeSet, target)
      .then((confirmation) => {
        if (active) {
          setLoaded(confirmation);
        }
      })
      .catch((error: unknown) => {
        if (active) {
          setLoadError(errorText(error));
        }
      });
    return () => {
      active = false;
    };
  }, [activeSet, loadVersion, target]);

  const blockers = useMemo(() => {
    const reasons: string[] = [];
    if (loaded === null) {
      reasons.push("The persisted macro and validation result are not loaded.");
    }
    if (currentSettings.activeSetId !== activeSet?.id) {
      reasons.push("The active macro set changed after this preview opened.");
    }
    if (stale) {
      reasons.push("The macro changed after this preview loaded.");
    }
    if (currentStatus.usbState !== "ready") {
      reasons.push(
        `USB is ${statusLabel(currentStatus.usbState)} instead of ready.`,
      );
    }
    if (currentStatus.executionState === "running") {
      reasons.push("Another macro execution is already running.");
    }
    return reasons;
  }, [
    activeSet?.id,
    currentSettings.activeSetId,
    currentStatus,
    loaded,
    stale,
  ]);

  const send = async (): Promise<void> => {
    if (
      activeSet === null ||
      target.kind !== "valid" ||
      loaded === null ||
      blockers.length !== 0 ||
      stage !== "idle"
    ) {
      return;
    }

    setStage("preflight");
    setSubmitError(null);
    try {
      const [latestStatus, latestSettings, latest] = await Promise.all([
        getDeviceStatus(),
        getSettings(),
        loadConfirmation(activeSet, target),
      ]);
      setCurrentStatus(latestStatus);
      setCurrentSettings(latestSettings);

      if (latestSettings.activeSetId !== activeSet.id) {
        setStale(true);
        setSubmitError(
          "The active macro set changed on the device. Sending was not started.",
        );
        return;
      }
      if (
        !sameMacroSnapshot(loaded.macro, latest.macro) ||
        !sameValidationSnapshot(loaded.validation, latest.validation)
      ) {
        setLoaded(latest);
        setStale(true);
        setSubmitError(
          "The macro changed on the device. Review the latest preview before sending.",
        );
        return;
      }
      if (latestStatus.usbState !== "ready") {
        setSubmitError(
          `USB changed to ${statusLabel(
            latestStatus.usbState,
          )}. Sending was not started.`,
        );
        return;
      }
      if (latestStatus.executionState === "running") {
        setSubmitError(
          "Another macro execution started. Sending was not started.",
        );
        return;
      }

      setLoaded(latest);
      setStage(
        latestSettings.requirePhysicalConfirmation
          ? "physical-confirmation"
          : "submitting",
      );
      const request: ExecutionSubmitRequest = {
        setId: activeSet.id,
        macroId: latest.macro.id,
        macroRevision: latest.macro.revision,
      };
      const accepted = await submitExecution(request);
      onAccepted(accepted, returnHash());
    } catch (error: unknown) {
      if (error instanceof ApiError && error.status === 409) {
        setStale(true);
      }
      setSubmitError(submissionError(error));
    } finally {
      setStage("idle");
    }
  };

  if (target.kind === "invalid") {
    return (
      <section aria-labelledby="confirm-execution-title">
        <h2 id="confirm-execution-title">Confirm send</h2>
        <p className="error-message" role="alert">
          The confirmation URL must contain exactly one macro ID.
        </p>
        <button
          onClick={() => {
            navigate("macros");
          }}
          type="button"
        >
          Back to macros
        </button>
      </section>
    );
  }

  if (activeSet === null) {
    return (
      <section aria-labelledby="confirm-execution-title">
        <h2 id="confirm-execution-title">Confirm send</h2>
        <p>Select an active macro set before sending a macro.</p>
        <button
          onClick={() => {
            navigate("sets");
          }}
          type="button"
        >
          Select a macro set
        </button>
      </section>
    );
  }

  if (loadError !== null || loaded === null) {
    return (
      <section aria-labelledby="confirm-execution-title">
        <h2 id="confirm-execution-title">Confirm send</h2>
        <ErrorBanner message={loadError} />
        {loadError === null ? (
          <p aria-busy="true" role="status">
            Loading and validating the persisted macro…
          </p>
        ) : (
          <div className="form-actions">
            <button
              onClick={() => {
                setLoadVersion((version) => version + 1);
              }}
              type="button"
            >
              Retry
            </button>
            <button
              onClick={() => {
                returnToSource();
              }}
              type="button"
            >
              Cancel
            </button>
          </div>
        )}
      </section>
    );
  }

  const submitting = stage !== "idle";

  return (
    <section aria-labelledby="confirm-execution-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">Execution preview</p>
          <h2 id="confirm-execution-title">Confirm send</h2>
          <p>Review the persisted macro before any USB keystrokes are sent.</p>
        </div>
      </div>

      <ErrorBanner message={submitError} />
      {stale ? (
        <div className="conflict-message" role="alert">
          This preview is stale. Reload it before sending.
        </div>
      ) : null}

      <article className="card confirmation-summary">
        <div>
          <h3>{loaded.macro.name}</h3>
          <p>Revision {String(loaded.macro.revision)}</p>
          <p>Active set: {activeSet.name}</p>
        </div>
      </article>

      <div className="validation-card">
        <h3>Macro preview</h3>
        <pre className="macro-preview">
          <code>{loaded.macro.source}</code>
        </pre>
        <dl className="metadata">
          <dt>Actions</dt>
          <dd>{String(loaded.validation.actionCount)}</dd>
          <dt>Estimated duration</dt>
          <dd>{String(loaded.validation.estimatedDurationMs)} ms</dd>
          <dt>Key press</dt>
          <dd>{String(loaded.macro.key_press_ms)} ms</dd>
          <dt>Inter-key gap</dt>
          <dd>{String(loaded.macro.inter_key_ms)} ms</dd>
        </dl>
      </div>

      <div className="validation-card">
        <h3>Readiness</h3>
        <dl className="metadata">
          <dt>USB</dt>
          <dd>{statusLabel(currentStatus.usbState)}</dd>
          <dt>Executor</dt>
          <dd>
            {currentStatus.executionState === "running" ? "busy" : "available"}
          </dd>
          <dt>Physical confirmation</dt>
          <dd>
            {currentSettings.requirePhysicalConfirmation
              ? "Required on the device"
              : "Not required"}
          </dd>
        </dl>
        {blockers.length === 0 ? (
          <p className="validation-good" role="status">
            Ready to request execution.
          </p>
        ) : (
          <div className="confirmation-blockers" role="status">
            <p className="validation-bad">Send is disabled:</p>
            <ul>
              {blockers.map((reason) => (
                <li key={reason}>{reason}</li>
              ))}
            </ul>
          </div>
        )}
      </div>

      {stage === "preflight" ? (
        <div className="confirmation-panel" role="status" aria-live="assertive">
          Rechecking the macro, USB state, executor, and confirmation policy…
        </div>
      ) : null}
      {stage === "physical-confirmation" ? (
        <div className="confirmation-panel" role="status" aria-live="assertive">
          <strong>
            Send the <code>confirm</code> command on the device serial console.
          </strong>
          <p>
            Keystrokes will not begin unless the device accepts the physical
            confirmation. Do not press the button to let the request time out.
          </p>
        </div>
      ) : null}
      {stage === "submitting" ? (
        <div className="confirmation-panel" role="status" aria-live="assertive">
          Transferring the validated execution plan to the device…
        </div>
      ) : null}

      <div className="form-actions confirmation-actions">
        <button
          disabled={submitting}
          onClick={() => {
            returnToSource();
          }}
          type="button"
        >
          Cancel
        </button>
        {stale ? (
          <button
            disabled={submitting}
            onClick={() => {
              setLoadVersion((version) => version + 1);
            }}
            type="button"
          >
            Reload preview
          </button>
        ) : null}
        <button
          className="primary"
          disabled={submitting || blockers.length !== 0}
          onClick={() => {
            void send();
          }}
          type="button"
        >
          {stage === "preflight"
            ? "Rechecking…"
            : stage === "physical-confirmation"
              ? "Waiting for device…"
              : stage === "submitting"
                ? "Sending…"
                : "Send now"}
        </button>
      </div>
    </section>
  );
}
