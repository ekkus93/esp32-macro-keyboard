import { apiRequest, setCsrfToken } from "./client";
import {
  isCancelAccepted,
  isDeviceStatus,
  isExecutionStatus,
  isEmptyRecord,
  isLoginResponse,
  isMacroSetList,
  isRestartAccepted,
  isSessionStatus,
  isSettings,
  isSetupState,
} from "./guards";
import type {
  CancelAccepted,
  DeviceStatus,
  ExecutionStatus,
  LoginResponse,
  MacroSet,
  RestartAccepted,
  SessionStatus,
  Settings,
  SetupState,
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

export async function listSets(): Promise<MacroSet[]> {
  return apiRequest("/api/v1/sets", {}, isMacroSetList);
}

export async function selectSet(
  setId: string,
  expectedRevision: number,
): Promise<Settings> {
  return apiRequest(
    `/api/v1/sets/${encodeURIComponent(setId)}/select`,
    {
      method: "POST",
      body: JSON.stringify({ expectedRevision }),
    },
    isSettings,
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
