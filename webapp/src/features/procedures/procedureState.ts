import { ApiError } from "../../api/client";
import { getProcedureProgress } from "../../api/routes";
import type {
  Procedure,
  ProcedureProgressSnapshot,
  ProcedureStep,
  ProcedureSummary,
} from "../../types/models";

export type ProcedureProgressState =
  | { kind: "not-started" }
  | { kind: "snapshot"; snapshot: ProcedureProgressSnapshot };

export type ProcedureStepState =
  | "completed"
  | "skipped"
  | "current"
  | "upcoming";

export async function loadProcedureProgressState(
  setId: string,
  procedureId: string,
): Promise<ProcedureProgressState> {
  try {
    return {
      kind: "snapshot",
      snapshot: await getProcedureProgress(setId, procedureId),
    };
  } catch (error: unknown) {
    if (error instanceof ApiError && error.status === 404) {
      return { kind: "not-started" };
    }
    throw error;
  }
}

function uniqueResolvedCount(snapshot: ProcedureProgressSnapshot): number {
  return (
    snapshot.progress.completed_step_ids.length +
    snapshot.progress.skipped_step_ids.length
  );
}

export function summaryProgressIssue(
  summary: ProcedureSummary,
  activeSetId: string,
  state: ProcedureProgressState,
): string | null {
  if (summary.set_id !== activeSetId) {
    return "The device returned a procedure outside the active set.";
  }
  if (state.kind === "not-started") {
    return null;
  }
  const { snapshot } = state;
  if (
    snapshot.progress.set_id !== activeSetId ||
    snapshot.progress.procedure_id !== summary.id
  ) {
    return "The device returned progress for a different procedure.";
  }
  if (snapshot.currentProcedureRevision !== summary.revision) {
    return "Procedure progress and procedure metadata disagree about the current revision.";
  }
  if (
    snapshot.status === "current" &&
    uniqueResolvedCount(snapshot) > summary.step_count
  ) {
    return "Procedure progress contains more resolved steps than the procedure.";
  }
  return null;
}

export function procedureContextIssue(
  procedure: Procedure,
  activeSetId: string,
  state: ProcedureProgressState,
): string | null {
  if (procedure.set_id !== activeSetId) {
    return "The device returned a procedure outside the active set.";
  }
  if (state.kind === "not-started") {
    return null;
  }

  const { snapshot } = state;
  if (
    snapshot.progress.set_id !== activeSetId ||
    snapshot.progress.procedure_id !== procedure.id
  ) {
    return "The device returned progress for a different procedure.";
  }
  if (snapshot.currentProcedureRevision !== procedure.revision) {
    return "Procedure progress and the loaded procedure disagree about the current revision.";
  }
  if (snapshot.status === "stale") {
    return null;
  }

  const stepIds = new Set(procedure.steps.map((step) => step.id));
  if (!stepIds.has(snapshot.progress.current_step_id)) {
    return "Current procedure progress references an unknown step.";
  }
  for (const stepId of snapshot.progress.completed_step_ids) {
    if (!stepIds.has(stepId)) {
      return "Completed procedure progress references an unknown step.";
    }
  }
  for (const stepId of snapshot.progress.skipped_step_ids) {
    if (!stepIds.has(stepId)) {
      return "Skipped procedure progress references an unknown step.";
    }
  }
  return null;
}

export function procedureResolvedCount(state: ProcedureProgressState): number {
  return state.kind === "snapshot" ? uniqueResolvedCount(state.snapshot) : 0;
}

export function procedureProgressLabel(
  summary: ProcedureSummary,
  state: ProcedureProgressState,
): string {
  if (state.kind === "not-started") {
    return `Not started · ${String(summary.step_count)} steps`;
  }
  if (state.snapshot.status === "stale") {
    return `Needs reset · saved revision ${String(
      state.snapshot.progress.procedure_revision,
    )}, current revision ${String(state.snapshot.currentProcedureRevision)}`;
  }
  const completed = state.snapshot.progress.completed_step_ids.length;
  const skipped = state.snapshot.progress.skipped_step_ids.length;
  const remaining = Math.max(
    0,
    summary.step_count - procedureResolvedCount(state),
  );
  return `${String(completed)} completed · ${String(
    skipped,
  )} skipped · ${String(remaining)} remaining`;
}

export function procedureIsFinished(
  procedure: Procedure,
  state: ProcedureProgressState,
): boolean {
  return (
    state.kind === "snapshot" &&
    state.snapshot.status === "current" &&
    procedureResolvedCount(state) === procedure.steps.length
  );
}

export function procedureCurrentStep(
  procedure: Procedure,
  state: ProcedureProgressState,
): ProcedureStep | null {
  if (state.kind !== "snapshot" || state.snapshot.status !== "current") {
    return null;
  }
  return (
    procedure.steps.find(
      (step) => step.id === state.snapshot.progress.current_step_id,
    ) ?? null
  );
}

export function procedureStepState(
  step: ProcedureStep,
  state: ProcedureProgressState,
): ProcedureStepState {
  if (state.kind === "not-started" || state.snapshot.status === "stale") {
    return "upcoming";
  }
  if (state.snapshot.progress.completed_step_ids.includes(step.id)) {
    return "completed";
  }
  if (state.snapshot.progress.skipped_step_ids.includes(step.id)) {
    return "skipped";
  }
  return state.snapshot.progress.current_step_id === step.id
    ? "current"
    : "upcoming";
}

export function canMutateProcedureStep(
  step: ProcedureStep,
  state: ProcedureProgressState,
): boolean {
  return (
    state.kind === "snapshot" &&
    state.snapshot.status === "current" &&
    state.snapshot.progress.current_step_id === step.id &&
    procedureStepState(step, state) === "current"
  );
}
