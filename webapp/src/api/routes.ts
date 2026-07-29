import { apiRequest, setCsrfToken } from "./client";
import { isExecutionAccepted } from "./executionGuards";
import {
  isCancelAccepted,
  isDeviceStatus,
  isExecutionStatus,
  isEmptyRecord,
  isLoginResponse,
  isMacro,
  isMacroList,
  isMacroSet,
  isMacroSetList,
  isMacroValidation,
  isProcedure,
  isProcedureProgressSnapshot,
  isProcedureSummaryList,
  isRestartAccepted,
  isSessionStatus,
  isSettings,
  isSetupState,
} from "./guards";
import {
  isFactoryResetAccepted,
  isQuarantineList,
  isSetDeletion,
  isStorageHealth,
} from "./managementGuards";
import type {
  CancelAccepted,
  DeviceStatus,
  ExecutionAccepted,
  ExecutionStatus,
  ExecutionSubmitRequest,
  FactoryResetAccepted,
  LoginResponse,
  Macro,
  MacroSet,
  MacroValidation,
  Procedure,
  ProcedureProgressSnapshot,
  ProcedureSummary,
  QuarantineList,
  RestartAccepted,
  SessionStatus,
  SetDeletion,
  Settings,
  SetupState,
  StorageHealth,
} from "../types/models";

export interface SetupSubmission {
  setupCode: string;
  apSsid: string;
  apPassphrase: string;
  administratorPassword: string;
  requirePhysicalConfirmation: boolean;
  alwaysSelectSet: boolean;
}

export interface SettingsUpdate {
  expectedRevision: number;
  requirePhysicalConfirmation: boolean;
  alwaysSelectSet: boolean;
  activeSetId: string | null;
}

export interface SetDuplicateRequest {
  id: string;
  name: string;
  expectedRevision: number;
}

const physicalConfirmationOptions = { timeoutMs: 25_000 } as const;

export async function getSetupState(): Promise<SetupState> {
  return apiRequest("/api/v1/setup-state", {}, isSetupState, {
    notifyOnUnauthorized: false,
  });
}

export async function submitSetup(
  submission: SetupSubmission,
): Promise<SetupState> {
  return apiRequest(
    "/api/v1/setup/credentials",
    {
      method: "POST",
      body: JSON.stringify(submission),
    },
    isSetupState,
    { notifyOnUnauthorized: false },
  );
}

export async function restartAfterSetup(): Promise<RestartAccepted> {
  return apiRequest(
    "/api/v1/setup/restart",
    { method: "POST" },
    isRestartAccepted,
    { notifyOnUnauthorized: false },
  );
}

export async function getDeviceStatus(): Promise<DeviceStatus> {
  return apiRequest("/api/v1/status", {}, isDeviceStatus);
}

export async function getSession(): Promise<SessionStatus> {
  const response = await apiRequest(
    "/api/v1/auth/session",
    {},
    isSessionStatus,
  );
  setCsrfToken(response.csrfToken);
  return response;
}

export async function login(password: string): Promise<LoginResponse> {
  const response = await apiRequest(
    "/api/v1/auth/login",
    {
      method: "POST",
      body: JSON.stringify({ password }),
    },
    isLoginResponse,
    { notifyOnUnauthorized: false },
  );
  setCsrfToken(response.csrfToken);
  return response;
}

export async function logout(): Promise<void> {
  await apiRequest("/api/v1/auth/logout", { method: "POST" }, isEmptyRecord);
  setCsrfToken(null);
}

export async function getSettings(): Promise<Settings> {
  return apiRequest("/api/v1/settings", {}, isSettings);
}

export async function updateSettings(
  replacement: SettingsUpdate,
): Promise<Settings> {
  return apiRequest(
    "/api/v1/settings",
    {
      method: "PUT",
      body: JSON.stringify(replacement),
    },
    isSettings,
  );
}

export async function resetSettings(expectedRevision: number): Promise<Settings> {
  return apiRequest(
    "/api/v1/device/reset-settings",
    {
      method: "POST",
      body: JSON.stringify({ expectedRevision }),
    },
    isSettings,
    physicalConfirmationOptions,
  );
}

