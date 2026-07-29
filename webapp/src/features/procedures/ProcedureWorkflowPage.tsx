import { useEffect, useMemo, useState } from "react";
import { ApiError } from "../../api/client";
import { errorText } from "../../api/errors";
import {
  completeProcedureStep,
  getSetProcedure,
  resetProcedureProgress,
  skipProcedureStep,
} from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { ProcedureTarget } from "../../routing";
import {
  navigateToProcedure,
  navigateToProcedureMacroConfirmation,
  navigateToProcedureStep,
} from "../../routing";
import type {
  MacroSet,
  Procedure,
  ProcedureProgressSnapshot,
  ProcedureStep,
} from "../../types/models";
import {
  canMutateProcedureStep,
  loadProcedureProgressState,
  procedureContextIssue,
  procedureCurrentStep,
  procedureIsFinished,
  procedureResolvedCount,
  procedureStepState,
  type ProcedureProgressState,
  type ProcedureStepState,
} from "./procedureState";

interface ProcedureWorkflowPageProps {
  activeSet: MacroSet | null;
  mode: "overview" | "step";
  target: ProcedureTarget;
}

type PendingConfirmation =
  | { kind: "checkpoint"; stepId: string }
  | { kind: "skip"; stepId: string }
  | { kind: "reset" };

function stepTypeLabel(step: ProcedureStep): string {
  switch (step.type) {
    case "macro":
      return "Macro";
    case "instruction":
      return "Instruction";
    case "checkpoint":
      return "Checkpoint";
  }
}

function stepStateLabel(state: ProcedureStepState): string {
  switch (state) {
    case "completed":
      return "Completed";
    case "skipped":
      return "Skipped";
    case "current":
      return "Current";
    case "upcoming":
      return "Upcoming";
  }
}

function stepStateClass(state: ProcedureStepState): string {
  return `procedure-step-state procedure-step-state-${state}`;
}

function StepContent({ step }: { step: ProcedureStep }): React.JSX.Element {
  return step.type === "macro" ? (
    <p>
      Macro reference: <code>{step.macro_id}</code>
      {step.auto_complete_on_success
        ? " · Mark complete after successful execution"
        : ""}
    </p>
  ) : (
    <p className="instruction-body">{step.body}</p>
  );
}

