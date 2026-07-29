import { useEffect, useMemo, useState } from "react";
import { ApiError } from "../../api/client";
import { errorText } from "../../api/errors";
import {
  getDeviceStatus,
  getGlobalMacro,
  getSetMacro,
  getSetProcedure,
  submitExecution,
  validateGlobalMacro,
  validateSetMacro,
} from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { ExecutionConfirmationTarget } from "../../routing";
import { navigate, navigateToProcedureStep } from "../../routing";
import type {
  DeviceStatus,
  ExecutionAccepted,
  ExecutionSubmitRequest,
  Macro,
  MacroSet,
  MacroValidation,
  Procedure,
  ProcedureStep,
  Settings,
} from "../../types/models";

type ProcedureMacroStep = Extract<ProcedureStep, { type: "macro" }>;

type SubmissionStage =
  | "idle"
  | "preflight"
  | "physical-confirmation"
  | "submitting";

interface LoadedProcedureContext {
  procedure: Procedure;
  step: ProcedureMacroStep;
}

interface LoadedConfirmation {
  macro: Macro;
  validation: MacroValidation;
  procedureContext: LoadedProcedureContext | null;
}

interface ConfirmExecutionPageProps {
  activeSet: MacroSet | null;
  onAccepted: (accepted: ExecutionAccepted, returnHash: string) => void;
  settings: Settings;
  status: DeviceStatus;
  target: ExecutionConfirmationTarget;
}

async function loadPersistedMacro(
  setId: string,
  macroId: string,
): Promise<Macro> {
  try {
    return await getSetMacro(setId, macroId);
  } catch (error: unknown) {
    if (!(error instanceof ApiError) || error.status !== 404) {
      throw error;
    }
    return getGlobalMacro(macroId);
  }
}

function validatePersistedMacro(
  activeSetId: string,
  macro: Macro,
): Promise<MacroValidation> {
  return macro.scope === "set"
    ? validateSetMacro(activeSetId, macro)
    : validateGlobalMacro(macro);
}

function macroIdentityIssue(
  activeSetId: string,
  requestedMacroId: string,
  macro: Macro,
): string | null {
  if (macro.id !== requestedMacroId) {
    return "The device returned a different macro than the requested macro.";
  }
  if (macro.scope === "set" && macro.set_id !== activeSetId) {
    return "The requested macro does not belong to the active macro set.";
  }
  return null;
}

function procedureContext(
  activeSetId: string,
  macroId: string,
  target: ExecutionConfirmationTarget,
  procedure: Procedure | null,
): LoadedProcedureContext | null {
  if (target.kind !== "valid" || target.sourceContext === null) {
    return null;
  }
  if (
    procedure === null ||
    procedure.id !== target.sourceContext.procedureId ||
    procedure.set_id !== activeSetId
  ) {
    throw new Error(
      "The procedure context no longer belongs to the active macro set.",
    );
  }
  const step = procedure.steps.find(
    (candidate) => candidate.id === target.sourceContext?.stepId,
  );
  if (step?.type !== "macro" || step.macro_id !== macroId) {
    throw new Error(
      "The procedure step no longer references the selected macro.",
    );
  }
  return { procedure, step };
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

  const procedurePromise: Promise<Procedure | null> =
    target.sourceContext === null
      ? Promise.resolve(null)
      : getSetProcedure(activeSet.id, target.sourceContext.procedureId);
  const [validation, loadedProcedure] = await Promise.all([
    validatePersistedMacro(activeSet.id, macro),
    procedurePromise,
  ]);

  return {
    macro,
    validation,
    procedureContext: procedureContext(
      activeSet.id,
      macro.id,
      target,
      loadedProcedure,
    ),
  };
}

function sameMacroSnapshot(left: Macro, right: Macro): boolean {
  return (
    left.id === right.id &&
    left.revision === right.revision &&
    left.scope === right.scope &&
    left.set_id === right.set_id &&
    left.source === right.source &&
    left.key_press_ms === right.key_press_ms &&
    left.inter_key_ms === right.inter_key_ms
  );
}

function sameProcedureContext(
  left: LoadedProcedureContext | null,
  right: LoadedProcedureContext | null,
): boolean {
  if (left === null || right === null) {
    return left === right;
  }
  return (
    left.procedure.id === right.procedure.id &&
    left.procedure.revision === right.procedure.revision &&
    left.step.id === right.step.id &&
    left.step.macro_id === right.step.macro_id
  );
}

function returnHash(target: ExecutionConfirmationTarget): string {
  if (target.kind !== "valid" || target.sourceContext === null) {
    return "/macros";
  }
  return `/instruction?procedureId=${encodeURIComponent(
    target.sourceContext.procedureId,
  )}&stepId=${encodeURIComponent(target.sourceContext.stepId)}`;
}

function returnToSource(target: ExecutionConfirmationTarget): void {
  if (target.kind === "valid" && target.sourceContext !== null) {
    navigateToProcedureStep(
      target.sourceContext.procedureId,
      target.sourceContext.stepId,
    );
    return;
  }
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
  const [currentStatus, setCurrentStatus] = useState(status);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [submitError, setSubmitError] = useState<string | null>(null);
  const [loadVersion, setLoadVersion] = useState(0);
  const [stale, setStale] = useState(false);
  const [stage, setStage] = useState<SubmissionStage>("idle");

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
    if (stale) {
      reasons.push("The macro or procedure changed after this preview loaded.");
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
  }, [currentStatus.executionState, currentStatus.usbState, loaded, stale]);

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
      const [latestStatus, latest] = await Promise.all([
        getDeviceStatus(),
        loadConfirmation(activeSet, target),
      ]);
      setCurrentStatus(latestStatus);

      if (
        !sameMacroSnapshot(loaded.macro, latest.macro) ||
        !sameProcedureContext(
          loaded.procedureContext,
          latest.procedureContext,
        )
      ) {
        setLoaded(latest);
        setStale(true);
        setSubmitError(
          "The macro or procedure changed on the device. Review the latest preview before sending.",
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
        settings.requirePhysicalConfirmation
          ? "physical-confirmation"
          : "submitting",
      );
      const request: ExecutionSubmitRequest = {
        setId: activeSet.id,
        macroId: latest.macro.id,
        macroRevision: latest.macro.revision,
        ...(target.sourceContext === null
          ? {}
          : { sourceContext: target.sourceContext }),
      };
      const accepted = await submitExecution(request);
      onAccepted(accepted, returnHash(target));
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
          The confirmation URL must contain one macro ID and either both procedure
          context IDs or neither context ID.
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
                returnToSource(target);
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

  const procedure = loaded.procedureContext;
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
          <p>
            {loaded.macro.scope === "global" ? "Global macro" : "Set macro"} ·
            revision {String(loaded.macro.revision)}
          </p>
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

      {procedure === null ? null : (
        <div className="validation-card">
          <h3>Procedure context</h3>
          <p>{procedure.procedure.name}</p>
          <p>
            Step: {procedure.step.title} · procedure revision{" "}
            {String(procedure.procedure.revision)}
          </p>
        </div>
      )}

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
            {settings.requirePhysicalConfirmation
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
          Rechecking the macro, procedure context, USB state, and executor…
        </div>
      ) : null}
      {stage === "physical-confirmation" ? (
        <div className="confirmation-panel" role="status" aria-live="assertive">
          <strong>Press the confirmation button on the device.</strong>
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
            returnToSource(target);
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