export async function restartDevice(): Promise<RestartAccepted> {
  return apiRequest(
    "/api/v1/device/restart",
    { method: "POST" },
    isRestartAccepted,
    physicalConfirmationOptions,
  );
}

export async function factoryResetDevice(): Promise<FactoryResetAccepted> {
  return apiRequest(
    "/api/v1/device/factory-reset",
    { method: "POST" },
    isFactoryResetAccepted,
    physicalConfirmationOptions,
  );
}

function setPath(setId: string): string {
  return `/api/v1/sets/${encodeURIComponent(setId)}`;
}

export async function listSets(): Promise<MacroSet[]> {
  return apiRequest("/api/v1/sets", {}, isMacroSetList);
}

export async function getSet(setId: string): Promise<MacroSet> {
  return apiRequest(setPath(setId), {}, isMacroSet);
}

export async function createSet(set: MacroSet): Promise<MacroSet> {
  return apiRequest(
    "/api/v1/sets",
    {
      method: "POST",
      body: JSON.stringify(set),
    },
    isMacroSet,
  );
}

export async function updateSet(
  set: MacroSet,
  expectedRevision: number,
): Promise<MacroSet> {
  return apiRequest(
    setPath(set.id),
    {
      method: "PUT",
      body: JSON.stringify({ expectedRevision, resource: set }),
    },
    isMacroSet,
  );
}

export async function deleteSet(
  setId: string,
  expectedRevision: number,
): Promise<SetDeletion> {
  return apiRequest(
    setPath(setId),
    {
      method: "DELETE",
      body: JSON.stringify({ expectedRevision }),
    },
    isSetDeletion,
  );
}

export async function duplicateSet(
  setId: string,
  request: SetDuplicateRequest,
): Promise<MacroSet> {
  return apiRequest(
    `${setPath(setId)}/duplicate`,
    {
      method: "POST",
      body: JSON.stringify(request),
    },
    isMacroSet,
  );
}

export async function reorderSets(ids: readonly string[]): Promise<MacroSet[]> {
  return apiRequest(
    "/api/v1/sets/order",
    {
      method: "PUT",
      body: JSON.stringify({ ids }),
    },
    isMacroSetList,
  );
}

export async function selectSet(
  setId: string,
  expectedRevision: number,
): Promise<Settings> {
  return apiRequest(
    `${setPath(setId)}/select`,
    {
      method: "POST",
      body: JSON.stringify({ expectedRevision }),
    },
    isSettings,
  );
}

function setMacrosPath(setId: string): string {
  return `${setPath(setId)}/macros`;
}

function setMacroPath(setId: string, macroId: string): string {
  return `${setMacrosPath(setId)}/${encodeURIComponent(macroId)}`;
}

function globalMacroPath(macroId: string): string {
  return `/api/v1/global/macros/${encodeURIComponent(macroId)}`;
}

export async function listSetMacros(setId: string): Promise<Macro[]> {
  return apiRequest(setMacrosPath(setId), {}, isMacroList);
}

export async function getSetMacro(
  setId: string,
  macroId: string,
): Promise<Macro> {
  return apiRequest(setMacroPath(setId, macroId), {}, isMacro);
}

export async function getGlobalMacro(macroId: string): Promise<Macro> {
  return apiRequest(globalMacroPath(macroId), {}, isMacro);
}

export async function createSetMacro(
  setId: string,
  macro: Macro,
): Promise<Macro> {
  return apiRequest(
    setMacrosPath(setId),
    {
      method: "POST",
      body: JSON.stringify(macro),
    },
    isMacro,
  );
}

export async function updateSetMacro(
  setId: string,
  macro: Macro,
  expectedRevision: number,
): Promise<Macro> {
  return apiRequest(
    setMacroPath(setId, macro.id),
    {
      method: "PUT",
      body: JSON.stringify({
        expectedRevision,
        resource: macro,
      }),
    },
    isMacro,
  );
}

