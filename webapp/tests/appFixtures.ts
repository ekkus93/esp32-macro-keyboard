import { planJsonResponse } from "./fakeFetch";

export const setId = "11111111-1111-4111-8111-111111111111";
export const macroId = "22222222-2222-4222-8222-222222222222";
export const executionId = "33333333-3333-4333-8333-333333333333";
export const procedureId = "44444444-4444-4444-8444-444444444444";
export const instructionStepId = "55555555-5555-4555-8555-555555555555";
export const macroStepId = "66666666-6666-4666-8666-666666666666";
export const checkpointStepId = "77777777-7777-4777-8777-777777777777";
export const oldStepId = "88888888-8888-4888-8888-888888888888";

export const deviceStatus = {
  version: "0.1.0",
  idf: "v5.5.5",
  usbState: "ready",
  wifiState: "started",
  wifiClients: 1,
  executionState: "idle",
} as const;

export const settings = {
  schemaVersion: 1,
  revision: 4,
  requirePhysicalConfirmation: true,
  alwaysSelectSet: true,
  activeSetId: setId,
} as const;

export const macroSet = {
  schema_version: 1,
  id: setId,
  revision: 2,
  name: "Lab Chromebook workflow",
} as const;

export const macro = {
  schema_version: 1,
  id: macroId,
  revision: 7,
  set_id: setId,
  name: "Open terminal",
  source: "{CTRL+ALT+T}",
  favorite: true,
  key_press_ms: 8,
  inter_key_ms: 15,
} as const;

export const procedureSummary = {
  schema_version: 1,
  id: procedureId,
  revision: 5,
  set_id: setId,
  name: "Install Debian",
  description: "Guided Chromebook conversion workflow",
  step_count: 3,
  sort_order: 0,
} as const;

export const procedure = {
  schema_version: 1,
  id: procedureId,
  revision: 5,
  set_id: setId,
  name: "Install Debian",
  description: "Guided Chromebook conversion workflow",
  steps: [
    {
      id: instructionStepId,
      type: "instruction",
      title: "Enter developer mode",
      body: "Use the device-specific recovery key sequence.",
      required: true,
    },
    {
      id: macroStepId,
      type: "macro",
      title: "Open terminal",
      macro_id: macroId,
      required: true,
      auto_complete_on_success: true,
    },
    {
      id: checkpointStepId,
      type: "checkpoint",
      title: "Verify firmware menu",
      body: "Confirm that the firmware utility menu is visible.",
      required: true,
    },
  ],
  sort_order: 0,
} as const;

interface ProgressOptions {
  currentStepId?: string;
  completedStepIds?: readonly string[];
  skippedStepIds?: readonly string[];
  status?: "current" | "stale";
  procedureRevision?: number;
  currentProcedureRevision?: number;
}

export function procedureProgressSnapshot(
  options: ProgressOptions = {},
): object {
  return {
    status: options.status ?? "current",
    currentProcedureRevision:
      options.currentProcedureRevision ?? procedure.revision,
    progress: {
      schema_version: 1,
      set_id: setId,
      procedure_id: procedureId,
      procedure_revision: options.procedureRevision ?? procedure.revision,
      current_step_id: options.currentStepId ?? macroStepId,
      completed_step_ids: [
        ...(options.completedStepIds ?? [instructionStepId]),
      ],
      skipped_step_ids: [...(options.skippedStepIds ?? [])],
    },
  };
}

export function executionStatus(
  state:
    | "idle"
    | "running"
    | "completed"
    | "cancelled"
    | "failed"
    | "timed_out",
  actionIndex = 2,
  actionCount = 5,
): object {
  return {
    executionId,
    setId,
    macroId,
    macroRevision: 7,
    state,
    error: state === "failed" ? "press_failed" : "",
    releaseError: "",
    actionIndex,
    actionCount,
    available: true,
    cancellationRequested: false,
    acceptedMs: 1000,
    startedMs: state === "idle" ? 0 : 1010,
    completedMs: state === "idle" || state === "running" ? 0 : 1200,
    currentAction: state === "running" ? "key" : "none",
  };
}

export function planNormalUnauthenticatedBootstrap(): void {
  planJsonResponse(
    {
      ok: false,
      error: {
        code: "auth_required",
        message: "authentication required",
      },
    },
    401,
  );
  planJsonResponse({ ok: true, data: deviceStatus });
  planJsonResponse(
    {
      ok: false,
      error: {
        code: "auth_required",
        message: "authentication required",
      },
    },
    401,
  );
}

export function planAuthenticatedBootstrap(
  overrides: {
    activeSetId?: string | null;
    sets?: readonly object[];
    usbState?: string;
  } = {},
): void {
  planJsonResponse(
    {
      ok: false,
      error: {
        code: "auth_required",
        message: "authentication required",
      },
    },
    401,
  );
  planJsonResponse({
    ok: true,
    data: {
      ...deviceStatus,
      usbState: overrides.usbState ?? deviceStatus.usbState,
    },
  });
  planJsonResponse({
    ok: true,
    data: { authenticated: true, csrfToken: "csrf-restored" },
  });
  planJsonResponse({
    ok: true,
    data: {
      ...settings,
      activeSetId:
        overrides.activeSetId === undefined
          ? settings.activeSetId
          : overrides.activeSetId,
    },
  });
  planJsonResponse({
    ok: true,
    data: overrides.sets ?? [macroSet],
  });
}

export function planPostLoginBootstrap(): void {
  planJsonResponse({
    ok: true,
    data: { csrfToken: "csrf-123" },
  });
  planJsonResponse({ ok: true, data: deviceStatus });
  planJsonResponse({ ok: true, data: settings });
  planJsonResponse({ ok: true, data: [macroSet] });
}