export function ProcedureWorkflowPage({
  activeSet,
  mode,
  target,
}: ProcedureWorkflowPageProps): React.JSX.Element {
  const [procedure, setProcedure] = useState<Procedure | null>(null);
  const [progressState, setProgressState] =
    useState<ProcedureProgressState | null>(null);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [loadVersion, setLoadVersion] = useState(0);
  const [busy, setBusy] = useState(false);
  const [actionError, setActionError] = useState<string | null>(null);
  const [actionMessage, setActionMessage] = useState<string | null>(null);
  const [pending, setPending] = useState<PendingConfirmation | null>(null);
  const [conflict, setConflict] = useState(false);

  const targetProcedureId = target.kind === "valid" ? target.procedureId : null;
  const targetStepId = target.kind === "valid" ? target.stepId : null;

  useEffect(() => {
    if (activeSet === null || targetProcedureId === null) {
      setProcedure(null);
      setProgressState(null);
      setLoadError(null);
      return;
    }

    let active = true;
    setProcedure(null);
    setProgressState(null);
    setLoadError(null);
    setActionError(null);
    setActionMessage(null);
    setPending(null);
    setConflict(false);
    const load = async (): Promise<void> => {
      try {
        const [loadedProcedure, loadedProgress] = await Promise.all([
          getSetProcedure(activeSet.id, targetProcedureId),
          loadProcedureProgressState(activeSet.id, targetProcedureId),
        ]);
        const issue = procedureContextIssue(
          loadedProcedure,
          activeSet.id,
          loadedProgress,
        );
        if (issue !== null) {
          throw new Error(issue);
        }
        if (active) {
          setProcedure(loadedProcedure);
          setProgressState(loadedProgress);
        }
      } catch (error: unknown) {
        if (active) {
          setLoadError(errorText(error));
        }
      }
    };
    void load();
    return () => {
      active = false;
    };
  }, [activeSet, loadVersion, targetProcedureId]);

  const selectedStepIndex = useMemo(() => {
    if (procedure === null || mode !== "step" || targetStepId === null) {
      return -1;
    }
    return procedure.steps.findIndex((step) => step.id === targetStepId);
  }, [mode, procedure, targetStepId]);

  const selectedStep =
    selectedStepIndex < 0 || procedure === null
      ? null
      : (procedure.steps[selectedStepIndex] ?? null);

  const installSnapshot = (snapshot: ProcedureProgressSnapshot): void => {
    if (procedure === null || activeSet === null) {
      throw new Error("Procedure context is unavailable.");
    }
    const nextState: ProcedureProgressState = { kind: "snapshot", snapshot };
    const issue = procedureContextIssue(procedure, activeSet.id, nextState);
    if (issue !== null) {
      throw new Error(issue);
    }
    setProgressState(nextState);
  };

  const recordMutationFailure = (error: unknown): void => {
    if (error instanceof ApiError && error.status === 409) {
      setConflict(true);
      setPending(null);
      setActionError(
        "Procedure progress changed on the device. Reload the latest procedure before continuing.",
      );
      return;
    }
    setActionError(errorText(error));
  };

  const performComplete = async (step: ProcedureStep): Promise<void> => {
    if (
      activeSet === null ||
      procedure === null ||
      progressState === null ||
      !canMutateProcedureStep(step, progressState)
    ) {
      return;
    }
    setBusy(true);
    setActionError(null);
    setActionMessage(null);
    try {
      const snapshot = await completeProcedureStep(
        activeSet.id,
        procedure.id,
        procedure.revision,
        step.id,
      );
      installSnapshot(snapshot);
      setPending(null);
      setActionMessage(
        "Step marked complete. The next step is ready; no macro was sent automatically.",
      );
    } catch (error: unknown) {
      recordMutationFailure(error);
    } finally {
      setBusy(false);
    }
  };

  const performSkip = async (step: ProcedureStep): Promise<void> => {
    if (
      activeSet === null ||
      procedure === null ||
      progressState === null ||
      !canMutateProcedureStep(step, progressState)
    ) {
      return;
    }
    setBusy(true);
    setActionError(null);
    setActionMessage(null);
    try {
      const snapshot = await skipProcedureStep(
        activeSet.id,
        procedure.id,
        procedure.revision,
        step.id,
      );
      installSnapshot(snapshot);
      setPending(null);
      setActionMessage(
        "Step skipped. The next step is ready; no macro was sent automatically.",
      );
    } catch (error: unknown) {
      recordMutationFailure(error);
    } finally {
      setBusy(false);
    }
  };

  const performReset = async (): Promise<void> => {
    if (activeSet === null || procedure === null) {
      return;
    }
    setBusy(true);
    setActionError(null);
    setActionMessage(null);
    try {
      const snapshot = await resetProcedureProgress(
        activeSet.id,
        procedure.id,
        procedure.revision,
      );
      installSnapshot(snapshot);
      setPending(null);
      setConflict(false);
      setActionMessage(
        progressState?.kind === "not-started"
          ? "Procedure started at the first step."
          : "Procedure progress reset to the first step.",
      );
    } catch (error: unknown) {
      recordMutationFailure(error);
    } finally {
      setBusy(false);
    }
  };

  if (activeSet === null) {
    return (
      <section aria-labelledby="procedure-workflow-title">
        <h2 id="procedure-workflow-title">
          {mode === "overview" ? "Procedure" : "Instruction"}
        </h2>
        <p>Select an active macro set before opening a procedure.</p>
      </section>
    );
  }

  if (target.kind === "invalid") {
    return (
      <section aria-labelledby="procedure-workflow-title">
        <h2 id="procedure-workflow-title">
          {mode === "overview" ? "Procedure" : "Instruction"}
        </h2>
        <p className="error-message" role="alert">
          The procedure URL is missing a valid procedure or step identifier.
        </p>
        <button
          onClick={() => {
            window.location.hash = "/procedures";
          }}
          type="button"
        >
          Back to procedures
        </button>
      </section>
    );
  }

  if (loadError !== null || procedure === null || progressState === null) {
    return (
      <section aria-labelledby="procedure-workflow-title">
        <h2 id="procedure-workflow-title">
          {mode === "overview" ? "Procedure" : "Instruction"}
        </h2>
        <ErrorBanner message={loadError} />
        {loadError === null ? (
          <p aria-busy="true" role="status">
            Loading procedure and progress…
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
                window.location.hash = "/procedures";
              }}
              type="button"
            >
              Back to procedures
            </button>
          </div>
        )}
      </section>
    );
  }

  if (mode === "step" && selectedStep === null) {
    return (
      <section aria-labelledby="procedure-workflow-title">
        <h2 id="procedure-workflow-title">Instruction</h2>
        <p className="error-message" role="alert">
          The requested step does not belong to this procedure.
        </p>
        <button
          onClick={() => {
            navigateToProcedure(procedure.id);
          }}
          type="button"
        >
          Procedure overview
        </button>
      </section>
    );
  }

  const currentStep = procedureCurrentStep(procedure, progressState);
  const finished = procedureIsFinished(procedure, progressState);
  const stale =
    progressState.kind === "snapshot" &&
    progressState.snapshot.status === "stale";
  const resolved = procedureResolvedCount(progressState);
  const progressText =
    progressState.kind === "not-started"
      ? "Not started"
      : stale
        ? `Saved progress uses revision ${String(
            progressState.snapshot.progress.procedure_revision,
          )}; procedure revision ${String(procedure.revision)} requires a reset.`
        : finished
          ? `Finished · ${String(resolved)} of ${String(
              procedure.steps.length,
            )} steps resolved`
          : `${String(resolved)} of ${String(
              procedure.steps.length,
            )} steps resolved`;

  const renderStepActions = (step: ProcedureStep): React.JSX.Element | null => {
    const state = procedureStepState(step, progressState);
    const mutable =
      canMutateProcedureStep(step, progressState) && !busy && !conflict;

    if (step.type === "macro" && state !== "upcoming" && !stale) {
      return (
        <div className="form-actions">
          <button
            className="primary"
            disabled={busy || conflict}
            onClick={() => {
              navigateToProcedureMacroConfirmation(
                procedure.id,
                step.id,
                step.macro_id,
              );
            }}
            type="button"
          >
            {state === "completed" ? "Resend macro" : "Send macro"}
          </button>
          {mutable ? (
            <button
              onClick={() => {
                setPending({ kind: "skip", stepId: step.id });
              }}
              type="button"
            >
              Skip step
            </button>
          ) : null}
        </div>
      );
    }

    if (!mutable) {
      return null;
    }
    return (
      <div className="form-actions">
        {step.type === "instruction" ? (
          <button
            className="primary"
            onClick={() => {
              void performComplete(step);
            }}
            type="button"
          >
            Mark instruction complete
          </button>
        ) : (
          <button
            className="primary"
            onClick={() => {
              setPending({ kind: "checkpoint", stepId: step.id });
            }}
            type="button"
          >
            Confirm checkpoint
          </button>
        )}
        <button
          onClick={() => {
            setPending({ kind: "skip", stepId: step.id });
          }}
          type="button"
        >
          Skip step
        </button>
      </div>
    );
  };

  const confirmationStep =
    pending !== null && pending.kind !== "reset"
      ? (procedure.steps.find((step) => step.id === pending.stepId) ?? null)
      : null;

  return (
    <section aria-labelledby="procedure-workflow-title">
      <div className="page-heading">
        <div>
          <p className="eyebrow dark">{activeSet.name}</p>
          <h2 id="procedure-workflow-title">{procedure.name}</h2>
          <p>{procedure.description}</p>
          <p>
            Revision {String(procedure.revision)} · {progressText}
          </p>
        </div>
        <button
          onClick={() => {
            window.location.hash = "/procedures";
          }}
          type="button"
        >
          Back to procedures
        </button>
      </div>

      <ErrorBanner message={actionError} />
      {actionMessage === null ? null : (
        <p className="save-message" role="status">
          {actionMessage}
        </p>
      )}

      {conflict ? (
        <div className="conflict-message" role="alert">
          <strong>Procedure progress is no longer current.</strong>
          <p>The local view was not changed or silently reconciled.</p>
          <button
            onClick={() => {
              setLoadVersion((version) => version + 1);
            }}
            type="button"
          >
            Reload latest
          </button>
        </div>
      ) : null}

      {stale ? (
        <div className="conflict-message" role="alert">
          <strong>Saved progress is stale.</strong>
          <p>
            The procedure changed after this progress was recorded. Reset is
            required; old step state will not be silently mapped to the new
            revision.
          </p>
        </div>
      ) : null}

      <div className="form-actions">
        {progressState.kind === "not-started" ? (
          <button
            className="primary"
            disabled={busy}
            onClick={() => {
              void performReset();
            }}
            type="button"
          >
            {busy ? "Starting…" : "Start procedure"}
          </button>
        ) : (
          <button
            disabled={busy || conflict}
            onClick={() => {
              setPending({ kind: "reset" });
            }}
            type="button"
          >
            Reset progress
          </button>
        )}
        {mode === "step" ? (
          <button
            onClick={() => {
              navigateToProcedure(procedure.id);
            }}
            type="button"
          >
            Procedure overview
          </button>
        ) : null}
      </div>

      {pending !== null ? (
        <div className="confirmation-panel" role="alertdialog">
          <strong>
            {pending.kind === "reset"
              ? "Reset all procedure progress?"
              : pending.kind === "skip"
                ? `Skip ${confirmationStep?.title ?? "this step"}?`
                : `Confirm ${confirmationStep?.title ?? "this checkpoint"}?`}
          </strong>
          <p>
            {pending.kind === "reset"
              ? "Completed and skipped markers will be cleared."
              : pending.kind === "skip"
                ? "The step will be recorded as skipped and the next step will become current."
                : "Confirm that the expected checkpoint result is present before continuing."}
          </p>
          <div className="form-actions">
            <button
              className={pending.kind === "reset" ? "danger" : "primary"}
              disabled={
                busy || (confirmationStep === null && pending.kind !== "reset")
              }
              onClick={() => {
                if (pending.kind === "reset") {
                  void performReset();
                } else if (
                  confirmationStep !== null &&
                  pending.kind === "skip"
                ) {
                  void performSkip(confirmationStep);
                } else if (confirmationStep !== null) {
                  void performComplete(confirmationStep);
                }
              }}
              type="button"
            >
              {busy
                ? "Saving…"
                : pending.kind === "reset"
                  ? "Confirm reset"
                  : pending.kind === "skip"
                    ? "Confirm skip"
                    : "Confirm completion"}
            </button>
            <button
              disabled={busy}
              onClick={() => {
                setPending(null);
              }}
              type="button"
            >
              Cancel
            </button>
          </div>
        </div>
      ) : null}

      {mode === "overview" ? (
        <div aria-label="Procedure steps" className="procedure-steps">
          {procedure.steps.map((step, index) => {
            const state = procedureStepState(step, progressState);
            const expanded = currentStep?.id === step.id;
            return (
              <article
                className={`procedure-step ${
                  expanded ? "procedure-step-current" : ""
                }`}
                key={step.id}
              >
                <div className="procedure-step-heading">
                  <div>
                    <p className="eyebrow dark">
                      Step {String(index + 1)} · {stepTypeLabel(step)}
                    </p>
                    <h3>{step.title}</h3>
                  </div>
                  <span className={stepStateClass(state)}>
                    {stepStateLabel(state)}
                  </span>
                </div>
                <p>{step.required ? "Required" : "Optional"}</p>
                {expanded ? <StepContent step={step} /> : null}
                {expanded ? renderStepActions(step) : null}
                <button
                  onClick={() => {
                    navigateToProcedureStep(procedure.id, step.id);
                  }}
                  type="button"
                >
                  Open step
                </button>
              </article>
            );
          })}
        </div>
      ) : selectedStep === null ? null : (
        <article className="procedure-step procedure-step-current">
          <div className="procedure-step-heading">
            <div>
              <p className="eyebrow dark">
                Step {String(selectedStepIndex + 1)} of{" "}
                {String(procedure.steps.length)} · {stepTypeLabel(selectedStep)}
              </p>
              <h3>{selectedStep.title}</h3>
            </div>
            <span
              className={stepStateClass(
                procedureStepState(selectedStep, progressState),
              )}
            >
              {stepStateLabel(procedureStepState(selectedStep, progressState))}
            </span>
          </div>
          <p>{selectedStep.required ? "Required" : "Optional"}</p>
          <StepContent step={selectedStep} />
          {renderStepActions(selectedStep)}
          <div className="procedure-navigation">
            <button
              disabled={selectedStepIndex === 0}
              onClick={() => {
                const previous = procedure.steps[selectedStepIndex - 1];
                if (previous !== undefined) {
                  navigateToProcedureStep(procedure.id, previous.id);
                }
              }}
              type="button"
            >
              Previous step
            </button>
            <button
              disabled={selectedStepIndex + 1 >= procedure.steps.length}
              onClick={() => {
                const next = procedure.steps[selectedStepIndex + 1];
                if (next !== undefined) {
                  navigateToProcedureStep(procedure.id, next.id);
                }
              }}
              type="button"
            >
              Next step
            </button>
          </div>
        </article>
      )}
    </section>
  );
}