export async function validateSetMacro(
  setId: string,
  macro: Macro,
): Promise<MacroValidation> {
  return apiRequest(
    `${setMacroPath(setId, macro.id)}/validate`,
    {
      method: "POST",
      body: JSON.stringify(macro),
    },
    isMacroValidation,
  );
}

export async function validateGlobalMacro(
  macro: Macro,
): Promise<MacroValidation> {
  return apiRequest(
    `${globalMacroPath(macro.id)}/validate`,
    {
      method: "POST",
      body: JSON.stringify(macro),
    },
    isMacroValidation,
  );
}

function setProceduresPath(setId: string): string {
  return `${setPath(setId)}/procedures`;
}

function setProcedurePath(setId: string, procedureId: string): string {
  return `${setProceduresPath(setId)}/${encodeURIComponent(procedureId)}`;
}

function procedureProgressPath(setId: string, procedureId: string): string {
  return `${setProcedurePath(setId, procedureId)}/progress`;
}

export async function listSetProcedures(
  setId: string,
): Promise<ProcedureSummary[]> {
  return apiRequest(setProceduresPath(setId), {}, isProcedureSummaryList);
}

export async function getSetProcedure(
  setId: string,
  procedureId: string,
): Promise<Procedure> {
  return apiRequest(setProcedurePath(setId, procedureId), {}, isProcedure);
}

export async function getProcedureProgress(
  setId: string,
  procedureId: string,
): Promise<ProcedureProgressSnapshot> {
  return apiRequest(
    procedureProgressPath(setId, procedureId),
    {},
    isProcedureProgressSnapshot,
  );
}

export async function resetProcedureProgress(
  setId: string,
  procedureId: string,
  expectedProcedureRevision: number,
): Promise<ProcedureProgressSnapshot> {
  return apiRequest(
    procedureProgressPath(setId, procedureId),
    {
      method: "DELETE",
      body: JSON.stringify({
        expectedRevision: expectedProcedureRevision,
      }),
    },
    isProcedureProgressSnapshot,
  );
}

export async function completeProcedureStep(
  setId: string,
  procedureId: string,
  expectedProcedureRevision: number,
  stepId: string,
): Promise<ProcedureProgressSnapshot> {
  return apiRequest(
    `${procedureProgressPath(setId, procedureId)}/complete`,
    {
      method: "POST",
      body: JSON.stringify({
        expectedProcedureRevision,
        stepId,
      }),
    },
    isProcedureProgressSnapshot,
  );
}

export async function skipProcedureStep(
  setId: string,
  procedureId: string,
  expectedProcedureRevision: number,
  stepId: string,
): Promise<ProcedureProgressSnapshot> {
  return apiRequest(
    `${procedureProgressPath(setId, procedureId)}/skip`,
    {
      method: "POST",
      body: JSON.stringify({
        expectedProcedureRevision,
        stepId,
        confirmed: true,
      }),
    },
    isProcedureProgressSnapshot,
  );
}

export async function submitExecution(
  request: ExecutionSubmitRequest,
): Promise<ExecutionAccepted> {
  return apiRequest(
    "/api/v1/executions",
    {
      method: "POST",
      body: JSON.stringify(request),
    },
    isExecutionAccepted,
    physicalConfirmationOptions,
  );
}

export async function getCurrentExecution(): Promise<ExecutionStatus> {
  return apiRequest("/api/v1/executions/current", {}, isExecutionStatus);
}

export async function cancelCurrentExecution(): Promise<CancelAccepted> {
  return apiRequest(
    "/api/v1/executions/current/cancel",
    { method: "POST" },
    isCancelAccepted,
  );
}

export async function getStorageHealth(): Promise<StorageHealth> {
  return apiRequest("/api/v1/diagnostics/storage", {}, isStorageHealth);
}

export async function getQuarantine(): Promise<QuarantineList> {
  return apiRequest(
    "/api/v1/diagnostics/quarantine",
    {},
    isQuarantineList,
  );
}
